/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/trap.cc
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
 * $Id: trap.cc,v 1.12 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(hypervisor.h)
#include "burnxen.h"

extern "C" void divide_error(void);
extern "C" void debug_exception(void);
extern "C" void int3(void);
extern "C" void overflow(void);
extern "C" void bounds(void);
extern "C" void invalid_op(void);
extern "C" void device_not_available(void);
extern "C" void double_fault(void);
extern "C" void coprocessor_segment_overrun(void);
extern "C" void invalid_TSS(void);
extern "C" void segment_not_present(void);
extern "C" void stack_segment(void);
extern "C" void general_protection(void);
extern "C" void pagefault(void);
extern "C" void spurious_interrupt_bug(void);
extern "C" void coprocessor_error(void);
extern "C" void alignment_check(void);
extern "C" void machine_check(void);
extern "C" void simd_coprocessor_error(void);
extern "C" void syscall_exception(void);

#define __KERNEL_DS 0x0821


extern bool debug_set_pte;

extern "C"
void
we_are_stuffed(void)
{
	con << "We are in trouble\n";
	while(1);
}

unsigned long fuzzbucket = 0;
extern int pf_count;

extern "C"
void
handle_exception(unsigned long except, unsigned long *regs)
{
	cpu_t &cpu = get_cpu();

	gate_t *idt = cpu.idtr.get_gate_table();
	gate_t &gate = idt[ except ];
	int offset = 0;
	bool to_user = false;

#if 0
	if (fuzzbucket) {
		con << "Evil fixup shit -- " << (void*) fuzzbucket << "\n";
		fuzzbucket = 0;
	}
#endif
	//con << "exception " << except << "\n";

	switch(except) {
	case 8:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 17:
		offset = 1;
	}

	if (except == 14) { /*pagefault exception */
		pf_count++;
		if (debug_set_pte)
			con << "Pagefault: " << (void*) regs[9] << "\n";
		get_cpu().cr2 = regs[9];
		if (regs[9] > get_vcpu().get_kernel_vaddr() && regs[9] < get_vcpu().get_kernel_vaddr() + memory_size) {
			unsigned long pfn = virt_to_phys(regs[9]) >> PAGE_BITS;
			con << "KERNEL PAGEFAULT: " << (void*) regs[9] << " " << (void*) regs[11] << " " <<
				pfn_pt_count(virt_to_phys(regs[9]) >> PAGE_BITS) << "\n";
			if (mfn_to_pfn(pfn_to_mfn(pfn)) != pfn) {
				con << "BAD: pfn: " << (void*) pfn <<
					" pfn_to_mfn: " << (void*) pfn_to_mfn(pfn) <<
					" mfn_to_pfn: " << (void*) mfn_to_pfn(pfn_to_mfn(pfn)) << "\n";
			}
			ASSERT(mfn_to_pfn(pfn_to_mfn(virt_to_phys(regs[9]) >> PAGE_BITS)) == virt_to_phys(regs[9]) >> PAGE_BITS);
			ASSERT(pfn_pt_count(virt_to_phys(regs[9]) >> PAGE_BITS) == 0);
		}
	}
	if (except == 0x80) {
#if 0
		con << "Exception: " << (void*) except << " " << (void*) regs[10+offset] << " " << (void*) regs[13+offset] << "\n";
		for(int i=0; i <8; i++)
			con << "Reg: " << i << " " << (void*) regs[i] << "\n";
#endif
	}
	if (except == 13) {
		con << "Badnes...\n";
		con << "Exception: " << except << " " << (void*) regs[10+offset] << " " << (void*) regs[13+offset] << "\n";
#if 0
		for(int i=0; i <15; i++)
			con << "Reg: " << i << " " << (void*) regs[i] << "\n";
		while(1);
#endif
	}


	ASSERT( gate.is_trap() || gate.is_interrupt() );
	ASSERT( gate.is_present() );
	ASSERT( gate.is_32bit() );

	flags_t old_flags = cpu.flags;
	cpu.flags.prepare_for_interrupt();

	u16_t old_cs = cpu.cs;
	cpu.cs = gate.get_segment_selector();

	if (cpu_t::get_segment_privilege(old_cs) !=
	    cpu_t::get_segment_privilege(cpu.cs))
	{
	    // ring switch
	    //con << "ring switch old cs=" << old_cs << " newcs=" << cpu.cs << "\n";
	    ASSERT(cpu_t::get_segment_privilege(cpu.cs)==0);
	    regs[14+offset] = cpu.ss;
 	    cpu.ss = cpu.get_tss()->ss0;
	    to_user = true;
	} else {
		//HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = 0;
		//ASSERT(HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending == 0);
	}
 
	// store old CS on stack
	regs[11+offset] = (u32_t)old_cs;

	//con << "CS=" << (void*)regs[11+offset] << " SS=" << (void*)regs[13+offset] << "\n";

	//con << "old_flags: "<< (void*) old_flags.x.raw << "\n";
	regs[12+offset] = (old_flags.x.raw & ~flags_user_mask) | (regs[12+offset] & flags_user_mask);

	// update IRQs according to gate configuration
	if (gate.is_interrupt())
	    cpu.flags.x.fields.fi = 0;
	else
	    cpu.flags.x.fields.fi = 1;


	if (to_user) {
		unsigned long *new_stack = (unsigned long *) cpu.get_tss()->esp0;
		//con << "New stack: " << new_stack << "\n";
		new_stack--;
		*(new_stack--) = regs[14+offset];
		*(new_stack--) = regs[13+offset];
		*(new_stack--) = regs[12+offset];
		*(new_stack--) = regs[11+offset];
		*(new_stack--) = regs[10+offset];
		if (offset)
			*(new_stack--) = regs[10];
		*(new_stack--) = gate.get_offset(); // new IP
		new_stack++;
		regs[9] = (unsigned long) (new_stack);
		//con << "blerg: " << gate.get_offset() << "\n";
		//con << "stack now: " << new_stack << " " << (void*) *new_stack << " " << (void*) new_stack[1] << "\n";

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
				      "pushfl; "
				      "push     %%cs; "
				      "call     hypervisor_callback; "
				      "no_pending1: popl     %%eax; "
				      "ret; " 
				      :
				      : "r"(regs));
	} else {
		regs[9] = gate.get_offset(); // new IP

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
				      "pushl    %%eax; "
				      "movl     HYPERVISOR_shared_info, %%eax; "
				      "movb     $0,1(%%eax); "
				      "cmpb     $1,0(%%eax); "
				      "jne      no_pending2;"
				      "movb     $1,1(%%eax); "   /* Set things as masked */
				      "pushfl; "
				      "push     %%cs; "
				      "call     hypervisor_callback; "
				      "no_pending2: popl     %%eax; "
				      "ret; " 
				      :
				      : "r"(regs));
	}
	ASSERT("Shouldn't get here\n");
	while(1);
}

