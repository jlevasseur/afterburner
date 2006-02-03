/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/ia32/cpu.cc
 * Description:   The front-end CPU emulation.  Invoked from the 
 *                afterburned guest OS.
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

#include INC_ARCH(cpu.h)
#include INC_ARCH(intlogic.h)
#include INC_ARCH(instr.h)

#include INC_WEDGE(debug.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)

#include <memory.h>
#include <burn_counters.h>
#include <device/sequencing.h>
#include <device/portio.h>

DECLARE_BURN_COUNTER(tlb_global_flush);
DECLARE_BURN_COUNTER(iret_vector_redirect);
DECLARE_BURN_COUNTER(iret_esp0_switch);
DECLARE_BURN_COUNTER(cpu_vector_redirect);

#if defined(CONFIG_WEDGE_STATIC)
#define OLD_EXPORT_TYPE extern "C"
#else
#define OLD_EXPORT_TYPE INLINE
#endif

#if defined(CONFIG_VMI_SUPPORT)
#define EXPORT_SCOPE extern "C"
#else
#define EXPORT_SCOPE INLINE
#endif

static const bool debug_sizes=1;
static const bool debug=0;
static const bool debug_dtr=0;
static const bool debug_seg_write=0;
static const bool debug_seg_read=0;
static const bool debug_cr0_write=0;
static const bool debug_cr2_write=0;
static const bool debug_cr3_write=0;
static const bool debug_cr4_write=0;
static const bool debug_cr_read=0;
static const bool debug_interrupts=0;
static const bool debug_port_io=0;
static const bool debug_dr=0;
static const bool debug_iret=0;
static const bool debug_iret_redirect=0;
static const bool debug_hlt=0;
static const bool debug_msr=0;
static const bool debug_flush=0;
static const bool debug_int=0;
static const bool debug_ltr=0;
static const bool debug_str=0;
static const bool debug_flags=0;


bool frontend_init( cpu_t *cpu )
{
    *cpu = (cpu_t) {
	flags: flags_boot,
	cs: cs_boot,
	ss: segment_boot,
	tss_base: 0,
	tss_seg: segment_boot,
	idtr: dtr_boot,

	ds: segment_boot,
	es: segment_boot,
	fs: segment_boot,
	gs: segment_boot,

	cr2: cr2_boot,
	cr3: cr3_boot,
	cr4: cr4_boot,

	ldtr: segment_boot,
	gdtr: dtr_boot,

	cr0: cr0_boot,
    };

    if( debug_sizes ) {
	ASSERT( sizeof(gate_t) == 8 );
	ASSERT( sizeof(segdesc_t) == 8 );
	ASSERT( sizeof(dtr_t) == 6 );
	ASSERT( sizeof(cr0_t) == 4 );
	ASSERT( sizeof(cr3_t) == 4 );
	ASSERT( sizeof(cr4_t) == 4 );
	ASSERT( sizeof(flags_t) == 4 );
	ASSERT( sizeof(tss_t) == 104 );
	ASSERT( sizeof(pgent_t) == 4 );
	ASSERT( sizeof(frame_t) == 36 );
	ASSERT( sizeof(iret_frame_t) == 12 );
	ASSERT( sizeof(iret_user_frame_t) == 20 );
	ASSERT( sizeof(ia32_modrm_t) == 1 );

	// Confirm offsets that assembler is using.
#if defined(OFS_CPU_FLAGS)
	ASSERT( offsetof(vcpu_t, cpu.flags)    == OFS_CPU_FLAGS );
	ASSERT( offsetof(vcpu_t, cpu.cs)       == OFS_CPU_CS    );
	ASSERT( offsetof(vcpu_t, cpu.ss)       == OFS_CPU_SS    );
	ASSERT( offsetof(vcpu_t, cpu.tss_base) == OFS_CPU_TSS   );
	ASSERT( offsetof(vcpu_t, cpu.idtr)     == OFS_CPU_IDTR  );
	ASSERT( offsetof(vcpu_t, cpu.cr2)      == OFS_CPU_CR2   );
	ASSERT( offsetof(vcpu_t, cpu.redirect) == OFS_CPU_REDIRECT );
#endif
#if defined(OFS_CPU_ESP0)
	ASSERT( offsetof(vcpu_t, xen_esp0)     == OFS_CPU_ESP0 );
#endif
	ASSERT( offsetof(intlogic_t, vector_cluster) == OFS_INTLOGIC_VECTOR_CLUSTER );
    }

    return true;
};

void cpu_t::validate_interrupt_redirect()
{
    if( this->redirect >= CONFIG_WEDGE_VIRT )
	con << "Incorrect use of prepare_interrupt_redirect() at " 
	    << (void *)__builtin_return_address(0) << '\n';
}

