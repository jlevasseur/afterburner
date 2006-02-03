#include INC_ARCH(cpu.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(comms.h)

#include <sys/time.h>
#include <signal.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

extern void sync_deliver_page_not_present( word_t fault_addr, word_t fault_rwx );
extern void sync_deliver_page_permissions_fault( word_t fault_addr, word_t fault_rwx );

extern bool debug_page_fault;
bool debug_irq_deliver = false;
extern bool backend_preboot(void *entry, void *sp, char *command_line);

#define STACK_SIZE 1024

static uint8_t vm_stack[STACK_SIZE] ALIGNED(CONFIG_STACK_ALIGN);

extern int physmem_handle;
uintptr_t end_stack;
extern uintptr_t executable_start;
extern hiostream_linux_t con_driver;

uintptr_t alt_stack[1000];
uintptr_t irq_old_cs;
uintptr_t irq_old_ip;
uintptr_t irq_handler;
pid_t child;

//extern "C" void int_fixup(void);

uintptr_t 
backend_handle_pagefault(uintptr_t fault_addr,
			 uintptr_t ip,
			 uint8_t & rwx,
			 uintptr_t & pagesize,
			 uint8_t & state)
{
	bool devmem = false;
	cpu_t &cpu = get_cpu();
	uint8_t fault_rwx = rwx;
	uintptr_t pfault = 0;
	pagesize = 0x1000;
	state = 0;
	if (debug_page_fault)
		con << "Pagefault: " << (void*) fault_addr << " prot: " << rwx << " from: " << (void*)  ip << "\n";

	rwx = 7;
	
	if (cpu.cr0.paging_enabled()) {
		int r;
		pgent_t *pdir_p = (pgent_t *)(cpu.cr3.get_pdir_addr());
		pdir_p = &pdir_p[pgent_t::get_pdir_idx(fault_addr)];
		pgent_t pdir;
		r = pread(physmem_handle, &pdir, sizeof(pgent_t), (uintptr_t) pdir_p);
		ASSERT(r == sizeof(pgent_t));
		if(!pdir.is_valid()) {
			//con << "pdir not present\n";
			goto not_present;
		}

		if (debug_page_fault)
			con << "Pagefault has valid pdir\n";

		if (pdir.is_superpage()) {
			pagesize = 4 * 1024 * 1024;
			if (debug_page_fault)
				con << "Pdir is superpage\n";
			pfault = (pdir.get_address() & SUPERPAGE_MASK) + 
				(fault_addr & ~SUPERPAGE_MASK);
			if( !pdir.is_writable() ) {
				con << "Pdir is not writable\n";
				rwx = 5;
			}
		} else {
			pgent_t *ptab = (pgent_t *)(pdir.get_address());
			ptab = &ptab[ pgent_t::get_ptab_idx(fault_addr) ];
			pgent_t pgent;
			r = pread(physmem_handle, &pgent, sizeof(pgent_t), (uintptr_t) ptab);
			ASSERT(r == sizeof(pgent_t));
			if(!pgent.is_valid()) {
				//con << "pgent not present\n";
				goto not_present;
			}
			if (debug_page_fault)
				con << "pgent is valid\n";

			pfault = pgent.get_address() + (fault_addr & ~PAGE_MASK);
			if(!pgent.is_writable()) {
				//con << "Pgent not writable\n";
				rwx = 5;
			}
		}
		
		if (debug_page_fault)
			con << "rwx now " << rwx << " cr0 writeprotect: " << cpu.cr0.write_protect() << "\n";

		if (((rwx & fault_rwx) != fault_rwx) && cpu.cr0.write_protect()) {
			//con << "permission fault\n";
			goto permissions_fault;
		}
	}

    if( devmem ) {
	    con << "device request, fault addr " << (void *)fault_addr
		<< ", ip " << (void *)ip << '\n';
	exit(0);
    } 
    state = 0;
    return pfault;

not_present:
    //con << "page not present, fault addr " << (void *)fault_addr
    //	<< ", ip " << (void *)ip << '\n';
    cpu.cr2 = fault_addr;
    state = 1;
    return (uintptr_t) -1;
permissions_fault:
    //con << "Delivering page fault for addr " << (void *)fault_addr
    //	<< ", permissions " << fault_rwx 
    //<< ", ip " << (void *)ip << '\n';
    cpu.cr2 = fault_addr;
    state = 2;
    return (uintptr_t) -1;
}

