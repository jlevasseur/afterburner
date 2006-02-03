/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe and
 *                      National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/ia32/rewrite_stackless.cc
 * Description:   The logic for rewriting an afterburned guest OS.
 *                The rewritten code assumes that it can use the 
 *                guest OS stack.
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
 * $Id: rewrite_stackless.cc,v 1.29 2005-12-27 09:12:09 joshua Exp $
 *
 ********************************************************************/

#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(debug.h)

#include INC_ARCH(ops.h)
#include INC_ARCH(instr.h)
#include INC_ARCH(intlogic.h)

#include <bind.h>
#include <memory.h>
#include <templates.h>

#if defined(CONFIG_IA32_HARDWARE_SEGMENTS)
static const bool hardware_segments = true;
#else
static const bool hardware_segments = false;
#endif

static const bool debug_nop_space = 0;

static word_t curr_virt_addr;
static word_t stack_offset;

enum prefix_e {
    prefix_lock=0xf0, prefix_repne=0xf2, prefix_rep=0xf3,
    prefix_operand_size=0x66, prefix_address_size=0x67,
    prefix_cs=0x2e, prefix_ss=0x36, prefix_ds=0x3e, prefix_es=0x26,
    prefix_fs=0x64, prefix_gs=0x65,
};

typedef void (*burn_func_t)();

static word_t cli_remain = ~0;
static word_t sti_remain = ~0;
static word_t popf_remain = ~0;
static word_t pushf_remain = ~0;
static word_t iret_remain = ~0;
static word_t pop_ds_remain = ~0;
static word_t pop_es_remain = ~0;
static word_t hlt_remain = ~0;
static word_t push_ds_remain = ~0;
static word_t push_es_remain = ~0;
static word_t push_fs_remain = ~0;
static word_t push_gs_remain = ~0;
static word_t read_seg_remain = ~0;
static word_t write_seg_remain = ~0;

static inline void update_remain( word_t &remain, u8_t *nop_start, u8_t *end )
{
    if( end >= nop_start ) {
	word_t diff = (word_t)end - (word_t)nop_start;
	remain = min( remain, diff );
    }
}


extern "C" void burn_wrmsr(void);
extern "C" void burn_rdmsr(void);
extern "C" void burn_interruptible_hlt(void);
extern "C" void burn_out(void);
extern "C" void burn_cpuid(void);
extern "C" void burn_in(void);
extern "C" void burn_int(void);
extern "C" void burn_iret(void);
extern "C" void burn_lret(void);
extern "C" void burn_idt(void);
extern "C" void burn_gdt(void);
extern "C" void burn_invlpg(void);
extern "C" void burn_lldt(void);
extern "C" void burn_ltr(void);
extern "C" void burn_str(void);
extern "C" void burn_clts(void);
extern "C" void burn_cli(void);
extern "C" void burn_sti(void);
extern "C" void burn_deliver_interrupt(void);
extern "C" void burn_popf(void);
extern "C" void burn_pushf(void);
extern "C" void burn_write_cr0(void);
extern "C" void burn_write_cr2(void);
extern "C" void burn_write_cr3(void);
extern "C" void burn_write_cr4(void);
extern "C" void burn_read_cr(void);
extern "C" void burn_write_dr(void);
extern "C" void burn_read_dr(void);
extern "C" void burn_ud2(void);
extern "C" void burn_unimplemented(void);

extern "C" void burn_write_cs(void);
extern "C" void burn_write_ds(void);
extern "C" void burn_write_es(void);
extern "C" void burn_write_fs(void);
extern "C" void burn_write_gs(void);
extern "C" void burn_write_ss(void);
extern "C" void burn_read_cs(void);
extern "C" void burn_read_ds(void);
extern "C" void burn_read_es(void);
extern "C" void burn_read_fs(void);
extern "C" void burn_read_gs(void);
extern "C" void burn_read_ss(void);
extern "C" void burn_lss(void);
extern "C" void burn_invd(void);
extern "C" void burn_wbinvd(void);
 
extern "C" void device_8259_0x20_in(void);
extern "C" void device_8259_0x21_in(void);
extern "C" void device_8259_0xa0_in(void);
extern "C" void device_8259_0xa1_in(void);
extern "C" void device_8259_0x20_out(void);
extern "C" void device_8259_0x21_out(void);
extern "C" void device_8259_0xa0_out(void);
extern "C" void device_8259_0xa1_out(void);

burn_func_t burn_write_cr[] = {
    burn_write_cr0, 0, burn_write_cr2, burn_write_cr3, burn_write_cr4,
};

#if 0
static u8_t *
and_eax_immediate( u8_t *newops, word_t immediate )
{
    newops[0] = 0x25;
    *(u32_t *)&newops[1] = immediate;
    return &newops[5];
}

static u8_t *
and_reg_offset_immediate( u8_t *newops, word_t reg, int offset, 
	word_t immediate )
{
    *newops = 0x81;	// AND r/m32, imm32
    newops++;

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 4;
    modrm.x.fields.rm = reg;

    if( (offset >= -128) && (offset <= 127) )
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp8;
    else
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp;

    *newops = modrm.x.raw;
    newops++;

    if( modrm.x.fields.rm == OP_REG_ESP ) {
	ia32_sib_t sib;
	sib.x.fields.base = OP_REG_ESP;
	sib.x.fields.index = 4;
	sib.x.fields.scale = 0;
	*newops = sib.x.raw;
	newops++;
    }

    if( modrm.x.fields.mod == ia32_modrm_t::mode_indirect_disp8 ) {
	*newops = (u8_t)offset;
	newops++;
    }
    else {
	*(u32_t *)newops = offset;
	newops += sizeof(u32_t);
    }

    *(u32_t *)newops = immediate;
    newops += sizeof(u32_t);
    return newops;
}

static u8_t *
or_reg_offset_reg( u8_t *newops, word_t src_reg, word_t dst_reg, int dst_offset )
{
    *newops = 0x09;	// Or into r/m32 with r32
    newops++;

    ia32_modrm_t modrm;
    modrm.x.fields.reg = src_reg;
    modrm.x.fields.rm = dst_reg;

    if( (dst_offset >= -128) && (dst_offset <= 127) )
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp8;
    else
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp;

    *newops = modrm.x.raw;
    newops++;

    if( modrm.x.fields.rm == OP_REG_ESP ) {
	ia32_sib_t sib;
	sib.x.fields.base = OP_REG_ESP;
	sib.x.fields.index = 4;
	sib.x.fields.scale = 0;
	*newops = sib.x.raw;
	newops++;
    }

    if( modrm.x.fields.mod == ia32_modrm_t::mode_indirect_disp8 ) {
	*newops = (u8_t)dst_offset;
	newops++;
    }
    else {
	*(u32_t *)newops = dst_offset;
	newops += sizeof(u32_t);
    }

    return newops;
}

static u8_t *
mov_regoffset_to_reg( u8_t *newops, word_t src_reg, int offset, 
	word_t dst_reg )
{
    newops[0] = 0x8b;	// Move r/m32 to r32

    ia32_modrm_t modrm;
    modrm.x.fields.reg = dst_reg;
    modrm.x.fields.rm = src_reg;

    if( (offset >= -128) && (offset <= 127) ) {
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp8;
	newops[1] = modrm.x.raw;
	newops[2] = offset;
	return &newops[3];
    }

    modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp;
    newops[1] = modrm.x.raw;
    *(u32_t *)&newops[2] = offset;
    return &newops[6];
}

static u8_t * 
btr_regoffset_immediate( u8_t *newops, word_t reg, int offset, u8_t immediate ) 
{
    newops[0] = 0x0f;
    newops[1] = 0xba;

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 6;
    modrm.x.fields.rm = reg;

    if( (offset >= -128) && (offset <= 127) ) {
	modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp8;
	newops[2] = modrm.x.raw;
	newops[3] = offset;
	newops[4] = immediate;
	return &newops[5];
    }

    modrm.x.fields.mod = ia32_modrm_t::mode_indirect_disp;
    newops[2] = modrm.x.raw;
    *(u32_t *)&newops[3] = offset;
    newops[7] = immediate;
    return &newops[8];
}
#endif


