/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/ia32/instr-kernel.S
 * Description:   Assembler macros for rewriting the kernel code of a
 *                guest OS, for static code substitution.
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
 * $Id: instr-kernel.S,v 1.21 2005/12/21 08:59:40 stoess Exp $
 *
 ********************************************************************/
/* C function clobbers: eax, ecx, edx */

/* Stack switching: if we want to switch to a temporary stack while
 * jumping into the afterburner, then consider:
 *    mov %esp, temp_stack_top
 *    lea temp_stack_top, %esp
 * or if atomicity is necessary (but which costs 141 cycles on a P4):
 *    xchg %esp, temp_stack_top
 */

.macro __afterburn_save_clobbers
	pushl	%eax
	pushl	%ecx
	pushl	%edx
.endm

.macro __afterburn_restore_clobbers drop
.if \drop
	addl	$(\drop), %esp
.endif
	popl	%edx
	popl	%ecx
	popl	%eax
.endm

.macro __afterburn_save_all
	/* The order matches that of frame_t in include/ia32/cpu.h. */
	/* TODO: consider using pusha and popa, to use a single instruction. */
	!pushfl
	pushl	%edi
	pushl	%esi
	pushl	%ebp
	pushl	%esp
	pushl	%ebx
	pushl	%edx
	pushl	%ecx
	pushl	%eax
.endm

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
	!popfl
.endm

.macro __afterburn_unimplemented
	__afterburn_save_clobbers
	call afterburn_cpu_unimplemented
	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * Global descriptor table
 *************************************************************************/
.macro lgdt mem_op
	__afterburn_save_clobbers
	lea	\mem_op, %eax
	pushl	%eax
	call	afterburn_cpu_write_gdt32
	__afterburn_restore_clobbers 4
.endm

.macro lgdtl mem_op
	lgdt \mem_op
.endm

.macro sgdt mem
	__afterburn_unimplemented
	!sgdt \mem
.endm

.macro sgdtl mem
	__afterburn_unimplemented
	!sgdtl \mem
.endm

/*************************************************************************
 * Interrupt descriptor table
 *************************************************************************/
.macro lidt mem_op
	__afterburn_save_clobbers
	lea	\mem_op, %eax
	pushl	%eax
	call	afterburn_cpu_write_idt32
	__afterburn_restore_clobbers 4
.endm

.macro lidtl mem_op
	lidt \mem_op
.endm

.macro sidt mem
	__afterburn_unimplemented
	!sidt \mem
.endm

.macro sidtl mem
	__afterburn_unimplemented
	!sidtl \mem
.endm

/*************************************************************************
 * mov
 *************************************************************************/

.macro __afterburn_read src, dst, c_func
	addl	$-4, %esp
	__afterburn_save_clobbers
	call	\c_func
	movl	%eax, 12(%esp)
	__afterburn_restore_clobbers 0
	popl	\dst
.endm

.macro __afterburn_read_dr dst, which
	addl	$-4, %esp
	__afterburn_save_clobbers
	pushl	$\which
	call	afterburn_cpu_read_dr
	movl	%eax, 16(%esp)
	__afterburn_restore_clobbers 4
	popl	\dst
.endm


.macro __afterburn_write src, dst, c_func
	__afterburn_save_clobbers
	pushl	\src
	call	\c_func
	__afterburn_restore_clobbers 4
.endm

.macro __afterburn_write_dr src, which
	__afterburn_save_clobbers
	pushl	\src
	pushl	$\which
	call	afterburn_cpu_write_dr
	__afterburn_restore_clobbers 8
.endm

.macro mov src, dst

/*
 * cr0
 */
.ifeqs "\src", "%cr0"
	__afterburn_read \src, \dst, afterburn_cpu_read_cr0 ; .exitm
.endif 
.ifeqs "\dst", "%cr0"
	__afterburn_write \src, \dst, afterburn_cpu_write_cr0 ; .exitm
