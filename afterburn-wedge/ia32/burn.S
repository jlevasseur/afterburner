/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe and
 *                      National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/ia32/burn.S
 * Description:   Trampolines for entering C code from the afterburned
 *                guest OS.
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
 ********************************************************************/

#include INC_ARCH(frontend.S)
#include INC_ARCH(intlogic.h)
#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(segment.h)

burn_func burn_unimplemented
	burn_call_c afterburn_cpu_unimplemented_ext

burn_func burn_gdt
	burn_call_c afterburn_cpu_write_gdt32_ext
burn_func burn_idt
	burn_call_c afterburn_cpu_write_idt32_ext
burn_func burn_invlpg
	burn_call_c afterburn_cpu_invlpg_ext
burn_func burn_lldt
	burn_call_c afterburn_cpu_lldt_ext
burn_func burn_ltr
	burn_call_c afterburn_cpu_ltr_ext
burn_func burn_str
	burn_call_c afterburn_cpu_str_ext

burn_func burn_clts
	burn_call_c afterburn_cpu_clts
	
burn_func burn_cli
	burn_call_c afterburn_cpu_cli
#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ)
burn_func burn_sti
	burn_call_c afterburn_cpu_sti
#endif

burn_func burn_interrupt_redirect
	pushl	OFS_CPU_REDIRECT + vcpu /* The guest kernel's return address. */
	/* Fall through */
burn_func burn_deliver_interrupt
	bt	$9, OFS_CPU_FLAGS + vcpu	/* flags.CF <-- flags.IF */
	jnc	1f			/* Return if interrupts disabled. */
	/* The stack already has a call frame. */
	sub	$(3*4), %esp	/* Make space for an iret frame. */
	__afterburn_save_clobbers
	mov	%esp, %eax
	call	afterburn_cpu_deliver_interrupt
	__afterburn_restore_clobbers 0
	testl	$-1, 0(%esp)	/* Do we have a redirect? */
	jnz	1f		/* Jump if we have a redirect. */
	addl	$(3*4), %esp	/* Remove the redirect frame. */
1:
	ret	/* Jump to the new code location. */

burn_func burn_interruptible_hlt
	/* The stack already has a call frame. */
	sub	$(3*4), %esp	/* Make space for an iret frame. */
	__afterburn_save_clobbers
	mov	%esp, %eax
	call	afterburn_cpu_interruptible_hlt
	__afterburn_restore_clobbers 0
	testl	$-1, 0(%esp)	/* Do we have a redirect? */
	jnz	1f		/* Jump if we have a redirect. */
	addl	$(3*4), %esp	/* Remove the redirect frame. */
1:
	ret	/* Jump to the new code location. */

burn_func burn_rdmsr
	burn_call_c afterburn_cpu_read_msr_ext
burn_func burn_wrmsr
	burn_call_c afterburn_cpu_write_msr_ext

#define OFS_IRET_FLAGS	8
#define OFS_IRET_CS	4
#define OFS_IRET_IP	0
#define SIZEOF_IRET	12

#if defined(CONFIG_IA32_FAST_VECTOR)
burn_func burn_iret, ".text.iret"
	/* We have come here via a jmp. */
#if defined(CONFIG_IA32_FAST_VECTOR_ESP0)
	pushl	%eax				/* Create scratch space. */
	movl	OFS_CPU_TSS + vcpu, %eax	/* Get the TSS base. */
	movl	4(%eax), %eax			/* Get tss.esp0. */
	cmpl	%eax, OFS_CPU_ESP0 + vcpu	/* Did esp0 change? */
	popl	%eax				/* Restore eax. */
	jne	burn_iret_esp0_switch	/* If so, install the new esp0. */
burn_iret_esp0_ready:
#endif
	.global burn_iret_restart
burn_iret_restart:
	cmp	$0, OFS_INTLOGIC_VECTOR_CLUSTER + intlogic	/* Pending interrupts? */
	jnz	burn_iret_redirect	/* If so, redirect to an interrupt. */
burn_iret_no_irq:
	/* After this point, if an interrupt arrives, it must be handled
	 * by rolling back to burn_iret (but only if the iret frame
	 * enables interrupts).
	 */
	testl	$3, OFS_IRET_CS(%esp)	/* Are we going to user? */
