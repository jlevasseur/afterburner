/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4ka/exports.cc
 * Description:   L4 exports for use by L4-aware code in the guest OS.
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
 * $Id: exports.cc,v 1.6 2005/07/25 07:34:21 joshua Exp $
 *
 ********************************************************************/

#include <l4-common/hthread.h>

#include INC_WEDGE(console.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(resourcemon.h)

#include INC_ARCH(intlogic.h)

#include <burn_symbols.h>


typedef void (*l4ka_wedge_thread_func_t)( void * );

extern "C" L4_Word_t l4ka_wedge_get_vcpu_handle( L4_Word_t l4_cpu )
{
    return (L4_Word_t)&get_vcpu();
}

extern "C" void l4ka_wedge_set_vcpu_handle( L4_Word_t handle )
{
    set_vcpu( (vcpu_t *)handle );
}

static void thread_create_trampoline( void *param, hthread_t *hthread )
{
    l4ka_wedge_thread_func_t func = (l4ka_wedge_thread_func_t)param;

    func( hthread->get_tlocal_data() );
}

extern "C" L4_ThreadId_t
l4ka_wedge_thread_create(
	L4_Word_t stack_bottom, L4_Word_t stack_size, L4_Word_t prio,
	l4ka_wedge_thread_func_t thread_func,
	void *tlocal_data, unsigned tlocal_size )
{
    L4_ThreadId_t monitor_tid = get_vcpu().monitor_gtid;

    hthread_t *hthread =
	get_hthread_manager()->create_thread( stack_bottom, stack_size,
		prio, thread_create_trampoline, monitor_tid, monitor_tid,
		(void *)thread_func, tlocal_data, tlocal_size);

    if( !hthread )
	return L4_nilthread;

    hthread->start();
    return hthread->get_global_tid();
}

extern "C" void l4ka_wedge_thread_delete( L4_ThreadId_t tid )
{
    get_hthread_manager()->terminate_thread( tid );
}

extern "C" L4_Word_t l4ka_wedge_get_irq_prio( void )
{
    return get_vcpu().get_vm_max_prio() + CONFIG_PRIO_DELTA_IRQ;
}

extern "C" void l4ka_wedge_raise_irq( L4_Word_t irq )
{
    get_intlogic().raise_irq( irq );
}

extern "C" void l4ka_wedge_raise_sync_irq( L4_Word_t irq )
{
    get_intlogic().raise_synchronous_irq( irq );
}


extern "C" L4_Word_t l4ka_wedge_phys_to_bus( L4_Word_t paddr )
{
    ASSERT( paddr < resourcemon_shared.phys_end );

    if( paddr < resourcemon_shared.phys_size )
	// Machine memory that resides in the VM.
	return paddr + resourcemon_shared.phys_offset;
    else
    {
	// Memory is arranged such that the VM's memory is swapped with
	// lower memory.  The memory above the VM is 1:1.
	if( (paddr-resourcemon_shared.phys_size) < resourcemon_shared.phys_offset )
	    // Machine memory that resides before the VM.
	    return paddr - resourcemon_shared.phys_size;
	else
	    // Machine memory that resides after the VM.
	    return paddr;
    }
}


DECLARE_BURN_SYMBOL(l4ka_wedge_get_vcpu_handle);
DECLARE_BURN_SYMBOL(l4ka_wedge_set_vcpu_handle);
DECLARE_BURN_SYMBOL(l4ka_wedge_thread_create);
DECLARE_BURN_SYMBOL(l4ka_wedge_thread_delete);
DECLARE_BURN_SYMBOL(l4ka_wedge_get_irq_prio);
DECLARE_BURN_SYMBOL(l4ka_wedge_raise_irq);
DECLARE_BURN_SYMBOL(l4ka_wedge_raise_sync_irq);
DECLARE_BURN_SYMBOL(l4ka_wedge_phys_to_bus);

DECLARE_BURN_SYMBOL(resourcemon_shared);

extern void * __L4_Ipc;
DECLARE_BURN_SYMBOL(__L4_Ipc);

