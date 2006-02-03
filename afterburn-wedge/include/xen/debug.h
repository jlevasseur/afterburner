/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA) and
 *                      University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/xen/debug.h
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
 * $Id: debug.h,v 1.3 2005/04/13 15:47:34 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__XEN__DEBUG_H__
#define __AFTERBURN_WEDGE__INCLUDE__XEN__DEBUG_H__

#include INC_WEDGE(console.h)

extern NORETURN void panic( void );

#define DEBUG_STREAM hiostream_xen_t

#define PANIC(sequence)                                                 \
    do {                                                                \
        con << sequence << "\nFile: " __FILE__                          \
            << ':' << __LINE__ << "\nFunc: " << __func__  << '\n';      \
        panic();                                                        \
    } while(0)

#define ASSERT(x)			\
    do { 				\
	if(EXPECT_FALSE(!(x))) { 	\
    	    con << "Assertion: " MKSTR(x) ",\nfile " __FILE__ \
	        << ":" << __LINE__ << ",\nfunc " \
	        << __func__ << '\n';	\
	    panic();			\
	}				\
    } while(0)

#endif	/* __AFTERBURN_WEDGE__INCLUDE__XEN__DEBUG_H__ */