void
unset_timer(void)
{
	struct itimerval val;
	int r;

	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 0;

	r = setitimer(ITIMER_REAL, &val, NULL);
	ASSERT(r == 0);
}

void
setup_timer(void)
{
	struct itimerval val;
	int r;

	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 100000; /* 100ms */
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 100000; /* 100ms */

	r = setitimer(ITIMER_REAL, &val, NULL);
	ASSERT(r == 0);
}

extern int physmem_handle;

extern "C"
void
mmap_map(void)
{
	void *r;
#if 1
	con << "MMAP MAP!\n";
	con << "Addr: " << (void*) comm_buffer->addr << " offset: " << (void*) comm_buffer->offset << " length: " << (void*) comm_buffer->length << "\n";
	con << "Prot: " << (void*) comm_buffer->prot << "\n";
#endif
	r = mmap(comm_buffer->addr,
		 comm_buffer->length, 
		 comm_buffer->prot,
		 MAP_SHARED | MAP_FIXED,
		 physmem_handle,
		 comm_buffer->offset);
	ASSERT(r != (void*) -1);
	//con << "pid: " << getpid() << "\n";
}


extern "C"
void 
munmap_all(void)
{
	int r;
	//con << "munmap_all\n";
	r = munmap(0, 0xbf000000);
	ASSERT(r == 0);
	kill(getpid(), SIGSTOP);
}

void backend_async_irq_deliver( intlogic_t &intlogic, ucontext_t* ctxt)
{
	//static const L4_Word_t temp_stack_words = 64;
	//static L4_Word_t temp_stacks[ temp_stack_words * CONFIG_NR_CPUS ];
    cpu_t &cpu = get_cpu();
    //L4_Word_t *stack = (L4_Word_t *)
    //	&temp_stacks[ (vcpu.get_id()+1) * temp_stack_words ];

    word_t vector, irq;
    if( !intlogic.pending_vector(vector, irq) ) {
	// No pending interrupts.
	return;
    }

    bool saved_int_state = cpu.disable_interrupts();
    if( !saved_int_state ) {
	// Interrupts were disabled. 
	//con << "IRQ disabled.. reraise\n";
	intlogic.raise_irq( irq, true ); // Reraise the IRQ.
	return;
    }

#if 0
    if( vcpu.in_dispatch_ipc() ) {
	if( debug_irq_forward )
	    con << "Interrupt forward to dispatch, vector " << vector << '\n';
	msg_vector_build( vector ); 
	// Don't raise the main thread's priority with time slice donation!!
	L4_MsgTag_t tag = L4_Reply( vcpu.main_ltid );
	if( L4_IpcFailed(tag) ) {
	    // The dispatch loop isn't yet listening for IPC.
	    intlogic.raise_irq( irq );
	    cpu.restore_interrupts( saved_int_state );
	}
	// Leave interrupts disabled.
	return;
    }
#endif

    if( debug_irq_deliver )
	con << "Interrupt deliver, vector " << vector << '\n';

    if( vector > cpu.idtr.get_total_gates() ) {
	con << "No IDT entry for vector " << vector << '\n';
	return;
    }

    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    u16_t old_cs = cpu.cs;
    cpu.cs = gate.get_segment_selector();

    if( debug_irq_deliver )
	con << "Delivering vector " << vector
	    << ", handler ip " << (void *)gate.get_offset() << '\n';

#if 0

    stack = &stack[-5]; // Allocate room on the temp stack.
    stack[3] = old_cs;
    stack[0] = gate.get_offset();

    L4_Word_t esp, eip, eflags, dummy;
    L4_ThreadId_t dummy_tid, result_tid;
    L4_ThreadState_t l4_tstate;
    result_tid = L4_ExchangeRegisters( vcpu.main_ltid, (3 << 3) | 2,
	    (L4_Word_t)stack, (L4_Word_t)async_irq_handle_exregs,
	    0, 0, L4_nilthread,
	    &l4_tstate.raw, &esp, &eip, &eflags,
	    &dummy, &dummy_tid );
    ASSERT( !L4_IsNilThread(result_tid) );

    ASSERT( eip != (L4_Word_t)async_irq_handle_exregs );

    flags_t old_flags = cpu.flags;
    old_flags.x.fields.fi = 1; // Interrupts were enabled.
    cpu.flags.prepare_for_interrupt();
    // Note: we disable interrupts even if a trap-gate.

    // Store the old values
    stack[4] = (old_flags.x.raw & ~flags_user_mask) | (eflags & flags_user_mask);
    stack[2] = eip;
    stack[1] = esp;
#endif
    uintptr_t stack_addr = ctxt->uc_mcontext.gregs[REG_ESP];
    uintptr_t *stack = (uintptr_t*)stack_addr;
    flags_t old_flags = cpu.flags;
    old_flags.x.fields.fi = 1; // Interrupts were enabled.
    cpu.flags.prepare_for_interrupt();
    
    stack -= 3;
    stack[2] = (ctxt->uc_mcontext.gregs[REG_EFL] & flags_user_mask) | (old_flags.x.raw & ~flags_user_mask);
    stack[1] = old_cs;
    stack[0] = ctxt->uc_mcontext.gregs[REG_EIP];
    
    ctxt->uc_mcontext.gregs[REG_EIP] = gate.get_offset(); 
    ctxt->uc_mcontext.gregs[REG_ESP] -= 12;
}

