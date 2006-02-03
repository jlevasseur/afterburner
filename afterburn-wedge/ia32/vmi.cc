/*********************************************************************
 *
 * Copyright (C) 2005,  Volkmar Uhlig
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/kaxen/vmi.cc
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
 * $Id: vmi.cc,v 1.12 2006/01/04 22:28:16 joshua Exp $
 *
 ********************************************************************/

#include INC_ARCH(types.h)
#include INC_ARCH(cpu.h)
#include INC_ARCH(ops.h)

#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)

#include <device/sequencing.h>
#include <device/portio.h>
#include <memory.h>

static const bool debug_init = 0;
static const bool debug_rewrite = 0;
static const bool debug_set_pte = 0;
static const bool debug_get_pte = 0;
static const bool debug_iret = 0;
static const bool debug_iret_redirect=0;
static const bool debug_read_port_string = 0;
static const bool debug_write_port_string = 0;
static const bool debug_phys2mach = 0;
static const bool debug_mach2phys = 0;
static const bool debug_pageassign = 0;

typedef struct dtr_t VMI_DTR;

typedef u64_t VMI_UINT64;
typedef u32_t VMI_UINT32;
typedef u16_t VMI_UINT16;
typedef u8_t VMI_UINT8;
typedef s8_t VMI_INT8;
typedef int VMI_INT;
typedef u16_t VMI_SELECTOR;
typedef u8_t VMI_BOOL;

typedef u32_t VMI_PTE;
typedef u64_t VMI_PAE_PTE;

#include INC_ARCH(vmiCalls.h)
#include INC_ARCH(vmiRom.h)

#define VMI_ROM_SIZE	(4096 * 4)

extern int __VMI_START;
extern trap_info_t xen_trap_table[];
extern "C" void trap_wrapper_13();

static inline pgent_t u32_to_pgent(u32_t val)
{
    pgent_t pgent;
    pgent.x.raw = val;
    return pgent;
}

void vmi_init ( void )
{
    VROMHeader *romstart = (VROMHeader*)CONFIG_VMI_ROM_START;
    con << "Initializing VMI ROM\n";
    memcpy(romstart, (void*)&__VMI_START, VMI_ROM_SIZE);
    romstart->romSignature = 0xaa55;
    romstart->vRomSignature = VMI_SIGNATURE;
    romstart->APIVersionMajor = VMI_API_REV_MAJOR;
    romstart->APIVersionMinor = VMI_API_REV_MINOR;
    romstart->romLength = VMI_ROM_SIZE / 512;
    romstart->dataPages = 0;

    con << "Relocating ROM from " << (void*)&__VMI_START 
	<< " to " << (void*)romstart << "\n";
}

extern "C" void 
vmi_rewrite_calls_ext( burn_clobbers_frame_t *frame )
{
    extern u8_t _vmi_rewrite_calls_return[], _vmi_rewrite_calls_invoke[];
    extern word_t _vmi_patchups_start[], _vmi_patchups_end[];
    s32_t delta = (s32_t)(word_t(_vmi_rewrite_calls_return) - frame->burn_ret_address);

    if( debug_rewrite )
	con << "ROM init link address: " << (void *)_vmi_rewrite_calls_return 
	    << ", new init address: " << (void *)frame->burn_ret_address
	    << ", ROM relocation: " << (void *)delta << '\n';

    for( word_t *call_addr = _vmi_patchups_start;
	    call_addr < _vmi_patchups_end; call_addr++ )
    {
	u8_t *instr = (u8_t *)(*call_addr - delta);
	if( debug_rewrite )
	    con << "VMI call at " << (void *)*call_addr
		<< ", relocated to " << (void *)instr << '\n';

	if( instr[0] != OP_CALL_REL32 && instr[0] != OP_JMP_REL32 )
	    PANIC( "Invalid VMI ROM contents." );

	s32_t *call_target = (s32_t *)&instr[1];
	*call_target = *call_target + delta;
    }

    /* Remove the initialization code.
     * Be sure to do this in the target ROM, not the original ROM.
     */
    u8_t *instr = _vmi_rewrite_calls_invoke - delta;
    *instr = OP_RET;
}

