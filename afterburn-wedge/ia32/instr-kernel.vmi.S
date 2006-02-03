/*********************************************************************
 *
 * Copyright (C) 2005, University of Karlsruhe 
 * Copyright (C) 2005, Volkmar Uhlig
 *
 * File path:     afterburn-wedge/ia32/instr-kernel.vmi.S
 * Description:   Assembler macros for rewriting the kernel code of a
 *                guest OS, for VMI support
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
 * $Id: instr-kernel.vmi.S,v 1.1 2005/11/17 16:18:10 uhlig Exp $
 *
 ********************************************************************/

	
/*************************************************************************
 * Global descriptor table
 *************************************************************************/
.macro lgdt mem_op
	push %eax
	lea \mem_op, %eax
	call VMI_SetGDT
	pop %eax
.endm

.macro lgdtl mem_op
	lgdt \mem_op
.endm

.macro sgdt mem
	push %eax
	lea \mem, %eax
	call VMI_GetGDT
.endm

.macro sgdtl mem
	sgdt \mem
.endm

/*************************************************************************
 * Interrupt descriptor table
 *************************************************************************/
.macro lidt mem_op
	push %eax
	lea \mem_op, %eax
	call VMI_SetIDT
	pop %eax
.endm

.macro lidtl mem_op
	lidt \mem_op
.endm

.macro sidt mem
	push %eax
	lea \mem_op, %eax
	call VMI_GetIDT
	pop %eax
.endm

.macro sidtl mem
	sidt \mem
.endm

/*************************************************************************
 * mov
 *************************************************************************/

.macro __afterburn_read src, dst, c_func
.ifnes	"\dst", "%eax"
	xchg	%eax, \dst
	call	\c_func
	xchg	%eax, \dst
.else
	call	\c_func
.endif
.endm

.macro __afterburn_read_dr dst, which
.ifnes  "\dst", "%eax"
	xchg	%eax, \dst
.endif
	movl	$\which, %eax
	call	VMI_GetDR
.ifnes	"\dst", "%eax"
	xchg	%eax, \dst
.endif	
.endm


.macro __afterburn_write src, dst, c_func
.ifnes	"\src", "%eax"
	xchg	%eax, \src
	call	\c_func
	xchg	%eax, \src
.else
	call	\c_func
.endif
.endm

.macro __afterburn_write_dr src, which
.ifnes	"\src", "%eax"
	xchg \src, %eax
.endif
	pushl	%edx
	movl	$\which, %edx
	call	VMI_SetDR
	popl	%edx
.ifnes	"\src", "%eax"
	xchg \src, %eax
.endif
.endm


.macro mov src, dst

/*
 * cr0
 */
.ifeqs "\src", "%cr0"
	__afterburn_read \src, \dst, VMI_GetCR0 ; .exitm
.endif 
.ifeqs "\dst", "%cr0"
	__afterburn_write \src, \dst, VMI_SetCR0 ; .exitm
.endif

/*
 * cr2
 */
.ifeqs "\src", "%cr2"
	__afterburn_read \src, \dst, VMI_GetCR2 ; .exitm
.endif 
.ifeqs "\dst", "%cr2"
	__afterburn_write \src, \dst, VMI_SetCR2 ; .exitm
.endif

/*
 * cr3
 */
.ifeqs "\src", "%cr3"
	__afterburn_read \src, \dst, VMI_GetCR3 ; .exitm
.endif 
.ifeqs "\dst", "%cr3"
	__afterburn_write \src, \dst, VMI_SetCR3 ; .exitm
.endif

/*
 * cr4
 */
.ifeqs "\src", "%cr4"
	__afterburn_read \src, \dst, VMI_GetCR4 ; .exitm
.endif 
.ifeqs "\dst", "%cr4"
	__afterburn_write \src, \dst, VMI_SetCR4 ; .exitm
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
 * flags
 *************************************************************************/
.macro pushfl
	push	%eax
	call	VMI_GetEFLAGS
	xchg	%eax, (%esp)
.endm

.macro pushfd
	pushfl
.endm

.macro popfl
	pushl	%eax
	movl	4(%esp), %eax
	call	VMI_SetEFLAGS
	popl	%eax
	add	$4, %esp
.endm

.macro popfd
	popfl
.endm

/*************************************************************************
 * clts
 *************************************************************************/
.macro clts
	call	VMI_CLTS
.endm

/*************************************************************************
 * lldt
 *************************************************************************/
.macro lldt seg
	push	%eax
	movzx	\seg, %eax
	call	VMI_SetLDT
	pop	%eax
.endm

/*************************************************************************
 * sldt
 *************************************************************************/
.macro sldt dst
.ifnes "\dst", "%ax"
	push	%eax
.endif
	call	VMI_GetLDT
.ifnes "\dst", "%ax"
	movl	%ax, \dst
	pop	%eax
.endif
.endm

.macro sldtl dst
	sldt	\dst
.endm

/*************************************************************************
 * cli
 *************************************************************************/
