/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/burnxen.cc
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
 * $Id: burnxen.cc,v 1.3 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/


#include "burnxen.h"

#if 0
bool
vpn_to_mpn(unsigned long vpn, unsigned long *mpn)
{
	int pdir_idx = vpn >> 10;
	int pt_idx = vpn & 0xfff;
	unsigned long pt;
	unsigned long pt_fn; /* pagetable frmaenumber */
	unsigned long pt_pn; /* pagetable physical number */
	unsigned long * pt_p;
	unsigned long pte;
	/* Find page directoy */
	pt = pde[pdir_idx];
	if (! (pt & 0x1)) {
		return false;
	}
	pt_fn = (pt >> 12); 
	pt_pn = mfn_to_pfn(pt_fn);
	pt_p = (unsigned long *) (virt_start + (pt_pn << 12));
	pte = pt_p[pt_idx];
	if (! (pte & 0x1)) {
		return false;
	}

	*mpn = pte >> 12;

	return true;
}
#endif

void
panic(void)
{
	con << "Entering xen debugger..... ";
	for(int i=0; i < 5000000; i++);
	con << "just kidding, Xen doesn't have a decent debugger.. lets just spin\n";
	while(1);
}

#define QUEUE_SIZE 1024
static mmu_update_t update_queue[QUEUE_SIZE];
unsigned int idx = 0;

void flush_page_update_queue(void)
{
	cpu_t &cpu = get_cpu();
    bool old_if_state = cpu.disable_interrupts();
    unsigned int _idx = idx;
    idx = 0;
    //wmb(); /* Make sure index is cleared first to avoid double updates. */
    if ( HYPERVISOR_mmu_update(update_queue, _idx, NULL) < 0)
    {
	    PANIC("baddness happend\n");
    }
    cpu.restore_interrupts( old_if_state );
}

static inline void increment_index(void)
{
    idx++;
    if (idx == QUEUE_SIZE) flush_page_update_queue();
}

void queue_update(unsigned long ptr, unsigned long val)
{
	cpu_t &cpu = get_cpu();
    bool old_if_state = cpu.disable_interrupts();
    update_queue[idx].ptr = ptr;
    update_queue[idx].val = val;
    increment_index();
    cpu.restore_interrupts( old_if_state );
}
