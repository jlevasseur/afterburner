/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4-common/vm.cc
 * Description:   The L4 state machine for implementing the idle loop,
 *                and the concept of "returning to user".  When returning
 *                to user, enters a dispatch loop, for handling L4 messages.
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
 * $Id: vm.cc,v 1.33 2005/12/16 16:00:31 joshua Exp $
 *
 ********************************************************************/

#include <l4/schedule.h>
#include <l4/ipc.h>

#include <bind.h>
#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(l4privileged.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(debug.h)
#include <l4-common/hthread.h>
#include <l4-common/setup.h>
#include <l4-common/message.h>
#include <l4-common/user.h>


static const bool debug_idle=0;
static const bool debug_user=0;
static const bool debug_user_pfault=0;
static const bool debug_user_startup=0;
static const bool debug_thread_exit=0;
static const bool debug_signal=0;

typedef void (*vm_entry_t)();

static word_t user_vaddr_end = 0x80000000;


static void vm_main_thread( void *param, hthread_t *hthread )
{
    con << "Entering main VM thread, TID " << hthread->get_global_tid() << '\n';
    backend_vm_init_t *init_info = 
    	(backend_vm_init_t *)hthread->get_tlocal_data();

    // Set our thread's local CPU.  This wipes out the hthread tlocal data.
    set_vcpu( (vcpu_t *)param );

    // Set our thread's exception handler.
    L4_Set_ExceptionHandler( get_vcpu().monitor_gtid );

#if defined(CONFIG_WEDGE_STATIC)
    // Minor runtime binding to the guest OS.
    afterburn_exit_hook = backend_exit_hook;
    afterburn_set_pte_hook = backend_set_pte_hook;
#else
    // Load the kernel into memory and rewrite its instructions.
    if( !backend_load_vm(init_info) )
	panic();
#endif

    // Prepare the emulated CPU and environment.  NOTE: this function 
    // may start the VM and never return!!
    if( !backend_preboot(init_info) )
	panic();

    // Start executing the VM's binary if backend_preboot() didn't do so.
    vm_entry_t entry = (vm_entry_t)init_info->entry_ip;
    entry();
}

L4_ThreadId_t vm_init( L4_Word_t prio,
	L4_ThreadId_t scheduler_tid, L4_ThreadId_t pager_tid,
	vcpu_t *vcpu, L4_Word_t vm_entry_ip, L4_Word_t vm_entry_sp )
{
    backend_vm_init_t init_info;

    if( !vm_entry_sp )
	vm_entry_sp = vcpu->get_vm_stack();
    init_info.entry_sp = vm_entry_sp;
    init_info.entry_ip = vm_entry_ip;

    hthread_t *main_thread = get_hthread_manager()->create_thread(
	    vcpu->get_vm_stack_bottom(), vcpu->get_vm_stack_size(),
	    prio, vm_main_thread, 
	    scheduler_tid, pager_tid,
	    vcpu, &init_info, sizeof(init_info) );

    if( !main_thread )
	return L4_nilthread;

    main_thread->start();

    return main_thread->get_local_tid();
}