UNUSED static u8_t *
btr_mem32_immediate( u8_t *newops, word_t address, u8_t immediate )
{
    newops[0] = 0x0f;
    newops[1] = 0xba;

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 6; // Op-code specific.
    modrm.x.fields.rm = 5;  // Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[2] = modrm.x.raw;

    *(u32_t *)&newops[3] = address;
    newops[7] = immediate;

    return &newops[8];
}

UNUSED static u8_t *
bts_mem32_immediate( u8_t *newops, word_t address, u8_t immediate )
{
    newops[0] = 0x0f;
    newops[1] = 0xba;

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 5; // Op-code specific.
    modrm.x.fields.rm = 5;  // Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[2] = modrm.x.raw;

    *(u32_t *)&newops[3] = address;
    newops[7] = immediate;

    return &newops[8];
}

static u8_t *
push_mem32( u8_t *newops, word_t address )
{
    newops[0] = 0xff;	// Push r/m32

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 6; // Op-code specific.
    modrm.x.fields.rm = 5;  // Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[1] = modrm.x.raw;

    *(u32_t *)&newops[2] = address;

    stack_offset += sizeof(word_t);
    return &newops[6];
}

static u8_t *
pop_mem32( u8_t *newops, word_t address )
{
    newops[0] = 0x8f;	// Push r/m32

    ia32_modrm_t modrm;
    modrm.x.fields.reg = 0; // Op-code specific.
    modrm.x.fields.rm = 5;  // Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[1] = modrm.x.raw;

    *(u32_t *)&newops[2] = address;

    stack_offset -= sizeof(word_t);
    return &newops[6];
}

static u8_t *
mov_mem32_to_reg( u8_t *newops, word_t address, word_t reg )
{
    newops[0] = 0x8b;	// Move r/m32 to r32

    ia32_modrm_t modrm;
    modrm.x.fields.reg = reg;
    modrm.x.fields.rm = 5;	// Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[1] = modrm.x.raw;

    *(u32_t *)&newops[2] = address;

    return &newops[6];
}

static u8_t *
mov_imm32_to_reg( u8_t *newops, word_t imm32, word_t reg )
{
    newops[0] = 0xc7;	// Move imm32 to r/m32

    ia32_modrm_t modrm;
    modrm.x.fields.reg = reg;
    modrm.x.fields.mod = ia32_modrm_t::mode_reg;
    modrm.x.fields.rm = 0;	// Immediate
    newops[1] = modrm.x.raw;

    *(u32_t *)&newops[2] = imm32;

    return &newops[6];
}

static u8_t *
mov_reg_to_mem32( u8_t *newops, word_t reg, word_t address )
{
    newops[0] = 0x89;	// Move r32 to r/m32.

    ia32_modrm_t modrm;
    modrm.x.fields.reg = reg;
    modrm.x.fields.rm = 5;	// Displacement 32
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    newops[1] = modrm.x.raw;

    *(u32_t *)&newops[2] = address;

    return &newops[6];
}

static u8_t *
mov_reg_to_reg( u8_t *newops, word_t src_reg, word_t dst_reg )
{
    newops[0] = 0x89; // Move r32 to r/m32.

    ia32_modrm_t modrm;
    modrm.x.fields.reg = src_reg;
    modrm.x.fields.rm = dst_reg;
    modrm.x.fields.mod = ia32_modrm_t::mode_reg;
    newops[1] = modrm.x.raw;
    return &newops[2];
}

#if 0
static u8_t *
mov_segoffset_eax( u8_t *newops, prefix_e seg_prefix, u32_t offset )
{
    if( seg_prefix != prefix_ds ) {
	newops[0] = seg_prefix;
	newops++;
    }
    newops[0] = 0xa1;	// Move word at (seg:offset) to EAX
    *(u32_t *)&newops[1] = offset;

    return &newops[5];
}
#endif

static u8_t *
clean_stack( u8_t *opstream, s8_t bytes )
{
    ia32_modrm_t modrm;

    opstream[0] = 0x83;		// add imm8 to r/m32
    modrm.x.fields.reg = 0;	// opcode specific
    modrm.x.fields.mod = ia32_modrm_t::mode_reg;
    modrm.x.fields.rm = 4;	// %esp
    opstream[1] = modrm.x.raw;
    opstream[2] = bytes;

    return &opstream[3];
}

static u8_t *
push_byte(u8_t *opstream, u8_t value) 
{
    opstream[0] = OP_PUSH_IMM8;
    opstream[1] = value;
    stack_offset += sizeof(word_t);
    return &opstream[2];
}

static u8_t *
push_word(u8_t *opstream, u32_t value) 
{
    opstream[0] = OP_PUSH_IMM32;
    * ((u32_t*)&opstream[1]) = value;
    stack_offset += sizeof(word_t);
    return &opstream[5];
}

#if 0
static u8_t *
insert_operand_size_prefix( u8_t *newops )
{
    *newops = prefix_operand_size;
    newops++;
    return newops;
}
#endif

static u8_t *
append_modrm( u8_t *newops, ia32_modrm_t modrm, u8_t *suffixes )
/* If any of the modrm suffixes are relative to the stack, we have to adjust to
 * compensate for data that we have pushed to the stack ourselves (since the
 * compiler was unaware of the data that we'd push to the stack when it
 * generated the modrm).  The stack_offset holds the number of bytes that we
 * have pushed on the stack.
 */
{
    ia32_sib_t sib; bool esp_relative = false;

    if( modrm.get_mode() == ia32_modrm_t::mode_reg )
	return newops;  // No suffixes.

    // Write to the new instruction stream any SIB and displacement info.

    if( modrm.get_rm() == 4 ) { // SIB byte
	sib.x.raw = *suffixes;
	esp_relative = sib.get_base() == ia32_sib_t::base_esp;
	*newops = sib.x.raw;
	newops++;
	suffixes++;
    }

    if( modrm.get_mode() == ia32_modrm_t::mode_indirect ) {
	if( modrm.get_rm() == 5 ) {	// 32-bit displacement
	    *(u32_t *)newops = *(u32_t *)suffixes;
	    newops += 4;
	    suffixes += 4;
	}
	else if( (modrm.get_rm() == 4) && sib.is_no_base() ) {
	    // 32-bit displacement
	    *(u32_t *)newops = *(u32_t *)suffixes;
	    newops += 4;
	    suffixes += 4;
	}
    }
    else if( modrm.get_mode() == ia32_modrm_t::mode_indirect_disp8 ) {
	*newops = *suffixes;
	if( esp_relative )
	    *newops += stack_offset;
	newops++;
	suffixes++;
    }
    else if( modrm.get_mode() == ia32_modrm_t::mode_indirect_disp ) {
	*(u32_t *)newops = *(u32_t *)suffixes;
	if( esp_relative )
	    *(u32_t *)newops += stack_offset;
	newops += 4;
	suffixes += 4;
    }
    else
	PANIC( "Unhandled modrm format." );

    return newops;
}

static u8_t *
push_modrm( u8_t *newops, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0xff;		// The PUSH instruction.
    modrm.x.fields.reg = 6;	// The PUSH extended opcode.
    newops[1] = modrm.x.raw;

    newops += 2;
    stack_offset += sizeof(word_t);
    return append_modrm( newops, modrm, suffixes );
}

static u8_t *
pop_modrm( u8_t *newops, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0x8f;		// The POP instruction.
    modrm.x.fields.reg = 0;	// The POP extended opcode.
    newops[1] = modrm.x.raw;

    newops += 2;
    stack_offset -= sizeof(word_t);
    return append_modrm( newops, modrm, suffixes );
}

