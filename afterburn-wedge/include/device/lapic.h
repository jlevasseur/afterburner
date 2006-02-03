/*********************************************************************
 *                
 * Copyright (C) 2005,  Karlsruhe University
 *                
 * File path:     device/lapic.h
 * Description:   
 *                
 * @LICENSE@
 *                
 * $Id: lapic.h,v 1.2 2005/12/23 15:22:23 stoess Exp $
 *                
 ********************************************************************/
#ifndef __DEVICE__LAPIC_H__
#define __DEVICE__LAPIC_H__

#include INC_ARCH(bitops.h)
#include INC_ARCH(types.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(debug.h)

const char lapic_virt_magic[8] = { 'V', 'L', 'A', 'P', 'I', 'C', 'V', '0' };
const bool debug_lapic = false;
const bool debug_lapic_write = false;

extern const char *lapic_delmode[8];

class i82093_t;

enum lapic_regno_t {
    APIC_ID			=0x020,
    APIC_VERSION		=0x030,
    APIC_TASK_PRIO		=0x080,
    APIC_ARBITR_PRIO		=0x090,
    APIC_PROC_PRIO		=0x0A0,
    APIC_EOI			=0x0B0,
    APIC_LOGICAL_DEST		=0x0D0,
    APIC_DEST_FORMAT		=0x0E0,
    APIC_SVR			=0x0F0,
    APIC_ISR_BASE		=0x100,
    APIC_TMR_BASE		=0x180,
    APIC_IRR_BASE		=0x200,
    APIC_ERR_STATUS		=0x280,
    APIC_INTR_CMDLO		=0x300,
    APIC_INTR_CMDHI		=0x310,
    APIC_LVT_TIMER		=0x320,
    APIC_LVT_THERMAL		=0x330,
    APIC_LVT_PMC		=0x340,
    APIC_LVT_LINT0		=0x350,
    APIC_LVT_LINT1		=0x360,
    APIC_LVT_ERROR		=0x370,
    APIC_TIMER_COUNT		=0x380,
    APIC_TIMER_CURRENT		=0x390,
    APIC_TIMER_DIVIDE		=0x3E0
};



typedef union {
    u32_t raw;
    struct {
	u32_t    : 24;
	u32_t id :  8;
    } x;
} lapic_id_reg_t;

typedef union {
    u32_t raw;
    struct {
	u32_t version  : 8;
	u32_t          : 8;
	u32_t max_lvt  : 8;
	u32_t          : 8;
    } x;
}  lapic_version_reg_t;

typedef union {
    u32_t raw;
    struct {
	u32_t raw;
	u32_t subprio		: 4;
	u32_t prio		: 4;
	u32_t reserved		: 24;
    } x;
} lapic_prio_reg_t;


typedef union {
    u32_t raw;
    struct {
	u32_t    vec	   :  8;
	u32_t		   :  4;
	u32_t    del	   :  1;
	u32_t		   :  3;
	u32_t    msk	   :  1;
	u32_t    mod	   :  1;
	u32_t		   : 14;
    } x;
} lapic_lvt_timer_t;

    
typedef union {
    u32_t raw;
    struct {
	u32_t    vec	   :  8;
	u32_t    dlm	   :  3;
	u32_t		   :  1;
	u32_t    dls	   :  1;
	u32_t    pol	   :  1;
	u32_t    irr	   :  1;
	u32_t    trg	   :  1;
	u32_t    msk	   :  1;
	u32_t		   : 15;
    } x;
} lapic_lvt_t;


typedef union {
    u32_t raw;
    struct {
	u32_t		: 28;
	u32_t model	: 4;
    } x;
} lapic_dest_format_reg_t;


typedef union {
    u32_t raw;
    struct {
	u32_t vector		: 8;
	u32_t enabled		: 1;
	u32_t focus_processor	: 1;
	u32_t reserved		: 22;
    } x;
} lapic_svr_reg_t;


typedef union {
    u32_t raw;
    struct {
	u32_t snd_csum_err		:  1;
	u32_t rcv_csum_err		:  1;
	u32_t snd_acc_err		:  1;
	u32_t rcv_acc_err		:  1;
	u32_t				:  1;
	u32_t snd_ill_err		:  1;
	u32_t rcv_ill_err		:  1;
	u32_t ill_reg_err		:  1;
	u32_t				: 24;
    } x;
} lapic_esr_reg_t;