.endif

/*
 * cr2
 */
.ifeqs "\src", "%cr2"
	__afterburn_read \src, \dst, afterburn_cpu_read_cr2 ; .exitm
.endif 
.ifeqs "\dst", "%cr2"
	__afterburn_write \src, \dst, afterburn_cpu_write_cr2 ; .exitm
.endif

/*
 * cr3
 */
.ifeqs "\src", "%cr3"
	__afterburn_read \src, \dst, afterburn_cpu_read_cr3 ; .exitm
.endif 
.ifeqs "\dst", "%cr3"
	__afterburn_write \src, \dst, afterburn_cpu_write_cr3 ; .exitm
.endif

/*
 * cr4
 */
.ifeqs "\src", "%cr4"
	__afterburn_read \src, \dst, afterburn_cpu_read_cr4 ; .exitm
.endif 
.ifeqs "\dst", "%cr4"
	__afterburn_write \src, \dst, afterburn_cpu_write_cr4 ; .exitm
.endif

/*
 * ss
 */
.ifeqs "\src", "%ss"
	__afterburn_read \src, \dst, afterburn_cpu_read_ss ; .exitm
.endif 
.ifeqs "\dst", "%ss"
	__afterburn_write \src, \dst, afterburn_cpu_write_ss ; .exitm
.endif

/*
 * ds
 */
.ifeqs "\src", "%ds"
	__afterburn_read \src, \dst, afterburn_cpu_read_ds ; .exitm
.endif 
.ifeqs "\dst", "%ds"
	__afterburn_write \src, \dst, afterburn_cpu_write_ds ; .exitm
.endif

/*
 * es
 */
.ifeqs "\src", "%es"
	__afterburn_read \src, \dst, afterburn_cpu_read_es ; .exitm
.endif 
.ifeqs "\dst", "%es"
	__afterburn_write \src, \dst, afterburn_cpu_write_es ; .exitm
.endif

/*
 * fs
 */
.ifeqs "\src", "%fs"
	__afterburn_read \src, \dst, afterburn_cpu_read_fs ; .exitm
.endif 
.ifeqs "\dst", "%fs"
	__afterburn_write \src, \dst, afterburn_cpu_write_fs ; .exitm
.endif

/*
 * gs
 */
.ifeqs "\src", "%gs"
	__afterburn_read \src, \dst, afterburn_cpu_read_gs ; .exitm
.endif 
.ifeqs "\dst", "%gs"
	__afterburn_write \src, \dst, afterburn_cpu_write_gs ; .exitm
.endif

/*
 * debug registers
 */
.ifeqs "\src", "%db0"
	__afterburn_read_dr \dst, 0 ; .exitm
.endif
.ifeqs "\dst", "%db0"
	__afterburn_write_dr \src, 0 ; .exitm
.endif
.ifeqs "\src", "%db1"
	__afterburn_read_dr \dst, 1 ; .exitm
.endif
.ifeqs "\dst", "%db1"
	__afterburn_write_dr \src, 1 ; .exitm
.endif
.ifeqs "\src", "%db2"
	__afterburn_read_dr \dst, 2 ; .exitm
.endif
.ifeqs "\dst", "%db2"
	__afterburn_write_dr \src, 2 ; .exitm
.endif
.ifeqs "\src", "%db3"
	__afterburn_read_dr \dst, 3 ; .exitm
.endif
.ifeqs "\dst", "%db3"
	__afterburn_write_dr \src, 3 ; .exitm
.endif
.ifeqs "\src", "%db4"
	__afterburn_read_dr \dst, 4 ; .exitm
.endif
.ifeqs "\dst", "%db4"
	__afterburn_write_dr \src, 4 ; .exitm
.endif
.ifeqs "\src", "%db5"
	__afterburn_read_dr \dst, 5 ; .exitm
.endif
.ifeqs "\dst", "%db5"
	__afterburn_write_dr \src, 5 ; .exitm
