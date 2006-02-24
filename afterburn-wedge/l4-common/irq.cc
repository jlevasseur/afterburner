/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4-common/irq.cc
 * Description:   The irq thread for handling asynchronous events.
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
 ********************************************************************/

#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>

#include INC_ARCH(intlogic.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(l4privileged.h)
#include <device/acpi.h>

#include <l4-common/hthread.h>
#include <l4-common/irq.h>
#include <l4-common/message.h>

static const bool debug_hw_irq=0;
static const bool debug_virq=0;
static const bool debug_ipi=0;

static unsigned char irq_stack[KB(16)] ALIGNED(CONFIG_STACK_ALIGN);
static const L4_Clock_t timer_length = {raw: 10000};
static const L4_Word_t timer_cpu = 0;

static void irq_handler_thread( void *param, hthread_t *hthread )
{
    L4_Word_t tid_user_base = L4_ThreadIdUserBase(L4_GetKernelInterface());

    con << "Entering IRQ loop, TID " << hthread->get_global_tid() << '\n';

    // Set our thread's vcpu object.
    set_vcpu( (vcpu_t *)param );
    vcpu_t &vcpu = get_vcpu();
    // Set our thread's exception handler.
    L4_Set_ExceptionHandler( get_vcpu().monitor_gtid );

    L4_ThreadId_t tid = L4_nilthread;
    L4_ThreadId_t ack_tid = L4_nilthread;
    L4_Clock_t last_time = {raw: 0}, current_time = {raw: 0};
    L4_Word_t cpu = L4_ProcessorNo();
    L4_Word_t timer_irq = INTLOGIC_TIMER_IRQ;
    L4_Time_t periodic;
    bool do_timer = false, dispatch_ipc = false, was_dispatch_ipc = false;
    word_t reraise_irq = 0;
    intlogic_t &intlogic = get_intlogic();
    word_t dispatch_ipc_nr = 0;
    
    if( cpu == timer_cpu )
	periodic = L4_TimePeriod( timer_length.raw );
    else
	periodic = L4_Never;

    for (;;)
    {
#if 0
	current_time = L4_SystemClock();
	L4_Word_t time_delta = current_time.raw - last_time.raw;
	if( time_delta > timer_length.raw )
	    periodic = L4_TimePeriod(0);
	else
	    periodic = L4_TimePeriod( timer_length.raw - time_delta );
#endif

	L4_MsgTag_t tag = L4_ReplyWait_Timeout( ack_tid, periodic, &tid );
	ack_tid = L4_nilthread;
	do_timer = false;
	was_dispatch_ipc = dispatch_ipc;
	dispatch_ipc = false;

	if( L4_IpcFailed(tag) )
	{
	    L4_Word_t err = L4_ErrorCode();

	    if( (err & 0xf) == 3 ) { // Receive timeout.
		// Timer interrupt.
		do_timer = true;
	    }
	    else if( (err & 0xf) == 2 ) { // Send timeout.
		if( was_dispatch_ipc )
		{
		    // User-level programs already in the send queue of the main
		    // thread can beat us to IPC delivery, causing
		    // us to time-out when sending a vector request to the
		    // dispatch loop.
		    intlogic.raise_irq(reraise_irq, true);
		}
		else
		    con << "IPC send timeout for IRQ delivery to main thread.\n";
		continue;
	    }
	    else {
		L4_KDB_Enter("IRQ IPC failure");
		continue;
	    }
	} /* IPC timeout */

	// Received message.
	else if( tid.global.X.thread_no < tid_user_base ) {
	    // Hardware IRQ.
	    L4_Word_t irq = tid.global.X.thread_no;
	    if(debug_hw_irq || intlogic.is_irq_traced(irq))
		con << "hardware irq: " << irq
		    << ", int flag: " << get_cpu().interrupts_enabled()
		    << '\n';
#if defined(CONFIG_DEVICE_PASSTHRU)
	    intlogic.set_hwirq_mask(irq);
	    intlogic.raise_irq( irq );
#else
	    L4_KDB_Enter("hardware irq");
#endif
	}
	else {
	    // Virtual interrupt from external source.
	    if( msg_is_virq(tag) ) {
		L4_Word_t irq;
		msg_virq_extract( &irq );
		if( debug_virq )
		    con << "virtual irq: " << irq 
			<< ", from TID " << tid << '\n';
		intlogic.raise_irq( irq );
	    }
	    else if( msg_is_device_ack(tag) ) {
		L4_Word_t irq;
		msg_device_ack_extract( &irq );
		if(debug_hw_irq || intlogic.is_irq_traced(irq))
		    con << "hardware irq ack " << irq << '\n';
#if defined(CONFIG_DEVICE_PASSTHRU)
		// Send an ack message to the L4 interrupt thread.
		// TODO: the main thread should be able to do this via
		// propagated IPC.
		ack_tid.global.X.thread_no = irq;
		ack_tid.global.X.version = 1;
		L4_LoadMR( 0, 0 );  // Ack msg.
		continue;  // Don't attempt other interrupt processing.
#else
		L4_KDB_Enter("irq ack");
#endif
	    }
	    else if( msg_is_ipi(tag) ) {
		L4_Word_t vector;
		msg_ipi_extract( &vector );
		if(debug_ipi) con << "IPI from TID " << tid << '\n';
	    }
	    else if( msg_is_device_enable(tag) ) {
#if defined(CONFIG_DEVICE_PASSTHRU)
		L4_Word_t irq;
		L4_Error_t errcode;
		L4_Word_t prio = resourcemon_shared.prio + CONFIG_PRIO_DELTA_IRQ;
		msg_device_enable_extract(&irq);
		tid.global.X.thread_no = irq;
		tid.global.X.version = 1;
		errcode = AssociateInterrupt( tid, L4_Myself() );
		if( errcode != L4_ErrOk )
		    con << "Attempt to claim an unavailable interrupt: "
			<< irq << ", L4 error: " 
			<< L4_ErrString(errcode) << ".\n";
		if( !L4_Set_Priority(tid, prio) )
		    con << "Unable to set the priority of interrupt: "
			<< irq << '\n';
#else
		L4_KDB_Enter("device irq enable");
#endif
	    }
	    else
		con << "unexpected IRQ message from " << tid << '\n';
	}

	// Make sure that we deliver our timer interrupts too!
	if( cpu == timer_cpu ) {
	    current_time = L4_SystemClock();
	    if( !do_timer && ((current_time - timer_length) > last_time) )
		do_timer = true;

	    if( do_timer ) {
		last_time = current_time;
		if(intlogic.is_irq_traced(timer_irq))
		    con << "timer irq " << timer_irq 
			<< ", int flag: " << get_cpu().interrupts_enabled()
			<< "\n";
		intlogic.raise_irq( timer_irq );
	    }
	}
	
	if( vcpu.in_dispatch_ipc() )
	{
	    word_t int_save = vcpu.cpu.disable_interrupts();
	    word_t new_dispatch_nr = vcpu.get_dispatch_ipc_nr();
	    
	    if (!int_save) 
	    {
		vcpu.cpu.restore_interrupts(int_save, false);
		continue;
	    }
	    
	    
#if 0
	    if (dispatch_ipc_nr == new_dispatch_nr) 
	    {
		// Verify that guy is running
		L4_Word_t dummy, control = 0;
		L4_ThreadId_t dummy_tid;
		dummy_tid = L4_ExchangeRegisters( vcpu.main_ltid, 0,
			0, 0, 0, 0, L4_nilthread, &control, &dummy, &dummy,
			&dummy, &dummy, &dummy_tid );
		
		if (control)
		    con << "ctrl " << control << "\n";
		ASSERT(control == 0);
		
		vcpu.cpu.restore_interrupts(int_save, false);
		continue;
	    }
	    else
	    {
		// Verify that guy is waiting for us
		L4_Word_t dummy, control = 0;
		L4_ThreadId_t dummy_tid;
		dummy_tid = L4_ExchangeRegisters( vcpu.main_ltid, 0,
			0, 0, 0, 0, L4_nilthread, &control, &dummy, &dummy,
			&dummy, &dummy, &dummy_tid );
		
		if (control != 2)
		    con << "ctrl " << control << "\n";
		ASSERT(control == 2);
		
	    }		
#endif		
	    word_t vector, irq;
	    
	    if( !intlogic.pending_vector(vector, irq) )
	    {
		vcpu.cpu.restore_interrupts(int_save, false);
		continue;
	    }
	    
	    dispatch_ipc = true;
	    reraise_irq = irq;
	    // Interrupts are enabled if we are in dispatch IPC.
	    dispatch_ipc_nr = new_dispatch_nr;
	    msg_vector_build( vector );
	    ack_tid = vcpu.main_ltid;
	    if(intlogic.is_irq_traced(irq))
		con << "Forward IRQ via IPC to idle VM: " << irq << '\n';

	}
	else
	    backend_async_irq_deliver( intlogic );

    } /* while */
}