typedef union {
	u32_t raw;
	struct {
	    u32_t	div0 :   1;
	    u32_t	div1 :   1;
	    u32_t	     :   1;
	    u32_t	div2 :   1;
	    u32_t	     :  28;
	} x;
} lapic_divide_reg_t;


typedef union {
    u32_t raw;
    struct {
	u32_t vector			: 8;
	u32_t delivery_mode		: 3;
	u32_t destination_mode		: 1;
	u32_t delivery_status		: 1;
	u32_t				: 1;
	u32_t level			: 1;
	u32_t trigger_mode		: 1;
	u32_t				: 2;
	u32_t dest_shorthand		: 2;
	u32_t				: 12;
    } x;
}  lapic_icrlo_reg_t;


typedef union {
    u32_t raw;
    struct {
	u32_t			: 24;
	u32_t dest_shorthand	:  8;
    } x;
} lapic_icrhi_reg_t;



typedef union {
    u32_t raw[2];
    struct {
	i82093_t *ioapic;
	u32_t pin;
    } x;
} lapic_vector_to_ioapic_pin_t;


class local_apic_t {

private:

    enum delmode {
	LAPIC_FIXED		=0x0,
	LAPIC_LOWESTPRIO	=0x1,
	LAPIC_SMI		=0x2,
	LAPIC_RES0		=0x3,
	LAPIC_NMI		=0x4,
	LAPIC_INIT		=0x5,
	LAPIC_STARTUP		=0x6,
	LAPIC_RES1		=0x7
    };
    

    union {
	u32_t raw[1024];
	struct {
	    u8_t				virt_magic[8];	/*  0x00 ..  0x08 */
	    u32_t				enabled;	/*  0x08 ..  0x0c */
	    u32_t				res0[5];	/*  0x0c ..  0x20 */
	    lapic_id_reg_t			id;		/*  0x20 ..  0x24 */
	    u32_t				res1[3];	/*  0x24 ..  0x30 */
	    lapic_version_reg_t			ver;		/*  0x30 ..  0x34 */
	    u32_t				res2[19];	/*  0x34 ..  0x80 */
	    lapic_prio_reg_t			tpr;		/*  0x80 ..  0x84 */
	    u32_t				res3[3];	/*  0x84 ..  0x90 */
	    lapic_prio_reg_t			apr;		/*  0x90 ..  0x94 */
	    u32_t				res4[3];	/*  0x94 ..  0xA0 */
	    lapic_prio_reg_t			ppr;		/*  0xA0 ..  0xA4 */
	    u32_t				res5[3];	/*  0xA4 ..  0xB0 */
	    u32_t				eoi;		/*  0xB0 ..  0xB4 */
	    u32_t				res6[3];	/*  0xB0 ..  0xC0 */
	    u32_t				res7[4];	/*  0xC0 ..  0xD0 */
	    lapic_id_reg_t			ldest;		/*  0xD0 ..  0xD4 */
	    u32_t				res8[3];	/*  0xD4 ..  0xE0 */
	    lapic_dest_format_reg_t		dfr;		/*  0xE0 ..  0xE4 */
	    u32_t				res9[3];	/*  0xE4 ..  0xF0 */
	    lapic_svr_reg_t	   		svr;		/*  0xF0 ..  0xF4 */
	    u32_t		   		res10[3];	/*  0xF4 .. 0x100 */
	    u32_t		   		isr[32];	/* 0x100 .. 0x180 */
	    u32_t		   		tmr[32];	/* 0x180 .. 0x200 */
	    u32_t		   		irr[32];	/* 0x200 .. 0x280 */
	    lapic_esr_reg_t	   		esr;		/* 0x280 .. 0x284 */
	    u32_t		   		res11[31];	/* 0x284 .. 0x300 */
	    lapic_icrlo_reg_t	   		icrlo;		/* 0x300 .. 0x304 */
	    u32_t		   		res12[3];	/* 0x304 .. 0x310 */
	    lapic_icrhi_reg_t	   		icrhi;		/* 0x310 .. 0x314 */
	    u32_t		  		res13[3];	/* 0x314 .. 0x320 */
	    lapic_lvt_timer_t	   		timer;		/* 0x320 .. 0x324 */
	    u32_t		   		res14[3];	/* 0x324 .. 0x330 */
	    lapic_lvt_t		   		thermal;	/* 0x330 .. 0x334 */
	    u32_t		   		res15[3];	/* 0x334 .. 0x340 */
	    lapic_lvt_t		   		pmc;		/* 0x340 .. 0x314 */
	    u32_t		   		res16[3];	/* 0x344 .. 0x350 */
	    lapic_lvt_t				lint0;		/* 0x350 .. 0x354 */
	    u32_t				res17[3];	/* 0x354 .. 0x360 */
	    lapic_lvt_t				lint1;		/* 0x360 .. 0x364 */
	    u32_t				res18[3];	/* 0x364 .. 0x370 */
	    lapic_lvt_t				error;		/* 0x370 .. 0x374 */
	    u32_t				res19[3];	/* 0x374 .. 0x380 */
	    u32_t				init_count;	/* 0x380 .. 0x384 */
	    u32_t				res20[3];	/* 0x384 .. 0x390 */
	    u32_t				curr_count;	/* 0x390 .. 0x394 */
	    u32_t				res21[19];	/* 0x394 .. 0x3e0 */
	    lapic_divide_reg_t			div_conf;	/* 0x3e0 .. 0x3e4 */
	    u32_t				res22[7];	/* 0x3e4 .. 0x400 */
	    lapic_vector_to_ioapic_pin_t	v_to_pin[256];  /* 0x400 .. 0xc00 */
	} fields __attribute__((packed));
	
    };

    
public:

