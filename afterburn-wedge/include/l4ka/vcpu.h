/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/l4ka/vcpu.h
 * Description:   The per-CPU data declarations for the L4Ka environment.
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
 * $Id: vcpu.h,v 1.19 2006/01/04 15:16:24 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__VCPU_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__VCPU_H__

#define OFS_VCPU_CPU	32
#define OFS_CPU_FLAGS	(0 + OFS_VCPU_CPU)
#define OFS_CPU_CS	(4 + OFS_VCPU_CPU)
#define OFS_CPU_SS	(8 + OFS_VCPU_CPU)
#define OFS_CPU_TSS	(12 + OFS_VCPU_CPU)
#define OFS_CPU_IDTR	(18 + OFS_VCPU_CPU)
#define OFS_CPU_CR2	(40 + OFS_VCPU_CPU)
#define OFS_CPU_REDIRECT	(64 + OFS_VCPU_CPU)


#if !defined(ASSEMBLY)

#include <l4/thread.h>
#include INC_ARCH(cpu.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(resourcemon.h)

struct vcpu_t
{
    L4_ThreadId_t monitor_ltid;		// 0
    L4_ThreadId_t monitor_gtid;		// 4
    L4_ThreadId_t irq_ltid;		// 8
    L4_ThreadId_t irq_gtid;		// 12
    L4_ThreadId_t main_ltid;		// 16
    L4_ThreadId_t main_gtid;		// 20

    volatile bool dispatch_ipc;		// 24
    volatile word_t dispatch_ipc_nr;	// 28

    cpu_t cpu;				// 32
    word_t cpu_id;
    word_t cpu_hz;

    static const word_t vm_stack_size = KB(16);
    word_t vm_stack;
    word_t vm_stack_bottom;

    word_t guest_vaddr_offset;

    word_t vaddr_flush_min;
    word_t vaddr_flush_max;
    bool   vaddr_global_only;
    bool   global_is_crap;

    void vaddr_stats_reset()
	{
	    vaddr_flush_max = 0; 
	    vaddr_flush_min = ~vaddr_flush_max;
	    vaddr_global_only = true;
	    global_is_crap = false;
	}

    void vaddr_stats_update( word_t vaddr, bool is_global)
	{
	    if(vaddr > vaddr_flush_max) vaddr_flush_max = vaddr;
	    if(vaddr < vaddr_flush_min) vaddr_flush_min = vaddr;
	    if( !is_global)
		vaddr_global_only = false;
	    else if( !cpu.cr4.is_page_global_enabled() )
		global_is_crap = true;
	}
    bool is_global_crap()
	{ return global_is_crap; }
    void invalidate_globals()
	{ global_is_crap = true; }

    word_t get_id()
	{ return cpu_id; }

    bool i_am_preemptible() __attribute__ ((const));

    volatile bool in_dispatch_ipc()
	{ return dispatch_ipc; }
    volatile word_t get_dispatch_ipc_nr()
	{ return dispatch_ipc_nr; }
    void dispatch_ipc_enter()
	{ ASSERT(dispatch_ipc == false); dispatch_ipc = true; ++dispatch_ipc_nr; }
    void dispatch_ipc_exit()
	{ ASSERT(dispatch_ipc == true); dispatch_ipc = false; }

    word_t get_vm_stack()
	{ return vm_stack; }
    word_t get_vm_stack_bottom()
	{ return vm_stack_bottom; }
    word_t get_vm_stack_size()
	{ return vm_stack_size; }

    void set_kernel_vaddr( word_t vaddr )
	{ guest_vaddr_offset = vaddr; }
    word_t get_kernel_vaddr()
	{ return guest_vaddr_offset; }

#if !defined(CONFIG_WEDGE_STATIC)
    word_t get_wedge_vaddr()
	{ return CONFIG_WEDGE_VIRT; }
    word_t get_wedge_end_vaddr()
	{ return CONFIG_WEDGE_VIRT_END; }
    word_t get_wedge_paddr()
	{ return CONFIG_WEDGE_PHYS; }
    word_t get_wedge_end_paddr()
	{ return CONFIG_WEDGE_PHYS + MB(CONFIG_WEDGE_PHYS_SIZE); }
#endif

    word_t get_vm_max_prio()
	{ return resourcemon_shared.prio; }


    vcpu_t()
	{ dispatch_ipc = false; dispatch_ipc_nr = 0; }
};

INLINE bool vcpu_t::i_am_preemptible()
{ 
    return L4_MyLocalId() == main_ltid;
}

#endif	/* !ASSEMBLY */

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4KA__VCPU_H__ */
