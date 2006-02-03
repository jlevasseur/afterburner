/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe and
 *                      National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/ia32/instr-kernel.patchup.S
 * Description:   Assembler macros to pre-virtualize a guest OS kernel,
 *                for runtime code rewriting.
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
 * $Id: instr-kernel.patchup.S,v 1.14 2005/12/16 11:54:18 joshua Exp $
 *
 ********************************************************************/

/*************************************************************************
 * Global descriptor table
 *************************************************************************/
.macro lgdt mem_op
9998:	!lgdt \mem_op  /* 7 bytes */
	nop            /* 8 bytes */
	nop
	nop
	nop
	nop           /* 12 bytes */
	nop
	nop
	nop
	nop           /* 16 bytes */
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro lgdtl mem_op
	lgdt \mem_op
.endm

.macro sgdtl mem_op
9998:	!sgdtl	\mem_op
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro sgdt mem_op
	sgdtl \mem_op
.endm

/*************************************************************************
 * Interrupt descriptor table
 *************************************************************************/
.macro lidt mem_op
9998:	!lidt \mem_op  /* 7 bytes */
	nop            /* 8 bytes */
	nop
	nop
	nop
	nop           /* 12 bytes */
	nop
	nop
	nop
	nop           /* 16 bytes */
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro lidtl mem_op
	lidt \mem_op
.endm

.macro sidtl mem_op
9998:	!sidtl	\mem_op
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro sidt
	sidtl	\mem_op
.endm

/*************************************************************************
 * mov
 *************************************************************************/

.macro __afterburn_mov src, dst
9998:	!mov	\src, \dst    /* 2 bytes (minimum) */
	nop
	nop
	nop
	nop
	nop
	nop            /* 8 bytes */
	nop
	nop
	nop
	nop           /* 12 bytes */
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro mov src, dst

/*
 * cr0
 */
.ifeqs "\src", "%cr0"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%cr0"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * cr2
 */
.ifeqs "\src", "%cr2"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%cr2"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * cr3
 */
.ifeqs "\src", "%cr3"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%cr3"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * cr4
 */
.ifeqs "\src", "%cr4"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%cr4"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * ss
 */
.ifeqs "\src", "%ss"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%ss"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * ds
 */
.ifeqs "\src", "%ds"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%ds"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * es
 */
.ifeqs "\src", "%es"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%es"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * fs
 */
.ifeqs "\src", "%fs"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%fs"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * gs
 */
.ifeqs "\src", "%gs"
	__afterburn_mov \src, \dst ; .exitm
.endif 
.ifeqs "\dst", "%gs"
	__afterburn_mov \src, \dst ; .exitm
.endif

/*
 * debug registers
 */
.ifeqs "\src", "%db0"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db0"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db1"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db1"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db2"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db2"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db3"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db3"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db4"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db4"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db5"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db5"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db6"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db6"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\src", "%db7"
	__afterburn_mov \src, \dst ; .exitm
.endif
.ifeqs "\dst", "%db7"
	__afterburn_mov \src, \dst ; .exitm
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
9998:	!ljmp	\cs, \loc    /* 7 bytes */
	nop            /* 8 bytes */
	nop
	nop
	nop
	nop           /* 12 bytes */
	nop
	nop
	nop
	nop           /* 16 bytes */
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro __afterburn_farptr op, loc, dst
9998:	!\op	\loc, \dst    /* 7 bytes */
	nop            /* 8 bytes */
	nop
	nop
	nop
	nop           /* 12 bytes */
	nop
	nop
	nop
	nop           /* 16 bytes */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro lds loc, dst
	__afterburn_farptr lds, \loc, \dst
.endm

.macro les loc, dst
	__afterburn_farptr les, \loc, \dst
.endm

.macro lfs loc, dst
	__afterburn_farptr lfs, \loc, \dst
.endm

.macro lgs loc, dst
	__afterburn_farptr lgs, \loc, \dst
.endm

.macro lss loc, dst
	__afterburn_farptr lss, \loc, \dst
.endm

/*************************************************************************
 * lock
 *************************************************************************/
.macro lock
9998:	!lock
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * flags
 *************************************************************************/
.macro pushfl
9998:	!pushfl		/* Push the unprivileged flags. 1-byte opcode. */
	nop		/* 5-bytes needed for call instruction. */
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro pushfd
	pushfl
.endm

.macro popfl
9998:	!popfl		/* Push the unprivileged flags. */
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
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro popfd
	popfl
.endm

/*************************************************************************
 * clts
 *************************************************************************/
.macro clts
9998:	!clts
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
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * hlt
 *************************************************************************/