void cpu_t::restore_interrupts( bool old_state, bool sync_deliver )
{
    if( !old_state && interrupts_enabled() )
	disable_interrupts();
    else if( old_state && 
	    !bit_test_and_set_atomic(flags_t::fi_bit, flags.x.raw) )
    {
	if( sync_deliver )
	    get_intlogic().deliver_synchronous_irq();
    }
}

tss_t * cpu_t::get_tss( u16_t segment )
{
    word_t idx = segment / 8;  // segment is a byte offset
    segdesc_t *gdt = this->gdtr.get_desc_table();
    segdesc_t &desc = gdt[ idx ];
    return (tss_t *)desc.get_base();
}

bool cpu_t::segment_exists( u16_t segment )
{
    word_t idx = segment / 8;
    if( idx >= this->gdtr.get_total_desc() )
	return false;
    return true;
}


EXPORT_SCOPE void afterburn_cpu_write_gdt32( dtr_t *dtr )
{
    get_cpu().gdtr = *dtr;
    if(debug_dtr) con << "gdt write, " << (void*)dtr << " " << get_cpu().gdtr << '\n';
} 

extern "C" void
afterburn_cpu_write_gdt32_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_gdt32( (dtr_t *)frame->eax );
}

EXPORT_SCOPE void afterburn_cpu_write_idt32( dtr_t *dtr )
{
    get_cpu().idtr = *dtr;
    if(debug_dtr) con << "idt write, " << (void*)dtr << " " << get_cpu().idtr << '\n';
}

extern "C" void
afterburn_cpu_write_idt32_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_idt32( (dtr_t *)frame->eax );
}

extern "C" void
afterburn_cpu_read_cr0_ext( burn_clobbers_frame_t *frame )
{
    if(debug_cr_read) con << "cr0 read: " << get_cpu().cr0 << '\n';
    frame->eax = get_cpu().cr0.x.raw;
}

extern "C" void
afterburn_cpu_read_cr2_ext( burn_clobbers_frame_t *frame )
{
    if(debug_cr_read) con << "cr2 read\n";
    frame->eax = get_cpu().cr2;
}

extern "C" void
afterburn_cpu_read_cr3_ext( burn_clobbers_frame_t *frame )
{
    if(debug_cr_read) con << "cr3 read\n";
    frame->eax = get_cpu().cr3.x.raw;
}

extern "C" void
afterburn_cpu_read_cr4_ext( burn_clobbers_frame_t *frame )
{
    if(debug_cr_read) con << "cr4 read: " << (void*)get_cpu().cr4.x.raw << '\n';
    frame->eax = get_cpu().cr4.x.raw;
}



EXPORT_SCOPE void
afterburn_cpu_write_cr0( u32_t data, word_t *ret_address )
{
    cpu_t & cpu = get_cpu();
    bool paging_enabled = cpu.cr0.paging_enabled();
    bool task_switched = cpu.cr0.task_switched();
    cpu.cr0.x.raw = data;
    if(debug_cr0_write) con << "cr0 write: " << cpu.cr0 << '\n';

    if( !paging_enabled && cpu.cr0.paging_enabled() )
	backend_enable_paging( ret_address );
    if( !task_switched && cpu.cr0.task_switched() )
	backend_protect_fpu();
}

EXPORT_SCOPE void
afterburn_cpu_write_cr2( u32_t data )
{
    if(debug_cr2_write) con << "cr2 write\n";
    get_cpu().cr2 = data;
}

EXPORT_SCOPE void
afterburn_cpu_write_cr3( u32_t data )
{
    cpu_t &cpu = get_cpu();
    cr3_t new_cr3 = {x: {raw:data}};

    cr3_t old_cr3 = cpu.cr3;
    cpu.cr3 = new_cr3;

    if(debug_cr3_write) con << "cr3 write: " << cpu.cr3 << '\n';

    if( old_cr3.get_pdir_addr() && cpu.cr3.get_pdir_addr() )
	backend_flush_old_pdir( cpu.cr3.get_pdir_addr(),
		old_cr3.get_pdir_addr() );
    else
	backend_flush_user();

    ASSERT( cpu.cr3.x.fields.pwt == 0 );
    ASSERT( cpu.cr3.x.fields.pcd == 0 );
}