void backend_interruptible_idle( burn_redirect_frame_t *redirect_frame )
{
    vcpu_t &vcpu = get_vcpu();
    bool saved_int_state = vcpu.cpu.interrupts_enabled();

    ASSERT( saved_int_state );
    if( redirect_frame->do_redirect() ) {
	return;	// We delivered an interrupt, so cancel the idle.
    }

    L4_ThreadId_t tid = L4_nilthread;
    L4_MsgTag_t tag;
    L4_Word_t err = 0;

    if( debug_idle )
	con << "Entering idle, interrupts enabled? " << saved_int_state << '\n';

    // Disable preemption to avoid race conditions with virtual, 
    // asynchronous interrupts.  TODO: make this work with interrupts of
    // physical devices too (i.e., lower their priorities).
    L4_DisablePreemption();
    vcpu.dispatch_ipc_enter();
    tag = L4_Wait( &tid );
    if( L4_IpcFailed(tag) )
	err = L4_ErrorCode();
    // Protect our IPC message registers by disabling interrupts.  If we just
    // received a vector forward from the IRQ loop, then interrupts
    // will already be disabled.
    vcpu.cpu.disable_interrupts();
    vcpu.dispatch_ipc_exit();
    L4_EnablePreemption();
    if( L4_PreemptionPending() )
	L4_Yield();

#warning Pistachio doesn't return local ID's!!
    if( L4_IpcSucceeded(tag) ) {
	if( msg_is_vector(tag) /* && L4_IsLocalId(tid) */ ) {
	    L4_Word_t vector;
	    msg_vector_extract( &vector );
	    vcpu.cpu.restore_interrupts( saved_int_state );

	    ASSERT( !redirect_frame->is_redirect() );
	    redirect_frame->do_redirect( vector );
	}
	else if( msg_is_virq(tag) ) {
	    L4_Word_t irq;
	    msg_virq_extract( &irq );
	    vcpu.cpu.restore_interrupts( saved_int_state );

	    get_intlogic().raise_irq(irq);
	    ASSERT( !redirect_frame->is_redirect() );
	    redirect_frame->do_redirect();
	}
	else {
	    vcpu.cpu.restore_interrupts( saved_int_state );
	    con << "Unexpected IPC in idle loop, from TID " << tid
		<< ", tag " << (void *)tag.raw << '\n';
	}
    }
    else {
	vcpu.cpu.restore_interrupts( saved_int_state );
	con << "IPC failure in idle loop.  L4 error code " << err << '\n';
    }
}    

static thread_info_t *allocate_thread()
{
    L4_Error_t errcode;
    vcpu_t &vcpu = get_vcpu();
    L4_ThreadId_t controller_tid = vcpu.main_gtid;

    // Allocate a thread ID.
    L4_ThreadId_t tid = get_hthread_manager()->thread_id_allocate();
    if( L4_IsNilThread(tid) )
	PANIC( "Out of thread ID's." );

    // Lookup the address space's management structure.
    L4_Word_t page_dir = vcpu.cpu.cr3.get_pdir_addr();
    task_info_t *task_info = 
	task_manager_t::get_task_manager().find_by_page_dir( page_dir );
    if( !task_info )
    {
	// New address space.
	task_info = task_manager_t::get_task_manager().allocate( page_dir );
	if( !task_info )
	    PANIC( "Hit task limit." );
	task_info->init();
    }

    // Choose a UTCB in the address space.
    L4_Word_t utcb, utcb_index;
    if( !task_info->utcb_allocate(utcb, utcb_index) )
	PANIC( "Hit task thread limit." );

    // Configure the TID.
    tid = L4_GlobalId( L4_ThreadNo(tid), task_info_t::encode_gtid_version(utcb_index) );
    if( task_info_t::decode_gtid_version(tid) != utcb_index )
	PANIC( "L4 thread version wrap-around." );
    if( !task_info->has_space_tid() ) {
	ASSERT( task_info_t::decode_gtid_version(tid) == 0 );
	task_info->set_space_tid( tid );
	ASSERT( task_info->get_space_tid() == tid );
    }

    // Allocate a thread info structure.
    thread_info_t *thread_info = 
	thread_manager_t::get_thread_manager().allocate( tid );
    if( !thread_info )
	PANIC( "Hit thread limit." );
    thread_info->init();
    thread_info->ti = task_info;

    // Init the L4 address space if necessary.
    if( task_info->get_space_tid() == tid )
    {
	// Create the L4 thread.
	errcode = ThreadControl( tid, task_info->get_space_tid(), 
		controller_tid, L4_nilthread, utcb );
	if( errcode != L4_ErrOk )
	    PANIC( "Failed to create initial user thread, TID " << tid 
		    << ", L4 error: " << L4_ErrString(errcode) );

	// Create an L4 address space + thread.
	// TODO: don't hardcode the size of a utcb to 512-bytes
	L4_Fpage_t utcb_fp = L4_Fpage( user_vaddr_end,
		512*CONFIG_L4_MAX_THREADS_PER_TASK );
	L4_Fpage_t kip_fp = L4_Fpage( L4_Address(utcb_fp) + L4_Size(utcb_fp),
		KB(16) );

	errcode = SpaceControl( tid, 0, kip_fp, utcb_fp, 
		L4_nilthread );
	if( errcode != L4_ErrOk )
	    PANIC( "Failed to create an address space, TID " << tid 
		    << ", L4 error: " << L4_ErrString(errcode) );
    }

    // Create the L4 thread.
    errcode = ThreadControl( tid, task_info->get_space_tid(), 
	    controller_tid, controller_tid, utcb );
    if( errcode != L4_ErrOk )
	PANIC( "Failed to create user thread, TID " << tid 
		<< ", space TID " << task_info->get_space_tid()
		<< ", utcb " << (void *)utcb 
		<< ", L4 error: " << L4_ErrString(errcode) );

    // Set the thread priority.
    L4_Word_t prio = vcpu.get_vm_max_prio() + CONFIG_PRIO_DELTA_USER;
    if( !L4_Set_Priority(tid, prio) )
	PANIC( "Failed to set user thread's priority to " << prio );

    // Assign the thread info to the guest OS's thread.
    afterburn_thread_assign_handle( thread_info );

    return thread_info;
}

