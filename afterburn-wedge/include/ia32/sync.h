/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/ia32/sync.h
 * Description:   IA32 synchronization support (atomic operations).
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
 * $Id: sync.h,v 1.3 2005/04/13 15:47:31 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__IA32__SYNC_H__
#define __AFTERBURN_WEDGE__INCLUDE__IA32__SYNC_H__

#include INC_ARCH(types.h)

#ifdef CONFIG_SMP
#define SMP_PREFIX "lock; "
#else
#define SMP_PREFIX ""
#endif

INLINE word_t 
cmpxchg( volatile word_t *addr, word_t old_val, word_t new_val )
{
    word_t actual;
    __asm__ __volatile__ (
	    SMP_PREFIX "cmpxchgl %1, %2"
	    : "=a"(actual)
	    : "q"(new_val), "m"(*addr), "0"(old_val)
	    : "memory"
	    );
    return actual;
}

INLINE void atomic_inc( volatile word_t *addr )
{
    __asm__ __volatile__ (
	    SMP_PREFIX "incl %0"
	    : "=m" (*addr)
	    : "m" (*addr)
	    );
}

INLINE bool atomic_dec_and_test( volatile word_t *addr )
{
    u8_t flag; // set to 1 if zero

    __asm__ __volatile__ (
	    SMP_PREFIX "decl %0; sete %1"
	    : "=m" (*addr), "=qm" (flag)
	    : "m" (*addr)
	    : "memory"
	    );
    return flag != 0;
}

#endif	/* __AFTERBURN_WEDGE__INCLUDE__IA32__SYNC_H__ */