#define DISABLE_IRQ 4

static trap_info_t trap_table[] = {
    {  0, 0 | DISABLE_IRQ, __KERNEL_CS, (unsigned long)divide_error                },
    {  1, 0 | DISABLE_IRQ, __KERNEL_CS, (unsigned long)debug_exception            },
    {  3, 3| DISABLE_IRQ, __KERNEL_CS, (unsigned long)int3                        },
    {  4, 3| DISABLE_IRQ, __KERNEL_CS, (unsigned long)overflow                    },
    {  5, 3| DISABLE_IRQ, __KERNEL_CS, (unsigned long)bounds                      },
    {  6, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)invalid_op                  },
    {  7, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)device_not_available        },
    {  8, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)double_fault                },
    {  9, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)coprocessor_segment_overrun },
    { 10, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)invalid_TSS                 },
    { 11, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)segment_not_present         },
    { 12, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)stack_segment               },
    { 13, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)general_protection          },
    { 14, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)pagefault                   },
    { 15, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)spurious_interrupt_bug      },
    { 16, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)coprocessor_error           },
    { 17, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)alignment_check             },
    { 18, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)machine_check               },
    { 19, 0| DISABLE_IRQ, __KERNEL_CS, (unsigned long)simd_coprocessor_error      },
    { 0x80, 3, __KERNEL_CS, (unsigned long)syscall_exception      },
    {  0, 0,           0, 0                           }
};

void
exception_init(void)
{
	int r;
	/* Setup exception handlers */
	r = HYPERVISOR_set_trap_table(trap_table);    
	ASSERT(r == 0);
	r = HYPERVISOR_set_fast_trap(0x80);    
	ASSERT(r == 0);
}
