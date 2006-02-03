/*********************************************************************
 *
 * Copyright (C) 2005, University of Karlsruhe
 *
 * File path:     afterburn-wedge/ia64/hthread.cc
 * Description:   IA64 support for the L4 hthread library.
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
 * $Id: hthread.cc,v 1.2 2005/04/13 14:28:25 joshua Exp $
 *
 ********************************************************************/

#include <l4-common/hthread.h>

// stack[-1] = the RSE location.
// stack[-2] = the C function IP address to execute.
// stack[-3] = the C function GP.

typedef struct
{
    L4_Word_t ip;
    L4_Word_t gp;
} ia64_func_info_t;

/*
 * Startup stub for setting up the appropriate GP, BSP and IP values.
 */
asm ("          .align  16                              \n"
     "		.text					\n"
     "          .global _ia64_hthread_startup_stub      \n"
     "          .proc   _ia64_hthread_startup_stub      \n"
     "                                                  \n"
     "_ia64_hthread_startup_stub:                       \n"
     "          movl    gp = __gp                       \n"
     "          ld8     r7 = [sp], 8  /* gp */          \n"
     "          ;;                                      \n"
     "          ld8     r8 = [sp], 8  /* ip */          \n"
     "          ;;                                      \n"
     "          ld8     r9 = [sp], 8  /* RSE */         \n"
     "          ;;                                      \n"
     "          mov     b0 = r8                         \n"
     "          alloc   r10 = ar.pfs,0,0,0,0            \n"
     "          mov     ar.rsc = 0                      \n"
     "          ;;                                      \n"
     "          loadrs                                  \n"
     "          ;;                                      \n"
     "          mov     ar.bspstore = r9                \n"
     "          ;;                                      \n"
     "          mov     ar.rsc = 3                      \n"
     "          br.sptk.many b0                         \n"
     "                                                  \n"
     "          .endp   _ia64_hthread_startup_stub      \n"
);

void hthread_t::arch_prepare_exreg( L4_Word_t &sp, L4_Word_t &ip )
{
    L4_Word_t *stack = (L4_Word_t *)sp;
    ia64_func_info_t *c_func = (ia64_func_info_t *)hthread_t::self_start;

    extern L4_Word_t _ia64_hthread_startup_stub;

    stack[-1] = sp - stack_size;
    stack[-2] = c_func->ip;
    stack[-3] = c_func->gp;

    sp = (L4_Word_t)&stack[-3];
    ip = (L4_Word_t)&_ia64_hthread_startup_stub;
}

