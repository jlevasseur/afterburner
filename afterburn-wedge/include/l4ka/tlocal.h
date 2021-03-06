/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/l4ka/tlocal.h
 * Description:   Thread-local declarations.
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
 * $Id: tlocal.h,v 1.6 2005/04/13 15:47:33 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__

#include <l4/thread.h>

#include INC_ARCH(cpu.h)
#include INC_WEDGE(vcpu.h)

INLINE vcpu_t & get_vcpu() __attribute__((const));
INLINE vcpu_t & get_vcpu()
    // Get the thread local virtual CPU object.  Return a reference, so that by 
    // definition, we must return a valid object.
{
#if defined(CONFIG_SMP)
    vcpu_t *vcpu = (vcpu_t *)L4_UserDefinedHandle();
    return *vcpu;
#else
    extern vcpu_t vcpu;
    return vcpu;
#endif
}

INLINE cpu_t & get_cpu() __attribute__((const));
INLINE cpu_t & get_cpu()
    // Get the thread local architecture CPU object.  Return a reference, so 
    // that by definition, we must return a valid object.
{
    return get_vcpu().cpu;
}

INLINE void set_vcpu( vcpu_t * vcpu )
{
    L4_Set_UserDefinedHandle( (L4_Word_t)vcpu );
}

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__ */