#if 0
static u8_t *
mov_to_modrm( u8_t *newops, u8_t src_reg, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0x89;			// MOV r/m,r
    modrm.x.fields.reg = src_reg;
    newops[1] = modrm.x.raw;

    newops += 2;
    return append_modrm( newops, modrm, suffixes );
}
#endif

static u8_t *
mov_from_modrm( u8_t *newops, u8_t dst_reg, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0x8b;			// MOV r, r/m
    modrm.x.fields.reg = dst_reg;
    newops[1] = modrm.x.raw;

    newops += 2;
    return append_modrm( newops, modrm, suffixes );
}


static u8_t *
lea_modrm( u8_t *newops, u8_t dst_reg, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0x8d;	// Create a lea instruction.
    modrm.x.fields.reg = dst_reg;
    newops[1] = modrm.x.raw;

    newops += 2;
    return append_modrm( newops, modrm, suffixes );
}

static u8_t *
movzx_modrm( u8_t *newops, u8_t dst_reg, ia32_modrm_t modrm, u8_t *suffixes )
{
    newops[0] = 0x0f;
    newops[1] = 0xb7;
    modrm.x.fields.reg = dst_reg;
    newops[2] = modrm.x.raw;

    newops += 3;
    return append_modrm( newops, modrm, suffixes );
}

#if defined(CONFIG_IA32_STRICT_IRQ) && !defined(CONFIG_IA32_STRICT_FLAGS)
static u8_t *
cmp_mem_imm8( u8_t *opstream, word_t mem_addr, u8_t imm8 )
{
    opstream[0] = 0x83;

    ia32_modrm_t modrm;
    modrm.x.fields.mod = ia32_modrm_t::mode_indirect;
    modrm.x.fields.rm = 5;	// disp32
    modrm.x.fields.reg = 7;	// CMP r/m32, imm8

    opstream[1] = modrm.x.raw;
    
    *((word_t *)&opstream[2]) = mem_addr;
    opstream[6] = imm8;

    return &opstream[7];
}
#endif

static u8_t *
push_reg(u8_t *opstream, u8_t regname) 
{
    opstream[0] = OP_PUSH_REG + regname;
    stack_offset += sizeof(word_t);
    return &opstream[1];
}

static u8_t *
pop_reg( u8_t *newops, u8_t regname )
{
    newops[0] = OP_POP_REG + regname;
    stack_offset -= sizeof(word_t);
    return &newops[1];
}

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_FLAGS)
static u8_t *
op_popf( u8_t *newops )
{
    newops[0] = OP_POPF;
    return &newops[1];
}
#endif
#if defined(CONFIG_IA32_STRICT_FLAGS)
static u8_t *
op_pushf( u8_t *newops )
{
    newops[0] = OP_PUSHF;
    return &newops[1];
}
#endif
static u8_t *
op_jmp(u8_t *opstream, void *func)
{
	opstream[0] = OP_JMP_REL32;
	* ((s32_t*)&opstream[1]) = ((s32_t) func - (((s32_t) &opstream[5] + (s32_t) curr_virt_addr)));
	return &opstream[5];
}

#if 0
static u8_t *
jmp_short_carry( u8_t *opstream, word_t target)
{
    u8_t *next = &opstream[2];
    opstream[0] = OP_JMP_NEAR_CARRY;
    opstream[1] = target - word_t(next);
    return next;
}
#endif

#if defined(CONFIG_IA32_STRICT_IRQ) && !defined(CONFIG_IA32_STRICT_FLAGS)
static u8_t *
jmp_short_zero( u8_t *opstream, word_t target)
{
    u8_t *next = &opstream[2];
    opstream[0] = OP_JMP_NEAR_ZERO;
    opstream[1] = target - word_t(next);
    return next;
}
#endif

static u8_t *
op_call(u8_t *opstream, void *func)
{
	opstream[0] = OP_CALL_REL32;
	* ((s32_t*)&opstream[1]) = ((s32_t) func - (((s32_t) &opstream[5] + (s32_t) curr_virt_addr)));
	return &opstream[5];
}

#if 0
static u8_t *
op_lcall(u8_t *opstream, void *func)
{
    opstream[0] = OP_LCALL_IMM;
    u16_t cs;
    __asm__ __volatile__ ("mov %%cs, %0" : "=r"(cs));
    *((u16_t *)&opstream[5]) = cs;
    *((word_t *)&opstream[1]) = word_t(func);
    return &opstream[7];
}
#endif


#if 0
static u8_t *
push_ip( u8_t *opstream, word_t next_instr )
{
    // Untested
    return push_mem32( opstream, next_instr + curr_virt_addr );
}
#endif

static u8_t *
set_reg(u8_t *opstream, int reg, u32_t value)
{
	opstream[0] = OP_MOV_IMM32 + reg;
	* ((u32_t*)&opstream[1]) = (u32_t) value;
	return &opstream[5];
}

static u8_t *
apply_port_in_patchup( u8_t *newops, u8_t port, bool &handled )
{
    void *which = NULL;
    stack_offset = 0;
    handled = true;
    switch( port ) {
	case 0x20: which = (void *)device_8259_0x20_in; break;
	case 0x21: which = (void *)device_8259_0x21_in; break;
	case 0xa0: which = (void *)device_8259_0xa0_in; break;
	case 0xa1: which = (void *)device_8259_0xa1_in; break;
	default:
		   handled = false;
		   return newops;
    }

    newops = push_reg( newops, OP_REG_ECX );
    newops = push_reg( newops, OP_REG_EDX );
    newops = op_call( newops, which );
    newops = pop_reg( newops, OP_REG_EDX );
    newops = pop_reg( newops, OP_REG_ECX );

    return newops;
}

static u8_t *
apply_port_out_patchup( u8_t *newops, u8_t port, bool &handled )
{
    void *which = NULL;
    stack_offset = 0;
    handled = true;
    switch( port ) {
	case 0x20: which = (void *)device_8259_0x20_out; break;
	case 0x21: which = (void *)device_8259_0x21_out; break;
	case 0xa0: which = (void *)device_8259_0xa0_out; break;
	case 0xa1: which = (void *)device_8259_0xa1_out; break;
	default:
		   handled = false;
		   return newops;
    }

    newops = push_reg( newops, OP_REG_ECX );
    newops = push_reg( newops, OP_REG_EDX );
    newops = op_call( newops, which );
    newops = pop_reg( newops, OP_REG_EDX );
    newops = pop_reg( newops, OP_REG_ECX );

    return newops;
}

static void
init_patchup()
{
#if defined(CONFIG_WEDGE_L4KA) || defined(CONFIG_WEDGE_XEN) || defined(CONFIG_WEDGE_KAXEN)
	curr_virt_addr = 0;
#endif
#if defined(CONFIG_WEDGE_LINUX)
	curr_virt_addr = -1 * get_vcpu().get_kernel_vaddr();
#endif
}