EXPORT_SCOPE void
afterburn_cpu_write_cr4( u32_t data )
{
    if(debug_cr4_write) con << "cr4 write: " << (void*)data << '\n';
    cpu_t &cpu = get_cpu();
    cr4_t old_cr4 = cpu.cr4;
    cpu.cr4.x.raw = data;

    if( cpu.cr4.x.fields.pse != old_cr4.x.fields.pse
	    || cpu.cr4.x.fields.pge != old_cr4.x.fields.pge
	    || cpu.cr4.x.fields.pae != old_cr4.x.fields.pae )
    {
	INC_BURN_COUNTER(tlb_global_flush);
	backend_invalidate_tlb();
    }
}

extern "C" void
afterburn_cpu_write_cr0_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr0( frame->params[0], &frame->guest_ret_address );
}

extern "C" void
afterburn_cpu_write_cr2_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr2( frame->params[0] );
}

extern "C" void
afterburn_cpu_write_cr3_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr3( frame->params[0] );
}

extern "C" void
afterburn_cpu_write_cr4_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr4( frame->params[0] );
}

#if defined(CONFIG_VMI_SUPPORT)
extern "C" void
afterburn_cpu_write_cr0_regs( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr0( frame->eax, &frame->guest_ret_address );
}

extern "C" void
afterburn_cpu_write_cr2_regs( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr2( frame->eax );
}

extern "C" void
afterburn_cpu_write_cr3_regs( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr3( frame->eax );
}

extern "C" void
afterburn_cpu_write_cr4_regs( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cr4( frame->eax );
}
#endif

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_cs()
{
    if(debug_seg_write) con << "cs read\n";
    return get_cpu().cs;
}

extern "C" void afterburn_cpu_read_cs_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_cs();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_cs( u16_t cs )
{
    if(debug_seg_write) con << "cs write: " << cs << '\n';
    get_cpu().cs = cs;
}

extern "C" void afterburn_cpu_write_cs_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_cs( frame->params[0] );
}

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_ds( void )
{
    if(debug_seg_read) con << "ds read\n";
    return get_cpu().ds;
}

extern "C" void afterburn_cpu_read_ds_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_ds();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_ds( u16_t ds )
{
    if(debug_seg_write) con << "ds write: " << ds << '\n';
    get_cpu().ds = ds;
}

extern "C" void afterburn_cpu_write_ds_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_ds( frame->params[0] );
}

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_es( void )
{
    if(debug_seg_read) con << "es read\n";
    return get_cpu().es;
}

extern "C" void afterburn_cpu_read_es_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_es();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_es( u16_t es )
{
    if(debug_seg_write) con << "es write: " << es << '\n';
    get_cpu().es = es;
}

extern "C" void afterburn_cpu_write_es_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_es( frame->params[0] );
}

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_fs( void )
{
    if(debug_seg_read) con << "fs read\n";
    return get_cpu().fs;
}

extern "C" void afterburn_cpu_read_fs_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_fs();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_fs( u16_t fs )
{
    if(debug_seg_write) con << "fs write: " << fs << '\n';
    get_cpu().fs = fs;
}

extern "C" void afterburn_cpu_write_fs_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_fs( frame->params[0] );
}

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_gs( void )
{
    if(debug_seg_read) con << "gs read\n";
    return get_cpu().gs;
}

extern "C" void afterburn_cpu_read_gs_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_gs();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_gs( u16_t gs )
{
    if(debug_seg_write) con << "gs write: " << gs << '\n';
    get_cpu().gs = gs;
}

extern "C" void afterburn_cpu_write_gs_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_gs( frame->params[0] );
}

OLD_EXPORT_TYPE u16_t afterburn_cpu_read_ss( void )
{
    if(debug_seg_read) con << "ss read\n";
    return get_cpu().ss;
}

extern "C" void afterburn_cpu_read_ss_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_ss();
}

OLD_EXPORT_TYPE void afterburn_cpu_write_ss( u16_t ss )
{
    if(debug_seg_write) con << "ss write: " << ss << '\n';
    get_cpu().ss = ss;
}

extern "C" void afterburn_cpu_write_ss_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_ss( frame->params[0] );
}

extern "C" void afterburn_cpu_lss_ext( burn_clobbers_frame_t *frame )
{
    u16_t *data = (u16_t *)frame->eax;
    u16_t ss = data[3];	// The 32-bit word preceeds the segment.
    afterburn_cpu_write_ss( ss );
}

extern "C" void afterburn_cpu_clts( void )
{
    if(debug_cr0_write) con << "cr0 clear ts\n";
    get_cpu().cr0.x.fields.ts = 0;
}

EXPORT_SCOPE void afterburn_cpu_lldt( u16_t segment )
{
    if(debug_seg_write) con << "ldtr write: " << segment << '\n';
    get_cpu().ldtr = segment;
}

