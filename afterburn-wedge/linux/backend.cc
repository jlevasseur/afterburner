
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(comms.h)

#include <stdlib.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
//#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/unistd.h>

#include <templates.h>
#include <memory.h>

#define PTRACE_GETSIGINFO	0x4202
#define PTRACE_SETSIGINFO	0x4203

static const bool debug_pdir_flush=0;
static const bool debug_global_page=0;
static const bool debug_user_pfault=true;
static bool child_need_to_wait = false;

#define PT_MODE PTRACE_SYSCALL

extern "C" void mmap_map(void);
extern "C" void munmap_all(void);
extern "C" void mmap_jmp(void);

word_t
backend_sync_flush_page(word_t addr, word_t bits, word_t permissions)
{
	con << "flush page\n";
	return 0;
}

word_t
backend_sync_unmap_page(word_t addr, word_t bits, word_t permissions)
{
	con << "unmap page\n";
	return 0;
}


void
flush_all(void)
{
	int r;
	/* For now flush all */
	r = munmap(0, 0xbf000000);
	ASSERT(r == 0);
	//con << "Flush user\n";
	backend_flush_user();
	//con << "Flushed\n";
}

void backend_exit_hook( void *handle )
{
	con << "Exit hook\n";
	while(1);
}

extern uintptr_t 
backend_handle_pagefault(uintptr_t fault_addr,
			 uintptr_t ip,
			 uint8_t & rwx,
			 uintptr_t & pagesize,
			 uint8_t & state);

extern pid_t child;

bool in_idle = false;

void
backend_enable_paging( word_t *ret_address )
{
	con << "Enable paging " << ret_address << "\n";
}


void backend_flush_user( void )
{
	struct user_regs_struct in;
	pid_t waitee;
	int status;
	int r;
	//con << "backend_flush_user\n";
	//r = munmap(0, 0xbf000000);
	//ASSERT(r == 0);
	
	if (child_need_to_wait) {
		waitee = waitpid(child, &status, 0);
		ASSERT(waitee == child);
	}

	r = ptrace(PT_GETREGS, child, NULL, &in);
	ASSERT(r == 0);
	comm_buffer->fn = (void*) munmap_all;
	in.eip = (uintptr_t) mmap_jmp;
	r = ptrace(PT_SETREGS, child, NULL, &in);
	ASSERT(r == 0);
	r = ptrace(PT_SYSCALL, child, NULL, NULL);
	ASSERT(r == 0);
	//con << "waiting\n";
	while(1) {
		waitee = waitpid(child, &status, 0);
		ASSERT(waitee == child);
		ASSERT(WIFSTOPPED(status));
		r = ptrace(PT_GETREGS, child, NULL, &in);
		ASSERT(r == 0);
		/* Evil haqery */
		if (WSTOPSIG(status) == 19) {
			break;
		}
		//con << "waitted " << WSTOPSIG(status) << " " << (void*) in.eip << "\n";
		r = ptrace(PT_SYSCALL, child, NULL, NULL);
		ASSERT(r == 0);
	}
	child_need_to_wait = false;
}

void backend_flush_page( u32_t addr )
{
	con << "flush page\n";
}

void backend_flush_ptab( u32_t addr )
{
	con << "flush  ptab\n";
}

void backend_flush_superpage( u32_t addr )
{
	con << "flush super page\n";
}

void backend_flush_old_ptab( word_t vaddr, 
	pgent_t *new_ptab, pgent_t *old_ptab )
{
	con << "flush old ptab\n";
}

bool backend_enable_device_interrupt( u32_t interrupt )
{
	return false;
}

void backend_check_pending_interrupts( void )
{
}


void backend_interruptible_idle( void )
{
	pause();
}

bool debug_kernel_sync_vector = false;

extern bool debug_iret;
extern bool debug_write_cs;
extern bool debug_iret_redirect;

void unset_timer(void);

