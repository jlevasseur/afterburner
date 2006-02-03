/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4ka/main.cc
 * Description:   The initialization logic for the L4Ka wedge.
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
 * $Id: main.cc,v 1.24 2005/12/23 15:33:20 stoess Exp $
 *
 ********************************************************************/

#include <l4/schedule.h>
#include <l4/kip.h>

#include INC_ARCH(cpu.h)
#include INC_WEDGE(resourcemon.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(tlocal.h)
#include <l4-common/hthread.h>
#include <l4-common/irq.h>
#include <l4-common/setup.h>
#include <l4-common/monitor.h>
#include <device/acpi.h>
#include <device/apic.h>
#include <burn_symbols.h>

hconsole_t con;
#if defined(CONFIG_DEVICE_APIC)
local_apic_t __attribute__((aligned(4096))) lapic;
#endif

vcpu_t vcpu;

DECLARE_BURN_SYMBOL(vcpu);

static L4_Word8_t vm_stack[vcpu_t::vm_stack_size] ALIGNED(CONFIG_STACK_ALIGN);

#if defined(CONFIG_DEVICE_APIC)
acpi_t acpi;
#endif

void afterburn_main()
{
    hiostream_kdebug_t con_driver;
    con_driver.init();

    con.init( &con_driver, "afterburn: ");
    con << "Console initialized.\n";

    get_hthread_manager()->init( resourcemon_shared.thread_space_start,
	    resourcemon_shared.thread_space_len );

    // Setup the per-CPU VM stack.
    vcpu.vm_stack_bottom = (word_t)vm_stack;
    vcpu.vm_stack = (vcpu.vm_stack_bottom + vcpu.get_vm_stack_size() 
	    - CONFIG_STACK_SAFETY) & ~(CONFIG_STACK_ALIGN-1);

    vcpu.cpu_id = 0;
    vcpu.cpu_hz = L4_InternalFreq( 
	    L4_ProcDesc(L4_GetKernelInterface(), vcpu.cpu_id) );
    vcpu.vaddr_stats_reset();
#if defined(CONFIG_WEDGE_STATIC)
    vcpu.set_kernel_vaddr( resourcemon_shared.link_vaddr );
#else
    vcpu.set_kernel_vaddr( 0 );
#endif
    vcpu.monitor_gtid = L4_Myself();
    vcpu.monitor_ltid = L4_MyLocalId();
    vcpu.irq_ltid = L4_nilthread;
    vcpu.irq_gtid = L4_nilthread;
    vcpu.main_ltid = L4_nilthread;
    vcpu.main_gtid = L4_nilthread;

    get_burn_symbols().init();
    if( !frontend_init(&vcpu.cpu) )
	return;
    set_vcpu( &vcpu );

#if defined(CONFIG_DEVICE_APIC)
    // Initialize ACPI parser.
    acpi.init(vcpu.cpu_id);
    lapic.set_id(vcpu.cpu_id);
    vcpu.cpu.set_lapic(&lapic);
    ASSERT(sizeof(local_apic_t) == 4096); 
#endif
   
    // Create and start the IRQ thread.
    L4_Word_t irq_prio = resourcemon_shared.prio + CONFIG_PRIO_DELTA_IRQ_HANDLER;
    vcpu.irq_ltid = irq_init( 
	    irq_prio, L4_Myself(), L4_Myself(), &vcpu);
    if( L4_IsNilThread(vcpu.irq_ltid) )
	return;
    vcpu.irq_gtid = L4_GlobalId( vcpu.irq_ltid );
    con << "irq thread's TID: " << vcpu.irq_ltid << '\n';

    // Create the main VM thread.
    L4_Word_t main_prio = resourcemon_shared.prio + CONFIG_PRIO_DELTA_MAIN;
    vcpu.main_ltid = vm_init( main_prio, 
	    L4_Myself(), L4_Myself(), &vcpu, resourcemon_shared.entry_ip,
	    resourcemon_shared.entry_sp );
    if( L4_IsNilThread(vcpu.main_ltid) )
	return;
    vcpu.main_gtid = L4_GlobalId( vcpu.main_ltid );
    con << "main thread's TID: " << vcpu.main_ltid << '\n';

    // Setup priority relationships.
    if( !L4_Set_PreemptionDelay(vcpu.main_ltid, irq_prio, 2000) )
	L4_KDB_Enter("preemption delay error");

    // Enter the monitor loop.
    monitor_loop( vcpu );
}

NORETURN void panic( void )
{
    while( 1 )
    {
	con << "VM panic.  Halting VM threads.\n";
	if( get_vcpu().main_ltid != L4_MyLocalId() )
	    L4_Stop( get_vcpu().main_ltid );
	if( get_vcpu().irq_ltid != L4_MyLocalId() )
	    L4_Stop( get_vcpu().irq_ltid );
	if( get_vcpu().monitor_ltid != L4_MyLocalId() )
	    L4_Stop( get_vcpu().monitor_ltid );
	L4_Stop( L4_MyLocalId() );
	L4_KDB_Enter( "VM panic" );
    }
}

