/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4ka/ia32/backend.cc
 * Description:   L4 backend implementation.  Specific to IA32 and to the
 *                research resource monitor.
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
 * $Id: backend.cc,v 1.29 2006-01-11 19:02:07 stoess Exp $
 *
 ********************************************************************/

#include INC_ARCH(page.h)
#include INC_ARCH(cpu.h)
#include INC_ARCH(intlogic.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(resourcemon.h)
#include <device/acpi.h>
#include <device/apic.h>

#include <l4-common/message.h>

#include <burn_counters.h>

static const bool debug=0;
static const bool debug_page_not_present=0;
static const bool debug_irq_forward=0;
static const bool debug_irq_deliver=0;
static const bool debug_pfault=0;
static const bool debug_superpages=0;
static const bool debug_user_access=0;
static const bool debug_dma=0;
static const bool debug_apic=0;
static const bool debug_device=0;

DECLARE_BURN_COUNTER(async_delivery_canceled);

INLINE bool async_safe( word_t ip )
{
    return ip < CONFIG_WEDGE_VIRT;
}

void backend_enable_paging( word_t *ret_address )
{
    vcpu_t &vcpu = get_vcpu();
    CORBA_Environment ipc_env = idl4_default_environment;
    bool int_save;

    // Reconfigure our virtual address window in the VMM.
    int_save = vcpu.cpu.disable_interrupts();
    IHypervisor_set_virtual_offset( 
	    resourcemon_shared.cpu[L4_ProcessorNo()].thread_server_tid,
	    vcpu.get_kernel_vaddr(), &ipc_env );
    vcpu.cpu.restore_interrupts( int_save, true );

    if( ipc_env._major != CORBA_NO_EXCEPTION ) {
	CORBA_exception_free( &ipc_env );
	panic();
    }

    // Convert the return address into a virtual address.
    // TODO: make this work with static stuff too.  Currently it depends
    // on the burn.S code.
    *ret_address += vcpu.get_kernel_vaddr();

    // Flush our current mappings.
    backend_flush_user();
}

static bool deliver_ia32_vector( 
	cpu_t & cpu, L4_Word_t vector, u32_t error_code )
{
    // - Byte offset from beginning of IDT base is 8*vector.
    // - Compare the offset to the limit value.  The limit is expressed in 
    // bytes, and added to the base to get the address of the last
    // valid byte.
    // - An empty descriptor slot should have its present flag set to 0.

    vcpu_t &vcpu = get_vcpu();

    ASSERT( L4_MyLocalId() == vcpu.monitor_ltid );

    if( vector > cpu.idtr.get_total_gates() ) {
	con << "No IDT entry for vector " << vector << '\n';
	return false;
    }

    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    if(debug)
	con << "Delivering vector " << vector
	    << ", handler ip " << (void *)gate.get_offset() << '\n';

    flags_t old_flags = cpu.flags;
    if( gate.is_interrupt() )
	cpu.disable_interrupts();
    cpu.flags.prepare_for_interrupt();

    u16_t old_cs = cpu.cs;
    cpu.cs = gate.get_segment_selector();

    L4_Word_t eip, esp, eflags, *stack, dummy;
    L4_ThreadId_t dummy_tid, res;
    L4_ThreadId_t main_ltid = vcpu.main_ltid;

    // Read registers of page fault.
    // TODO: get rid of this call to exchgregs ... perhaps enhance page fault
    // protocol with more info.
    res = L4_ExchangeRegisters( main_ltid, 0, 0, 0, 0, 0, L4_nilthread,
	    &dummy, &esp, &eip, &eflags, &dummy, &dummy_tid );
    if( L4_IsNilThread(res) )
	return false;

    if( !async_safe(eip) )
	con << "Interrupting the wedge to handle a fault, ip "
	    << (void *)eip << '\n';

    // Store values on the stack.
    stack = (L4_Word_t *)esp;
    *(--stack) = (eflags & flags_user_mask) | (old_flags.x.raw & ~flags_user_mask);
    *(--stack) = old_cs;
    *(--stack) = eip;
    *(--stack) = error_code;

    // Update thread registers with target execution point.
    res = L4_ExchangeRegisters( main_ltid, 3 << 3 /* i,s */,
	    (L4_Word_t)stack, gate.get_offset(), 0, 0, L4_nilthread,
	    &dummy, &dummy, &dummy, &dummy, &dummy, &dummy_tid );
    if( L4_IsNilThread(res) )
	return false;

    return true;
}