extern "C" void vmi_init_ext ( burn_clobbers_frame_t *frame )
{
    con << "VMI init called from guest.\n";
    if( debug_init ) {
	con << "  from ROM @ " << (void *)frame->burn_ret_address << '\n';
	con << "  from guest @ " << (void *)frame->guest_ret_address << '\n';
    }

    /* VU: At this point interrupts are enabled; make sure the virt
     * cpu knows that */
    get_cpu().disable_interrupts();

    /* hand back trap 13 */
    xen_trap_table[13].address = (word_t)trap_wrapper_13;

    if( XEN_set_trap_table(xen_trap_table) )
	PANIC( "Unable to configure the Xen trap table." );

    frame->eax = 0; // success
}

extern "C" void vmi_flush_tlb_ext ( burn_clobbers_frame_t *frame )
{
    backend_flush_old_pdir( get_cpu().cr3.get_pdir_addr(),
			    get_cpu().cr3.get_pdir_addr() );
}

extern "C" void
vmi_esp0_update_ext( burn_clobbers_frame_t *frame )
{
    backend_esp0_sync();
}

/* mov %eax, (%edx) */
extern "C" void
vmi_set_pte_ext( burn_clobbers_frame_t *frame )
{
    if (debug_set_pte) 
	con << "vmi_set_pte_ext pteptr=" << (void*)frame->edx << " pteval=" 
	    << (void*)frame->eax << " level=" << frame->ecx << " ret=" 
	    << (void*)frame->params[2] << '\n';
    
    pgent_t pgent = u32_to_pgent(frame->eax);

#if defined(CONFIG_WEDGE_KAXEN)
    /* Silly Xen can't handle global bits.  And OF COURSE they don't
     * clear them, nooooo way... they barf */ 
    pgent.x.fields.global = 0; 
#endif

    xen_memory.change_pgent( (pgent_t*)frame->edx, pgent, frame->ecx == 2 );

    if( get_intlogic().maybe_pending_vector() )
	get_cpu().prepare_interrupt_redirect();
}


/* LOCK bts %eax, (%edx); sbb %eax, %eax */
extern "C" void
vmi_test_and_set_pte_bit_ext( burn_clobbers_frame_t *frame )
{
    if (debug_set_pte)
	con << "vmi_test_and_set_pte_bit_ext " << (void*)frame->eax 
	    << " " << (void*)frame->edx << "\n";

    u32_t val = frame->edx;

    if( (val & (1 << frame->eax)) == 0 ) {
	xen_memory.change_pgent( (pgent_t*)frame->edx, 
		u32_to_pgent(val | (1 << frame->eax)), true );
	if( get_intlogic().maybe_pending_vector() )
    	    get_cpu().prepare_interrupt_redirect();
    }

    frame->eax = val & (1 << frame->eax);
}

extern "C" void
vmi_phys_to_machine_ext( burn_clobbers_frame_t *frame )
{
#if defined(CONFIG_DEVICE_PASSTHRU)
    if (debug_phys2mach) con << "VMI_PhysToMachine: " << (void*)frame->eax;
    frame->eax = backend_phys_to_dma_hook( frame->eax );
    if (debug_phys2mach) con << " --> " << (void*)frame->eax << '\n';
#endif
}

extern "C" void
vmi_machine_to_phys_ext( burn_clobbers_frame_t *frame )
{
#if defined(CONFIG_DEVICE_PASSTHRU)
    if (debug_mach2phys) con << "VMI_MachineToPhys: " << (void*)frame->eax;
    frame->eax = backend_dma_to_phys_hook( frame->eax );
    if (debug_mach2phys) con << " --> " << (void*)frame->eax << '\n';
#endif
}