    /*
     * Selectors for word-granularity access 
     */
    typedef enum{
	lapic_rr_isr	=(0x100),
	lapic_rr_tmr    =(0x180),
	lapic_rr_irr	=(0x200),
    } rr_sel_t;


    /*
     * ISR, IRR, TMR: vector x can be found at register
     *   base + ((vector / 32) * 16),  at bit location (vector % 32)
     */

    static const word_t so_to_raw(const rr_sel_t rr_sel, const word_t offset)
	{ return ((word_t) rr_sel + (word_t) offset) >> 2; }
    static const word_t sb_to_raw(const rr_sel_t rr_sel, const word_t bit)
	{ return ( (rr_sel + ( (bit >> 1) & ~0xf)) >> 2); }
    word_t msb_rr_cluster(const rr_sel_t rr_sel, const word_t cluster)  
	{ 
	    // 0 .. 15 is reserved anyways, so use 0 for failure
	    if (cluster == 0)
		return 0;
	    /* 
	     * Each bit in cluster represents 8 vectors
	     */
	    word_t cluster_bit = (msb(cluster) << 3) & ~0x1f;
	    word_t cluster_raw = raw[sb_to_raw(rr_sel, cluster_bit)];
	    
	    return (cluster_raw ? (cluster_bit + msb(cluster_raw)) : 0);
	    
	}

    word_t msb_rr(const rr_sel_t rr_sel)
	{
	    // 0 .. 15 is reserved anyways, so use 0 for failure
	    return
		(raw[so_to_raw(rr_sel,0x70)]) ? msb(raw[so_to_raw(rr_sel,0x70)]) +  0xe0 :
		(raw[so_to_raw(rr_sel,0x60)]) ? msb(raw[so_to_raw(rr_sel,0x60)]) +  0xc0 :
		(raw[so_to_raw(rr_sel,0x50)]) ? msb(raw[so_to_raw(rr_sel,0x50)]) +  0xa0 :
		(raw[so_to_raw(rr_sel,0x40)]) ? msb(raw[so_to_raw(rr_sel,0x40)]) +  0x80 :
		(raw[so_to_raw(rr_sel,0x30)]) ? msb(raw[so_to_raw(rr_sel,0x30)]) +  0x60 :
		(raw[so_to_raw(rr_sel,0x20)]) ? msb(raw[so_to_raw(rr_sel,0x20)]) +  0x40 :
		(raw[so_to_raw(rr_sel,0x10)]) ? msb(raw[so_to_raw(rr_sel,0x10)]) +  0x20 :
		(raw[so_to_raw(rr_sel,   0)]) ? msb(raw[rr_sel		    ]) : 0;
	    
	}

 
    void bit_set_atomic_rr(const word_t bit, const rr_sel_t rr_sel)  
	{ bit_set_atomic(bit & 0x1F, raw[sb_to_raw(rr_sel,bit)]); }
    void bit_clear_atomic_rr(const word_t bit, const rr_sel_t rr_sel)  
	{ bit_clear_atomic(bit & 0x1F, raw[sb_to_raw(rr_sel,bit)]); }
    bool bit_test_and_set_atomic_rr(const word_t bit, const rr_sel_t rr_sel)  
	{ return bit_test_and_set_atomic(bit & 0x1f, raw[sb_to_raw(rr_sel,bit)]); }
    bool bit_test_and_clear_atomic_rr(const word_t bit, const rr_sel_t rr_sel)  
	{ return bit_test_and_clear_atomic(bit & 0x1f, raw[sb_to_raw(rr_sel,bit)]); }