.macro cli
	call VMI_CLI
.endm

/*************************************************************************
 * sti
 *************************************************************************/
.macro sti
	call VMI_STI
.endm

/*************************************************************************
 * hlt
 *************************************************************************/
.macro hlt
	call VMI_Halt
.endm

/*************************************************************************
 * ltr
 *************************************************************************/
.macro ltr src
	pushl	%eax
	movzx	\src, %eax
	call	VMI_SetTR
	popl	%eax
.endm

.macro str dst
.ifnes "\dst", "%ax"
	push	%eax
.endif
	call	VMI_GetTR
.ifnes "\dst", "%ax"
	movl	%ax, \dst
	pop	%eax
.endif
.endm

.macro strl dst
	str \dst
.endm

/*************************************************************************
 * invlpg
 *************************************************************************/
.macro invlpg src
	pushl	%eax
	lea	\src, %eax
	call	VMI_InvalPage
	popl	%eax
.endm

/*************************************************************************
 * invd
 *************************************************************************/
.macro invd
	call	VMI_INVD
.endm

.macro wbinvd
	call	VMI_WBINVD
.endm

/*************************************************************************
 * in
 *************************************************************************/
.macro __afterburn_read_port func, d, a
	.ifnes "\d", "%dx"
	pushl	%edx
	movl	\d, %edx
	call	\func
	popl	%edx
	.else
	call	\func
	.endif
.endm

.macro inb d, a
	__afterburn_read_port VMI_INB, \d, \a
.endm
.macro inw d, a
	__afterburn_read_port VMI_INW, \d, \a
.endm
.macro inl d, a
	__afterburn_read_port VMI_INL, \d, \a
.endm

.macro __afterburn_read_port_string func, reg
	pushl	%eax
	call	\func
	mov	\reg, (%edi)
.endm

.macro insb
	__afterburn_read_port_string VMI_INB, %al
.endm
.macro insw
	__afterburn_read_port_string VMI_INW, %ax
.endm
.macro insl
	__afterburn_read_port_string VMI_INL, %eax
.endm
	

/*************************************************************************
 * out
 *************************************************************************/
.macro __afterburn_write_port func, a, d
	.ifnes "\d", "%dx"
	pushl	%edx
	movl	\d, %edx
	call	\func
	popl	%edx
	.else
	call	\func
	.endif
.endm

.macro outb a, d
	__afterburn_write_port VMI_OUTB, \a, \d
.endm
.macro outw a, d
	__afterburn_write_port VMI_OUTW, \a, \d
.endm
.macro outd a, d
	__afterburn_write_port VMI_OUTL, \a, \d
.endm

.macro __afterburn_write_port_string func, reg
	pushl	%eax
	mov	(%esi), \reg
	call	\func
	popl	%eax
.endm

.macro outsb
	__afterburn_write_port_string VMI_OUTB, %al
.endm
.macro outsw
	__afterburn_write_port_string VMI_OUTW, %ax
.endm
.macro outsl
	__afterburn_write_port_string VMI_OUTL, %eax
.endm



/*************************************************************************
 * iret
 *************************************************************************/
.macro iret
	jmp	VMI_IRET
.endm

.macro iretd
	iret
.endm

.macro iretl
	iret
.endm

/*************************************************************************
 * smsw + lmsw
 *************************************************************************/
.macro smsw dst
.ifnes "\dst", "%ax"
	push	%eax
.endif
	call	VMI_SMSW
.ifnes "\dst", "%ax"
	mov	%ax, \dst
	pop	%eax
.endif
.endm

.macro smswl dst
	smsw \dst
.endm

.macro lmsw src
	pushl	%eax
	movzx	\src, %eax
	call	VMI_LMSW
	popl	%eax
.endm

/*************************************************************************
 * cpuid
 *************************************************************************/
.macro cpuid
	call VMI_CPUID
.endm

/*************************************************************************
 * rdmsr
 *************************************************************************/
.macro rdmsr
	call VMI_RDMSR
.endm

/*************************************************************************
 * wrmsr
 *************************************************************************/
.macro wrmsr
	call VMI_WRMSR
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
 * rep IO
 *************************************************************************/
.macro rep p1 p2 p3
9998:
.ifeqs "\p1", "insb"
	call VMI_INSB ; .exitm
.endif
.ifeqs "\p1", "insw"
	call VMI_INSW ; .exitm
.endif
.ifeqs "\p1", "insl"
	call VMI_INSL ; .exitm
.endif
.ifeqs "\p1", "outsb"
	call VMI_OUTSB ; .exitm
.endif
.ifeqs "\p1", "outsw"
	call VMI_OUTSW ; .exitm
.endif
.ifeqs "\p1", "outsl"
	call VMI_OUTSL ; .exitm
.endif
	!rep
.ifnes "\p3", ""
	\p1 \p2, \p3 
.else
.ifnes "\p2", ""
	\p1 \p2
.else
	\p1
.endif
.endif
.endm