extern "C" void
vmi_cpu_stts( burn_clobbers_frame_t *frame )
{
    get_cpu().cr0.x.fields.ts = 1;
}

extern "C" void 
vmi_cpu_shutdown( burn_clobbers_frame_t *frame )
{
    backend_shutdown();
}

extern "C" void 
vmi_cpu_reboot( burn_clobbers_frame_t *frame )
{
    backend_reboot();
}    

/**************************************************************************
 * Port string operations.
 **************************************************************************/

struct vmi_port_string_t {
    word_t burn_ret_address;
    word_t frame_pointer;
    word_t edx;
    word_t ecx;
    word_t eax;
    word_t addr;
    word_t guest_ret_address;
};

template <const word_t bitwidth, typename type>
INLINE void vmi_cpu_write_port_string( vmi_port_string_t *frame )
{
    u16_t port = frame->edx & 0xffff;
    while( frame->ecx-- ) {
	portio_write( port, *(type *)frame->addr, bitwidth );
	frame->addr += bitwidth / 8;
    }
}

extern "C" void vmi_cpu_write_port_string_8( vmi_port_string_t *frame ) {
    vmi_cpu_write_port_string<8, u8_t>(frame);
}
extern "C" void vmi_cpu_write_port_string_16( vmi_port_string_t *frame ) {
    vmi_cpu_write_port_string<16, u16_t>(frame);
}
extern "C" void vmi_cpu_write_port_string_32( vmi_port_string_t *frame ) {
    vmi_cpu_write_port_string<32, u32_t>(frame);
}


template <const word_t bitwidth, typename type>
INLINE void vmi_cpu_read_port_string( vmi_port_string_t *frame )
{
    u16_t port = frame->edx & 0xffff;
    while( frame->ecx-- ) {
	u32_t value;
	portio_read( port, value, bitwidth );
	*(type *)frame->addr = (type)value;
	frame->addr += bitwidth / 8;
    }
}

extern "C" void vmi_cpu_read_port_string_8( vmi_port_string_t *frame ) {
    vmi_cpu_read_port_string<8, u8_t>(frame);
}
extern "C" void vmi_cpu_read_port_string_16( vmi_port_string_t *frame ) {
    vmi_cpu_read_port_string<16, u16_t>(frame);
}
extern "C" void vmi_cpu_read_port_string_32( vmi_port_string_t *frame ) {
    vmi_cpu_read_port_string<32, u32_t>(frame);
}

/**************************************************************************/

extern "C" void
vmi_register_page_mapping_ext( burn_clobbers_frame_t *frame )
{

}

extern "C" void
vmi_register_page_usage_ext( burn_clobbers_frame_t *frame )
{
    if (debug_pageassign)
	con << "register page use: " << (void*)frame->eax << ", flags=" << (void*)frame->edx << '\n';
}

extern "C" void
vmi_release_page_ext( burn_clobbers_frame_t *frame )
{ 
    if (debug_pageassign)
	con << "release page: " << (void*)frame->eax << ", flags=" << (void*)frame->edx << '\n';
}

/**************************************************************************/

// Size of a call-relative instruction:
static const word_t call_relative_size = 5;

extern "C" void
vmi_rewrite_rdtsc_ext( burn_clobbers_frame_t *frame )
    // Rewrite the guest's rdtsc callsite to directly execute rdtsc.
{
    u8_t *opstream = (u8_t *)(frame->guest_ret_address - call_relative_size);
    if( opstream[0] != OP_CALL_REL32 )
	PANIC( "Unexpected call to the VMI ROM at " 
		<< (void *)frame->guest_ret_address );

    opstream[0] = OP_2BYTE;
    opstream[1] = OP_RDTSC;
    opstream[2] = OP_NOP1;
    opstream[3] = OP_NOP1;
    opstream[4] = OP_NOP1;

    // Restart the instruction.
    frame->guest_ret_address -= call_relative_size;
}