static bool
apply_patchup( u8_t *opstream, u8_t *opstream_end )
{
	bool handled;
	u32_t value;
	u32_t jmp_target;
	u16_t segment;
	ia32_modrm_t modrm;
	u8_t suffixes[6];

	stack_offset = 0;

	u8_t *newops = opstream;
	bool rep_prefix = false;
	bool repne_prefix = false;
	bool operand_size_prefix = false;
	bool address_size_prefix = false;
	while( 1 ) {

	/* Instruction decoding */
	switch(opstream[0]) {
	    case prefix_lock:
		*newops++ = OP_NOP1;
		opstream++;
		break;
	    case prefix_repne:
		con << "repne prefix not supported @ " << (void *)opstream << '\n';
		return false;
		repne_prefix = true;
		opstream++;
		continue;
	    case prefix_rep:
		con << "rep prefix not supported @ " << (void *)opstream << '\n';
		return false;
		rep_prefix = true;
		opstream++;
		continue;
	    case prefix_operand_size:
		operand_size_prefix = true;
		opstream++;
		continue;
	    case prefix_address_size:
		address_size_prefix = true;
		opstream++;
		continue;
	    case prefix_cs:
	    case prefix_ss:
	    case prefix_ds:
	    case prefix_es:
	    case prefix_fs:
	    case prefix_gs:
		con << "Ignored segment prefix (because we assume flat "
		       "segments) at " << (void *)opstream << '\n';
		opstream++;
		continue;

	    case OP_HLT:
		newops = op_call(newops, (void*) burn_interruptible_hlt);
		update_remain( hlt_remain, newops, opstream_end );
		break;
	    case OP_INT3:
#if defined(CONFIG_WEDGE_L4KA)
		// Permit int3, for the L4Ka::Pistachio kernel debugger.
		// But the kernel debugger uses a protocol based on the
		// instructions following the int3.  So move the int3 to
		// the end of the NOPs.
		opstream_end[-1] = opstream[0];
		opstream[0] = OP_NOP1;
		break;
#else
		opstream[1] = 3; // Cheat, and reuse the fall through code.
		// Fall through.
#endif
	    case OP_INT:
		value = opstream[1];
		newops = push_word( newops, value ); // Vector no.
		newops = op_call( newops, (void*) burn_int );
		// Note: the stack must be cleaned by burn_int.
		// 10 bytes of new instructions, so jump back 10+2 using a 
		// 2-byte jump.
		*newops++ = 0xeb;	// JMP rel8
		*newops++ = (u8_t)-12;	// JMP -12
		break;
	    case OP_IRET:
		newops = op_jmp(newops, (void*) burn_iret);
		update_remain( iret_remain, newops, opstream_end );
		break;
	    case OP_LRET:
		newops = op_jmp(newops, (void*) burn_lret);
		break;
    	    case OP_JMP_FAR:
		/* We make the hellish assumption that now we are no 
		 * longer in physical mode!  We assume that the last
		 * afterburned instruction prior to this enabled
		 * paging in cr0.  Since paging is already enabled,
		 * we need to use proper relative offsets, even for this instr.
		 */ 
#if defined(CONFIG_WEDGE_L4KA) || defined(CONFIG_WEDGE_KAXEN)
		curr_virt_addr = get_vcpu().get_kernel_vaddr();
#endif
		jmp_target = *((u32_t*)&opstream[1]);
		segment = *((u16_t*)&opstream[5]);
		newops = push_word( newops, segment );
		newops = push_word( newops, jmp_target ); // Return addr.
		newops = op_jmp( newops, (void *)burn_write_cs );
#if defined(CONFIG_WEDGE_XEN)
		curr_virt_addr = get_vcpu().get_kernel_vaddr();
#endif
#if defined(CONFIG_WEDGE_LINUX)
		curr_virt_addr = 0;
#endif
		break;
	    case OP_OUT:
		value = opstream[1]; // Port number, 8-bit immediate
		newops = apply_port_out_patchup( newops, value, handled );
		if( !handled ) {
		    if( operand_size_prefix )
			newops = push_byte(newops, 16);
		    else
			newops = push_byte(newops, 32);
		    newops = push_word(newops, value);
		    newops = op_call(newops, (void*) burn_out);
		    newops = clean_stack( newops, 8 );
		}
		break;
	    case OP_OUT_DX:
		if( operand_size_prefix )
		    newops = push_byte(newops, 16);
		else
		    newops = push_byte(newops, 32);
		newops = push_reg(newops, OP_REG_EDX);
		newops = op_call(newops, (void*) burn_out);
		newops = clean_stack( newops, 8 );
		break;
	    case OP_OUTB:
		value = opstream[1];
		newops = apply_port_out_patchup( newops, value, handled );
		if( !handled ) {
		    newops = push_byte(newops, 0x8);
		    newops = push_word(newops, value);
		    newops = op_call(newops, (void*) burn_out);
		    newops = clean_stack( newops, 8 );
		}
		break;
	    case OP_OUTB_DX:
		newops = push_byte(newops, 0x8);
		newops = push_reg(newops, OP_REG_EDX);
		newops = op_call(newops, (void*) burn_out);
		newops = clean_stack( newops, 8 );
		break;
	    case OP_IN:
		value = opstream[1];
		newops = apply_port_in_patchup( newops, value, handled );
		if( !handled ) {
		    if( operand_size_prefix )
	    		newops = push_byte(newops, 16);
    		    else
			newops = push_byte(newops, 32);
		    newops = push_word(newops, value);
		    newops = op_call(newops, (void*) burn_in);
		    newops = clean_stack( newops, 8 );
		}
		break;
	    case OP_IN_DX:
		if( operand_size_prefix )
		    newops = push_byte(newops, 16);
		else
		    newops = push_byte(newops, 32);
		newops = push_reg(newops, OP_REG_EDX);
		newops = op_call(newops, (void*) burn_in);
		newops = clean_stack( newops, 8 );
		break;
	    case OP_INB:
		value = opstream[1];
		newops = apply_port_in_patchup( newops, value, handled );
		if( !handled ) {
		    newops = push_byte(newops, 0x8);
		    newops = push_word(newops, value);
		    newops = op_call(newops, (void*) burn_in);
		    newops = clean_stack( newops, 8 );
		}
		break;
	    case OP_INB_DX:
		newops = push_byte(newops, 0x8);
		newops = push_reg(newops, OP_REG_EDX);
		newops = op_call(newops, (void*) burn_in);
		newops = clean_stack( newops, 8 );
		break;

	    case OP_PUSHF:
#if 0 && defined(CONFIG_WEDGE_L4KA)
		extern vcpu_t *VCPU;
		newops = op_pushf( newops ); // Push the unprivileged flags.
		newops = push_reg( newops, OP_REG_EAX );
		newops = mov_segoffset_eax( newops, prefix_gs, 0 );
		newops = mov_regoffset_to_reg( newops, OP_REG_EAX, -52, OP_REG_EAX );
		newops = mov_regoffset_to_reg( newops, OP_REG_EAX, 
			(word_t)&(VCPU->cpu.flags.x.raw) - (word_t)VCPU,
			OP_REG_EAX );
		newops = and_eax_immediate( newops, ~flags_user_mask );
		newops = and_reg_offset_immediate( newops, OP_REG_ESP, 4,
			flags_user_mask );
		newops = or_reg_offset_reg( newops, OP_REG_EAX, OP_REG_ESP, 4 );
		newops = pop_reg( newops, OP_REG_EAX );
#else
#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_FLAGS)
		newops = op_pushf( newops ); // Push the unprivileged flags.
		newops = op_call( newops, (void *)burn_pushf );
#else
		newops = push_mem32( newops, (word_t)&get_cpu().flags.x.raw );
		update_remain( pushf_remain, newops, opstream_end );
#endif
#endif
		update_remain( pushf_remain, newops, opstream_end );
		break;
	    case OP_POPF:
#if defined(CONFIG_IA32_STRICT_IRQ) && !defined(CONFIG_IA32_STRICT_FLAGS)
#if defined(CONFIG_SMP)
#error Broken for SMP!
#endif
		newops = pop_mem32( newops, (word_t)&get_cpu().flags.x.raw );
		newops = cmp_mem_imm8( newops, (word_t)&get_intlogic().vector_cluster, 0 );
		newops = jmp_short_zero( newops, word_t(opstream_end) );
		newops = op_call(newops, (void*) burn_deliver_interrupt);
#else
#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_FLAGS)
		newops = op_call( newops, (void *)burn_popf );
		newops = op_popf( newops ); // Pop the unprivileged flags.
#else
		newops = pop_mem32( newops, (word_t)&get_cpu().flags.x.raw );
		update_remain( popf_remain, newops, opstream_end );
#endif
#endif
		break;
		
	    case OP_MOV_TOSEG:
		if( hardware_segments )
		    break;
		modrm.x.raw = opstream[1];
		memcpy( suffixes, &opstream[2], sizeof(suffixes) );
		if( modrm.is_register_mode() ) {
		    switch( modrm.get_reg() ) {
			case 0:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().es );
			    break;
			case 1:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().cs );
			    break;
			case 2:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().ss );
			    break;
			case 3:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().ds );
			    break;
			case 4:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().fs );
			    break;
			case 5:
			    newops = mov_reg_to_mem32( newops, modrm.get_rm(),
				    (word_t)&get_cpu().gs );
			    break;
			default:
			    con << "unknown segment @ " << (void *)opstream << '\n';
			    return false;
		    }
		}
		else {
		    switch( modrm.get_reg() ) {
			case 0:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().es);
			    break;
			case 1:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().cs);
			    break;
			case 2:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().ss);
			    break;
			case 3:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().ds);
			    break;
			case 4:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().fs);
			    break;
			case 5:
			    newops = push_modrm( newops, modrm, suffixes );
			    newops = pop_mem32( newops, (word_t)&get_cpu().gs);
			    break;
			default:
			    con << "unknown segment @ " << (void *)opstream << '\n';
			    return false;

		    }
		}