#if defined(CONFIG_VMI_SUPPORT)
	jz	burn_iret_slow
#else
	jz	burn_iret_kernel
#endif
#if defined(CONFIG_KAXEN_UNLINK_AGGRESSIVE) || defined(CONFIG_KAXEN_WRITABLE_PGTAB)
	movl	$0, vcpu + OFS_CPU_CR2
#endif
	bts	$9, vcpu + OFS_CPU_FLAGS /* Assume that the iret enables interrupts. */
	iret
burn_iret_end:
	.global burn_iret_end

#if defined(CONFIG_IA32_FAST_VECTOR_ESP0)
burn_iret_esp0_switch:
	__afterburn_save_clobbers
	call	afterburn_cpu_esp0_update
	__afterburn_restore_clobbers 0
	jmp	burn_iret_esp0_ready
#endif

burn_iret_kernel:
	/* We need to virtualize the iret frame, since it will try to
	 * activate privilege level 0.  We don't worry about the IRQ
	 * race condition here, because we expect the kernel to execute
	 * a future iret, which will deliver any interrupt that we miss here.
	 */
	pushl	%eax
	movl	(OFS_IRET_FLAGS+4)(%esp), %eax
	movl	%eax, OFS_CPU_FLAGS + vcpu
	popl	%eax
#if defined(CONFIG_WEDGE_KAXEN)
	movl	$XEN_CS_KERNEL, OFS_IRET_CS(%esp)
#else
	movl	%cs, OFS_IRET_CS(%esp)
#endif
	iret

burn_iret_redirect:
	/* We potentially have a pending interrupt to deliver, although 
	 * another CPU may handle the interrupt before us.  Additionally,
	 * if the iret doesn't enable interrupts, then we don't deliver.
	 * We have come here via a jmp.
	 */
	bt	$9, OFS_IRET_FLAGS(%esp) /* Does the iret enable interrupts? */
	jnc	burn_iret_no_irq /* No interrupt redirect. */
	
burn_func burn_iret_slow, ".text.iret"
#else
burn_func burn_iret, ".text.iret"
#endif
	/* We have come here via a jmp. */
	subl	$12, %esp	/* Allocate space for redirect frame. */
	__afterburn_save_all
	movl	%esp, %eax	/* 1st parameter: iret_handler_frame_t */
	jmp	afterburn_cpu_iret

.globl burn_iret_resume
burn_iret_resume:
	__afterburn_restore_all 0
	testl	$-1, 0(%esp)	/* Do we have a redirect? */
	jnz	1f		/* Jump if we have a redirect. */
	addl	$12, %esp	/* Remove the redirect frame. */
1:
	iret	/* Jump to the new code location. */

burn_func burn_lret
	/* We have come here via a jmp. */
	__afterburn_save_clobbers
	movl	%esp, %eax
	addl	$12, %eax	/* 1st parameter: location of lret info. */
	call	afterburn_cpu_lret
	__afterburn_restore_clobbers 0
	lret


burn_func burn_int
	pushf			/* Flags for saved-frame. */
	pushl	%ebx		/* Allocate space for int-frame flags. */
	pushl	%eax		/* Allocate space for int-frame cs. */
	pushl	%eax		/* Allocate space for int-frame ip. */
	__afterburn_save_clobbers
	movl	%esp, %edx
	addl	$12, %edx	/* 2nd parameter: location of int-frame. */
	movl	%esp, %eax
	addl	$24, %eax	/* 1st parameter: location of save-frame. */
	call	afterburn_cpu_int
	__afterburn_restore_clobbers 0

	iret			/* Invoke the interrupt handler and restore */
				/* the stack. */

burn_func burn_cpuid
	__afterburn_save_all
	pushl	%esp		/* Push the bottom of the parameter block. */
	subl	$8, 0(%esp)	/* Adjust to point at the call frame. */
	call	afterburn_cpu_read_cpuid_ext
	__afterburn_restore_all 4
	ret