extern "C" void
afterburn_cpu_lldt_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_lldt( frame->eax );
}

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_FLAGS)
EXPORT_SCOPE u32_t afterburn_cpu_read_flags( u32_t flags_to_push )
{
    cpu_t & cpu = get_cpu();
    flags_to_push = (cpu.flags.x.raw & ~flags_user_mask) | (flags_to_push & flags_user_mask);
    return flags_to_push;
}

extern "C" void
afterburn_cpu_read_flags_ext( burn_clobbers_frame_t *frame )
{
    frame->params[0] = afterburn_cpu_read_flags( frame->params[0] );
    if (debug_flags) con << "read flags" << (void*)frame->params[0] << " " << (void*)get_cpu().flags.x.raw << '\n';
}
#endif

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ) || defined(CONFIG_IA32_STRICT_FLAGS)
EXPORT_SCOPE u32_t afterburn_cpu_write_flags( u32_t flags_to_pop )
{
    cpu_t & cpu = get_cpu();
    bool was_enabled = cpu.interrupts_enabled();

    cpu.flags.x.raw = flags_to_pop;

    if( !was_enabled && cpu.interrupts_enabled() )
    {
	if(debug_interrupts) con << "interrupts enabled via popf\n";
	get_intlogic().deliver_synchronous_irq();
    }

    return (cpu.flags.x.raw & ~flags_user_mask) | flags_system_at_user;
}

extern "C" void
afterburn_cpu_write_flags_ext( burn_clobbers_frame_t *frame )
{
    if (debug_flags) con << "write flags" << (void*)frame->params[0] << " " 
			 << (void*)get_cpu().flags.x.raw << '\n';
    frame->params[0] = afterburn_cpu_write_flags( frame->params[0] );
}
#endif

extern "C" void afterburn_cpu_deliver_irq( void )
{
    get_intlogic().deliver_synchronous_irq();
}

extern "C" void afterburn_cpu_unimplemented( void )
{
    con << "error: unimplemented afterburn instruction, "
	<< "ip: " << __builtin_return_address(0) << '\n';
    panic();
}

extern "C" void afterburn_cpu_unimplemented_ext( burn_clobbers_frame_t *frame )
{
    con << "error: unimplemented afterburn instruction, "
	<< "ip: " << (void *)frame->guest_ret_address << '\n';
    panic();
}

extern "C" void afterburn_cpu_ud2_ext( burn_clobbers_frame_t *frame )
{
    con << "error: execution of ud2, the unimplemented instruction, "
	<< "ip: " << (void*)frame->guest_ret_address << '\n';
    panic();
}

extern "C" void afterburn_cpu_cli( void )
{
    cpu_t & cpu = get_cpu();
    if( cpu.disable_interrupts() )
    {
	if(debug_interrupts) con << "cli\n";
    }
}

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ)
extern "C" bool afterburn_cpu_sti( void )
{
    cpu_t & cpu = get_cpu();
    if( 0 == cpu.flags.x.fields.fi )
    {
	if(debug_interrupts) con << "sti\n";
	bit_set_atomic( flags_t::fi_bit, cpu.flags.x.raw );
	return get_intlogic().deliver_synchronous_irq();
    }
    return false;
}
#endif

bool burn_redirect_frame_t::do_redirect()
{
    word_t vector, irq;
    if( !get_intlogic().pending_vector(vector, irq) ) {
	this->x.out_no_redirect.iret.ip = 0;
	return false;
    }
    do_redirect( vector );
    return true;
}

void burn_redirect_frame_t::do_redirect( word_t vector )
{
    word_t caller = this->x.in.ip;

    cpu_t &cpu = get_cpu();
    ASSERT( vector < cpu.idtr.get_total_gates() );
    gate_t &gate = cpu.idtr.get_gate_table()[ vector ];
    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    this->x.out_redirect.iret.cs = cpu.cs;
    this->x.out_redirect.iret.flags.x.raw = cpu.flags.get_raw();
    this->x.out_redirect.iret.ip = caller;

    this->x.out_redirect.ip = gate.get_offset();

    cpu.flags.prepare_for_interrupt();
    cpu.cs = gate.get_segment_selector();
    cpu.flags.x.fields.fi = gate.is_trap();
}

extern "C" void __attribute__(( regparm(1) ))
afterburn_cpu_deliver_interrupt( burn_redirect_frame_t *frame )
{
    ASSERT( get_cpu().interrupts_enabled() );

    frame->do_redirect();
}


extern "C" void __attribute__(( regparm(1) ))
afterburn_cpu_interruptible_hlt( burn_redirect_frame_t *frame )
{
    // Execute timing hooks
    device_flush( true );

    backend_interruptible_idle( frame );
}


