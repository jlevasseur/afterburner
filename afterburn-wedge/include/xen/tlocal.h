/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA) and
 *                      University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/xen/tlocal.h
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
 * $Id: tlocal.h,v 1.2 2005/04/13 15:47:34 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__

#include INC_ARCH(cpu.h)
#include INC_WEDGE(vcpu.h)
#include <hiostream.h>

extern vcpu_t the_vcpu;

INLINE vcpu_t & get_vcpu() __attribute__((const));

INLINE vcpu_t & get_vcpu()
    // Get the thread local virtual CPU object.  Return a reference, so that by 
    // definition, we must return a valid object.
{
    return the_vcpu;
}

INLINE cpu_t & get_cpu() __attribute__((const));

INLINE cpu_t & get_cpu()
    // Get the thread local architecture CPU object.  Return a reference, so 
    // that by definition, we must return a valid object.
{
    return get_vcpu().cpu;
}

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__ */