void backend_handle_pagefault( 
    word_t fault_addr,
    word_t ip,
    word_t & mapped_addr,
    word_t & page_bits,
    word_t & rwx )
{
    CORBA_Environment ipc_env = idl4_default_environment;
    idl4_fpage_t fp;
    vcpu_t &vcpu = get_vcpu();
    cpu_t &cpu = vcpu.cpu;
    L4_Word_t link_addr = vcpu.get_kernel_vaddr();
    L4_Word_t paddr = fault_addr;
    L4_Word_t fault_rwx = rwx;
    L4_Word_t dev_req_page_size;
    
    page_bits = PAGE_BITS;
    rwx = 7;

    L4_Word_t wedge_addr = vcpu.get_wedge_vaddr();
    L4_Word_t wedge_end_addr = vcpu.get_wedge_end_vaddr();

#if !defined(CONFIG_WEDGE_STATIC)
    if( (fault_addr >= wedge_addr) && (fault_addr < wedge_end_addr) )
    {
	// A page fault in the wedge.
	mapped_addr = fault_addr;
	idl4_set_rcv_window( &ipc_env, L4_CompleteAddressSpace );
	IHypervisor_pagefault( L4_Pager(), mapped_addr, ip, rwx, &fp, &ipc_env);
	return;
    }
#endif

    if( debug_pfault )
	con << "Page fault, " << (void *)fault_addr
	    << ", ip " << (void *)ip << ", rwx " << fault_rwx << '\n';

    if( cpu.cr0.paging_enabled() )
    {
	pgent_t *pdir = (pgent_t *)(cpu.cr3.get_pdir_addr() + link_addr);
	pdir = &pdir[ pgent_t::get_pdir_idx(fault_addr) ];
	if( !pdir->is_valid() ) {
	    if( debug_page_not_present )
		con << "pdir not present\n";
	    goto not_present;
	}

	if( pdir->is_superpage() && cpu.cr4.is_pse_enabled() ) {
	    paddr = (pdir->get_address() & SUPERPAGE_MASK) + 
		(fault_addr & ~SUPERPAGE_MASK);
	    page_bits = SUPERPAGE_BITS;
	    if( !pdir->is_writable() )
		rwx = 5;
	    if( debug_superpages )
		con << "super page fault at " << (void *)fault_addr << '\n';
	    if( debug_user_access && 
		    !pdir->is_kernel() && (fault_addr < link_addr) )
	       	con << "user access, ip " << (void *)ip << '\n';

	    vcpu.vaddr_stats_update( fault_addr & SUPERPAGE_MASK, pdir->is_global());
	}
	else 
	{
	    pgent_t *ptab = (pgent_t *)(pdir->get_address() + link_addr);
	    pgent_t *pgent = &ptab[ pgent_t::get_ptab_idx(fault_addr) ];
	    if( !pgent->is_valid() )
		goto not_present;
	    paddr = pgent->get_address() + (fault_addr & ~PAGE_MASK);
	    if( !pgent->is_writable() )
		rwx = 5;
	    if( debug_user_access &&
		    !pgent->is_kernel() && (fault_addr < link_addr) )
	       	con << "user access, ip " << (void *)ip << '\n';

	    vcpu.vaddr_stats_update( fault_addr & PAGE_MASK, pgent->is_global());
	}

	if( ((rwx & fault_rwx) != fault_rwx) && cpu.cr0.write_protect() )
	    goto permissions_fault;
    }
    else if( paddr > link_addr )
	paddr -= link_addr;
 
    L4_Fpage_t fp_recv, fp_req;

#if defined(CONFIG_DEVICE_APIC)
    word_t ioapic_id;
    if (acpi.is_lapic_addr(paddr))
    {
	if (debug_apic)
	    con << "LAPIC  access @ " << (void *)fault_addr << " (" << (void *) paddr 
		<< "), mapping softLAPIC @" << (void *) get_cpu().get_lapic() << " \n";
	    
	mapped_addr = fault_addr;
	fp_recv = L4_FpageLog2( mapped_addr, PAGE_BITS );
	    
	word_t lapic_paddr = (word_t) get_cpu().get_lapic() - 
	    get_vcpu().get_wedge_vaddr() + get_vcpu().get_wedge_paddr();
	    
	fp_req = L4_FpageLog2(lapic_paddr, PAGE_BITS );
	idl4_set_rcv_window( &ipc_env, fp_recv );
	IHypervisor_request_pages( L4_Pager(), fp_req.raw, 5, &fp, &ipc_env );
	return;
    }
    else if (acpi.is_ioapic_addr(paddr, &ioapic_id))
    {
	word_t ioapic_addr = 0;
	intlogic_t &intlogic = get_intlogic();
	for (word_t idx = 0; idx < CONFIG_MAX_IOAPICS; idx++)
	    if ((intlogic.ioapic[idx].get_id()) == ioapic_id)
		ioapic_addr = (word_t) &intlogic.ioapic[idx];
	    
	if (ioapic_addr == 0)
	{
	    con << "BUG: Access to nonexistent IOAPIC (id " << ioapic_id << ")\n"; 
	    L4_KDB_Enter("BUG");
	    panic();
	}
		    
	if (debug_apic)
	    con << "IOAPIC access @ " << (void *)fault_addr 
		<< " (id" << ioapic_id << " / "   << (void *) paddr  
		<< "), mapping softIOAPIC @ " << (void *) ioapic_addr << " \n";

	mapped_addr = fault_addr;
	fp_recv = L4_FpageLog2( mapped_addr, PAGE_BITS );
	word_t ioapic_paddr = (word_t) ioapic_addr -
	    get_vcpu().get_wedge_vaddr() + get_vcpu().get_wedge_paddr();
	fp_req = L4_FpageLog2(ioapic_paddr, PAGE_BITS );
	idl4_set_rcv_window( &ipc_env, fp_recv );
	IHypervisor_request_pages( L4_Pager(), fp_req.raw, 5, &fp, &ipc_env );
	return;
    }
#endif

#if 1
    // Only get a page, no matter if PT entry is a superpage
    dev_req_page_size = PAGE_SIZE;
#else 
    // Get the whole superpage for devices
    dev_req_page_size = (1UL << page_bits);
#endif
    
    if (contains_device_mem(paddr, paddr + (dev_req_page_size - 1)))
#if defined(CONFIG_DEVICE_PASSTHRU)
	goto device_access;
#else
    PANIC( "unservicable page fault (device access)" );
#endif
    
    mapped_addr = paddr + link_addr;
    fp_recv = L4_FpageLog2( mapped_addr, page_bits );
    // TODO: use 4MB pages
    fp_req = L4_FpageLog2( paddr, PAGE_BITS );
    idl4_set_rcv_window( &ipc_env, fp_recv );
    IHypervisor_request_pages( L4_Pager(), fp_req.raw, 7, &fp, &ipc_env );

    if( ipc_env._major != CORBA_NO_EXCEPTION ) {
	CORBA_exception_free( &ipc_env );
	con << "IPC request failure to the pager.\n";
	panic();
    }

    if( L4_IsNilFpage(idl4_fpage_get_page(fp)) ) {
	con << "The resource monitor denied a page request at "
	    << (void *)L4_Address(fp_req)
	    << ", size " << (1 << page_bits) << '\n';
	panic();
    }

    return;

device_access:
    mapped_addr = fault_addr & ~(dev_req_page_size -1);	
    paddr &= ~(dev_req_page_size -1);
 
    if (debug_device)
	con << "device access, vaddr " << (void *)fault_addr
	    << ", mapped_addr " << (void *)mapped_addr
	    << ", paddr " << (void *)paddr 
	    << ", size "  << dev_req_page_size
	    << ", ip " << (void *)ip << '\n';

    for (word_t pt=0; pt < dev_req_page_size ; pt+= PAGE_SIZE)
    {
	fp_recv = L4_FpageLog2( mapped_addr + pt, PAGE_BITS );
	fp_req = L4_FpageLog2( paddr + pt, PAGE_BITS);
	idl4_set_rcv_window( &ipc_env, fp_recv);
	IHypervisor_request_device( L4_Pager(), fp_req.raw, L4_FullyAccessible, &fp, &ipc_env );
	vcpu.vaddr_stats_update(mapped_addr + pt, false);
    }
    
    return;
    
not_present:
    if( debug_page_not_present )
	con << "page not present, fault addr " << (void *)fault_addr
	    << ", ip " << (void *)ip << '\n';
    cpu.cr2 = fault_addr;
    if( deliver_ia32_vector(cpu, 14, (fault_rwx & 2) | 0) ) {
	mapped_addr = fault_addr;
	return;
    }
    goto unhandled;

permissions_fault:
    cpu.cr2 = fault_addr;
    if (debug)
	con << "Delivering page fault for addr " << (void *)fault_addr
	    << ", permissions " << fault_rwx 
	    << ", ip " << (void *)ip << '\n';
    if( deliver_ia32_vector(cpu, 14, (fault_rwx & 2) | 1) ) {
	mapped_addr = fault_addr;
	return;
    }
    /* fall through */

unhandled:
    con << "Unhandled page permissions fault, fault addr " << (void *)fault_addr
	<< ", ip " << (void *)ip << ", fault rwx " << fault_rwx << '\n';
    panic();
}