extern "C"
void
alarm_handler(int signum, siginfo_t *info, void *context)
{

	uintptr_t timer_irq = 0;
	intlogic_t &intlogic = get_intlogic();
	intlogic.raise_irq(timer_irq);
	backend_async_irq_deliver(intlogic, (ucontext_t* ) context);
}

extern "C"
void
ill_handler(int signum, siginfo_t *info, void *context)
{
	ucontext_t* ctxt = (ucontext_t*) context;
	uintptr_t fault_addr = ctxt->uc_mcontext.gregs[REG_EIP];
	con << "Ill instruction @ "<< (void*) fault_addr << " val " <<  (void*) (*((uintptr_t*) fault_addr)) << "\n";
	
	ASSERT(0);
}

#define PRIV_INSTRUCTION 128


extern "C"
void
segv_handler(int signum, siginfo_t *info, void *context)
{
	ucontext_t* ctxt = (ucontext_t*) context;
	uintptr_t eip = ctxt->uc_mcontext.gregs[REG_EIP];
	uintptr_t fault_addr = (uintptr_t) info->si_addr;
	uintptr_t offset;
	void *r;
	//con << "segv handler\n";
	if (info->si_code == PRIV_INSTRUCTION) {
		con << "Priv instruction @ "<< (void*) fault_addr << "\n";
		exit(1);
	} else {
		uint8_t rwx = 0;
		uintptr_t pagesize;
		uint8_t status;

		if (info->si_code == 1 && eip == fault_addr) {
			rwx = 1;
		} else if (info->si_code == 1) {
			rwx = 4;
		} else {
			rwx = 2;
		}
		con << "Pagefault: " << (void*) fault_addr << " eip: " << (void*) eip << "\n";
		offset = backend_handle_pagefault(fault_addr, eip, rwx, pagesize, status);

		if (status == 0) {
			int prot = 0;
			if (rwx & 0x1) 
				prot |= PROT_EXEC;
			if (rwx & 0x2) 
				prot |= PROT_WRITE;
			if (rwx & 0x4) 
				prot |= PROT_READ;

			r = mmap((void*) (fault_addr & (~ (pagesize - 1))), 
				 pagesize, prot, MAP_SHARED | MAP_FIXED, 
				 physmem_handle,
				 (offset & (~(pagesize - 1))));
			if (r == MAP_FAILED) {
				con << "Failed to map at " << (void*) (fault_addr & (~ (pagesize - 1))) << " EIP: " << (void*) eip << "\n";
			}
			ASSERT(r != MAP_FAILED);
			return;
		} else {
			/* Need to do the idt dance! */
			gate_t *idt = get_cpu().idtr.get_gate_table();
			u16_t old_cs = get_cpu().cs;
			flags_t old_flags = get_cpu().flags;
			gate_t &gate = idt[14];
			//con << "Idt is at " << idt << " "  << (void*) eip << "\n";
			ASSERT(gate.is_trap() || gate.is_interrupt());
			ASSERT(gate.is_present());
			ASSERT(gate.is_32bit());

			if(gate.is_interrupt())
				get_cpu().disable_interrupts();
			get_cpu().flags.prepare_for_interrupt();

			uintptr_t stack_addr = ctxt->uc_mcontext.gregs[REG_ESP];
			uintptr_t *stack = (uintptr_t*)stack_addr;
			get_cpu().cs = gate.get_segment_selector();
			
			stack -= 4;
			stack[3] = (ctxt->uc_mcontext.gregs[REG_EFL] & flags_user_mask) | (old_flags.x.raw & ~flags_user_mask);
			stack[2] = old_cs;
			stack[1] = ctxt->uc_mcontext.gregs[REG_EIP];
			stack[0] = 3; /*error_code; */
			//con << "offset " << (void*) gate.get_offset() << "\n";
			ctxt->uc_mcontext.gregs[REG_EIP] = gate.get_offset(); 
			ctxt->uc_mcontext.gregs[REG_ESP] -= 16;
			return;
		}
	}

	ASSERT(0);
}

