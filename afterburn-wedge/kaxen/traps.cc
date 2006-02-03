/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/kaxen/traps.cc
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
 * $Id: traps.cc,v 1.23 2005/12/27 09:18:54 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(console.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(memory.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(xen_hypervisor.h)
#include INC_WEDGE(wedge.h)

#include INC_ARCH(instr.h)
#include INC_ARCH(ops.h)

#include <burn_counters.h>
#include <memory.h>

static const bool debug_pfault=0;
static const bool debug_trap=0;
static const bool debug_soft_trap=0;
static const bool debug_vmi_trap=0;
static const bool debug_vmi_emul=0;

DECLARE_BURN_COUNTER(page_faults);
DECLARE_BURN_COUNTER(traps);
DECLARE_BURN_COUNTER(trap_pop_segment);
DECLARE_BURN_COUNTER(soft_traps);

extern "C" void trap_wrapper_0();
extern "C" void trap_wrapper_1();
extern "C" void trap_wrapper_2();
extern "C" void trap_wrapper_3();
extern "C" void trap_wrapper_4();
extern "C" void trap_wrapper_5();
extern "C" void trap_wrapper_6();
extern "C" void trap_wrapper_7();
extern "C" void trap_wrapper_8();
extern "C" void trap_wrapper_9();
extern "C" void trap_wrapper_10();
extern "C" void trap_wrapper_11();
extern "C" void trap_wrapper_12();
extern "C" void trap_wrapper_13();
extern "C" void page_fault_wrapper();
extern "C" void trap_wrapper_16();
extern "C" void trap_wrapper_17();
extern "C" void trap_wrapper_18();
extern "C" void trap_wrapper_19();

extern "C" void int_wrapper_0x20();
extern "C" void int_wrapper_0x21();
extern "C" void wedge_trap_wrapper_0x69();
extern "C" void int_wrapper_0x80();

extern "C" void trap_wrapper_vmi_boot();
extern "C" void trap_wrapper_kdb();

word_t pgfault_gate_ip = 0;

static const word_t trap_event_disable = 4;
static const word_t xen_fast_trap = 0x80;

trap_info_t xen_trap_table[] = {
    {    0,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_0 }, 
    {    1,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_1 }, 
    {    2,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_2 },
#if !defined(CONFIG_DEBUGGER)
    {    3,   3, XEN_CS_KERNEL, (word_t)trap_wrapper_3 },
#else
    {    3,   3, XEN_CS_KERNEL, (word_t)trap_wrapper_kdb }, 
#endif
    {    4,   3, XEN_CS_KERNEL, (word_t)trap_wrapper_4 }, 
    {    5,   3, XEN_CS_KERNEL, (word_t)trap_wrapper_5 }, 
    {    6,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_6 }, 
    {    7,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_7 }, 
    {    8,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_8 }, 
    {    9,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_9 }, 
    {   10,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_10 }, 
    {   11,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_11 }, 
    {   12,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_12 },
#ifdef CONFIG_VMI_SUPPORT
    {   13,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_vmi_boot },
#else
    {   13,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_13 },
#endif
    {   14,   1, XEN_CS_KERNEL, (word_t)page_fault_wrapper }, 
    {   16,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_16 }, 
    {   17,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_17 }, 
    {   18,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_18 }, 
    {   19,   1, XEN_CS_KERNEL, (word_t)trap_wrapper_19 }, 
    { 0x20,   3, XEN_CS_KERNEL, (word_t)int_wrapper_0x20 },
    { 0x21,   3, XEN_CS_KERNEL, (word_t)int_wrapper_0x21 },
    { 0x69,   3, XEN_CS_KERNEL, (word_t)wedge_trap_wrapper_0x69 },
    { 0x80,   3, XEN_CS_KERNEL, (word_t)int_wrapper_0x80 },
    {    0,   0, 0, 0 }
};
static const word_t xen_trap_table_cnt = 
	sizeof(xen_trap_table)/sizeof(xen_trap_table[0]);


void init_xen_traps()
    // Note: this function may be called multiple times.  For example,
    // the debugger calls this function.
{
    ASSERT( sizeof(xen_frame_id_t) == sizeof(word_t) );

    if( XEN_set_trap_table(xen_trap_table) )
	PANIC( "Unable to configure the Xen trap table." );
#ifdef CONFIG_XEN_2_0
    if( XEN_set_fast_trap(xen_fast_trap) )
	con << "Unable to install the Xen fast trap.\n";
#endif
}



#ifdef CONFIG_VMI_SUPPORT

extern "C" void afterburn_cpu_write_gdt32( dtr_t *dtr );
extern "C" void afterburn_cpu_write_idt32( dtr_t *dtr );
extern "C" void afterburn_cpu_write_cr0( u32_t data, word_t *ret_address );
extern "C" void afterburn_cpu_write_cr2( u32_t data );
extern "C" void afterburn_cpu_write_cr3( u32_t data );
extern "C" void afterburn_cpu_write_cr4( u32_t data );
extern "C" void afterburn_cpu_lldt( u16_t segment );

extern inline u32_t get_reg_val (u8_t reg, xen_frame_t *frame)
{
    switch (reg) {
    case 0: return frame->eax;
    case 1: return frame->ecx;
    case 2: return frame->edx;
    case 3: return frame->ebx;
	//case 4: return frame->iret.esp;
    case 5: return frame->ebp;
    case 6: return frame->esi;
    case 7: return frame->edi;
    default: return 0;
    }
}

extern inline void set_reg_val (u8_t reg, u32_t val, xen_frame_t *frame)
{
    switch (reg) {
    case 0: frame->eax = val; break;
    case 1: frame->ecx = val; break;
    case 2: frame->edx = val; break;
    case 3: frame->ebx = val; break;
    case 5: frame->ebp = val; break;
    case 6: frame->esi = val; break;
    case 7: frame->edi = val; break;
    default: PANIC("Unsupported register");
    }
}

extern "C" void __attribute__(( regparm(1) ))
vmi_trap( xen_frame_t *frame )
{
    if( frame->get_privilege() == 3 ) {
	// The wedge doesn't process GPFs from user-level.  Pass them
	// to the guest kernel.
	xen_deliver_async_vector( 13, frame, true );
	return;
    }

    if( debug_vmi_trap )
	con << "VMI trap " << frame->get_id() << " at ip " 
	    << (void *)frame->iret.ip << '\n';

    // VMI patches only cover a subset of the instructions and require
    // partial interpretation.  Do that here...
    u8_t * opstream = (u8_t*)frame->iret.ip;
    ia32_modrm_t modrm;

    switch (opstream[0]) 
    {
    case OP_2BYTE: 
	modrm.x.raw = opstream[2];
	switch (opstream[1]) {
	case OP_LLTL:
	    if (debug_vmi_emul) con << "LLTL " << modrm.get_opcode() << "\n";
	    switch (modrm.get_opcode()) {
	    case 2:
		afterburn_cpu_lldt( get_reg_val(modrm.get_reg(), frame) ); break;
	    default:
		PANIC ("unsupported LLTL");
	    }
	    frame->iret.ip += 3;
	    return;
		
	case OP_LDTL: // LGDT, LIDT
	    
	    if (debug_vmi_emul) con << "LGDT " << modrm.get_opcode() << "\n";
	    switch (modrm.get_opcode()) {
	    case 2: // lgdt
		afterburn_cpu_write_gdt32(*(dtr_t**)&opstream[3]); break;
	    case 3: // lidt
		afterburn_cpu_write_idt32(*(dtr_t**)&opstream[3]); break;
	    default:
		PANIC ("unsupported LGDL");
	    }
	    frame->iret.ip += 7;
	    return;

	case OP_MOV_TOCREG: {
	    ASSERT( modrm.is_register_mode() );
	    if (debug_vmi_emul) 
		con << "mov CR"<< modrm.get_reg() << ", r" << modrm.get_rm() << "\n";
	    word_t dummy;
	    u32_t val = get_reg_val( modrm.get_rm(), frame );
	    switch (modrm.get_reg()) {
	    case 0: afterburn_cpu_write_cr0 (val, &dummy); break;
	    case 2: afterburn_cpu_write_cr2 (val); break;
	    case 3: afterburn_cpu_write_cr3 (val); break;
	    case 4: afterburn_cpu_write_cr4 (val); break;
	    default: PANIC("Unsupported CR write");
	    }
	    frame->iret.ip += 3;
	    return;
	}

	case OP_MOV_FROMCREG: {
	    ASSERT( modrm.is_register_mode() );
	    if (debug_vmi_emul) 
		con << "mov r" << modrm.get_rm() << ", CR"<< modrm.get_reg() << "\n";
	    u32_t val = ( modrm.get_reg() == 0 ? get_cpu().cr0.x.raw :
			  modrm.get_reg() == 2 ? get_cpu().cr2       :
			  modrm.get_reg() == 3 ? get_cpu().cr3.x.raw :
			  modrm.get_reg() == 4 ? get_cpu().cr4.x.raw : 0 );
	    set_reg_val( modrm.get_rm(), val, frame );
	    frame->iret.ip += 3;
	    return;
	}

	case OP_CLTS:
	    get_cpu().cr0.x.fields.ts = 0;
	    frame->iret.ip += 2;
	    return;

	case OP_LSS: {
	    u32_t *addr = *((u32_t**)&opstream[3]);
	    modrm.x.raw = opstream[2];
	    if (debug_vmi_emul)	con << "LSS " << (void*)addr << "\n";

	    // ESP has to be handled specially, copy the stack exception frame...
	    frame->iret.ip += 7;
	    get_cpu().ss = addr[1];

	    if (modrm.get_reg() == 4) {
		u32_t new_stack = addr[0] - sizeof(xen_frame_t) - 4;
		memcpy((void*)new_stack, (void*)((u32_t)frame - 4), sizeof(xen_frame_t) + 4);
		if (debug_vmi_emul) 
		    con << "new stack=" << (void*)new_stack;
		asm volatile ("movl %0, %%esp; ret" : : "r"(new_stack));
	    } else 
		set_reg_val( modrm.get_reg(), addr[0], frame );
	}
	default: 
	    PANIC("unknown opcode");
	}

    case OP_MOV_TOSEG:
	modrm.x.raw = opstream[1];
	if ( modrm.is_register_mode() ) {
	    if (debug_vmi_emul) 
		con << (void *)frame->iret.ip << ": mov reg, seg (reg" 
		    << modrm.get_rm() << "=" << get_reg_val( modrm.get_rm(), frame) 
		    << ", seg=" << modrm.get_reg() << ")\n";
	    opstream[0] = OP_NOP1;
	    opstream[1] = OP_NOP1;
#if 0
	    switch ( modrm.get_reg() ) {
	    case 0: get_cpu().es = get_reg_val( modrm.get_rm(), frame ); break;
	    case 1: get_cpu().cs = get_reg_val( modrm.get_rm(), frame ); break;
	    case 2: get_cpu().ss = get_reg_val( modrm.get_rm(), frame ); break;
	    case 3: get_cpu().ds = get_reg_val( modrm.get_rm(), frame ); break;
	    case 4: get_cpu().fs = get_reg_val( modrm.get_rm(), frame ); break;
	    case 5: get_cpu().gs = get_reg_val( modrm.get_rm(), frame ); break;
	    }

#endif
	    frame->iret.ip += 2;
	    return;
	}
	else
	    PANIC( "Unsupported instruction" );

    case OP_JMP_FAR:
	frame->iret.ip = *((u32_t*)&opstream[1]);
	get_cpu().cs = *((u16_t*)&opstream[5]);
	return;

    case OP_CLI:
	get_cpu().disable_interrupts();
	frame->iret.ip += 1;
	return;

    default:
	con << "VMI trap: unknown opcode:" << frame->get_id() << " at ip " 
	    << (void *)frame->iret.ip << '\n';
	PANIC("unknown opcode");
    }

}
#endif


extern "C" void __attribute__(( regparm(1) )) SECTION(".text.pte")
page_fault_trap( xen_frame_t *frame )
{
    if( EXPECT_FALSE(frame->iret.ip >= CONFIG_WEDGE_VIRT) ) {
#if defined(CONFIG_DEBUGGER)
	if( dbg_pgfault_perf_resolve(frame) )
	    return;
#endif
	PANIC( "Unexpected page fault in the wedge, fault address "
		<< (void *)frame->info.fault_vaddr
		<< ",\nip " << (void *)frame->iret.ip, 
		frame );
    }

    INC_BURN_COUNTER(page_faults);

    if( debug_pfault ) {
	con << "page fault: " << (void *)frame->info.fault_vaddr
	    << ' ' << frame->info.error_code
	    << ' ' << (void *)frame->iret.ip << '\n';
    }

    if( xen_memory.resolve_page_fault(frame) ) {
	// An emulation page fault, doesn't need to be exposed to Linux.
	return;
    }

    get_cpu().cr2 = frame->info.fault_vaddr;
    xen_deliver_async_vector( 14, frame, true );
}

#if defined(CONFIG_KAXEN_UNLINK_HEURISTICS)
extern "C" void __attribute__(( regparm(1) )) SECTION(".text.pte")
page_fault_relink( xen_relink_frame_t *frame )
{
    xen_memory.relink_ptab( frame->fault_vaddr );
}
#endif

extern "C" void __attribute__(( regparm(1) ))
trap( xen_frame_t *frame )
{
    INC_BURN_COUNTER(traps);

    if( EXPECT_FALSE(frame->iret.ip >= CONFIG_WEDGE_VIRT) ) {
	con << "Unexpected fault in the wedge, ip " << (void *)frame->iret.ip
	    << '\n';
	DEBUGGER_ENTER(frame);
	panic();
    }

    u8_t *opstream = (u8_t *)frame->iret.ip;

    if( cpu_t::get_segment_privilege(frame->iret.cs) == 3 )
    {
	// A user-level fault.
	if( opstream[0] == OP_MOV_TOSEG )
	    PANIC( "Unsupported move to segment, at ip " 
		    << (void *)frame->iret.ip 
		    << ".  Be sure that you disable glibc's TLS." );
	xen_deliver_async_vector( frame->get_id(), frame, 
		frame->uses_error_code());
    }


#if defined(CONFIG_VMI_SUPPORT)
    word_t *stack_bottom = (word_t *)&frame->iret.sp;

    switch (opstream[0])
    {
    case OP_MOV_TOSEG:
    {
	ia32_modrm_t modrm;
	modrm.x.raw = ((u8_t*)frame->iret.ip)[1];

	/* VU: mov to segment selectors are not covered by VMI.  We
	 * simply patch away the operation when loading into a segment
	 * selector since Xen takes care of that and emulate the
	 * behavior for pop seg. */
	if( modrm.is_register_mode() ) {
	    if( debug_vmi_emul )
		con << (void *)frame->iret.ip << ": mov reg, seg (reg" 
		    << modrm.get_rm() << "=" << get_reg_val( modrm.get_rm(), frame) 
		    << ", seg=" << modrm.get_reg() << ")\n";
	    /* VU: patch mov instructions with nops */
	    if (debug_vmi_trap)con << "patching segment selector reloads...\n";
	    opstream[0] = OP_NOP1;
	    opstream[1] = OP_NOP1;
	    return;
	}
	else
	    PANIC( "Unsupported instruction." );
    }
    case OP_POP_DS:
    case OP_POP_ES:
    case OP_POP_SS:
	INC_BURN_COUNTER( trap_pop_segment );
	if (debug_vmi_emul) {
	    u32_t seg = *stack_bottom;
	    con << (void*)frame->iret.ip << ": segment pop val=" << (void*)seg 
		<< '\n';
	}
	/* VU: on trapping pop seg there is a broken segment selector
	 * on the stack.  We just fix it up and try again.
	 */
	/* TODO: generally pop %ds and pop %es are successive instructions.
	 * They can be rewritten with add $4, %esp./
	 */
	*stack_bottom = XEN_DS_KERNEL;
	return;
    case OP_2BYTE:
	switch( opstream[1] ) {
	    case OP_POP_FS:
	    case OP_POP_GS:
		INC_BURN_COUNTER( trap_pop_segment );
		*stack_bottom = XEN_DS_KERNEL;
		return;
	}
	break;
    }
#endif /* CONFIG_VMI_SUPPORT */

    if( debug_trap )
	con << "Trap " << frame->get_id() << " at ip " 
	    << (void *)frame->iret.ip << '\n';

    xen_deliver_async_vector( frame->get_id(), frame, frame->uses_error_code());
}

extern "C" void __attribute__(( regparm(1) ))
soft_trap( xen_frame_t *frame )
{
    INC_BURN_COUNTER(soft_traps);
    if( debug_soft_trap ) {
	if( frame->get_privilege() < 3 )
	    con << "Kernel system call, eax " << frame->eax << '\n';
	else
	    con << "System call, eax=" << frame->eax 
		<< " ebx=" << (void*)frame->ebx 
		<< " ecx=" << (void*)frame->ecx << '\n';
    }
    xen_deliver_async_vector( frame->get_id(), frame, false );
}

extern "C" void __attribute__(( regparm(1) ))
wedge_trap( xen_frame_t *frame )
{
#if defined(CONFIG_BURN_COUNTERS)
    burn_counters_dump();
#endif
}