.endif
.ifeqs "\src", "%db6"
	__afterburn_read_dr \dst, 6 ; .exitm
.endif
.ifeqs "\dst", "%db6"
	__afterburn_write_dr \src, 6 ; .exitm
.endif
.ifeqs "\src", "%db7"
	__afterburn_read_dr \dst, 7 ; .exitm
.endif
.ifeqs "\dst", "%db7"
	__afterburn_write_dr \src, 7 ; .exitm
.endif

/*
 * innocuous
 */
	!movl \src, \dst
.endm

.macro movl src, dst
	mov \src, \dst
.endm

/*************************************************************************
 * Descriptor stuff
 *************************************************************************/
.macro ljmp cs, loc
	/* Install the segment info into the cs register. */
	__afterburn_write \cs, %cs, afterburn_cpu_write_cs

	/* Jump to the actual destination.  It could be an absolute
	 * address, so indirect via the stack.
	 */
	pushl	\loc
	ret
.endm

.macro __afterburn_farptr loc, dst, c_func
	/* Install the segment info into the segment register. */
	__afterburn_save_clobbers
	lea	\loc, %eax
	addl	$4, %eax
	pushl	(%eax)
	call	\c_func
	__afterburn_restore_clobbers 4

	/* Install the pointer. */
	movl	\loc, \dst
.endm

.macro lds loc, dst
	__afterburn_farptr \loc, \dst, afterburn_cpu_write_ds
.endm

.macro les loc, dst
	__afterburn_farptr \loc, \dst, afterburn_cpu_write_es
.endm

.macro lfs loc, dst
	__afterburn_farptr \loc, \dst, afterburn_cpu_write_fs
.endm

.macro lgs loc, dst
	__afterburn_farptr \loc, \dst, afterburn_cpu_write_gs
.endm

.macro lss loc, dst
	__afterburn_farptr \loc, \dst, afterburn_cpu_write_ss
.endm


/*************************************************************************
 * flags
 *************************************************************************/
.macro pushfl
	!pushfl		/* Push the unprivileged flags. */
	__afterburn_save_clobbers
	movl	12(%esp), %eax
	pushl	%eax	/* We need to merge the unprivileged with the privileged */
	call	afterburn_cpu_read_flags
	movl	%eax, 16(%esp)	/* Save the merged flags. */
	__afterburn_restore_clobbers 4
.endm

.macro pushfd
	pushfl
.endm

.macro popfl
	__afterburn_save_clobbers
	movl	12(%esp), %eax
	pushl	%eax
	call	afterburn_cpu_write_flags
	movl	%eax, 16(%esp)
	__afterburn_restore_clobbers 4
	!popfl
.endm

.macro popfd
	popfl
.endm

/*************************************************************************
 * clts
 *************************************************************************/
.macro clts
	__afterburn_save_clobbers
	call afterburn_cpu_clts
	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * lldt
 *************************************************************************/
.macro lldt seg
	__afterburn_save_clobbers
	movzx	\seg, %eax
	pushl	%eax
	call afterburn_cpu_lldt
	__afterburn_restore_clobbers 4
.endm

/*************************************************************************
 * sldt
 *************************************************************************/
.macro sldt dst
	__afterburn_unimplemented
	!sldt \dst
.endm

.macro sldtl dst
	__afterburn_unimplemented
	!sldtl \dst
.endm

/*************************************************************************
 * push
 *************************************************************************/
.macro push src
.ifeqs "\src", "%cs"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif
.ifeqs "\src", "%ds"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif
.ifeqs "\src", "%es"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif
.ifeqs "\src", "%fs"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif
.ifeqs "\src", "%gs"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif
.ifeqs "\src", "%ss"
	__afterburn_unimplemented ; !push \src ; .exitm
.endif

/*
 * innocuous
 */
	!push	\src
.endm

/*************************************************************************
 * pop
 *************************************************************************/
