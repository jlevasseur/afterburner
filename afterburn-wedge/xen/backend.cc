/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/backend.cc
 * Description:   
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: backend.cc,v 1.14 2005/04/14 19:54:23 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)

#include INC_WEDGE(hypervisor.h)
#include <templates.h>
#include <memory.h>
#include "burnxen.h"

extern unsigned long *pgd;
unsigned long evtchn_to_irq[1024];
unsigned long irq_to_evtchn[255];

extern bool debug_set_pte;
static const bool debug_pdir_flush=0;
static const bool debug_global_page=0;
static const bool debug_user_pfault=true;

bool exit_hook_called = false;
unsigned long exit_old_pdir;

void mach_swap(long frame, long phys);
int find_next_interrupt(void);

extern unsigned long *pagefault_handler;

extern "C"
void evtchn_do_upcall(void);

extern void switch_as(unsigned long offset);
extern void switch_as_initial(unsigned long offset);

void
backend_mask_device_interrupt( word_t masked_irq )
{
	unsigned long port;
	port = irq_to_evtchn[masked_irq];
	con << "Masking: " << masked_irq << " " << port << "\n";
	HYPERVISOR_shared_info->evtchn_mask[port >> 5] |= ((1 << (port & 0x1f))); /* Mask */
}

void
backend_unmask_device_interrupt( word_t masked_irq )
{
	unsigned long port;
	physdev_op_t op;
	
	port = irq_to_evtchn[masked_irq];
#if 0
	if (port == 5) {
	con << "Unmasking: " << masked_irq << " " << port << "\n";
	con << "pending: " << (HYPERVISOR_shared_info->evtchn_pending[port >> 5] & ((1 << (port & 0x1f)))) 
	    << " mask: " << (HYPERVISOR_shared_info->evtchn_mask[port >> 5] & ((1 << (port & 0x1f)))) 
	    << " global mask: " << 	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask
	    << " global pend: " << 	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending
	    << "\n";
	}
#endif
	HYPERVISOR_shared_info->evtchn_mask[port >> 5] &= (~(1 << (port & 0x1f))); /* UnMask */
	if (masked_irq != 0) {
		op.cmd = PHYSDEVOP_IRQ_UNMASK_NOTIFY;
		//op.u.irq = masked_irq;
		(void)HYPERVISOR_physdev_op(&op);
	}
	
	/* Check pending? */
#if 0
	if (port == 5)
	con << "pending: " << (HYPERVISOR_shared_info->evtchn_pending[port >> 5] & ((1 << (port & 0x1f)))) 
	    << " mask: " << (HYPERVISOR_shared_info->evtchn_mask[port >> 5] & ((1 << (port & 0x1f)))) 
	    << " global mask: " << 	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask
	    << " global pend: " << 	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending
	    << "\n";
#endif
}

void
backend_irq_enable(void)
{
	//cpu_t &cpu = get_cpu();
	//int irq;
	con << "Enabled interrupts... better serach for the next event from Xen\n";
	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
#if 0
	irq = find_next_interrupt();
	if (irq == -1) {
		return;
	}

	con << "irq: " << irq << "\n";

	gate_t *idt = cpu.idtr.get_gate_table();
	gate_t &gate = idt[irq]; /* Timer */
	
	ASSERT( gate.is_trap() || gate.is_interrupt() );
	ASSERT( gate.is_present() );
	ASSERT( gate.is_32bit() );

	flags_t old_flags = cpu.flags;
	cpu.flags.prepare_for_interrupt(); /* FIMXE : only if it is an interrupt...
					      if it is a trap this is different */
	cpu.flags.x.fields.fi = 0; /* Disable irq */

	u16_t old_cs = cpu.cs;
	cpu.cs = gate.get_segment_selector();


	__asm__ __volatile__ (
			      "pushl	%0 ;"
			      "pushl	%1 ;"
			      "pushl	$1f ;"
			      "jmp	*%2 ;"
			      "1:"
			      :
			      : "r"(old_flags.x.raw), 
				"r"((u32_t)old_cs),
				"r"(gate.get_offset())
			      : "flags", "memory" );
	//con << "Finished handling irq enable\n";
#endif
}

word_t
backend_sync_flush_page(word_t addr, word_t bits, word_t permissions)
{
	con << "flush page\n";
	return 0;
}