extern "C" void async_irq_handle_exregs( void );
__asm__ ("\n\
	.text								\n\
	.global async_irq_handle_exregs					\n\
async_irq_handle_exregs:						\n\
	pushl	%eax							\n\
	pushl	%ebx							\n\
	movl	(4+8)(%esp), %eax	/* old stack */			\n\
	subl	$16, %eax		/* allocate iret frame */	\n\
	movl	(16+8)(%esp), %ebx	/* old flags */			\n\
	movl	%ebx, 12(%eax)						\n\
	movl	(12+8)(%esp), %ebx	/* old cs */			\n\
	movl	%ebx, 8(%eax)						\n\
	movl	(8+8)(%esp), %ebx	/* old eip */			\n\
	movl	%ebx, 4(%eax)						\n\
	movl	(0+8)(%esp), %ebx	/* new eip */			\n\
	movl	%ebx, 0(%eax)						\n\
	xchg	%eax, %esp		/* swap to old stack */		\n\
	movl	(%eax), %ebx		/* restore ebx */		\n\
	movl	4(%eax), %eax		/* restore eax */		\n\
	ret				/* activate handler */		\n\
	");


void backend_async_irq_deliver( intlogic_t &intlogic )
{
    static const L4_Word_t temp_stack_words = 64;
    static L4_Word_t temp_stacks[ temp_stack_words * CONFIG_NR_CPUS ];
    vcpu_t &vcpu = get_vcpu();
    cpu_t &cpu = vcpu.cpu;
    L4_Word_t *stack = (L4_Word_t *)
	&temp_stacks[ (vcpu.get_id()+1) * temp_stack_words ];

    ASSERT( L4_MyLocalId() != vcpu.main_ltid );
    ASSERT( L4_MyLocalId() != vcpu.monitor_ltid );

    word_t vector, irq, save_int;
    
    save_int = cpu.disable_interrupts();
    
    if (!save_int)
	return;
    
    if( !intlogic.pending_vector(vector, irq) )
    {
	cpu.restore_interrupts(true);
	// No pending interrupts.
	return;
    }
     
    if( debug_irq_deliver || intlogic.is_irq_traced(irq)  )
 	con << "Interrupt deliver, vector " << vector << '\n';
 
    ASSERT( vector < cpu.idtr.get_total_gates() );
    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    if (gate.is_trap())
	cpu.restore_interrupts(true);
    
    if( debug_irq_deliver || intlogic.is_irq_traced(irq)  )
 	con << "Delivering vector " << vector
 	    << ", handler ip " << (void *)gate.get_offset() << '\n';
 
    stack = &stack[-5]; // Allocate room on the temp stack.
    stack[3] = cpu.cs;
    stack[0] = gate.get_offset();

    L4_Word_t esp, eip, eflags, dummy;
    L4_ThreadId_t dummy_tid, result_tid;
    L4_ThreadState_t l4_tstate;
    result_tid = L4_ExchangeRegisters( vcpu.main_ltid, (3 << 3) | 2,
	    (L4_Word_t)stack, (L4_Word_t)async_irq_handle_exregs,
	    0, 0, L4_nilthread,
	    &l4_tstate.raw, &esp, &eip, &eflags,
	    &dummy, &dummy_tid );
    ASSERT( !L4_IsNilThread(result_tid) );

#if defined(CONFIG_L4KA_REENTRANT)
    if( EXPECT_FALSE((esp >= (word_t)temp_stacks) &&
		    (esp < ((word_t)temp_stacks + sizeof(temp_stacks)))) ) 
 	// Double interrupt delivery.
#else
	if( EXPECT_FALSE(!async_safe(eip)) ) 
	    // We are already executing somewhere in the wedge.
#endif
	{
	    // Cancel the interrupt delivery.
	    INC_BURN_COUNTER(async_delivery_canceled);
	    intlogic.raise_irq( irq, true ); // Reraise the IRQ.
	    // Resume execution at the original esp + eip.
	    result_tid = L4_ExchangeRegisters( vcpu.main_ltid, (3 << 3) | 2,
		    esp, eip, 0, 0, L4_nilthread, &l4_tstate.raw, &esp, &eip,
		    &eflags, &dummy, &dummy_tid );
	    ASSERT( !L4_IsNilThread(result_tid) );
	    ASSERT( eip == (L4_Word_t)async_irq_handle_exregs );
	    cpu.restore_interrupts(true); // Restore the interrupt status.
	    return;
	}
 
    // Side effects are now permitted to the CPU object.
    flags_t old_flags = cpu.flags;
    cpu.flags.prepare_for_interrupt();
    cpu.cs = gate.get_segment_selector();

    old_flags.x.fields.fi = 1; // Interrupts were enabled.
#if !defined(CONFIG_L4KA_REENTRANT)
    // Leave interrupts enabled only for traps.
    cpu.flags.x.fields.fi = gate.is_trap(); 
#endif
    // Store the old values
    stack[4] = (old_flags.x.raw & ~flags_user_mask) | (eflags & flags_user_mask);
    stack[2] = eip;
    stack[1] = esp;

    //L4_ThreadSwitch(vcpu.main_ltid);
}