static void delete_thread( thread_info_t *thread_info )
{
    if( !thread_info )
	return;
    ASSERT( thread_info->ti );

    L4_ThreadId_t tid = thread_info->get_tid();

    if( thread_info->is_space_thread() ) {
	// Keep the space thread alive, until the address space is empty.
	// Just flip a flag to say that the space thread is invalid.
	if( debug_thread_exit )
	    con << "Space thread invalidate, TID " << tid << '\n';
	thread_info->ti->invalidate_space_tid();
    }
    else
    {
	// Delete the L4 thread.
	thread_info->ti->utcb_release( task_info_t::decode_gtid_version(tid) );
	if( debug_thread_exit )
	    con << "Thread delete, TID " << tid << '\n';
	ThreadControl( tid, L4_nilthread, L4_nilthread, L4_nilthread, ~0UL );
	get_hthread_manager()->thread_id_release( tid );
    }

    if( thread_info->ti->has_one_thread() 
	    && !thread_info->ti->is_space_tid_valid() )
    {
	// Retire the L4 address space.
	tid = thread_info->ti->get_space_tid();
	if( debug_thread_exit )
	    con << "Space delete, TID " << tid << '\n';
	ThreadControl( tid, L4_nilthread, L4_nilthread, L4_nilthread, ~0UL );
	get_hthread_manager()->thread_id_release( tid );

	// Release the task info structure.
	task_manager_t::get_task_manager().deallocate( thread_info->ti );
    }

    // Release the thread info structure.
    thread_manager_t::get_thread_manager().deallocate( thread_info );
}


static void delay_message( L4_MsgTag_t tag, L4_ThreadId_t from_tid )
{
    // Message isn't from the "current" thread.  Delay message 
    // delivery until Linux chooses to schedule the from thread.
    thread_info_t *thread_info = 
	thread_manager_t::get_thread_manager().find_by_tid( from_tid );
    if( !thread_info ) {
	con << "Unexpected message from TID " << from_tid << '\n';
	L4_KDB_Enter("unexpected msg");
	return;
    }

    L4_StoreMRs( 0, 1 + L4_UntypedWords(tag) + L4_TypedWords(tag),
	    thread_info->mr_save.raw );
    thread_info->state = thread_info_t::state_pending;
}

static bool handle_user_pagefault( vcpu_t &vcpu, thread_info_t *thread_info, 
	L4_ThreadId_t tid )
    // When entering and exiting, interrupts must be disabled
    // to protect the message registers from preemption.
{
    word_t map_addr, map_bits, map_rwx;

    ASSERT( !vcpu.cpu.interrupts_enabled() );

    // Extract the fault info.
    L4_Word_t fault_rwx = L4_Label(thread_info->mr_save.pfault_msg.tag) & 0x7;
    L4_Word_t fault_addr = thread_info->mr_save.pfault_msg.addr;
    L4_Word_t fault_ip = thread_info->mr_save.pfault_msg.ip;

    if( debug_user_pfault )
	con << "User fault from TID " << tid
	    << ", addr " << (void *)fault_addr
	    << ", ip " << (void *)fault_ip
	    << ", rwx " << fault_rwx << '\n';

    // Lookup the translation, and handle the fault if necessary.
    bool complete = backend_handle_user_pagefault( 
	    thread_info->ti->get_page_dir(), 
	    fault_addr, fault_ip, fault_rwx,
	    map_addr, map_bits, map_rwx );
    if( !complete )
	return false;

    ASSERT( !vcpu.cpu.interrupts_enabled() );

    // Build the reply message to user.
    if( debug_user_pfault )
	con << "Page fault reply to TID " << tid
	    << ", kernel addr " << (void *)map_addr
	    << ", size " << (1 << map_bits)
	    << ", rwx " << map_rwx
	    << ", user addr " << (void *)fault_addr << '\n';
    L4_MapItem_t map_item = L4_MapItem(
	    L4_FpageAddRights(L4_FpageLog2(map_addr, map_bits),
		map_rwx), 
	    fault_addr );
    L4_Msg_t msg;
    L4_MsgClear( &msg );
    L4_MsgAppendMapItem( &msg, map_item );
    L4_MsgLoad( &msg );
    return true;
}