    local_apic_t()
	{

	    /* Zero out */
	    for (int i = 0; i < 1024; i++)
		raw[i] = 0;


	    /* Set magic */
	    for (int i = 0; i < 8; i++)
		fields.virt_magic[i] = lapic_virt_magic[i]; 

	    /* Disable */
	    fields.enabled = false;

	    /* Set version register (max_lvt = 5, version = 0x1X) */
	    fields.ver.x.version = 0x10;
	    fields.ver.x.max_lvt = 0x5;

	    /* Set destination format register (flat) */
	    fields.dfr.raw = 0xFFFFFFFF;

	    /* Set spurious vector register (apic off, sv = 0xFF) */
	    fields.svr.raw = 0x1FF;

	    /* Set lvt entries (trigger = 1) and timer (mask = 1) */
	    fields.lint0.raw   = 0x00010000;
	    fields.lint1.raw   = 0x00010000;
	    fields.error.raw   = 0x00010000;
	    fields.pmc.raw     = 0x00010000;
	    fields.thermal.raw = 0x00010000;
	    fields.timer.raw   = 0x00010000;
	    
	}
    
    bool is_valid_lapic()
	{
	    for (int i = 0; i < 8; i++)
		if (fields.virt_magic[i] != lapic_virt_magic[i])
		    return false;
	    return true;
	}

    bool is_enabled() { return fields.enabled; }
    bool is_pic_virtually_wired() 
	{ return (fields.lint0.x.dlm == 7 && fields.lint0.x.msk == 0); }

    word_t get_id() { return fields.id.x.id;};
    void set_id(int id) { fields.id.x.id = id;};

    word_t get_logical_dest() { return fields.ldest.x.id; };


    word_t get_pin(word_t vector) 
	{ ASSERT(vector < 256); return fields.v_to_pin[vector].x.pin; };
    void set_pin(word_t vector, word_t pin) 
	{ ASSERT(vector < 256); fields.v_to_pin[vector].x.pin = pin; };
    
    i82093_t *get_ioapic(word_t vector)
	{ ASSERT(vector < 256); return fields.v_to_pin[vector].x.ioapic; };
    void set_ioapic(word_t vector, i82093_t *ioapic) 
	{ ASSERT(vector < 256); fields.v_to_pin[vector].x.ioapic = ioapic; };

    static word_t addr_to_reg(word_t addr) 
	{ return (addr & 0xFFF); }

    void write(word_t value, word_t reg);

    void raise_vector(word_t vector, word_t irq, i82093_t *from, bool from_eoi, bool reraise);
    void raise_timer_irq(bool reraise)
	{ 
	    if (fields.timer.x.msk == 0)
		raise_vector(fields.timer.x.vec, 0, 0, false, reraise); 
	}
    bool pending_vector( word_t &irq, word_t &vector);
};
        
extern "C" void __attribute__((regparm(2))) lapic_write_patch( word_t value, word_t addr );
extern "C" word_t __attribute__((regparm(1))) lapic_read_patch( word_t addr );
extern "C" word_t __attribute__((regparm(2))) lapic_xchg_patch( word_t value, word_t addr );




#endif /* !__DEVICE__LAPIC_H__ */