OLD_EXPORT_TYPE u32_t afterburn_cpu_read_port( 
	u32_t bit_width, u32_t edx, u32_t eax )
{
    u16_t port = edx & 0xffff;
    u32_t value;

    if( !portio_read(port, value, bit_width) )
	con << "Unhandled port read, port " << port
	    << ", ip " << (void *)__builtin_return_address(0) << '\n';
    else if( debug_port_io )
	con << "read port " << port << ", value " << value << '\n';

    // Preserve the remaining parts of the eax register.
    if( bit_width < 32 )
	return (eax & ~((1UL << bit_width)-1)) | value;
    return value;
}

extern "C" void
afterburn_cpu_read_port_ext( burn_clobbers_frame_t *frame )
{
    u32_t bit_width = frame->params[1];
    u16_t port = frame->params[0] & 0xffff;
    u32_t value;

    if( !portio_read(port, value, bit_width) )
	con << "Unhandled port read, port " << port
	    << ", ip " << (void *)frame->guest_ret_address << '\n';
    else if( debug_port_io )
	con << "read port " << port << ", value " << value << '\n';

    // Preserve the remaining parts of the eax register.
    if( bit_width < 32 )
	frame->eax = (frame->eax & ~((1UL << bit_width)-1)) | value;
    else
	frame->eax = value;
}

extern "C" void
afterburn_cpu_read_port8_regs( burn_clobbers_frame_t *frame )
{
    portio_read( frame->edx & 0xffff, frame->eax, 8 );
}

extern "C" void
afterburn_cpu_read_port16_regs( burn_clobbers_frame_t *frame )
{
    portio_read( frame->edx & 0xffff, frame->eax, 16 );
}

extern "C" void
afterburn_cpu_read_port32_regs( burn_clobbers_frame_t *frame )
{
    portio_read( frame->edx & 0xffff, frame->eax, 32 );
}

OLD_EXPORT_TYPE void
afterburn_cpu_write_port( u32_t bit_width, u32_t edx, u32_t eax )
{
    u16_t port = edx & 0xffff;
    u32_t data = eax;

    // gcc is generating code that *rotates*, rather than shifts, 
    // so only mask if the bit width is less than 32.
    if( bit_width < 32 )
	data &= (1UL << bit_width) - 1;

    if( !portio_write(port, data, bit_width) )
	con << "Unhandled port write, port " << port
	    << ", value " << data
	    << ", ip " << (void *)__builtin_return_address(0) << '\n';
    else if( debug_port_io )
	con << "write port " << port << ", value " << data << '\n';
}

extern "C" void
afterburn_cpu_write_port_ext( burn_clobbers_frame_t *frame )
{
    u32_t bit_width = frame->params[1];
    u16_t port = frame->params[0] & 0xffff;
    u32_t data = frame->eax;

    // gcc is generating code that *rotates*, rather than shifts, 
    // so only mask if the bit width is less than 32.
    if( bit_width < 32 )
	data &= (1UL << bit_width) - 1;

    if( !portio_write(port, data, bit_width) )
	con << "Unhandled port write, port " << port
	    << ", value " << data
	    << ", ip " << (void *)frame->guest_ret_address << '\n';
    else if( debug_port_io )
	con << "write port " << port << ", value " << data << '\n';
}

extern "C" void
afterburn_cpu_write_port8_regs( burn_clobbers_frame_t *frame )
{
    portio_write( frame->edx & 0xffff, frame->eax & 0xff, 8 );
}

extern "C" void
afterburn_cpu_write_port16_regs( burn_clobbers_frame_t *frame )
{
    portio_write( frame->edx & 0xffff, frame->eax & 0xffff, 16 );
}

extern "C" void
afterburn_cpu_write_port32_regs( burn_clobbers_frame_t *frame )
{
    portio_write( frame->edx & 0xffff, frame->eax, 32 );
}


extern "C" void afterburn_cpu_out_port(u32_t eax, u32_t edx, u8_t bit_width)
{
    u16_t port = edx & 0xffff;
    u32_t data = eax;

    // gcc is generating code that *rotates*, rather than shifts, 
    // so only mask if the bit width is less than 32.
    if( bit_width < 32 )
	data &= (1UL << bit_width) - 1;

    if( !portio_write(port, data, bit_width) )
	con << "Unhandled port write, port " << (void*)(u32_t)port
	    << ", value " << data
	    << ", ip " << __builtin_return_address(0) << '\n';
    //else if( debug_port_io )
    //   con << "write port " << (void*) port << ", value " << data << '\n';
}

