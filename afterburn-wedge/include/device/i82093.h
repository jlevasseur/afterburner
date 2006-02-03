/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/device/i82093
 * Description:   Front-end emulation for the IO APIC 82093.
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
 * $Id: i82093.h,v 1.1 2005/12/21 08:27:51 stoess Exp $
 *
 ********************************************************************/
#ifndef __DEVICE__I82093_H__
#define __DEVICE__I82093_H__

#define IOAPIC_MAX_REDIR_ENTRIES	64

const bool debug_ioapic = false;
const bool debug_ioapic_passthru = false;
const bool debug_ioapic_regsel = false;
const bool debug_ioapic_rdtbl = false;

extern const char ioapic_virt_magic[8];
extern const char *ioapic_delmode[8];

typedef union {
    u32_t raw;
    struct {
	u32_t      : 24;
	u32_t id   :  4;
	u32_t      :  4;
    } x;
} i82093_id_reg_t;

typedef union
{
    u32_t raw;
    struct {
	u32_t ver  :  8;
	u32_t      :  8;
	u32_t mre  :  8;
	u32_t      :  8;
    } x;
} i82093_version_reg_t;
    
typedef union 
{
    u32_t raw;
    struct {
	u32_t ver  :  24;
	u32_t id   :   4;
	u32_t      :   4;
    }x;
} i82093_arb_reg_t;

typedef union{
    u32_t raw[2];
    struct {
	u32_t  vec   :  8;
	u32_t  del   :  3;
	u32_t  dstm  :  1;
	u32_t  dls   :  1;
	u32_t  pol   :  1;
	u32_t  irr   :  1;
	u32_t  trg   :  1;
	u32_t  msk   :  1;
	u32_t  pnd   :  1;
	u32_t        : 14;
	union {
	    struct {
		u32_t      : 24;
		u32_t pdst :  4;
		u32_t      :  4;
	    } __attribute__((packed)) phys;
	    struct {
		u32_t      : 24;
		u32_t ldst :  8;
	    } __attribute__((packed)) log;
	} dest;
    } x;
    
} i82093_redtbl_t;

class i82093_t 
{
public:
    enum mm_regno_t {
	IOAPIC_SEL	       =0x0,
	IOAPIC_WIN	       =0x10
    };
    
    enum io_regno_t {
	IOAPIC_ID	       =0x0,
	IOAPIC_VER	       =0x1,
	IOAPIC_ARB	       =0x2,
	IOAPIC_RDTBL_0_0       =0x10,
	IOAPIC_RDTBL_23_1      =0x3F
    };
    
    enum delmode {
	IOAPIC_FIXED		=0x0,
	IOAPIC_LOWESTPRIO	=0x1,
	IOAPIC_SMI		=0x2,
	IOAPIC_RES0		=0x3,
	IOAPIC_NMI		=0x4,
	IOAPIC_INIT		=0x5,
	IOAPIC_RES1		=0x6,
	IOAPIC_EXTINT		=0x7
    };


    union {
	u8_t raw[4096];
	struct {
	    struct {
		u32_t regsel;		/*  0x00 ..  0x04 */
		u32_t res0[3];		/*  0x04 ..  0x10 */
		u32_t regwin;		/*  0x10 ..  0x14 */
		u32_t res1[3];		/*  0x10 ..  0x20 */
	    } mm_regs;
	    
	    u8_t			virt_magic[8];	/*  0x00 ..  0x08 */
	    u32_t			irq_base;	/*  0x08 ..  0x0c */
	    u32_t			pad;		/*  0x0c ..  0x10 */
	    
	    union {
		u32_t raw[16 + (2 * IOAPIC_MAX_REDIR_ENTRIES)];
		struct {
		    i82093_id_reg_t		id;		/* 0x00 .. 0x04 */
		    i82093_version_reg_t	ver;		/* 0x04 .. 0x08 */
		    i82093_arb_reg_t		arb;		/* 0x08 .. 0x0c */
		    u32_t			pad[13];	/* 0x0c .. 0x40 */ 
		    i82093_redtbl_t		redtbl[IOAPIC_MAX_REDIR_ENTRIES]; /* 0x040 .. 0x240 */
		} x;
	    } io_regs; 
	} fields __attribute__((packed));
    };

    i82093_t()
    {
	/* Zero out */
	for (word_t i = 0; i < 1024; i++)
	    raw[i] = 0;


	/* Set magic */
	for (word_t i = 0; i < 8; i++)
	    fields.virt_magic[i] = ioapic_virt_magic[i]; 
	    
	/* Set version register (version = 0x1X, mre = 0x17) */
	fields.io_regs.x.ver.x.ver = 0x13;
	fields.io_regs.x.ver.x.mre = 0x17;
	    
    }	    
	    
    static i82093_t *addr_to_ioapic(const word_t addr) 
    { return (i82093_t *) (addr & 0xFFFFFF00); }

    static word_t addr_to_reg(word_t addr) 
    { return (addr & 0xFF); }
    
    void set_id(word_t i) 
    { fields.io_regs.x.id.x.id = i; }

    word_t get_id() 
    { return fields.io_regs.x.id.x.id; }

    void set_max_redir_entry(word_t m) 
    { fields.io_regs.x.ver.x.mre = m; }

    word_t get_max_redir_entry() 
    { return fields.io_regs.x.ver.x.mre; }

    void set_base(word_t i) 
    { fields.irq_base = i; }

    word_t get_base() 
    { return fields.irq_base; }

    bool is_valid_ioapic() { return (get_id() != 0xf); }
    
    bool is_valid_virtual_ioapic()
    {
	for (word_t i = 0; i < 8; i++)
	    if (fields.virt_magic[i] != ioapic_virt_magic[i])
		return false;
	return true;
    }
    void eoi(word_t hwirq);
    
    void raise_irq (word_t irq, bool reraise);
    void write(word_t value, word_t reg);
};
extern "C" void __attribute__((regparm(2))) ioapic_write_patch( word_t value, word_t addr );
extern "C" word_t __attribute__((regparm(1))) ioapic_read_patch( word_t addr );

#endif /* !__DEVICE__I82093_H__ */