.macro pop dst
.ifeqs "\dst", "%cs"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif
.ifeqs "\dst", "%ds"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif
.ifeqs "\dst", "%es"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif
.ifeqs "\dst", "%fs"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif
.ifeqs "\dst", "%gs"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif
.ifeqs "\dst", "%ss"
	__afterburn_unimplemented ; !pop \dst ; .exitm
.endif

/*
 * innocuous
 */
	!pop	\dst
.endm

/*************************************************************************
 * pushl
 *************************************************************************/
.macro __afterburn_push c_func
	addl	$-4, %esp
	__afterburn_save_clobbers
	call	\c_func
	movl	%eax, 12(%esp)
	__afterburn_restore_clobbers 0
.endm

.macro pushl src
.ifeqs "\src", "%cs"
	__afterburn_push afterburn_cpu_read_cs ; .exitm
.endif
.ifeqs "\src", "%ds"
	__afterburn_push afterburn_cpu_read_ds ; .exitm
.endif
.ifeqs "\src", "%es"
	__afterburn_push afterburn_cpu_read_es ; .exitm
.endif
.ifeqs "\src", "%fs"
	__afterburn_push afterburn_cpu_read_fs ; .exitm
.endif
.ifeqs "\src", "%gs"
	__afterburn_push afterburn_cpu_read_gs ; .exitm
.endif
.ifeqs "\src", "%ss"
	__afterburn_push afterburn_cpu_read_ss ; .exitm
.endif

/*
 * innocuous
 */
	!pushl	\src
.endm

/*************************************************************************
 * popl
 *************************************************************************/
.macro __afterburn_pop c_func
	__afterburn_save_clobbers
	pushl	12(%esp)
	call	\c_func
	__afterburn_restore_clobbers 4
	addl	$4, %esp
.endm

.macro popl dst
.ifeqs "\dst", "%cs"
	__afterburn_pop afterburn_cpu_write_cs ; .exitm
.endif
.ifeqs "\dst", "%ds"
	__afterburn_pop afterburn_cpu_write_ds ; .exitm
.endif
.ifeqs "\dst", "%es"
	__afterburn_pop afterburn_cpu_write_es ; .exitm
.endif
.ifeqs "\dst", "%fs"
	__afterburn_pop afterburn_cpu_write_fs ; .exitm
.endif
.ifeqs "\dst", "%gs"
	__afterburn_pop afterburn_cpu_write_gs ; .exitm
.endif
.ifeqs "\dst", "%ss"
	__afterburn_pop afterburn_cpu_write_ss ; .exitm
.endif

/*
 * innocuous
 */
	!popl	\dst
.endm


/*************************************************************************
 * ud2
 *************************************************************************/
.macro ud2
 	__afterburn_save_clobbers
	call afterburn_cpu_ud2
 	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * cli
 *************************************************************************/
.macro cli
 	__afterburn_save_clobbers
	call afterburn_cpu_cli
 	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * sti
 *************************************************************************/
.macro sti
 	__afterburn_save_clobbers
	call afterburn_cpu_sti
 	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * hlt
 *************************************************************************/
.macro hlt
 	__afterburn_save_clobbers
	call afterburn_cpu_hlt
 	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * ltr
 *************************************************************************/
.macro ltr src
 	__afterburn_save_clobbers
	movzx	\src, %eax
	pushl	%eax
	call	afterburn_cpu_ltr
 	__afterburn_restore_clobbers 4
.endm

.macro str dst
	__afterburn_unimplemented
	!str \dst
.endm

.macro strl dst
	__afterburn_unimplemented
	!strl \dst
.endm

/*************************************************************************
 * invlpg
 *************************************************************************/
.macro invlpg src
 	__afterburn_save_clobbers
	lea	\src, %eax
	pushl	%eax
	call	afterburn_cpu_invlpg
 	__afterburn_restore_clobbers 4
.endm

/*************************************************************************
 * invd
 *************************************************************************/