void
setup_sighandler(void)
{
	struct sigaction sig_handler;
	stack_t alt_stack_desc;
	int r;

	alt_stack_desc.ss_sp = &alt_stack;
	alt_stack_desc.ss_flags = 0;
	alt_stack_desc.ss_size = sizeof(alt_stack);
	r = sigaltstack(&alt_stack_desc, NULL);
	ASSERT(r == 0);

	sig_handler.sa_flags = SA_ONSTACK | SA_SIGINFO;

	r = sigemptyset(&sig_handler.sa_mask);
	ASSERT(r == 0);

	sig_handler.sa_sigaction = alarm_handler;

	r = sigaction(SIGALRM, &sig_handler, NULL);
	ASSERT(r == 0);

	/* We don't want the alram triggering during SIGILL and SIGSEGV handlers */
	r = sigaddset(&sig_handler.sa_mask, SIGALRM);
	ASSERT(r == 0);

	sig_handler.sa_sigaction = ill_handler;
	r = sigaction(SIGILL, &sig_handler, NULL);
	ASSERT(r == 0);

	sig_handler.sa_sigaction = segv_handler;
	r = sigaction(SIGSEGV, &sig_handler, NULL);
	ASSERT(r == 0);
}

void
child_start(void *entry, char *command_line)
{
	void *sp = (void*) (((uintptr_t) (vm_stack + STACK_SIZE
					  - CONFIG_STACK_SAFETY)) & ~(CONFIG_STACK_ALIGN-1));
	/* Start up a child process */
	child = fork() ;
	if (child == 0) {
		int r;
		con.init( &con_driver, "\e[32mafterburn:\e[0m ");
		con << "Child starting\n";
		r = ptrace(PT_TRACE_ME, getpid(), 0, 0);
		ASSERT(r == 0);
		con << "Child stopped\n";
		kill(getpid(), SIGSTOP);
		ASSERT(!"Shouldn't ever get here");
	} else {
		pid_t waitee;
		int status;
		setup_timer();
		setup_sighandler();
		waitee = waitpid(child, &status, 0);
		ASSERT(waitee == child);
	}
	con << "Backend preboot\n";
	backend_preboot(entry, sp, command_line);
}