#if defined(CONFIG_GUEST_DMA_HOOK)
word_t backend_phys_to_dma_hook( word_t phys )
{
    word_t dma;

    if( EXPECT_FALSE(phys > resourcemon_shared.phys_end) ) {
	con << "Fatal DMA error: excessive address " << (void *)phys
	    << " > " << (void *)resourcemon_shared.phys_end << '\n';
	panic();
    }

    if( phys < resourcemon_shared.phys_size )
	// Machine memory that resides in the VM.
	dma = phys + resourcemon_shared.phys_offset;
    else
    {   
        // Memory is arranged such that the VM's memory is swapped with
        // lower memory.  The memory above the VM is 1:1.
        if ((phys-resourcemon_shared.phys_size) < resourcemon_shared.phys_offset)
            // Machine memory which resides before the VM.
            dma = phys - resourcemon_shared.phys_size;
        else
            // Machine memory which resides after the VM.
            dma = phys;
    }

    if (debug_dma)
	con << "phys to dma, " << (void *)phys << " to " << (void *)dma << '\n';
    return dma;
}

word_t backend_dma_to_phys_hook( word_t dma )
{
    unsigned long vm_offset = resourcemon_shared.phys_offset;
    unsigned long vm_size = resourcemon_shared.phys_size;
    unsigned long paddr;

    if ((dma >= 0x9f000) && (dma < 0x100000))
        // An address within a shadow BIOS region?
        paddr = dma;
    else if (dma < vm_offset)
        // Machine memory which resides before the VM.
        paddr = dma + vm_size;
    else if (dma < (vm_offset + vm_size))
        // Machine memory in the VM.
        paddr = dma - vm_offset;
    else
        // Machine memory which resides above the VM.
        paddr = dma;

    if( EXPECT_FALSE(paddr > resourcemon_shared.phys_end) )
	con << "DMA range error\n";

    if (debug_dma)
	con<< "dma to phys, " << (void *)dma << " to " << (void *)paddr << '\n';
    return paddr;
}
#endif