#if 0 && defined(CONFIG_WEDGE_L4KA)
#if 0
/* Test stuff to see whether we can eliminate the real pushf. */
burn_func burn_pushf
	pushl	%eax
	/* Stack frame: (0)eax, (4)return address, (8)pushed flags */
	movl	%gs:0, %eax		/* UTCB */
	movl	-52(%eax), %eax		/* UTCB's user-defined handle: vcpu. */
	movl	0xc(%eax), %eax		/* vcpu.cpu.flags */
	movl	%eax, 8(%esp)
	/*
	lahf
	or	%ah, 8(%esp)
	*/
	popl	%eax
	ret
#else
burn_func burn_pushf
	pushl	%eax
	/* Stack frame: (0)eax, (4)return address, (8)flags */
	movl	%gs:0, %eax		/* UTCB */
	movl	-52(%eax), %eax		/* UTCB's user-defined handle: vcpu. */
	movl	0xc(%eax), %eax		/* vcpu.cpu.flags */
	andl	$~0xffc08cff, %eax
	andl	$0xffc08cff, 8(%esp)
	orl	%eax, 8(%esp)
	popl	%eax
	ret
#endif
burn_func burn_popf
	pushl	%eax
	pushl	%ecx
	/* Stack frame: (0) ecx, (4)eax, (8)return address, (12)flags */
	movl	%gs:0, %eax		/* UTCB */
	movl	-52(%eax), %eax		/* UTCB's user-defined handle: vcpu. */
	movl	12(%esp), %ecx
	movl	%ecx, 0xc(%eax)		/* vcpu.cpu.flags */
	bt	$9, %ecx		/* Test the interrupt flag. */
	jnc	1f
	pushl	%edx
	call	afterburn_cpu_deliver_irq
	popl	%edx
1:
	popl	%ecx
	popl	%eax
	ret
#endif

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_FLAGS)
burn_func burn_pushf
	burn_call_c afterburn_cpu_read_flags_ext
#endif

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ) || defined(CONFIG_IA32_STRICT_FLAGS)
burn_func burn_popf
	burn_call_c afterburn_cpu_write_flags_ext
#endif

burn_func burn_write_cr0
	burn_call_c afterburn_cpu_write_cr0_ext
burn_func burn_write_cr2
	burn_call_c afterburn_cpu_write_cr2_ext
burn_func burn_write_cr3
	burn_call_c afterburn_cpu_write_cr3_ext
burn_func burn_write_cr4
	burn_call_c afterburn_cpu_write_cr4_ext

burn_func burn_write_dr
	burn_call_c afterburn_cpu_write_dr_ext
burn_func burn_read_dr
	burn_call_c afterburn_cpu_read_dr_ext

burn_func burn_write_cs
	burn_call_c afterburn_cpu_write_cs_ext
burn_func burn_write_ds
	burn_call_c afterburn_cpu_write_ds_ext
burn_func burn_write_es
	burn_call_c afterburn_cpu_write_es_ext
burn_func burn_write_fs
	burn_call_c afterburn_cpu_write_fs_ext
burn_func burn_write_gs
	burn_call_c afterburn_cpu_write_gs_ext
burn_func burn_write_ss
	burn_call_c afterburn_cpu_write_ss_ext

burn_func burn_lss
	burn_call_c afterburn_cpu_lss_ext

burn_func burn_read_cs
	burn_call_c afterburn_cpu_read_cs_ext
burn_func burn_read_ds
	burn_call_c afterburn_cpu_read_ds_ext
burn_func burn_read_es
	burn_call_c afterburn_cpu_read_es_ext
burn_func burn_read_fs
	burn_call_c afterburn_cpu_read_fs_ext
burn_func burn_read_gs
	burn_call_c afterburn_cpu_read_gs_ext
burn_func burn_read_ss
	burn_call_c afterburn_cpu_read_ss_ext

burn_func burn_ud2
	burn_call_c afterburn_cpu_ud2_ext

burn_func burn_out
	burn_call_c afterburn_cpu_write_port_ext
burn_func burn_in
	burn_call_c afterburn_cpu_read_port_ext
burn_func burn_invd
	burn_call_c afterburn_cpu_invd
burn_func burn_wbinvd
	burn_call_c afterburn_cpu_wbinvd
	