void backend_flush_vaddr( word_t vaddr )
{
	//int suc;
	//mmu_update_t req;
//	con << "flush_vaddr " << (void*)vaddr << '\n';
	if (debug_set_pte)
		con << "flush vaddr\n";
	/*
	req.ptr = vaddr & (~0xfff) | MMU_EXTENDED_COMMAND;
	req.val = MMUEXT_INVLPG;
	HYPERVISOR_mmu_update(&req, 1, &suc);
	ASSERT(suc == 1);
	*/
	queue_update(vaddr & (~0xfff) | MMU_EXTENDED_COMMAND, MMUEXT_INVLPG);
	flush_page_update_queue();

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
	con << "flush all\n";
}

void backend_exit_hook( void *handle )
{
	exit_hook_called = true;
	exit_old_pdir = get_cpu().cr3.get_pdir_addr();
	//debug_vm << "EXIT HOOK CALLED: " << exit_old_pdir << "\n";

}

void backend_reboot( void )
{
	con << "Reboot ...";
}

unsigned long tmp[0x1000/sizeof(long)];

bool in_idle = false;
extern unsigned long the_old_pdir_paddr;


void
backend_enable_paging(word_t *ret_address)
{
	get_vcpu().set_phys_pdir(get_cpu().cr3.get_pdir_addr());
	get_vcpu().set_kernel_vaddr(0x80000000);
	switch_as_initial(0);
}


