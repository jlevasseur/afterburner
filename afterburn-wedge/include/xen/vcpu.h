/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA) and
 *                      University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/xen/vcpu.h
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
 * $Id: vcpu.h,v 1.3 2005/04/13 15:47:34 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__XEN__VCPU_H__
#define __AFTERBURN_WEDGE__INCLUDE__XEN__VCPU_H__

#include INC_ARCH(cpu.h)

struct vcpu_t
{
        cpu_t cpu; // VU: hard coded asm--don't touch (see entry.S)
	word_t guest_vaddr_offset;
	word_t phys_pdir;
	unsigned long l1, l2, l1i, l2i;
	word_t vaddr_flush_min;
	word_t vaddr_flush_max;
	bool   global_is_crap;
	bool   vaddr_global_only;

	bool pse_supported(void) { return false; }
	bool pge_supported(void) { return false; }

	word_t *get_pdir(void) {
		return (word_t *) (phys_pdir + guest_vaddr_offset);
	}

	void set_phys_pdir(word_t new_phys_pdir) {
		phys_pdir = new_phys_pdir;
	}

	void set_kernel_vaddr( word_t vaddr )
	{ guest_vaddr_offset = vaddr; }
	word_t get_kernel_vaddr()
	{ return guest_vaddr_offset; }

	void vaddr_stats_reset()
	{
	    vaddr_flush_max = 0; 
	    vaddr_flush_min = ~vaddr_flush_max;
	    vaddr_global_only = true;
	    global_is_crap = false;
	}


	bool is_global_crap()
	{ return global_is_crap; }
	
	void invalidate_globals()
	{ global_is_crap = true; }
};

#endif	/* __AFTERBURN_WEDGE__INCLUDE__XEN__VCPU_H__ */