#if 0
		modrm.x.raw = opstream[1];
		memcpy( suffixes, &opstream[2], sizeof(suffixes) );
		if( modrm.is_register_mode() )
		    newops = push_reg( newops, modrm.get_rm() );
		else {
		    // We can't push the modrm, because it is a 16-bit operand.
		    newops = push_reg( newops, OP_REG_EAX );  // Preserve
		    newops = insert_operand_size_prefix( newops );  // 16-bit
		    newops = mov_from_modrm( newops, OP_REG_EAX, modrm, suffixes );
		    newops = push_reg( newops, OP_REG_EAX );
		}
		switch( modrm.get_reg() ) {
		    case 0:
			newops = op_call( newops, (void *)burn_write_es );
			break;
		    case 1:
			newops = op_call( newops, (void *)burn_write_cs );
			break;
		    case 2:
			newops = op_call( newops, (void *)burn_write_ss );
			break;
		    case 3:
			newops = op_call( newops, (void *)burn_write_ds );
			break;
		    case 4:
			newops = op_call( newops, (void *)burn_write_fs );
			break;
		    case 5:
			newops = op_call( newops, (void *)burn_write_gs );
			break;
		    default:
			con << "unknown segment @ " << (void *)opstream << '\n';
			return false;
		}
		newops = clean_stack( newops, 4 );
		if( !modrm.is_register_mode() )
		    newops = pop_reg( newops, OP_REG_EAX );  // Restore
#endif
		update_remain( write_seg_remain, newops, opstream_end );
		break;
	    case OP_PUSH_CS:
		if( hardware_segments )
		    break;
		newops = push_mem32( newops, (word_t)&get_cpu().cs );
//		newops = push_reg( newops, OP_REG_EAX ); // Create stack space.
//		newops = op_call( newops, (void *)burn_read_cs );
		break;
	    case OP_PUSH_SS:
		if( hardware_segments )
		    break;
		newops = push_mem32( newops, (word_t)&get_cpu().ss );
//		newops = push_reg( newops, OP_REG_EAX ); // Create stack space.
//		newops = op_call( newops, (void *)burn_read_ss );
		break;
	    case OP_POP_SS:
		if( hardware_segments )
		    break;
		newops = pop_mem32( newops, (word_t)&get_cpu().ss );
//		newops = op_call( newops, (void *)burn_write_ss );
//		newops = clean_stack( newops, 4 );
		break;
	    case OP_PUSH_DS:
		if( hardware_segments )
		    break;
		newops = push_mem32( newops, (word_t)&get_cpu().ds );
//		newops = push_reg( newops, OP_REG_EAX ); // Create stack space.
//		newops = op_call( newops, (void *)burn_read_ds );
		update_remain( push_ds_remain, newops, opstream_end );
		break;
	    case OP_POP_DS:
		if( hardware_segments )
		    break;
		newops = pop_mem32( newops, (word_t)&get_cpu().ds );
//		newops = op_call( newops, (void *)burn_write_ds );
//		newops = clean_stack( newops, 4 );
		update_remain( pop_ds_remain, newops, opstream_end );
		break;
	    case OP_PUSH_ES:
		if( hardware_segments )
		    break;
		newops = push_mem32( newops, (word_t)&get_cpu().es );
//		newops = push_reg( newops, OP_REG_EAX ); // Create stack space.
//		newops = op_call( newops, (void *)burn_read_es );
		update_remain( push_es_remain, newops, opstream_end );
		break;
	    case OP_POP_ES:
		if( hardware_segments )
		    break;
		newops = pop_mem32( newops, (word_t)&get_cpu().es );
//		newops = op_call( newops, (void *)burn_write_es );
//		newops = clean_stack( newops, 4 );
		update_remain( pop_es_remain, newops, opstream_end );
		break;
	    case OP_MOV_FROMSEG:
		if( hardware_segments )
		    break;
		modrm.x.raw = opstream[1];
		memcpy( suffixes, &opstream[2], sizeof(suffixes) );
		if( modrm.is_register_mode() ) {
		    switch( modrm.get_reg() ) {
		       	case 0:
		    	    newops = mov_mem32_to_reg( newops, 
		    		    (word_t)&get_cpu().es, modrm.get_rm() );
		    	    break;
			case 1:
			    newops = mov_mem32_to_reg( newops, 
			    	    (word_t)&get_cpu().cs, modrm.get_rm() );
			    break;
			case 2:
			    newops = mov_mem32_to_reg( newops, 
				    (word_t)&get_cpu().ss, modrm.get_rm() );
			    break;
			case 3:
			    newops = mov_mem32_to_reg( newops, 
				    (word_t)&get_cpu().ds, modrm.get_rm() );
			    break;
			case 4:
			    newops = mov_mem32_to_reg( newops, 
				    (word_t)&get_cpu().fs, modrm.get_rm() );
			    break;
			case 5:
			    newops = mov_mem32_to_reg( newops, 
				    (word_t)&get_cpu().gs, modrm.get_rm() );
			    break;
			default:
			    con << "unknown segment @ " << (void *)opstream << '\n';
			    return false;
		    }
		}
		else {
		    switch( modrm.get_reg() ) {
			case 0:
			    newops = push_mem32( newops, (word_t)&get_cpu().es);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			case 1:
			    newops = push_mem32( newops, (word_t)&get_cpu().cs);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			case 2:
			    newops = push_mem32( newops, (word_t)&get_cpu().ss);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			case 3:
			    newops = push_mem32( newops, (word_t)&get_cpu().ds);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			case 4:
			    newops = push_mem32( newops, (word_t)&get_cpu().fs);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			case 5:
			    newops = push_mem32( newops, (word_t)&get_cpu().gs);
			    newops = pop_modrm( newops, modrm, suffixes );
			    break;
			default:
			    con << "unknown segment @ " << (void *)opstream << '\n';
			    return false;
		    }
		}
#if 0
		modrm.x.raw = opstream[1];
		memcpy( suffixes, &opstream[2], sizeof(suffixes) );
		if( !modrm.is_register_mode() )
		    newops = push_reg( newops, OP_REG_EAX ); // Preserve EAX
		newops = push_reg( newops, OP_REG_EAX ); // Create stack space.
		switch( modrm.get_reg() ) {
		    case 0:
			newops = op_call( newops, (void *)burn_read_es );
			break;
		    case 1:
			newops = op_call( newops, (void *)burn_read_cs );
			break;
		    case 2:
			newops = op_call( newops, (void *)burn_read_ss );
			break;
		    case 3:
			newops = op_call( newops, (void *)burn_read_ds );
			break;
		    case 4:
			newops = op_call( newops, (void *)burn_read_fs );
			break;
		    case 5:
			newops = op_call( newops, (void *)burn_read_gs );
			break;
		    default:
			con << "unknown segment @ " << (void *)opstream << '\n';
			return false;
		}
		if( modrm.is_register_mode() )
		    newops = pop_reg( newops, modrm.get_rm() );
		else {
		    // We can't pop the modrm, because it is a 16-bit operand.
		    newops = pop_reg( newops, OP_REG_EAX );	// Get result.
		    newops = insert_operand_size_prefix( newops );	// 16-bit
		    newops = mov_to_modrm( newops, OP_REG_EAX, modrm, suffixes );
		    newops = pop_reg( newops, OP_REG_EAX ); // Restore EAX.
		}