extern "C" u32_t afterburn_cpu_in_port(u32_t eax, u32_t edx, u8_t bit_width)
{
    u16_t port = edx & 0xffff;
    u32_t value;
	
    if( !portio_read(port, value, bit_width) )
	con << "Unhandled port read, port " << (void*)(u32_t)port
	    << ", ip " << __builtin_return_address(0) << '\n';
    //else if( debug_port_io )
    //	    con << "read port " << (void*) port << ", value " << value << '\n';

    // Preserve the remaining parts of the eax register.
    return (eax & (0xffffffff << bit_width)) | value;
}

OLD_EXPORT_TYPE void afterburn_cpu_ltr( u32_t src )
{
    cpu_t &cpu = get_cpu();
    cpu.tss_seg = src;
    u32_t tss_idx = cpu.tss_seg/8;	// tss_seg is a byte offset

    if( debug_ltr )
	con << "ltr " << cpu.tss_seg 
	    << ", gdt size " << cpu.gdtr.get_total_desc() << '\n';
    ASSERT( tss_idx < cpu.gdtr.get_total_desc() );

    segdesc_t *gdt = cpu.gdtr.get_desc_table();
    segdesc_t tss_desc = gdt[ tss_idx ];
    cpu.tss_base = tss_desc.get_base();

    if( debug_ltr ) {
	con << "system? " << tss_desc.is_system()
	    << ", present? " << tss_desc.is_present()
	    << ", privilege " << tss_desc.get_privilege()
	    << ", base " << (void *)tss_desc.get_base()
	    << ", limit " << tss_desc.get_limit()
	    << '\n';
    }
}

extern "C" void
afterburn_cpu_ltr_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_ltr( frame->eax );
}

OLD_EXPORT_TYPE u32_t afterburn_cpu_str( void )
{
    if( debug_str ) con << "str\n";
    return get_cpu().tss_seg;
}

extern "C" void
afterburn_cpu_str_ext( burn_clobbers_frame_t *frame )
{
    frame->eax = afterburn_cpu_str();
}


OLD_EXPORT_TYPE void afterburn_cpu_invlpg( u32_t addr )
{
    if(debug_flush) con << "invlpg " << (void *)addr << '\n';
    backend_flush_vaddr( addr );
}

extern "C" void
afterburn_cpu_invlpg_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_invlpg( frame->eax );
}

extern "C" void afterburn_cpu_invd()
{
    con << "warning: untested INVD\n";
    DEBUGGER_ENTER(0);
    backend_flush_user();
}

extern "C" void afterburn_cpu_wbinvd( void )
{
    backend_flush_user();
}

OLD_EXPORT_TYPE u32_t afterburn_cpu_read_dr( u32_t which )
{
    if(debug_dr) con << "read dr[" << which << "]\n";
    return get_cpu().dr[which];
}

extern "C" void afterburn_cpu_read_dr_ext( burn_clobbers_frame_t *frame )
{
    frame->params[1] = afterburn_cpu_read_dr( frame->params[0] );
}

OLD_EXPORT_TYPE void afterburn_cpu_write_dr( u32_t which, u32_t value )
{
    if(debug_dr) con << "dr[" << which << "] = " << value << '\n';
    get_cpu().dr[which] = value;
}

extern "C" void afterburn_cpu_write_dr_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_dr( frame->params[0], frame->params[1] );
}

#if defined(CONFIG_VMI_SUPPORT)
extern "C" void afterburn_cpu_read_dr_regs( burn_clobbers_frame_t *frame )
{
    frame->eax = afterburn_cpu_read_dr( frame->eax );
}

extern "C" void afterburn_cpu_write_dr_regs( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_dr( frame->eax, frame->edx );
}
#endif


extern "C" void __attribute__(( regparm(1) ))
afterburn_cpu_lret( lret_frame_t *lret )
{
    get_cpu().cs = lret->cs;
    __asm__ __volatile__ ("movl %%cs, %%eax" : "=a"(lret->cs) );
}

#if defined(CONFIG_IA32_FAST_VECTOR_ESP0)
extern "C" void
afterburn_cpu_esp0_update( void )
{
    backend_esp0_sync();
}
#endif

#define afterburn_cpu_iret_resume(ret_esp)		\
do {							\
    __asm__("mov %0, %%esp         \n"			\
	    "jmp burn_iret_resume  \n"			\
	    : : "r"(ret_esp)				\
	   );						\
} while(1)