#if defined(CONFIG_DEVICE_APIC)
/**
 * this is a major guess fest; L4 doesn't export the APIC IRQ sources
 * so we have to guess what they could have been 
 */

static void init_io_apics()
{
    int sources = L4_ThreadIdSystemBase(L4_GetKernelInterface());
    int apic;
    int nr_ioapics = acpi.get_nr_ioapics();
    intlogic_t &intlogic = get_intlogic();

    if (!nr_ioapics) {
	con << "IOAPIC Initialization of APICs not possible, ignore...\n";
	L4_KDB_Enter("IOAPIC initialization failed");
	return;
    }
    
    if (nr_ioapics >= CONFIG_MAX_IOAPICS)
    {
	con << "IOAPIC more real IOAPICs than virtual APICs\n";
	panic();
    }
    
    con << "IOAPIC found " << sources << " interrupt sources on " 
	<< nr_ioapics << " apics\n";  

   
    for (apic=0; apic < nr_ioapics; apic++)
    {
	intlogic.ioapic[apic].set_id(acpi.get_ioapic_id(apic));
	intlogic.ioapic[apic].set_base(acpi.get_ioapic_irq_base(apic));
    }
    
    /* 
     * This is an incredible hack: we _guess_ the real IO-APIC's 
     * redirection entries
     */
    
    // first the very simple version--identical APIC wire numbers
    if ((sources / nr_ioapics) == 16 || 
	(sources / nr_ioapics) == 24 || 
	(sources / nr_ioapics) == 44) 
    {
	for (apic = 0; apic < nr_ioapics; apic++)
	    intlogic.ioapic[apic].set_max_redir_entry(sources / nr_ioapics - 1);
    } 
    else {
	switch (nr_ioapics) {
	case 3: // as seen on Opterons
	    if ((sources - 16) == 8) {
		intlogic.ioapic[0].set_max_redir_entry(15);
		intlogic.ioapic[1].set_max_redir_entry(3);
		intlogic.ioapic[2].set_max_redir_entry(3);
		break;
	    }
	    if ((sources - 24) == 8) {
		intlogic.ioapic[0].set_max_redir_entry(23);
		intlogic.ioapic[1].set_max_redir_entry(3);
		intlogic.ioapic[2].set_max_redir_entry(3);
		break;
	    }
	default:
	    con << "IOAPIC unknown HW IOAPIC configuration (" << nr_ioapics
		<< "IO-APICs, " << sources << " IRQ sources)\n";
	}
    }
   
    
    // mark all remaining IO-APICs as invalid
    for (apic = nr_ioapics; apic < CONFIG_MAX_IOAPICS; apic++)
	intlogic.ioapic[apic].set_id(0xf);

    // pin/hwirq to IO-APIC association
    for (word_t i = 0; i < CONFIG_MAX_IOAPICS; i++)
    {
	con << "IOAPIC id " << intlogic.ioapic[i].get_id() << "\n";  
	if (!intlogic.ioapic[i].is_valid_ioapic())
	    continue;

	word_t hwirq_min = intlogic.ioapic[i].get_base();
	word_t hwirq_max = hwirq_min + intlogic.ioapic[i].get_max_redir_entry() + 1;
	hwirq_max = hwirq_max >= INTLOGIC_MAX_HWIRQS ? INTLOGIC_MAX_HWIRQS: hwirq_max;
	    
	for (word_t hwirq = hwirq_min; hwirq < hwirq_max; hwirq++)
	{
	    con << "IOAPIC registering hwirq " << hwirq 
		<< " with apic " << intlogic.ioapic[i].get_id() << "\n";
	    intlogic.hwirq_register_ioapic(hwirq, &intlogic.ioapic[i]);
	}
    }
	
}
    

#endif

L4_ThreadId_t irq_init( L4_Word_t prio, 
	L4_ThreadId_t scheduler_tid, L4_ThreadId_t pager_tid,
	vcpu_t *vcpu )
{
    hthread_t *irq_thread =
	get_hthread_manager()->create_thread( 
		(L4_Word_t)irq_stack, sizeof(irq_stack),
		prio, irq_handler_thread,
		scheduler_tid, pager_tid, vcpu );

    if( !irq_thread )
	return L4_nilthread;

    irq_thread->start();
#if defined(CONFIG_DEVICE_APIC)
    init_io_apics();
#endif

    return irq_thread->get_local_tid();
}

