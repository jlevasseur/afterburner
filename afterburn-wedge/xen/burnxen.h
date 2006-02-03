/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/burnxen.h
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
 * $Id: burnxen.h,v 1.5 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(console.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(tlocal.h)
#include <memory.h>

#include "hypervisor.h"

extern start_info_t start_info;
extern unsigned long end_of_memory;
extern unsigned long memory_size;

extern hconsole_t con;
extern hconsole_t debug_init;
extern hconsole_t debug_vm;

#define DEBUG_INIT_DRIVER con_driver
#define DEBUG_VM_DRIVER con_driver
/* or void_driver */

/* Init functions */
void exception_init(void);
void vm_init (unsigned long);

/* Utility functions */
bool vpn_to_mpn(unsigned long vpn, unsigned long *mpn);

void vm_unmap_region(unsigned long start, unsigned long len);

/* Linker script defined symbols */
extern unsigned long __executable_start;

static inline unsigned long align_up(unsigned long val, unsigned long alignment)
{
	/* This could be optomised to shifts if the alignment is power of
	   two. But the compiler should be able to work that out */
	return (((val - 1) / alignment) + 1) * alignment;
}


static inline unsigned long
virt_to_phys(unsigned long virt)
{
	return (virt - get_vcpu().get_kernel_vaddr());
}

static inline unsigned long
phys_to_virt(unsigned long phys)
{
	return (phys + get_vcpu().get_kernel_vaddr());
}

void queue_update(unsigned long ptr, unsigned long val);
void flush_page_update_queue(void);

#define __KERNEL_CS 0x0819 /* This is likely a Linux hack */