static void handle_forced_user_pagefault( vcpu_t &vcpu, L4_MsgTag_t tag, 
	L4_ThreadId_t from_tid )
{
    L4_Word_t fault_addr, fault_ip, fault_rwx;
    extern word_t afterburner_user_start_addr;

    ASSERT( !vcpu.cpu.interrupts_enabled() );

    fault_rwx = L4_Label(tag) & 0x7;
    L4_StoreMR( 1, &fault_addr );
    L4_StoreMR( 2, &fault_ip );

    ASSERT( fault_addr >= user_vaddr_end );
    ASSERT( fault_rwx & 0x5 );

    L4_MapItem_t map_item = L4_MapItem(
	    L4_FpageAddRights(
		L4_FpageLog2(afterburner_user_start_addr, PAGE_BITS), 0x5 ),
	    fault_addr );
    L4_Msg_t msg;
    L4_MsgClear( &msg );
    L4_MsgAppendMapItem( &msg, map_item );
    L4_MsgLoad( &msg );
}



NORETURN void backend_activate_user( user_frame_t *user_frame )
{
    vcpu_t &vcpu = get_vcpu();
    bool complete;

    if( debug_user )
	con << "Request to enter user"
	    << ", to ip " << (void *)user_frame->iret->ip
	    << ", to sp " << (void *)user_frame->iret->sp << '\n';

    // Protect our message registers from preemption.
    vcpu.cpu.disable_interrupts();
    user_frame->iret->flags.x.fields.fi = 0;

    // Update emulated CPU state.
    vcpu.cpu.cs = user_frame->iret->cs;
    vcpu.cpu.flags = user_frame->iret->flags;
    vcpu.cpu.ss = user_frame->iret->ss;

    L4_ThreadId_t reply_tid = L4_nilthread;

    thread_info_t *thread_info = (thread_info_t *)afterburn_thread_get_handle();
    if( EXPECT_FALSE(!thread_info || 
	    (thread_info->ti->get_page_dir() != vcpu.cpu.cr3.get_pdir_addr())) )
    {
	if( thread_info ) {
	    // The thread switched to a new address space.  Delete the
	    // old thread.  In Unix, for example, this would be a vfork().
	    delete_thread( thread_info );
	}

	// We are starting a new thread, so the reply message is the
	// thread startup message.
	thread_info = allocate_thread();
	reply_tid = thread_info->get_tid();
	if( debug_user_startup )
	    con << "New thread start, TID " << thread_info->get_tid() << '\n';
	thread_info->state = thread_info_t::state_force;
	msg_startup_build( user_vaddr_end + 0x1000000, 0 );
	// Prepare the reply to the forced exception
	for( u32_t i = 0; i < 8; i++ )
	    thread_info->mr_save.raw[5+7-i] = user_frame->regs->x.raw[i];
	thread_info->mr_save.exc_msg.eflags = user_frame->iret->flags.x.raw;
	thread_info->mr_save.exc_msg.eip = user_frame->iret->ip;
	thread_info->mr_save.exc_msg.esp = user_frame->iret->sp;
    }
    else if( thread_info->state == thread_info_t::state_except_reply )
    {
	reply_tid = thread_info->get_tid();
	thread_info->state = thread_info_t::state_user;
#if 0
	if( thread_info->mr_save.exc_msg.eax == 3 /*&&
		thread_info->mr_save.exc_msg.ecx > 0x7f000000*/ )
	{
	    con << "< read " << thread_info->mr_save.exc_msg.ebx 
		<< " " << (void *)thread_info->mr_save.exc_msg.ecx 
		<< " " << thread_info->mr_save.exc_msg.edx 
		<< " result: " << user_frame->regs->x.fields.eax << '\n';
	}
	else if( thread_info->mr_save.exc_msg.eax == 5 )
	    con << "< open " << user_frame->regs->x.fields.eax 
		<< " " << (void *)thread_info->mr_save.exc_msg.ebx << '\n';
	else if( thread_info->mr_save.exc_msg.eax == 90 ) {
	    con << "< mmap " << (void *)user_frame->regs->x.fields.eax  << '\n';
	    L4_KDB_Enter("mmap return");
	}
	else if( thread_info->mr_save.exc_msg.eax == 192 )
	    con << "< mmap2 " << (void *)user_frame->regs->x.fields.eax  << '\n';
#endif
	// Prepare the reply to the exception
#if 0 && defined(CONFIG_DO_UTCB_EXCEPTION)
	word_t *msg_regs = (word_t*)__L4_X86_Utcb();
	msg_regs[0] = thread_info->mr_save.envelope.tag.raw;
	msg_regs[1] = user_frame->iret->ip;
	msg_regs[2] = user_frame->iret->flags.x.raw;
	for( u32_t i = 0; i < 8; i++ )
	    msg_regs[5+7-i] = user_frame->regs->x.raw[i];
	msg_regs[8] = user_frame->iret->sp;
#else
	for( u32_t i = 0; i < 8; i++ )
	    thread_info->mr_save.raw[5+7-i] = user_frame->regs->x.raw[i];
	thread_info->mr_save.exc_msg.eflags = user_frame->iret->flags.x.raw;
	thread_info->mr_save.exc_msg.eip = user_frame->iret->ip;
	thread_info->mr_save.exc_msg.esp = user_frame->iret->sp;
//	con << "eax " << (void *)thread_info->mr_save.exc_msg.eax << '\n';
	// Load the message registers.
	L4_LoadMRs( 0, 1 + L4_UntypedWords(thread_info->mr_save.envelope.tag), 
		thread_info->mr_save.raw );
#endif
    }
    else if( thread_info->state == thread_info_t::state_pending )
    {
	// Pre-existing message.  Figure out the target thread's pending state.
	// We discard the iret user-state because it is supposed to be bogus
	// (we haven't given the kernel good state, and via the signal hook,
	// asked the guest kernel to cancel signal delivery).
	reply_tid = thread_info->get_tid();
	switch( L4_Label(thread_info->mr_save.envelope.tag) >> 4 )
	{
	case msg_label_pfault:
	    // Reply to fault message.
	    thread_info->state = thread_info_t::state_pending;
	    complete = handle_user_pagefault( vcpu, thread_info, reply_tid );
	    ASSERT( complete );
	    break;

	case msg_label_except:
	    thread_info->state = thread_info_t::state_except_reply;
	    backend_handle_user_exception( thread_info );
	    panic();
	    break;

    	default:
	    reply_tid = L4_nilthread;	// No message to user.
	    break;
	}
	// Clear the pre-existing message to prevent replay.
	thread_info->mr_save.envelope.tag.raw = 0;
	thread_info->state = thread_info_t::state_user;
    }
    else
    {
	// No pending message to answer.  Thus the app is already at
	// L4 user, with no expectation of a message from us.
	reply_tid = L4_nilthread;
	if( user_frame->iret->ip != 0 )
	    con << "Attempted signal delivery during async interrupt.\n";
    }

    L4_ThreadId_t current_tid = thread_info->get_tid(), from_tid;

    for(;;)
    {
	// Send and wait for message.
	// Disable preemption to avoid race conditions with virtual, 
	// asynchronous interrupts.  TODO: make this work with interrupts of
	// physical devices too (i.e., lower their priorities).
	L4_DisablePreemption();
	vcpu.dispatch_ipc_enter();
	vcpu.cpu.restore_interrupts(true, false);
	L4_MsgTag_t tag = L4_ReplyWait( reply_tid, &from_tid );
	vcpu.cpu.disable_interrupts();
	vcpu.dispatch_ipc_exit();
	L4_EnablePreemption();
	if( L4_PreemptionPending() )
	    L4_Yield();

	reply_tid = L4_nilthread;

	if( L4_IpcFailed(tag) ) {
	    con << "Dispatch IPC error.\n";
	    continue;
	}

	switch( L4_Label(tag) >> 4 )
	{
	case msg_label_pfault:
	    if( EXPECT_FALSE(from_tid != current_tid) ) {
		if( debug_user_pfault )
		    con << "Delayed user page fault from TID " << from_tid 
			<< '\n';
		delay_message( tag, from_tid );
	    }
	    else if( thread_info->state == thread_info_t::state_force ) {
		// We have a pending register set and want to preserve it.
		// The only page fault possible is on L4-specific code.
		if( debug_user_startup )
		    con << "Forced user page fault, TID " << from_tid << '\n';
		handle_forced_user_pagefault( vcpu, tag, from_tid );
		reply_tid = current_tid;
		// Maintain state_force
	    }
	    else {
		thread_info->state = thread_info_t::state_pending;
		thread_info->mr_save.envelope.tag = tag;
		ASSERT( !vcpu.cpu.interrupts_enabled() );
		L4_StoreMR( 1, &thread_info->mr_save.pfault_msg.addr );
		L4_StoreMR( 2, &thread_info->mr_save.pfault_msg.ip );
		complete = handle_user_pagefault( vcpu, thread_info, from_tid );
		if( complete ) {
		    // Immediate reply.
		    reply_tid = current_tid;
		    thread_info->state = thread_info_t::state_user;
		}
	    }
	    continue;

	case msg_label_except:
	    if( EXPECT_FALSE(from_tid != current_tid) )
		delay_message( tag, from_tid );
	    else if( EXPECT_FALSE(thread_info->state == thread_info_t::state_force) ) {
		// We forced this exception.  Respond with the pending 
		// register set.
		if( debug_user_startup )
		    con << "Official user start, TID " << current_tid << '\n';
		thread_info->mr_save.envelope.tag = tag;
		L4_LoadMRs( 0, 1 + L4_UntypedWords(tag), 
			thread_info->mr_save.raw );
		reply_tid = current_tid;
		thread_info->state = thread_info_t::state_user;
	    }
	    else {
		thread_info->state = thread_info_t::state_except_reply;
		thread_info->mr_save.envelope.tag = tag;
#if defined(CONFIG_DO_UTCB_EXCEPTION)
		L4_StoreMR( 1, &thread_info->mr_save.exc_msg.eip );
#else
		L4_StoreMRs( 1, L4_UntypedWords(tag), 
			&thread_info->mr_save.raw[1] );
#endif
		backend_handle_user_exception( thread_info );
		panic();
	    }
	    continue;
	}

	switch( L4_Label(tag) )
	{
	case msg_label_vector:
	    L4_Word_t vector;
	    msg_vector_extract( &vector );
	    backend_handle_user_vector( vector );
	    panic();
	    break;

	case msg_label_virq:
	    L4_Word_t irq;
	    msg_virq_extract( &irq );
	    get_intlogic().raise_synchronous_irq( irq );
	    break;

	default:
	    con << "Unexpected message from TID " << from_tid
		<< ", tag " << (void *)tag.raw << '\n';
	    L4_KDB_Enter("unknown message");
	    break;
	}

    }

    panic();
}

void backend_exit_hook( void *handle )
{
    cpu_t &cpu = get_cpu();
    bool saved_int_state = cpu.disable_interrupts();
    delete_thread( (thread_info_t *)handle );
    cpu.restore_interrupts( saved_int_state, true );
}

int backend_signal_hook( void *handle )
    // Return 1 to cancel signal delivery.
    // Return 0 to permit signal delivery.
{
    thread_info_t *thread_info = (thread_info_t *)handle;
    if( EXPECT_FALSE(!thread_info) )
	return 0;

    if( thread_info->state != thread_info_t::state_except_reply ) {
	if( debug_signal )
	    con << "Delayed signal delivery.\n";
	return 1;
    }

    return 0;
}