#endif
		update_remain( read_seg_remain, newops, opstream_end );
		break;
	    case OP_CLI:
#if 0 /*defined(CONFIG_WEDGE_XEN)*/
		newops = op_call(newops, (void*) burn_cli);
#else
#if 0 && defined(CONFIG_WEDGE_L4KA)
		extern vcpu_t *VCPU;
		newops = push_reg( newops, OP_REG_EAX );
		newops = mov_segoffset_eax( newops, prefix_gs, 0 );
		newops = mov_regoffset_to_reg( newops, OP_REG_EAX, -52, OP_REG_EAX );
		newops = btr_regoffset_immediate( newops, OP_REG_EAX,
			(word_t)&(VCPU->cpu.flags.x.raw) - (word_t)VCPU, 9 );
		newops = pop_reg( newops, OP_REG_EAX );
#else
#if defined(CONFIG_SMP)
#error Broken instruction rewrite for SMP
#endif
		newops = btr_mem32_immediate(newops, (word_t)&get_cpu().flags.x.raw, 9);
#endif
		update_remain( cli_remain, newops, opstream_end );
#endif
		break;
	    case OP_STI:
#if defined(CONFIG_IA32_STRICT_IRQ) && !defined(CONFIG_IA32_STRICT_FLAGS)
#if defined(CONFIG_SMP)
#error Broken for SMP!
#endif
		newops = bts_mem32_immediate( newops, (word_t)&get_cpu().flags.x.raw, 9 );
		newops = cmp_mem_imm8( newops, (word_t)&get_intlogic().vector_cluster, 0 );
		newops = jmp_short_zero( newops, word_t(opstream_end) );
		newops = op_call(newops, (void*) burn_deliver_interrupt);
#else
#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ)
		newops = op_call(newops, (void*) burn_sti);
#else
		newops = bts_mem32_immediate(newops, (word_t)&get_cpu().flags.x.raw, 9);
		update_remain( sti_remain, newops, opstream_end );
#endif
#endif
		break;
	    case OP_2BYTE:
		switch(opstream[1]) {
		    case OP_WBINVD:
			newops = op_call( newops, (void *)burn_wbinvd );
			break;
		    case OP_INVD:
			newops = op_call( newops, (void *)burn_invd );
			break;
		    case OP_UD2:
			newops = op_call( newops, (void *)burn_ud2 );
			break;
		    case OP_PUSH_FS:
			if( hardware_segments )
			    break;
			newops = push_mem32( newops, (word_t)&get_cpu().fs );
//			newops = push_reg( newops, OP_REG_EAX );
//			newops = op_call( newops, (void *)burn_read_fs );
			update_remain( push_fs_remain, newops, opstream_end );
			break;
		    case OP_POP_FS:
			if( hardware_segments )
			    break;
			newops = pop_mem32( newops, (word_t)&get_cpu().fs );
//			newops = op_call( newops, (void *)burn_write_fs );
//			newops = clean_stack( newops, 4 );
			break;
		    case OP_PUSH_GS:
			if( hardware_segments )
			    break;
			newops = push_mem32( newops, (word_t)&get_cpu().gs );
//			newops = push_reg( newops, OP_REG_EAX );
//			newops = op_call( newops, (void *)burn_read_gs );
			update_remain( push_gs_remain, newops, opstream_end );
			break;
		    case OP_POP_GS:
			if( hardware_segments )
			    break;
			newops = pop_mem32( newops, (word_t)&get_cpu().gs );
//			newops = op_call( newops, (void *)burn_write_gs );
//			newops = clean_stack( newops, 4 );
			break;
		    case OP_CPUID:
			newops = op_call( newops, (void *)burn_cpuid );
			break;
	    	    case OP_LSS:
			// TODO: delay preemption during this operation.
			modrm.x.raw = opstream[2];
			memcpy( suffixes, &opstream[3], sizeof(suffixes) );
			newops = push_reg( newops, OP_REG_EAX ); // Preserve
			newops = lea_modrm( newops, OP_REG_EAX, modrm, 
				suffixes );
			newops = op_call( newops, (void *)burn_lss );
			newops = pop_reg( newops, OP_REG_EAX ); // Restore
			newops = mov_from_modrm( newops, OP_REG_ESP, modrm,
				suffixes );
			break;
		    case OP_MOV_FROMCREG:
			// The modrm reg field is the control register.
			// The modrm r/m field is the general purpose register.
			modrm.x.raw = opstream[2];
			memcpy( suffixes, &opstream[3], sizeof(suffixes) );
			if( modrm.is_register_mode() ) {
			    switch( modrm.get_reg() ) {
				case 0:
				    newops = mov_mem32_to_reg( newops, 
					    (word_t)&get_cpu().cr0, modrm.get_rm() );
				    break;
				case 2:
				    newops = mov_mem32_to_reg( newops, 
					    (word_t)&get_cpu().cr2, modrm.get_rm() );
				    break;
				case 3:
				    newops = mov_mem32_to_reg( newops, 
					    (word_t)&get_cpu().cr3, modrm.get_rm() );
				    break;
				case 4:
				    newops = mov_mem32_to_reg( newops, 
					    (word_t)&get_cpu().cr4, modrm.get_rm() );
				    break;
				default:
				    con << "unknown CR @ " << (void *)opstream << '\n';
				    return false;
			    }
			}
			else {
			    switch( modrm.get_reg() ) {
				case 0:
				    newops = push_mem32( newops, (word_t)&get_cpu().cr0);
				    newops = pop_modrm( newops, modrm, suffixes );
				    break;
				case 2:
				    newops = push_mem32( newops, (word_t)&get_cpu().cr2);
				    newops = pop_modrm( newops, modrm, suffixes );
				    break;
				case 3:
				    newops = push_mem32( newops, (word_t)&get_cpu().cr3);
				    newops = pop_modrm( newops, modrm, suffixes );
				    break;
				case 4:
				    newops = push_mem32( newops, (word_t)&get_cpu().cr4);
				    newops = pop_modrm( newops, modrm, suffixes );
				    break;
				default:
				    con << "unknown CR @ " << (void *)opstream << '\n';
				    return false;
			    }
			}
			break;
		    case OP_MOV_FROMDREG:
			// The modrm reg field is the debug register.
			// The modrm r/m field is the general purpose register.
			modrm.x.raw = opstream[2];
			ASSERT( modrm.is_register_mode() );
			newops = push_reg( newops, OP_REG_EAX ); // Allocate
			newops = push_byte( newops, modrm.get_reg() );
			newops = op_call( newops, (void *)burn_read_dr );
			newops = clean_stack( newops, 4 );
			newops = pop_reg( newops, modrm.get_rm() );
			break;
		    case OP_MOV_TOCREG:
			// The modrm reg field is the control register.
			// The modrm r/m field is the general purpose register.
			modrm.x.raw = opstream[2];
			ASSERT( modrm.is_register_mode() );
			ASSERT( (modrm.get_reg() != 1) && 
				(modrm.get_reg() <= 4) );
			newops = push_reg( newops, modrm.get_rm() );
			newops = op_call( newops, (void *)burn_write_cr[modrm.get_reg()] );
			newops = clean_stack( newops, 4 );
			break;
		    case OP_MOV_TODREG:
			// The modrm reg field is the debug register.
			// The modrm r/m field is the general purpose register.
			modrm.x.raw = opstream[2];
			ASSERT( modrm.is_register_mode() );
			newops = push_reg( newops, modrm.get_rm() );
			newops = push_byte( newops, modrm.get_reg() );
			newops = op_call( newops, (void *)burn_write_dr );
			newops = clean_stack( newops, 8 );
			break;
		    case OP_CLTS:
			newops = op_call(newops, (void*) burn_clts);
			break;
		    case OP_LLTL:
			modrm.x.raw = opstream[2]; 
			memcpy( suffixes, &opstream[3], sizeof(suffixes) );
			newops = push_reg( newops, OP_REG_EAX );
			newops = movzx_modrm( newops, 
				OP_REG_EAX, modrm, suffixes );
			if( modrm.get_opcode() == 3 )
			    newops = op_call(newops, (void*) burn_ltr);
			else if( modrm.get_opcode() == 2 )
			    newops = op_call(newops, (void*) burn_lldt);
			else if( modrm.get_opcode() == 1 )
			    newops = op_call(newops, (void*) burn_str);
			else {
			    con << "Unknown lltl sub type @ " << (void *)opstream << '\n';
			    return false;
			}
			newops = pop_reg( newops, OP_REG_EAX );
			break;
		    case OP_LDTL:
			modrm.x.raw = opstream[2]; 
			memcpy( suffixes, &opstream[3], sizeof(suffixes) );
			newops = push_reg( newops, OP_REG_EAX );  // Preserve
			newops = lea_modrm( newops, 
				OP_REG_EAX, modrm, suffixes );
			if( modrm.get_opcode() == 2 ) // lgdt
			    newops = op_call( newops, (void *)burn_gdt );
			else if( modrm.get_opcode() == 3 ) // lidt
			    newops = op_call( newops, (void *)burn_idt );
			else if( modrm.get_opcode() == 7 ) // invlpg
			    newops = op_call( newops, (void*)burn_invlpg);
			else if( modrm.get_opcode() == 0 ) // sgdt
			    newops = op_call(newops, (void*)burn_unimplemented);
			else if( modrm.get_opcode() == 4 ) // smsw
			    newops = op_call(newops, (void*)burn_unimplemented);
			else if( modrm.get_opcode() == 6 ) // lmsw
			    newops = op_call(newops, (void*)burn_unimplemented);
			else {
			    con << "Uknown opcode extension for ldtl @ " << (void *)opstream << '\n';
			    return false;
			}
			newops = pop_reg( newops, OP_REG_EAX );  // Restore
			break;
		    case OP_WRMSR:
			newops = op_call(newops, (void*) burn_wrmsr);
			break;
		    case OP_RDMSR:
			newops = op_call(newops, (void*) burn_rdmsr);
			break;
		    default:
			con << "Unknown 2 byte opcode @ " << (void *)opstream << '\n';
			return false;
		}
		break;
	    default:
		con << "Unknown opcode " << (void*) (u32_t) opstream[0] 
		    << " at " << opstream << "\n";
		return false;
	}

	break;
	}

	if( newops > opstream_end ) {
	    con << "Insufficient space for the afterburner instructions, at "
		   << (void *)opstream << ", need " << (newops - opstream_end)
		   << " bytes." << '\n';
	    return false;
	}

	return true;
}