extern "C" void NORETURN __attribute__(( regparm(1) )) SECTION(".text.iret")
afterburn_cpu_iret( iret_handler_frame_t *x )
// Note: This function is entered via a jmp from assembler.  The iret
// frame contains all relevant information, and thus there is no
// reason to save the C calling convention state on the stack, nor
// to execute the C function prologue.  To return to the caller, 
// use a jmp.
{
    iret_user_frame_t *iret = &x->iret;
    frame_t *frame = &x->frame;
    iret_frame_t *redirect_iret = &x->redirect_iret;

    cpu_t &cpu = get_cpu();

    if(debug_iret)
	con << "iret, ip " << (void *)iret->ip
	    << ", cs " << iret->cs 
	    << ", flags " << (void *)iret->flags.get_raw() << '\n';
 
    ASSERT( !iret->flags.x.fields.nt );

    // Interrupts are enabled.  Check for a pending interrupt,
    // and arrange to have it delivered by redirecting this iret.
    word_t vector, irq;
    if( iret->flags.interrupts_enabled() 
	    && EXPECT_FALSE(get_intlogic().pending_vector(vector, irq)) )
    {
	// We redirect the iret to a vector.  We provide the current iret
	// frame to the vector's handler, thus immitating the hardware: If the
	// current iret is to user, then the vector handler will think that
	// user was interrupted, and if the current iret is to kernel, then
	// the vector handler will think that the kernel was interrupted.
	// Most importantly, the vector handler never sees an IP address
	// from the wedge.

	device_flush( false );

	INC_BURN_COUNTER(iret_vector_redirect);
	ASSERT( vector < cpu.idtr.get_total_gates() );
    	gate_t &gate = cpu.idtr.get_gate_table()[ vector ];
	ASSERT( gate.is_trap() || gate.is_interrupt() );
	ASSERT( gate.is_present() );
	ASSERT( gate.is_32bit() );

	cpu.flags.prepare_for_interrupt();
	cpu.cs = gate.get_segment_selector();
	cpu.flags.x.fields.fi = gate.is_trap();

       	__asm__ __volatile__ ("movl %%cs, %%eax" : "=a"(redirect_iret->cs) );
	redirect_iret->flags.x.raw = flags_system_at_user;
	redirect_iret->ip = gate.get_offset();

	if( debug_iret_redirect ) {
	    con << "iret redirect, new values, ip " << (void *)redirect_iret->ip
		<< ", cs " << redirect_iret->cs 
		<< ", flags " << (void *)redirect_iret->flags.get_raw() << '\n';
	    con << "iret redirect, interrupted values, ip " << (void *)iret->ip
		<< ", cs " << iret->cs
		<< ", flags " << (void *)iret->flags.get_raw() << '\n';
	}

	/* We have to switch to the proper esp0.
	 */
	word_t esp0 = cpu.get_tss()->esp0;
	iret_handler_frame_t *esp0_frame = (iret_handler_frame_t *)(esp0 - sizeof(iret_handler_frame_t));
	if( (x != esp0_frame) && 
		(cpu_t::get_segment_privilege(iret->cs) == 3) )
	{
	    INC_BURN_COUNTER(iret_esp0_switch);
	    memcpy( esp0_frame, x, sizeof(iret_handler_frame_t) );
	    afterburn_cpu_iret_resume( esp0_frame );
	}
	afterburn_cpu_iret_resume( frame );
    }

    redirect_iret->ip = 0;
 
    // Go to user mode?
#if defined(CONFIG_VMI_SUPPORT)
    if( cpu_t::get_segment_privilege(iret->cs) == 3 )
#else
    if( cpu_t::get_segment_privilege(iret->cs)
	    > cpu_t::get_segment_privilege(cpu.cs) )
#endif
    {
	device_flush( true );

	user_frame_t user_frame = {iret:iret, regs:frame};
	backend_activate_user( &user_frame );
    }
    else
    {
	device_flush( false );

	cpu.flags = iret->flags;
	iret->flags.x.raw = 
	    (cpu.flags.get_raw() & flags_user_mask) | flags_system_at_user;
	__asm__ __volatile__ ("movl %%cs, %%eax" : "=a"(iret->cs) );
    }

    if(debug_iret)
	con << "iret new values, ip " << (void *)iret->ip
	    << ", cs " << iret->cs 
	    << ", flags " << (void *)iret->flags.get_raw() << '\n';

    afterburn_cpu_iret_resume( frame );
}