.macro hlt
9998:	!hlt
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

	
/*************************************************************************
 * cli
 *************************************************************************/
.macro cli
9998:	!cli
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * sti
 *************************************************************************/
.macro sti
9998:	!sti
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
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * lldt
 *************************************************************************/
.macro lldt seg
9998:	!lldt \seg
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
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * sldt
 *************************************************************************/

.macro sldt dst
9998:	!sldt	\dst
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro sldtl dst
	sldt	\dst
.endm

/*************************************************************************
 * ltr
 *************************************************************************/
.macro ltr seg
9998:	!ltr \seg
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
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro str dst
9998:	!str \dst
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
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro strl dst
	str	\dst
.endm

/*************************************************************************
 * in
 *************************************************************************/
.macro __afterburn_read_port op, d, a
9998:	!\op \d, \a
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
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro inb d, a
	__afterburn_read_port inb, \d, \a
.endm
.macro inw d, a
	__afterburn_read_port inw, \d, \a
.endm
.macro inl d, a
	__afterburn_read_port inl, \d, \a
.endm

/*************************************************************************
 * out
 *************************************************************************/
.macro __afterburn_write_port op, d, a
9998:	!\op \d, \a
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
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro outb d, a
	__afterburn_write_port outb, \d, \a
.endm
.macro outw d, a
	__afterburn_write_port outw, \d, \a
.endm
.macro outl d, a
	__afterburn_write_port outl, \d, \a
.endm


/*************************************************************************
 * invlpg
 *************************************************************************/
.macro invlpg src
9998:	!invlpg \src
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm


/*************************************************************************
 * iret
 *************************************************************************/
.macro iret
9998:	!iret
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro iretd
	iret
.endm

.macro iretl
	iret
.endm

.macro lret
9998:	!lret
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm



.macro cpuid
9998:	!cpuid
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * wrmsr
 *************************************************************************/
.macro wrmsr
9998:	!wrmsr
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * rdmsr
 *************************************************************************/
.macro rdmsr
9998:	!rdmsr
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm



/*************************************************************************
 * int
 *************************************************************************/
.macro int vector
9998:	!int \vector
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
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * push
 *************************************************************************/
.macro __afterburn_push src
9998:	!pushl	\src
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro pushl src
.ifeqs "\src", "%cs"
	__afterburn_push \src ; .exitm
.endif
.ifeqs "\src", "%ds"
	__afterburn_push \src ; .exitm
.endif
.ifeqs "\src", "%es"
	__afterburn_push \src ; .exitm
.endif
.ifeqs "\src", "%fs"
	__afterburn_push \src ; .exitm
.endif
.ifeqs "\src", "%gs"
	__afterburn_push \src ; .exitm
.endif
.ifeqs "\src", "%ss"
	__afterburn_push \src ; .exitm
.endif

/*
 * innocuous
 */
	!pushl	\src
.endm

.macro push src
	pushl	\src
.endm

/*************************************************************************
 * popl
 *************************************************************************/
.macro __afterburn_pop dst
9998:	!popl	\dst
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro popl dst
.ifeqs "\dst", "%cs"
	__afterburn_pop \dst ; .exitm
.endif
.ifeqs "\dst", "%ds"
	__afterburn_pop \dst ; .exitm
.endif
.ifeqs "\dst", "%es"
	__afterburn_pop \dst ; .exitm
.endif
.ifeqs "\dst", "%fs"
	__afterburn_pop \dst ; .exitm
.endif
.ifeqs "\dst", "%gs"
	__afterburn_pop \dst ; .exitm
.endif
.ifeqs "\dst", "%ss"
	__afterburn_pop \dst ; .exitm
.endif

/*
 * innocuous
 */
	!popl	\dst
.endm

.macro pop dst
	popl	\dst
.endm

/*************************************************************************
 * ud2
 *************************************************************************/
.macro ud2
9998:	!ud2
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * invd
 *************************************************************************/
.macro invd
9998:	!invd
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * wbinvd
 *************************************************************************/
.macro wbinvd
9998:	!wbinvd
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * smsw + lmsw
 *************************************************************************/
.macro smsw dst
9998:	!smsw \dst
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

.macro smswl dst
	smsw	\dst
.endm

.macro lmsw src
9998:	!lmsw \src
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * arpl
 *************************************************************************/
.macro arpl
9998:	!arpl
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * lar
 *************************************************************************/
.macro lar
9998:	!lar
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * lsl
 *************************************************************************/
.macro lsl
9998:	!lsl
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

/*************************************************************************
 * rsm
 *************************************************************************/
.macro rsm
9998:	!rsm
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
9999:
.pushsection .afterburn
.align 4
.long 9998b
.long 9999b
.popsection
.endm