void backend_flush_user( void )
{
	con << "flush user\n";
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

extern unsigned long evtchn_to_irq[];


/*
   This is really evil!
   
   In bind pirq we set new_irq to the new irq being bound.
   We may get an interrupt from this port before the 
   evtchn_to_irq data structures can be updated. (This is mainly
   because Xen allocates port numbers not the operating system,
   which is really brain dead!).

   So if we get an interrupt, and can't find it, and new_irq is
   set, then we can safely assume we were just setting a new irq
   up, and in this case the interrupt code does the patchup for 
   us.

   Like I said -- evil!
*/

long new_irq;

int bind_pirq(int irq)
{
    evtchn_op_t op;
    int evtchn = 0;
    int ret;
    new_irq = irq;
    op.cmd              = EVTCHNOP_bind_pirq;
    op.u.bind_pirq.pirq = irq;
    op.u.bind_pirq.flags = 0;
    ret = HYPERVISOR_event_channel_op(&op);
    evtchn = op.u.bind_pirq.port;
    ASSERT(ret == 0);
    evtchn_to_irq[evtchn] = irq;
    irq_to_evtchn[irq] = evtchn;
    new_irq = -1;
    con << "Bound " << irq << " to " << evtchn << " "
	<< " " << (HYPERVISOR_shared_info->evtchn_mask[evtchn >> 5] & ((1 << (evtchn & 0x1f)))) /* UnMask */
	<< " " << (HYPERVISOR_shared_info->evtchn_pending[evtchn >> 5] & ((1 << (evtchn & 0x1f)))) /* UnMask */
	<< "\n";
    physdev_op_t op2;
    op2.cmd = PHYSDEVOP_IRQ_STATUS_QUERY;
    op2.u.irq_status_query.irq = irq;
    (void)HYPERVISOR_physdev_op(&op2);
    con << "Status: " << op2.u.irq_status_query.flags << "\n";
    return 0;
}


bool backend_enable_device_interrupt( u32_t interrupt )
{
	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;

	if (interrupt == 2)
		return true;

	if (interrupt == 0) {
	    evtchn_to_irq[0] = 0;
	    irq_to_evtchn[0] = 0;
	    return true;
	}
	    
	bind_pirq(interrupt);

	return true;
}

void backend_check_pending_interrupts( void )
{
	con << "check pending\n";
}


void backend_interruptible_idle( void )
{
    ASSERT( get_cpu().interrupts_enabled() );
    get_intlogic().deliver_synchronous_irq();
#if 0
    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 1;
    u64 foo = HYPERVISOR_shared_info->system_time + 10000000;
    if (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending == 1) {
	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
	HYPERVISOR_xen_version( 0 );
   } else {
	ASSERT( HYPERVISOR_set_timer_op(&foo) == 0 );
	HYPERVISOR_block();
    }
#endif
	/* This needs to be atomic with checking interrupts */
	/*irq_or_block(); */
	//con <<"Halt\n";
	//HYPERVISOR_block();
	//con << "idle\n";
}
bool debug_kernel_sync_vector = false;

extern bool debug_iret;
extern bool debug_write_cs;
extern bool debug_iret_redirect;

void unset_timer(void);


#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))

static inline int ffs(int x)
{
	int r;

	__asm__("bsfl %1,%0\n\t"
		"jnz 1f\n\t"
		"movl $-1,%0\n"
		"1:" : "=r" (r) : "rm" (x));
	return r+1;
}

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			__asm__ __volatile__("xchgb %b0,%1"
				:"=q" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 2:
			__asm__ __volatile__("xchgw %w0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 4:
			__asm__ __volatile__("xchgl %0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
	}
	return x;
}

#define __KERNEL_DS 0x0821

extern "C" void no_pending(void);
extern int irq_count, irq_0_count;
extern "C"
void xen_upcall(unsigned long *regs)
{
    //* Interrupts are disabled here */
    word_t vector;
    int irq;
    word_t the_irq;
    intlogic_t &intlogic = get_intlogic();
    cpu_t &cpu = get_cpu();
    bool to_user = false;
    irq_count++;
    ASSERT( HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask != 0 );
    if (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending != 1) {
	    con << "Warning.. interrupt but no interrupts found: " << (void*)regs[10] << "\n";
	    return;
    }
    //con << "#";
    //if (regs[10] == (unsigned long) &no_pending)
    //con << "$" << (void*) regs[10] << "\n";
    //ASSERT ((unsigned long) regs < 0xbf000000);
    //con << "############### XEN UPCALL ############\n";
    int irqs_found = 0;
 find_irqs:
    while ( (irq = find_next_interrupt()) != -1) {
	    if (irq == 0) {
		    irq_0_count++;
	    }
	    irqs_found++;
	    intlogic.raise_irq(irq);
    }
    /* We don't want to do this if we are going to suer */
#if 0
    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
    if (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending != 0) {
	    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 1;
	    goto find_irqs;
    }
#endif

    /* Now interrupts are enabled */
    if (!cpu.interrupts_enabled()) {
	    /* need to re-enable here */
	    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
	    if (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending != 0) {
		    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 1;
		    goto find_irqs;
	    }
	    return;
    }

    //con << " global mask: " << 	HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask << "\n";
		       

    if( !intlogic.pending_vector(vector, the_irq) ) {
	// No pending interrupts.
	    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
	    if (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending != 0) {
		    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 1;
		    goto find_irqs;
	    }
	return;
    }
    //con << "Found pending: " << the_irq << "\n";
    flags_t old_flags = cpu.flags;

    bool saved_int_state = cpu.disable_interrupts();
    if( !saved_int_state ) {
	PANIC("We expected to be able to deliver if required\n");
    }

    //if( debug_irq_deliver )
    //	con << "Interrupt deliver, vector " << vector << '\n';

    if( vector > cpu.idtr.get_total_gates() ) {
	con << "No IDT entry for vector " << vector << '\n';
	return;
    }

    //con << "^";
    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    cpu.flags.prepare_for_interrupt();

    u16_t old_cs = cpu.cs;
    cpu.cs = gate.get_segment_selector();

    regs[9] = gate.get_offset(); // new IP

    if (cpu_t::get_segment_privilege(old_cs) !=
	cpu_t::get_segment_privilege(cpu.cs))
    {
	// ring switch
	//con << "ring switch old cs=" << old_cs << " newcs=" << cpu.cs << "\n";
	//con << "@";

	ASSERT(cpu_t::get_segment_privilege(cpu.cs)==0);
	regs[14] = cpu.ss;
	cpu.ss = cpu.get_tss()->ss0;
	to_user = true;

    }

    // store old CS on stack
    regs[11] = (u32_t)old_cs;

    //con << "old_flags: "<< (void*) old_flags.x.raw << "\n";
    regs[12] = (old_flags.x.raw & ~flags_user_mask) | (regs[12] & flags_user_mask);

    if (gate.is_interrupt())
	cpu.flags.x.fields.fi = 0;
    else
	cpu.flags.x.fields.fi = 1;

	if (to_user) {
		unsigned long *new_stack = (unsigned long *) cpu.get_tss()->esp0;
		//con << "I.New stack: " << new_stack << "\n";
		new_stack--;
		*(new_stack--) = regs[14];
		*(new_stack--) = regs[13];
		*(new_stack--) = regs[12];
		*(new_stack--) = regs[11];
		*(new_stack--) = regs[10];
		*(new_stack--) = gate.get_offset(); // new IP
		new_stack++;
		regs[9] = (unsigned long) (new_stack);
		//con << "I.blerg: " << gate.get_offset() << "\n";
		//con << "I.stack now: " << new_stack << " " << (void*) *new_stack << " " << (void*) new_stack[1] << "\n";

		__asm__ __volatile__ (
				      "movl	%0, %%esp ;"	// Switch stack
				      "popl     %%eax; " 
				      "popl     %%ecx; " 
				      "popl     %%edx; " 
				      "popl     %%ebx; " 
				      "popl     %%ebp; " 
				      "popl     %%esi; " 
				      "popl     %%edi; " 
				      "popl     %%ds; " 
				      "popl     %%es; " 
				      "popl     %%esp; " 
				      "pushl    %%eax; "
				      "movl     HYPERVISOR_shared_info, %%eax; "
				      "movb     $0,1(%%eax); "
				      "cmpb     $1,0(%%eax); "
				      "jne      no_pending1;"
				      "movb     $1,1(%%eax); "   /* Set things as masked */
				      "popl     %%eax; "
				      "pushfl; "
				      "push     %%cs; "
				      "call     hypervisor_callback; "
				      "no_pending1: "
				      "popl     %%eax; "
				      "ret; " 
				      :
				      : "r"(regs));
	} else {
		ASSERT ((unsigned long ) regs < 0xbf000000);
		__asm__ __volatile__ (
				      "movl     %0, %%esp ;"    // Switch stack
				      "popl     %%eax; " 
				      "popl     %%ecx; " 
				      "popl     %%edx; " 
				      "popl     %%ebx; " 
				      "popl     %%ebp; " 
				      "popl     %%esi; " 
				      "popl     %%edi; " 
				      "popl     %%ds; " 
				      "popl     %%es; " 

				      "pushl    %%eax; "
				      "movl     HYPERVISOR_shared_info, %%eax; "
				      "movb     $0,1(%%eax); "
				      "cmpb     $1,0(%%eax); "
				      "jne      no_pending2;"
				      "movb     $1,1(%%eax); "   /* Set things as masked */
				      "popl     %%eax; "
				      "pushfl; "
				      "push     %%cs; "
				      "call     hypervisor_callback; "

				      "no_pending2:"
				      "popl     %%eax; "
				      "ret; " 
				      :
				      : "r"(regs));
	}
	ASSERT("Shouldn't get here\n");
	while(1);
}

/* Find next interrupt */
int
find_next_interrupt(void)
{
	unsigned int port;
	unsigned long irq;
	shared_info_t *s = HYPERVISOR_shared_info;
	vcpu_t &vcpu = get_vcpu();

	if (vcpu.l2 == 0) {
		if (vcpu.l1 == 0) {
			if (s->vcpu_data[0].evtchn_upcall_pending == 0) {
				return -1;
			}
			s->vcpu_data[0].evtchn_upcall_pending = 0;
			vcpu.l1 = xchg(&s->evtchn_pending_sel, 0);
			ASSERT(vcpu.l1 != 0);
		}
		vcpu.l1i = ffs(vcpu.l1);
		ASSERT( vcpu.l1i );
		vcpu.l1i--;

		vcpu.l1 &= ~(1 << vcpu.l1i);
		vcpu.l2 = s->evtchn_pending[vcpu.l1i] & ~s->evtchn_mask[vcpu.l1i];
	}
	vcpu.l2i = ffs(vcpu.l2);
	vcpu.l2i--;
	vcpu.l2 &= ~(1 << vcpu.l2i);
	port = (vcpu.l1i << 5) + vcpu.l2i;
	HYPERVISOR_shared_info->evtchn_pending[vcpu.l1i] &= (~ (1 << vcpu.l2i)); /* Ack */
	irq = evtchn_to_irq[port];
	if (irq == (unsigned long) -1) {
		irq = new_irq;
		new_irq = -1;
		evtchn_to_irq[port] = new_irq;
		irq_to_evtchn[new_irq] = port;
	}
	ASSERT(irq != (unsigned long) -1);
	return irq;
}
