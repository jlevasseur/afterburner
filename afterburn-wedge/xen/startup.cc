/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA) and
 *                      University of Karlsruhe
 *
 * File path:     afterburn-wedge/xen/startup.cc
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
 * $Id: startup.cc,v 1.3 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(console.h)
#include INC_WEDGE(hypervisor.h)

extern void afterburn_main(start_info_t *si, unsigned long stack);

static void ctors_exec( void )
{
    extern void (*afterburn_ctors_start)(void);
    void (**ctors)(void) = &afterburn_ctors_start;

    // ctors start *after* the afterburn_ctors_start symbol.
    for( unsigned int i = 1; ctors[i]; i++ )
	ctors[i]();
}

static void dtors_exec( void )
{
    extern void (*afterburn_dtors_start)(void);
    void (**dtors)(void) = &afterburn_dtors_start;

    // dtors start *after the afterburn_dtors_start symbol.
    for( unsigned int i = 1; dtors[i]; i++ )
	dtors[i]();
}

extern "C" NORETURN void afterburn_c_runtime_init(start_info_t *start, unsigned long stack)
{
    ctors_exec();
    afterburn_main(start, stack);
    dtors_exec();

    while( 1 ) {
    }
}