bool
arch_apply_patchups( patchup_info_t *patchups, word_t total, word_t vaddr_offset )
{
    init_patchup();
    word_t back_to_back = 0;
    for( word_t i = 0; i < total; i++ )
    {
	bool good = apply_patchup( (u8_t *)(patchups[i].start - vaddr_offset),
		(u8_t *)(patchups[i].end - vaddr_offset));
	if( !good )
	    return false;

	if( i && (patchups[i].start == patchups[i-1].end) ) {
//	    con << "contiguous: " << (void *)patchups[i-1].start << ' ' 
//		<< (void *)patchups[i].start << '\n';
	    back_to_back++;
	}
    }

    if( debug_nop_space ) {
       	con << "contiguous patch-ups: " << back_to_back << '\n';
	con << "cli space " << cli_remain << '\n';
	con << "sti space " << sti_remain << '\n';
	con << "popf space " << popf_remain << '\n';
	con << "pushf space " << pushf_remain << '\n';
	con << "iret space " << iret_remain << '\n';
	con << "write ds space " << pop_ds_remain << '\n';
	con << "write es space " << pop_es_remain << '\n';
	con << "hlt space " << hlt_remain << '\n';
	con << "read ds space " << push_ds_remain << '\n';
	con << "read es space " << push_es_remain << '\n';
	con << "read fs space " << push_fs_remain << '\n';
	con << "read gs space " << push_gs_remain << '\n';
	con << "read seg space " << read_seg_remain << '\n';
	con << "write seg space " << write_seg_remain << '\n';
    }

    con << "Total patchups: " << total << '\n';
    return true;
}