.macro invd
 	__afterburn_save_clobbers
	call afterburn_cpu_invd
 	__afterburn_restore_clobbers 0
.endm

.macro wbinvd
 	__afterburn_save_clobbers
	call afterburn_cpu_wbinvd
 	__afterburn_restore_clobbers 0
.endm

/*************************************************************************
 * in
 *************************************************************************/
.macro __afterburn_read_port bit_width, d, a
	__afterburn_save_clobbers
	pushl	%eax
	.ifeqs "\d", "%dx"
	pushl	%edx
	.else
	pushl	\d
	.endif
	pushl	\bit_width
	call	afterburn_cpu_read_port
	movl	%eax, (12+8)(%esp)
	__afterburn_restore_clobbers 12
.endm

.macro inb d, a
	__afterburn_read_port $8, \d, \a
.endm
.macro inw d, a
	__afterburn_read_port $16, \d, \a
.endm
.macro inl d, a
	__afterburn_read_port $32, \d, \a
.endm

/*************************************************************************
 * out
 *************************************************************************/
.macro __afterburn_write_port bit_width, a, d
	__afterburn_save_clobbers
	pushl	%eax
	.ifeqs "\d", "%dx"
	pushl	%edx
	.else
	pushl	\d
	.endif
	pushl	\bit_width
	call	afterburn_cpu_write_port
	__afterburn_restore_clobbers 12
.endm

.macro outb a, d
	__afterburn_write_port $8, \a, \d
.endm
.macro outw a, d
	__afterburn_write_port $16, \a, \d
.endm
.macro outl a, d
	__afterburn_write_port $32, \a, \d
.endm

/*************************************************************************
 * iret
 *************************************************************************/
.macro iret
	subl	$12, %esp	/* Allocate space for redirect frame. */
	__afterburn_save_all
	movl	%esp, %eax
	addl	$(12+36), %eax	/* 1st parameter: location of iret info. */
	movl	%esp, %edx	/* 2nd parameter: location of register frame. */
	movl	%edx, %ecx
	addl	$36, %ecx	/* 3rd parameter: location of redirect frame. */
	call	afterburn_cpu_iret
	__afterburn_restore_all 0

	testl	$-1, 0(%esp)	/* Do we have a redirect? */
	jnz	1f		/* Jump if we have a redirect. */
	addl	$12, %esp	/* Remove the redirect frame. */
1:
	!iret	/* Jump to the new code location. */
.endm

.macro iretd
	iret
.endm

.macro iretl
	iret
.endm

/*************************************************************************
 * int
 *************************************************************************/
.macro int vector
.ifeqs "\vector", "$3"
	/* Support real debug exceptions. */
	!int	\vector
	.exitm
.endif

1:
	!pushf			/* Flags for saved-frame. */
	pushl	%eax		/* Allocate space for saved-frame cs. */
	pushl	$99f		/* Saved-frame return ip address. */
	pushl	%ebx		/* Allocate space for int-frame flags. */
	pushl	%eax		/* Allocate space for int-frame cs. */
	pushl	$99f		/* Allocate space for int-frame ip; also gives a sane value in case something goes wrong. */
	__afterburn_save_clobbers
	movl	%esp, %eax
	addl	$12, %eax	/* Calculate location of int frame. */
	pushl	%eax		/* 3rd parameter: location of int-frame. */
	addl	$12, %eax	/* Calculate location of save-frame. */
	pushl	%eax		/* 2nd parameter: location of save-frame. */
	pushl	\vector		/* 1st parameter: int vector. */
	call	afterburn_cpu_int
	__afterburn_restore_clobbers 12

	!iret			/* Invoke the interrupt handler and restore */
				/* the stack. */

	jmp	1b		/* Restart the int handler.  Must be a */
				/* 2-byte instruction. */
99:
.endm

/*************************************************************************
 * smsw + lmsw
 *************************************************************************/
