/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/synchronous.cc
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
 * $Id: synchronous.cc,v 1.5 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include "burnxen.h"
#include INC_ARCH(page.h)
#include INC_ARCH(cpu.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)

#include <memory.h>
#include <templates.h>
#include "hypervisor.h"

extern void switch_as(unsigned long);
unsigned long the_old_pdir_paddr;
extern bool debug_set_pte;
void
backend_flush_old_pdir(u32_t new_pdir_paddr, u32_t old_pdir_paddr)
{
	if (! get_cpu().cr0.paging_enabled()) {
		/* We don't need to do anything if paging is disabled */
		return;
	}

	if (new_pdir_paddr == old_pdir_paddr) {
		int r;
		/* Just a flush */
		r = xen_tlb_flush();
		ASSERT(r == 0);
		return;
	}

	the_old_pdir_paddr = old_pdir_paddr;
	get_vcpu().set_phys_pdir(new_pdir_paddr);
	switch_as(0x80000000);
}

void
backend_invalidate_tlb( void )
{
	if (debug_set_pte)
		con << "TLB flush\n";

	xen_tlb_flush();
}