bool backend_request_device_mem( word_t base, word_t size, word_t rwx, bool boot)
{
    CORBA_Environment ipc_env = idl4_default_environment;
    idl4_fpage_t idl4fp;
    L4_Fpage_t fp = L4_Fpage ( base, size);
    idl4_set_rcv_window( &ipc_env, fp );

    IHypervisor_request_device( L4_Pager(), fp.raw, rwx, &idl4fp, &ipc_env );
    
    if( ipc_env._major != CORBA_NO_EXCEPTION ) {
	word_t err = CORBA_exception_id(&ipc_env);
	CORBA_exception_free( &ipc_env );
	
	if (debug_device)
	    con << "backend_request_device_mem: error " << err 
		<< " base " << (void*) base << "\n";

	return false;
    }
    return true;
}    
    

bool backend_unmap_device_mem( word_t base, word_t size, word_t &rwx, bool boot)
{
    
    CORBA_Environment ipc_env = idl4_default_environment;
    L4_Fpage_t fp = L4_Fpage ( base, size);

    L4_Word_t old_rwx;
    
    IHypervisor_unmap_device(
	resourcemon_shared.cpu[get_vcpu().cpu_id].hypervisor_tid, 
	fp.raw,
	rwx, 
	&old_rwx,
	&ipc_env );

    if( ipc_env._major != CORBA_NO_EXCEPTION ) {
	word_t err = CORBA_exception_id(&ipc_env);
	CORBA_exception_free( &ipc_env );
	
	if (debug_device)
	    con << "backend_unmap_device_mem: error " << err 
		<< " base " << (void*) base << "\n";
	
	rwx = 0;
	return false;
    }
    
    rwx = old_rwx;
    return true;
}