OLD_EXPORT_TYPE void afterburn_cpu_read_cpuid( frame_t *frame )
{
    u32_t func = frame->x.fields.eax;
    static u32_t max_basic=0, max_extended=0;

    // Note: cpuid is a serializing instruction!

    if( max_basic == 0 )
    {
	// We need to determine the maximum inputs that this CPU permits.

	// Query for the max basic input.
	frame->x.fields.eax = 0;
	__asm__ __volatile__ ("cpuid"
    		: "=a"(frame->x.fields.eax), "=b"(frame->x.fields.ebx), 
		  "=c"(frame->x.fields.ecx), "=d"(frame->x.fields.edx)
    		: "0"(frame->x.fields.eax));
	max_basic = frame->x.fields.eax;

	// Query for the max extended input.
	frame->x.fields.eax = 0x80000000;
	__asm__ __volatile__ ("cpuid"
    		: "=a"(frame->x.fields.eax), "=b"(frame->x.fields.ebx),
		  "=c"(frame->x.fields.ecx), "=d"(frame->x.fields.edx)
    		: "0"(frame->x.fields.eax));
	max_extended = frame->x.fields.eax;

	// Restore the original request.
	frame->x.fields.eax = func;
    }

    // TODO: constrain basic functions to 3 if 
    // IA32_CR_MISC_ENABLES.BOOT_NT4 (bit 22) is set.

    // Execute the cpuid request.
    __asm__ __volatile__ ("cpuid"
	    : "=a"(frame->x.fields.eax), "=b"(frame->x.fields.ebx), 
	      "=c"(frame->x.fields.ecx), "=d"(frame->x.fields.edx)
	    : "0"(frame->x.fields.eax));

    // Start our over-ride logic.
    backend_cpuid_override( func, max_basic, max_extended, frame );
}

extern "C" void
afterburn_cpu_read_cpuid_ext( burn_frame_t *frame )
{
    afterburn_cpu_read_cpuid( &frame->regs );
}

OLD_EXPORT_TYPE void afterburn_cpu_read_msr( u32_t msr_addr, 
	u32_t &lower, u32_t &upper)
{
    if( debug_msr )
	con << "MSR read for " << msr_addr << '\n';
    lower = upper = 0;
}

extern "C" void
afterburn_cpu_read_msr_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_read_msr( frame->ecx, frame->eax, frame->edx );
}

OLD_EXPORT_TYPE void afterburn_cpu_write_msr( u32_t msr_addr, 
	u32_t lower, u32_t upper)
{
    if( debug_msr )
	con << "MSR write for " << msr_addr
	    << ", lower " << (void *)lower 
	    << ", upper " << (void *)upper << '\n';

    switch( msr_addr ) {
	case 0x300 ... 0x311:	/* Pentium 4 perfmon counters. */
	case 0x360 ... 0x371:	/* Pentium 4 perfmon CCCRs. */
	case 0x3a0 ... 0x3af:	/* Pentium 4 perfmon ESCRs. */
	case 0x3b0 ... 0x3be:
	case 0x3c0 ... 0x3cd:
	case 0x3e0 ... 0x3e1:
	    backend_write_msr( msr_addr, lower, upper );
	    return;
    }

    con << "MSR write was unhandled (MSR " << msr_addr << ").\n";
}

extern "C" void
afterburn_cpu_write_msr_ext( burn_clobbers_frame_t *frame )
{
    afterburn_cpu_write_msr( frame->ecx, frame->eax, frame->edx );
}

extern "C" void __attribute__(( regparm(2) ))
afterburn_cpu_int( iret_frame_t *save_frame, iret_frame_t *int_frame )
{
    cpu_t &cpu = get_cpu();

    /* We have to construct the iret save frame.  On entry to this function,
     * it has the vector in its flags, the return addr in the cs, and
     * the flags in the IP.
     */
    word_t vector = save_frame->flags.x.raw;
    word_t user_ip = save_frame->cs;
    word_t user_flags = save_frame->ip;

    save_frame->flags.x.raw = user_flags;
    save_frame->ip = user_ip+2; // Go to the instruction after the int.

    __asm__ __volatile__ ("movl %%cs, %%eax" : "=a"(int_frame->cs) );
    int_frame->flags.x.raw = 
	(save_frame->flags.x.raw & flags_user_mask) | flags_system_at_user;

    if( debug_int )
	con << "int " << vector << ", return ip " << (void *)save_frame->ip
	    << ", user flags " << (void *)save_frame->flags.x.raw << '\n';
  
    ASSERT( vector <= cpu.idtr.get_total_gates() );

    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    flags_t old_flags = cpu.flags;

    bool old_if_state = cpu.disable_interrupts();
    cpu.flags.prepare_for_interrupt();

    u16_t old_cs = cpu.cs;
    cpu.cs = gate.get_segment_selector();

    save_frame->cs = old_cs;
    save_frame->flags.x.raw = (old_flags.x.raw & ~flags_user_mask) | (save_frame->flags.x.raw & flags_user_mask);
    int_frame->ip = gate.get_offset();

    if( gate.is_trap() )
	cpu.restore_interrupts( old_if_state );

    if( debug_int )
	con << "int target ip " << (void *)int_frame->ip
	    << ", return flags " << (void *)save_frame->flags.x.raw << '\n'; 
}