void
backend_activate_user( user_frame_t *user_frame, word_t ip )
{
	//con << " iret to user \n";
	int r;
	pid_t waitee;
	int status;
	struct user_regs_struct in;
	cpu_t &cpu = get_cpu();
	//debug_iret = true;
	unset_timer();

#if 1
	con << "Request to enter user from ip " << (void *)ip 
	    << ", to ip " << (void *)user_frame->iret->ip
	    << ", to sp " << (void *)user_frame->iret->sp
	    << ", esp " << __builtin_frame_address(0) << '\n';

#endif
	//con << "activate user " << (void*) ip << "\n";
	//con << "Activating user..." << child_need_to_wait << "\n";
	if (child_need_to_wait) {
		waitee = waitpid(child, &status, 0);
		ASSERT(waitee == child);
#if 0
		con << "--child: " << child << " " << "waitee: " << waitee << "\n";
		con << "--status: " << status << "\n";
		if (WIFSIGNALED(status)) {
			con << "/signalled -- " << WTERMSIG(status) << '\n';
		}
		if (WIFSTOPPED(status)) {
			con << "/stopped -- " << WSTOPSIG(status) << '\n';
		}
#endif
	}
	r = ptrace(PT_GETREGS, child, NULL, &in);
	ASSERT(r == 0);

	cpu.cs = user_frame->iret->cs;
	cpu.flags = user_frame->iret->flags;
	cpu.ss = user_frame->iret->ss;

	in.eax = user_frame->regs->x.fields.eax;
	in.ecx = user_frame->regs->x.fields.ecx;
	in.edx = user_frame->regs->x.fields.edx;
	in.ebx = user_frame->regs->x.fields.ebx;
	in.ebp = user_frame->regs->x.fields.ebp;
	in.esi = user_frame->regs->x.fields.esi;
	in.edi = user_frame->regs->x.fields.edi;
	in.eflags = user_frame->regs->x.fields.flags.x.raw;
	in.eip = user_frame->iret->ip;
	in.esp = user_frame->iret->sp;
#if 0
	con << " +EIP: " << (void*) in.eip 
	    << " ESP: " << (void*) in.esp 
	    << " EAX: " << (void*) in.eax 
	    << " EBX: " << (void*) in.ebx 
	    << " ECX: " << (void*) in.ecx 
	    << " EDX: " << (void*) in.edx 
	    << " CS:  " << cpu.cs << '\n';

	con << "Setting regs\n";
#endif
	r = ptrace(PT_SETREGS, child, NULL, &in);
	ASSERT(r == 0);
#if 0
	con << "PT_SYSCALL\n";
	sleep(3);
#endif
	r = ptrace(PT_MODE, child, NULL, NULL);
	ASSERT(r == 0);
	while(1) {
		siginfo_t siginfo;
		uintptr_t offset= 0;
		uintptr_t pagesize=0 ;
		uint8_t state =0;
		uint8_t fault_rwx, rwx;
		int sig;
#if 0
			con << "waiting on pid -- "
			    << " cs:  " << cpu.cs << '\n';
#endif
		waitee = waitpid(child, &status, 0);

		ASSERT(waitee == child);

		if (WIFSIGNALED(status)) {
			con << "signalled -- " << WTERMSIG(status) << '\n';
			r = ptrace(PT_GETREGS, child, NULL, &in);
			ASSERT(r == 0);
#if 0
			con << "waitted on pid -- eip: " << (void*)in.eip 
			    << " esp:" << (void*)in.esp 
			    << " eax:" << (void*)in.eax 
			    << " ebx:" << (void*)in.ebx 
			    << " ecx:" << (void*)in.ecx 
			    << " edx:" << (void*)in.edx 
			    << " cs:  " << cpu.cs << '\n';
#endif
			sig = WTERMSIG(status);
			ASSERT(!"Shouldn't let the child terminate\n");
		} else
		if (WIFSTOPPED(status)) {
			//con << "stopped -- " << WSTOPSIG(status) << '\n';
			sig = WSTOPSIG(status);
		} else {
			con << "badness status\n";
			exit(0);
		}
		
		if (sig == SIGSEGV) {
			r = ptrace(PT_GETREGS, child, NULL, &in);
			ASSERT(r == 0);
#if 1
			con << "waitted on pid -- eip: " << (void*)in.eip 
			    << " esp:" << (void*)in.esp 
			    << " eax:" << (void*)in.eax 
			    << " ebx:" << (void*)in.ebx 
			    << " ecx:" << (void*)in.ecx 
			    << " edx:" << (void*)in.edx 
			    << " cs:  " << cpu.cs << '\n';
#endif
			r = ptrace((__ptrace_request) PTRACE_GETSIGINFO, child, NULL, &siginfo);
			if (r != 0) {
				con << "something reallly bad happened somehow :/\n";
			}


			ASSERT(r == 0);
			con << "got siginfo " << siginfo.si_addr << ' ' << siginfo.si_code << "\n";

			if (siginfo.si_code == SEGV_MAPERR && (uintptr_t) in.eip == (uintptr_t) siginfo.si_addr) {
				rwx = 1;
			} else if (siginfo.si_code == SEGV_MAPERR) {
				rwx = 4;
			} else {
				rwx = 2;
			}

			//con << "backend handle pagefault " << cpu.cs << " " << get_cpu().cs << "\n";
			//backend_handle_user_pagefault(0, 
#if 0
			offset = backend_handle_pagefault((uintptr_t) siginfo.si_addr,
							  in.eip,
							  rwx,
							  pagesize,
							  state);
#endif
			//con << "done pagefault handle " << cpu.cs << " " << get_cpu().cs << "\n";
			if (state == 0) {
				//con << "Lookup: " << (void*) offset << "\n";
				/* Do a dance */
				comm_buffer->old_ip = in.eip;
				comm_buffer->old_sp = in.esp;
				comm_buffer->fn = (void*) mmap_map;
				comm_buffer->prot = 0;
				if (rwx & 0x1) 
					comm_buffer->prot |= PROT_EXEC;
				if (rwx & 0x2) 
					comm_buffer->prot |= PROT_WRITE;
				if (rwx & 0x4) 
					comm_buffer->prot |= PROT_READ;
				fault_rwx = rwx;
				comm_buffer->length = pagesize;
				comm_buffer->addr = (void*) ((uintptr_t) siginfo.si_addr & (~ (pagesize - 1)));
				comm_buffer->offset = (offset & (~(pagesize - 1)));
				in.eip = (long) mmap_jmp;
				/* Sneding thingy */
				r = ptrace(PT_SETREGS, child, NULL, &in);
				ASSERT(r == 0);
#if 0
				con << " EIP: " << (void*) in.eip 
				    << " ESP: " << (void*) in.esp 
				    << " EAX: " << (void*) in.eax 
				    << " EBX: " << (void*) in.ebx 
				    << " ECX: " << (void*) in.ecx 
				    << " EDX: " << (void*) in.edx 
				    << " CS:  " << get_cpu().cs << '\n';
#endif
				r = ptrace(PT_MODE, child, NULL, NULL);
				ASSERT(r == 0);
			} else {
				//con << "pagefault: " << state << " \n";
				child_need_to_wait = false;
#if 0
				con << " *EIP: " << (void*) in.eip 
				    << " ESP: " << (void*) in.esp 
				    << " EAX: " << (void*) in.eax 
				    << " EBX: " << (void*) in.ebx 
				    << " ECX: " << (void*) in.ecx 
				    << " EDX: " << (void*) in.edx 
				    << " CS:  " << get_cpu().cs << '\n';
#endif

#if 0
				/* hmmm... */
				if (state == 1) 
					deliver_ia32_user_vector(cpu, 14, true, 4 | (0), &in);
				else
					deliver_ia32_user_vector(cpu, 14, true, 4 | (3), &in);
				/* Now i send iret */
#endif
				ASSERT(!"Shouldn't get here\n");
			}
		} else if (sig == SIGTRAP) {
			r = ptrace(PT_GETREGS, child, NULL, &in);
			ASSERT(r == 0);
			int call = in.orig_eax;

			if (call == -1) {
				word_t value = ptrace(PTRACE_PEEKDATA, child, in.eip, NULL) & 0xffff;
				if ((unsigned) in.eip < 0xbf000000UL) {
					con << "Trapped a signal " << (void*) in.eip << " - " << call << " " << (void*) in.ebx << "\n";
				} else {
					//con << "::TRAPPED a signal " << (void*) in.eip << " - " << call << " " << (void*) in.ebx << " " << (void*) value << "\n";
				}
				if (value == 0x80cd) {
					ptrace(PT_SYSCALL, child, 0, 0);
				} else {
					ptrace(PT_MODE, child, 0, 0);
				}
				continue;
			}
			
			if ((unsigned) in.eip > 0xbf000000UL) {
				/* Any code in the magic area proceeds without
				   a problem */
				//con << "!!TRAPPED a signal " << (void*) in.eip << " - " << call << " " << (void*) in.ebx << "\n";
				r = ptrace(PT_SYSCALL, child, 0, 0);
				ASSERT(r == 0);
				waitee = waitpid(child, &status, 0);
				ASSERT(waitee == child);
				ASSERT(WIFSTOPPED(status));
				r = ptrace(PT_MODE, child, NULL, NULL);
				ASSERT(r == 0);
				continue;
			} else {
				con << "Trapped a signal " << (void*) in.eip << " - " << call << " " << (void*) in.ebx << "\n";
				/* Now we change to a getpid() */
				in.orig_eax = __NR_getpid; /* Rewrite the thing */
				r = ptrace(PT_SETREGS, child, NULL, &in);
				ASSERT(r == 0);
				r = ptrace(PT_SYSCALL, child, 0, 0);
				ASSERT(r == 0);
				waitee = waitpid(child, &status, 0);
				ASSERT(waitee == child);
				ASSERT(WIFSTOPPED(status));
				//con << "***************************** Syscall gone through returned: " << in.eax << " " << in.orig_eax << "\n";
				/* Here we need to call the actual linux kernel */
				in.eax = call;
#if 0
				deliver_ia32_user_vector(cpu, 0x80, false, 0, &in);
#endif
			}
		} else {
			con << "FOO BAR BAZ\n";
			while(1);
		}
	}

	exit(0);
}

word_t backend_phys_to_dma_hook( word_t phys )
{
	con << "backendphysotdmahook\n";
	return 0;
}

word_t backend_dma_to_phys_hook( word_t dma )
{
	//con << "backenddmatophyshook\n";
	return 0;
}