.macro smsw dst
	__afterburn_unimplemented
	!smsw \dst
.endm

.macro smswl dst
	__afterburn_unimplemented
	!smswl \dst
.endm

.macro lmsw src
	__afterburn_unimplemented
	!lmsw \src
.endm

/*************************************************************************
 * arpl
 *************************************************************************/
.macro arpl p1, p2
	__afterburn_unimplemented
	!arpl \p1, \p2
.endm

/*************************************************************************
 * lar
 *************************************************************************/
.macro lar p1, p2
	__afterburn_unimplemented
	!lar \p1, \p2
.endm

/*************************************************************************
 * lsl
 *************************************************************************/
.macro lsl p1, p2
	__afterburn_unimplemented
	!lsl \p1, \p2
.endm

/*************************************************************************
 * rsm
 *************************************************************************/
.macro rsm
	__afterburn_unimplemented
	!rsm
.endm

/*************************************************************************
 * cpuid
 *************************************************************************/
.macro cpuid
	__afterburn_save_all
	pushl	%esp	/* The register save frame. */
	call afterburn_cpu_read_cpuid
	__afterburn_restore_all 4
.endm

/*************************************************************************
 * rdmsr
 *************************************************************************/
.macro rdmsr
	pushl	%ecx		/* preserve ecx */
	subl	$8, %esp	/* allocate space for eax and edx output */
	pushl	%ecx		/* push ecx as an input parameter */
	call	afterburn_cpu_read_msr
	addl	$4, %esp	/* drop the input parameter */
	popl	%eax		/* pop the eax output parameter */
	popl	%edx		/* pop the edx output parameter */
	popl	%ecx		/* restore ecx */
.endm

/*************************************************************************
 * wrmsr
 *************************************************************************/
.macro wrmsr
	__afterburn_save_clobbers
	pushl	%edx
	pushl	%eax
	pushl	%ecx
	call	afterburn_cpu_write_msr
	__afterburn_restore_clobbers 12
.endm

/*************************************************************************
 * Profiler feedback macros.
 *************************************************************************/
.macro __rep_nullify rep_type
	/* This macro is called when a rep prefix is found.  We drop the
	 * prefix, since it will be passed to the replacement macro
	 * for the instruction that accompanies the rep prefix.
	 */
.endm

/*************************************************************************
 * User memory accesses with the rep prefix.
 *************************************************************************/
.macro __afterburn_rep_user_memory p1 p2 p3 p4
9998:
	\p1	/* The rep prefix. */

.ifnes "\p4", ""
	\p2 \p3, \p4 
.else
.ifnes "\p3", ""
	\p2 \p3
.else
	\p2
.endif
.endif
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn.user_memory
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * User memory accesses (with an optional rep prefix as the first param)
 *************************************************************************/
.macro __afterburn_user_memory p1 p2 p3 p4
.ifeqs "\p1", "rep"
	__afterburn_rep_user_memory \p1, \p2, \p3, \p4 ; .exitm
.endif
.ifeqs "\p1", "repe"
	__afterburn_rep_user_memory \p1, \p2, \p3, \p4 ; .exitm
.endif
.ifeqs "\p1", "repne"
	__afterburn_rep_user_memory \p1, \p2, \p3, \p4 ; .exitm
.endif
.ifeqs "\p1", "repz"
	__afterburn_rep_user_memory \p1, \p2, \p3, \p4 ; .exitm
.endif
.ifeqs "\p1", "repnz"
	__afterburn_rep_user_memory \p1, \p2, \p3, \p4 ; .exitm
.endif

9998:
.ifnes "\p4", ""
	\p1 \p2, \p3, \p4
.else
.ifnes "\p3", ""
	\p1 \p2, \p3
.else
.ifnes "\p2", ""
	\p1 \p2
.else
	\p1
.endif
.endif
.endif
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn.user_memory
.align 4
.long 9998b
.long 9999b
.popsection
.endm