static bool
apply_device_patchup( u8_t *opstream, u8_t *opstream_end, 
	word_t read_func, word_t write_func, word_t xchg_func,
	word_t or_func, word_t and_func )
{
    u8_t *newops = opstream;
    ia32_modrm_t modrm;
    u8_t suffixes[6];

    stack_offset = 0;

    switch( opstream[0] )
    {
	case 0x09: // OR r32, r/m32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );
    	    // Preserve according to the C calling conventions.
	    newops = push_reg( newops, OP_REG_EAX );
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    // We want the source in eax, and the target in edx.  The
	    // target is the device memory address.
	    if( modrm.get_reg() == OP_REG_EAX )
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    else if( modrm.get_reg() != OP_REG_EDX ) {
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, modrm.get_reg(), OP_REG_EAX );
	    }
	    else if( modrm.get_rm() == 0 ) {
	     	// The source is in EDX, and the target is in EAX.  Store the 
		// src temporarily in ECX.
    		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_ECX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, OP_REG_ECX, OP_REG_EAX );
	    }
	    else {
		// The source is in EDX, but the target is not in EAX.
		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_EAX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    }
	    // Call the write function.
	    ASSERT( or_func );
	    newops = op_call( newops, (void *)or_func );
	    // Restore the clobbered registers.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    newops = pop_reg( newops, OP_REG_EAX );
	    break;
	case 0x21: // AND r32, r/m32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );
    	    // Preserve according to the C calling conventions.
	    newops = push_reg( newops, OP_REG_EAX );
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    // We want the source in eax, and the target in edx.  The
	    // target is the device memory address.
	    if( modrm.get_reg() == OP_REG_EAX )
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    else if( modrm.get_reg() != OP_REG_EDX ) {
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, modrm.get_reg(), OP_REG_EAX );
	    }
	    else if( modrm.get_rm() == 0 ) {
	     	// The source is in EDX, and the target is in EAX.  Store the 
		// src temporarily in ECX.
    		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_ECX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, OP_REG_ECX, OP_REG_EAX );
	    }
	    else {
		// The source is in EDX, but the target is not in EAX.
		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_EAX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    }
	    // Call the write function.
	    ASSERT( and_func );
	    newops = op_call( newops, (void *)and_func );
	    // Restore the clobbered registers.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    newops = pop_reg( newops, OP_REG_EAX );
	    break;
	case 0x89: // Move r32 to r/m32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );

	    // Preserve according to the C calling conventions.
	    newops = push_reg( newops, OP_REG_EAX );
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    // We want the source in eax, and the target in edx.  The
	    // target is the device memory address.
	    if( modrm.get_reg() == OP_REG_EAX )
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    else if( modrm.get_reg() != OP_REG_EDX ) {
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, modrm.get_reg(), OP_REG_EAX );
	    }
	    else if( modrm.get_rm() == 0 ) {
	     	// The source is in EDX, and the target is in EAX.  Store the 
		// src temporarily in ECX.
    		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_ECX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, OP_REG_ECX, OP_REG_EAX );
	    }
	    else {
		// The source is in EDX, but the target is not in EAX.
		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_EAX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    }
	    // Call the write function.
	    ASSERT( write_func );
	    newops = op_call( newops, (void *)write_func );
	    // Restore the clobbered registers.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    newops = pop_reg( newops, OP_REG_EAX );
	    break;
	case 0xa3: // Move EAX to moffs32
	    memcpy( suffixes, &opstream[1], sizeof(suffixes) );
	
	    // Preserve according to the C calling conventions.
	    newops = push_reg( newops, OP_REG_EAX );
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    
	    // We want the source in eax, and the target in edx.  The
	    // target is the device memory address.
	    newops = set_reg ( newops, OP_REG_EDX, * (u32_t *) suffixes );
	    
	    // Call the write function.
	    ASSERT( write_func );
	    newops = op_call( newops, (void *)write_func );
	    // Restore the clobbered registers.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    newops = pop_reg( newops, OP_REG_EAX );
	    
	    break;
	case 0xa1: // MOV moffs32 to EAX
	    // Convert the MOV instruction into a load immediate.
	    newops = mov_imm32_to_reg( newops, *(u32_t *)&newops[1] /*moffs32*/,
		    OP_REG_EAX );
	    // Preserve.
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    // Execute the read function.
	    newops = set_reg ( newops, OP_REG_EAX, * (u32_t *) suffixes );
	    ASSERT( read_func );
	    newops = op_call( newops, (void *)read_func );
	    // Restore.  The return value is in EAX, as desired.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    break;
	case 0x8b: // Move r/m32 to r32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );
	    // Preserve according to the C calling conventions.
	    if( modrm.get_reg() != OP_REG_EAX )
		newops = push_reg( newops, OP_REG_EAX );
	    if( modrm.get_reg() != OP_REG_ECX )
		newops = push_reg( newops, OP_REG_ECX );
	    if( modrm.get_reg() != OP_REG_EDX )
		newops = push_reg( newops, OP_REG_EDX );
	    // Build a pointer to the device memory.
	    newops = lea_modrm( newops, OP_REG_EAX, modrm, suffixes );
	    // Execute the read function.
	    ASSERT( read_func );
	    newops = op_call( newops, (void *)read_func );
	    // Return value is in EAX.  Restore the preserved registers
	    // without clobbering the return value.
	    if( modrm.get_reg() == OP_REG_EDX )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, OP_REG_EDX );
	    else
		newops = pop_reg( newops, OP_REG_EDX );
	    if( modrm.get_reg() == OP_REG_ECX )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, OP_REG_ECX );
	    else
		newops = pop_reg( newops, OP_REG_ECX );
	    if( (modrm.get_reg() != OP_REG_EAX) && (modrm.get_reg() != OP_REG_ECX) && (modrm.get_reg() != OP_REG_EDX) )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, modrm.get_reg() );
	    if( modrm.get_reg() != OP_REG_EAX )
		newops = pop_reg( newops, OP_REG_EAX );
	    break;
	case 0x87: // XCHG r32, r/m32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );
	    if( modrm.get_reg() != OP_REG_EAX )
		newops = push_reg( newops, OP_REG_EAX );
	    if( modrm.get_reg() != OP_REG_ECX )
		newops = push_reg( newops, OP_REG_ECX );
	    if( modrm.get_reg() != OP_REG_EDX )
		newops = push_reg( newops, OP_REG_EDX );
	    // We want the source in eax, and the target in edx.  The
	    // target is the device memory address.
	    if( modrm.get_reg() == OP_REG_EAX )
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    else if( modrm.get_reg() != OP_REG_EDX ) {
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, modrm.get_reg(), OP_REG_EAX );
	    }
	    else if( modrm.get_rm() == 0 ) {
	     	// The source is in EDX, and the target is in EAX.  Store the 
		// src temporarily in ECX.
    		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_ECX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
		newops = mov_reg_to_reg( newops, OP_REG_ECX, OP_REG_EAX );
	    }
	    else {
		// The source is in EDX, but the target is not in EAX.
		newops = mov_reg_to_reg( newops, OP_REG_EDX, OP_REG_EAX );
		newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    }
	    // Execute the read function.
	    ASSERT( xchg_func );
	    newops = op_call( newops, (void *)xchg_func );
	    // Return value is in EAX.  Restore the preserved registers
	    // without clobbering the return value.
	    if( modrm.get_reg() == OP_REG_EDX )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, OP_REG_EDX );
	    else
		newops = pop_reg( newops, OP_REG_EDX );
	    if( modrm.get_reg() == OP_REG_ECX )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, OP_REG_ECX );
	    else
		newops = pop_reg( newops, OP_REG_ECX );
	    if( (modrm.get_reg() != OP_REG_EAX) && (modrm.get_reg() != OP_REG_ECX) && (modrm.get_reg() != OP_REG_EDX) )
		newops = mov_reg_to_reg( newops, OP_REG_EAX, modrm.get_reg() );
	    if( modrm.get_reg() != OP_REG_EAX )
		newops = pop_reg( newops, OP_REG_EAX );
	    break;

	default:
	    con << "Unsupported device patchup at " << (void *)opstream << '\n';
	    return false;
    }

    if( newops > opstream_end ) {
	con << "Insufficient space for the afterburner instructions, at "
    	    << (void *)opstream << ", need " << (newops - opstream_end)
	    << " bytes." << '\n';
	return false;
    }
    return true;
}

bool
arch_apply_device_patchups( patchup_info_t *patchups, word_t total, 
	word_t vaddr_offset, word_t read_func, word_t write_func, 
	word_t xchg_func, word_t or_func, word_t and_func )
{
    for( word_t i = 0; i < total; i++ )
	apply_device_patchup( (u8_t *)(patchups[i].start - vaddr_offset),
		(u8_t *)(patchups[i].end - vaddr_offset), 
		read_func, write_func, xchg_func, or_func, and_func );

    con << "Total device patchups: " << total << '\n';
    return true;
}

static bool
apply_bitop_patchup( u8_t *opstream, u8_t *opstream_end, 
	word_t clear_bit_func, word_t set_bit_func )
{
    u8_t *newops = opstream;
    ia32_modrm_t modrm;
    u8_t suffixes[6];

    ASSERT( !set_bit_func );
    stack_offset = 0;

    // Bitop instructions are two-byte
    if( opstream[0] != 0x0f ) {
	con << "Unsupported instruction rewrite at " << (void *)opstream << '\n';
	return false;
    }
    opstream++;

    // Note: we assume that the bit is specified in %eax, and that
    // the value of the CF flag is to be left in %eax.

    switch( opstream[0] ) {
	case 0xb3: // BTR r32, r/m32
	    modrm.x.raw = opstream[1];
	    memcpy( suffixes, &opstream[2], sizeof(suffixes) );
	    ASSERT( modrm.get_reg() == OP_REG_EAX );
	    // Preserve ECX and EDX.
	    newops = push_reg( newops, OP_REG_ECX );
	    newops = push_reg( newops, OP_REG_EDX );
	    // Build a pointer to the device memory.
	    newops = lea_modrm( newops, OP_REG_EDX, modrm, suffixes );
	    // Execute the read function.
	    ASSERT( clear_bit_func );
	    newops = op_call( newops, (void *)clear_bit_func );
	    // Restore ECX and EDX.
	    newops = pop_reg( newops, OP_REG_EDX );
	    newops = pop_reg( newops, OP_REG_ECX );
	    break;
	default:
	    con << "Unsupported bitop patchup at " << (void *)opstream << '\n';
	    return false;
    }

    if( newops > opstream_end ) {
	con << "Insufficient space for the afterburner instructions, at "
    	    << (void *)opstream << ", need " << (newops - opstream_end)
	    << " bytes." << '\n';
	return false;
    }
    return true;
}

bool
arch_apply_bitop_patchups( patchup_info_t *patchups, word_t total, 
	word_t vaddr_offset, word_t clear_bit_func, word_t set_bit_func )
{
    for( word_t i = 0; i < total; i++ )
	apply_bitop_patchup( (u8_t *)(patchups[i].start - vaddr_offset),
		(u8_t *)(patchups[i].end - vaddr_offset), 
		clear_bit_func, set_bit_func );

    con << "Total bitop patchups: " << total << '\n';
    return true;
}


