/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/entry.S
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
 * $Id: entry.S,v 1.12 2005/04/14 19:54:43 joshua Exp $
 *
 ********************************************************************/

__KERNEL_DS = 0x0821 /* Linux hack */
__USER_DS = 0x0833
__USER_CS = 0x082b

#define ENTRY(x) .global x	;  x:

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %edi; \
	pushl %esi; \
	pushl %ebp; \
	pushl %ebx; \
	pushl %edx; \
	pushl %ecx; \
	pushl %eax; \
	movl $(__KERNEL_DS),%edx; \
	movl %edx,%ds; \
	movl %edx,%es;


#define RESTORE_ALL	\
	popl %eax;	\
	popl %ecx;	\
	popl %edx;	\
	popl %ebx;	\
	popl %ebp;	\
	popl %esi;	\
	popl %edi;	\
	popl %ds;	\
	popl %es;	\
	addl $4, %esp;  \
	!iret;		\

/* The etra space is to make room for a return address */	
#define EXCEPT(x, y) \
	ENTRY(x) \
	pushl	$1234567	;  \
	SAVE_ALL		; \
	pushl	%esp 		; \
	pushl	$y  	; \
	call handle_exception


#define PFEXCEPT(x, y) \
	ENTRY(x) \
	SAVE_ALL		; \
	pushl	%esp 		; \
	pushl	$y  	; \
	call handle_exception

		
/* All our exception entry points */	
EXCEPT(divide_error, 0)
EXCEPT(debug_exception, 1)
EXCEPT(int3, 3)
EXCEPT(overflow, 4)
EXCEPT(bounds, 5)
EXCEPT(invalid_op, 6)
EXCEPT(device_not_available, 7)
EXCEPT(double_fault, 8)
EXCEPT(coprocessor_segment_overrun, 9)
EXCEPT(invalid_TSS, 10)
EXCEPT(segment_not_present, 11)
EXCEPT(stack_segment, 12)
EXCEPT(general_protection, 13)
PFEXCEPT(pagefault, 14)		
EXCEPT(spurious_interrupt_bug, 15)
EXCEPT(coprocessor_error, 16)
EXCEPT(alignment_check, 17)
EXCEPT(machine_check, 18)
EXCEPT(simd_coprocessor_error, 19)

ENTRY(syscall_exception)
crit_start:
	pushl   %eax		;
	movl	HYPERVISOR_shared_info, %eax ;
	movb	$1, 1(%eax)	;
	popl	%eax		; 
crit_end:
#ifdef CONFIG_XEN_SYSCALL_FP
/* VU: the assumption for syscall_exception is, that it is always
 * called on ring switches.  Otherwise we would afterburn it and then
 * we don't end up here */
	#define OFF_CPU_CS	32
	#define OFF_CPU_SS	42
	#define OFF_CPU_TSS	48
	#define OFF_CPU_IDTR	26
	#define OFF_CPU_FLAGS	0
	
	// first save the old seg sel on stack
	push	%eax
	push	%ebx
	push	%ecx
	// read and merge the segment selector
	movl	OFF_CPU_TSS + 2 + the_vcpu, %eax
	andl	$0x00ffffff, %eax
	movl	OFF_CPU_TSS + 4 + the_vcpu, %ebx
	andl	$0xff000000, %ebx
	orl	%ebx, %eax	// eax=tss.base
	
	movl	4(%eax), %ebx	// get esp0, assume trap to ring0
	xchg	%ebx, %esp	// switch to "real" kernel stack
	pushl	OFF_CPU_SS + the_vcpu // user ss
	andl	$0xffff, (%esp)
	pushl	24(%ebx)	// transfer uesp
	pushl	20(%ebx)	// transfer uflags
	pushl	OFF_CPU_CS + the_vcpu // user cs
	andl	$0xffff, (%esp)
	pushl	12(%ebx)	//  transfer uip

	// now update the VCPU with the new segment selectors, 
	// eax is still tss.base
	movzxw	8(%eax), %ecx	// get ss0
	movl	%ecx, OFF_CPU_SS + the_vcpu

	movl	OFF_CPU_IDTR + 2 + the_vcpu, %eax // eax=IDT base
	movzxw	0x80 * 8 + 2 (%eax), %ecx
	movl	%ecx, OFF_CPU_CS + the_vcpu // store KCS

	// now get the interrupt gate we go to
	movl	0x80 * 8 (%eax), %ecx
	andl	$0xffff, %ecx
	pushl	%ecx
	movl	0x80 * 8 + 4 (%eax), %ecx
	andl	$0xffff0000, %ecx
	orl	%ecx, (%esp)

	// XXX:	int 0x80 leaves IRQs enabled
	orl	$(1<<9), OFF_CPU_FLAGS + the_vcpu

	// restore register values
	movl	 0(%ebx), %ecx
	movl	 8(%ebx), %eax
	movl	 4(%ebx), %ebx

	// re-enable Xen IRQs
	pushl   %eax		;
	movl	HYPERVISOR_shared_info, %eax ;
	movb	$0, 1(%eax)	;
	popl	%eax		; 

	ret
#else
	pushl	$1234567	;
	SAVE_ALL		;
	pushl	%esp 		;
	pushl	$0x80  	;
	call handle_exception
#endif /* CONFIG_XEN_SYSCALL_FP */
	
ENTRY(hypervisor_callback)
	cmpl	$crit_start,(%esp)
	jb	11f
	cmpl	$crit_end,(%esp)
	jb	fixup
11:	
	pushl	$1234567	;  
	SAVE_ALL
	pushl	%esp
	call	xen_upcall
	addl	$4, %esp
	RESTORE_ALL
	addl	$4, %esp
fixup:
	movl	$37, fuzzbucket
	iret

.macro __afterburn_restore_all drop
.if \drop
	addl	$(\drop), %esp
.endif
	popl	%eax
	popl	%ecx
	popl	%edx
	popl	%ebx
	addl	$4, %esp
	popl	%ebp
	popl	%esi
	popl	%edi
	/*!popfl */
	addl	$4, %esp
.endm


#define FLAGS_USER_MASK 0xffc08cff
#define FLAGS_SYS_AT_USER 0x00083200
#define DISABLE_HACK	0xfffffdff

ENTRY(user_restore_stub)
	movl $(__USER_DS),%edx;
	movl %edx,%ds;
	movl %edx,%es;

	movl 40(%esp), %edx
	movl %edx, OFF_CPU_CS + the_vcpu // user cs
	movl $(__USER_CS),40(%esp) ; 

	movl 52(%esp), %edx
	movl %edx, OFF_CPU_SS + the_vcpu // user cs
	movl $(__USER_DS),52(%esp)	; 

#if 0
	movl 44(%esp), %edx
	andl  $DISABLE_HACK, %edx
	movl %edx, OFF_CPU_FLAGS + the_vcpu
	andl $FLAGS_USER_MASK, %edx
	orl  $FLAGS_SYS_AT_USER, %edx
	movl %edx, 44(%esp)
#endif

	__afterburn_restore_all 0
	!iret	/* Jump to the new code location. */
