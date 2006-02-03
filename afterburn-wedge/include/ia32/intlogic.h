/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/ia32/intlogic.h
 * Description:   The IA32 interrupt logic model.
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
 * $Id: intlogic.h,v 1.15 2006-01-11 17:53:24 stoess Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__IA32__INTLOGIC_H__
#define __AFTERBURN_WEDGE__INCLUDE__IA32__INTLOGIC_H__

/*
 * Systems can have multiple interrupt controllers.  The interrupt 
 * routing is hidden behind the abstract interrupt logic type.
 *
 * The interrupt logic exposes three interfaces categories:
 * 1. support for devices to raise interrupts.
 * 2. support for the front-end and back-end to query for pending 
 *    interrupt requests.
 * 3. support for the interrupt controller state machine, for 
 *    configuring, masking, acking, eoi, etc.
 *
 * Devices call get_intlogic()->raise_irq(irq) to raise an interrupt
 * request.  This operates on the irq name space.
 *
 * The wedge calls get_intlogic()->pending_vector(&vector) to retrieve
 * the next pending vector.  The vector belongs to the CPU's vector name
 * space.  The interrupt logic is responsible for translating irq names
 * into vector names.
 *
 * Legacy interrupts:
 * 0 - internal timer, Counter 0 output.
 * 1 - keyboard
 * 2 - slave controller INTR output
 * 3 - serial port A
 * 4 - serial port B
 * 5 - parallel port
 * 6 - floppy disk
 * 7 - parallel port
 * 8 - internal RTC
 * 9 - 
 * 10 -
 * 11 -
 * 12 - PS/2 mouse
 * 13 - 
 * 14 - primary IDE
 * 15 - secondary IDE
 */

#define OFS_INTLOGIC_VECTOR_CLUSTER	0

#define INTLOGIC_TIMER_IRQ	  0
#define INTLOGIC_MAX_HWIRQS	256

#if !defined(ASSEMBLY)

#include INC_ARCH(types.h)
#include INC_WEDGE(tlocal.h)
#include <device/i8259a.h>

#if defined(CONFIG_DEVICE_APIC)
#include <device/lapic.h>
#include <device/i82093.h>
#endif

class intlogic_t 
{
public:
    word_t vector_cluster;		// Clusters LAPIC ISR and PIC irq_request
private:
#if defined(CONFIG_DEVICE_PASSTHRU)
    volatile word_t hwirq_latch;	// Real device interrupts in use.
    volatile word_t hwirq_mask;	        // Real device interrupts mask
    word_t hwirq_squash;		// Squashed device interrupts
    word_t irq_trace;			// Tracing of irqs
#endif

#if defined(CONFIG_DEVICE_APIC)
    i82093_t *pin_to_ioapic[INTLOGIC_MAX_HWIRQS];
public:
    i82093_t __attribute__((aligned(4096))) ioapic[CONFIG_MAX_IOAPICS];
#endif


public:
    i8259a_t master;
    i8259a_t slave;

	    
 
#if defined(CONFIG_DEVICE_PASSTHRU)
    intlogic_t()
	{
	    hwirq_latch = 0;
	    
	    //device IRQ masked=0
	    hwirq_mask = 0;

	    // Don't mix virtual IRQs with device IRQs.  Hard coded to squash
	    // IRQs 3/4=serial port
	    hwirq_squash = ( (1<<4) | (1<<3) );

#if !defined(CONFIG_WEDGE_XEN)
	    // add IRQs 0=timer
	    hwirq_squash |= ( (1<<0) );
#endif

#if defined(CONFIG_WEDGE_L4KA)
	    hwirq_squash |= ( (1<<9) );
#endif
	    
	    // add IRQs 2=cascade, APIC mode disables that if necessary
	    hwirq_squash |= ( (1<<2) );
	    
#if defined(CONFIG_DEVICE_APIC)
	    for (word_t i=0; i<INTLOGIC_MAX_HWIRQS; i++)
		pin_to_ioapic[i] = NULL;
#endif
	    
	    vector_cluster = 0;
	    
	    irq_trace = 0;

	}

    void set_hwirq_latch(const word_t hwirq)
	{ bit_set_atomic(hwirq, hwirq_latch); }
    bool test_and_set_hwirq_latch(word_t hwirq)
	{ return bit_test_and_set_atomic(hwirq, hwirq_latch);} 
    word_t get_hwirq_latch()
	{ return hwirq_latch;} 

    void set_hwirq_mask(const word_t hwirq)
	{ bit_set_atomic(hwirq, hwirq_mask); }
    void clear_hwirq_mask(const word_t hwirq)
	{ bit_clear_atomic(hwirq, hwirq_mask); }
    bool test_and_clear_hwirq_mask(word_t hwirq)
	{ return bit_test_and_clear_atomic(hwirq, hwirq_mask);} 
    word_t get_hwirq_mask()
	{ return hwirq_mask;}

    void clear_hwirq_squash(word_t hwirq)
	{ hwirq_squash &= ~(1 << hwirq); }
    bool is_hwirq_squashed(word_t hwirq)
	{ return (hwirq_squash & (1<<hwirq));} 
    word_t get_hwirq_squash()
	{ return hwirq_squash; }

    bool is_irq_traced(word_t irq)
	{ return (irq_trace & (1<<irq));} 

    
#endif /* CONFIG_DEVICE_PASSTHRU */	   

    /*
     * We compress pending APIC and PIC vectors in that cluster
     * each bit represents 8 vectors. 
     */
    void set_vector_cluster(word_t vector)
	{ ASSERT(vector < 256); bit_set_atomic((vector >> 3), vector_cluster); }
    void clear_vector_cluster(word_t vector) 
	{ ASSERT(vector < 256); bit_clear_atomic((vector >> 3), vector_cluster); }
    word_t get_vector_cluster(bool pic=false) 
	{ return ((pic == true) ? (vector_cluster & 0x3) : (vector_cluster & ~0x3)); }
    
    void raise_irq ( word_t irq, bool reraise = false )
	{
	    bool pic_deliver = true;
	    if (is_irq_traced(irq))
		con << "INTLOGIC: IRQ" << irq << "\n";
	    
#if defined(CONFIG_DEVICE_APIC)
	    local_apic_t *la = get_cpu().get_lapic();
	    ASSERT(la);
	    if (EXPECT_TRUE(la->is_enabled()))
	    {
		if (EXPECT_TRUE(!la->is_pic_virtually_wired()))
		    pic_deliver = false;

		i82093_t *ia;
		if (irq == INTLOGIC_TIMER_IRQ)
		{
		    la->raise_timer_irq(reraise);
		    
#if !defined(CONFIG_WEDGE_L4KA) 
		    /* 
		     * jsXXX: this assumes a virtual PIC and an IRQ override
		     * IRQ0->IRQ2 in APIC mode (see also device/portio.cc)
		     */
		    if ((ia = hwirq_get_ioapic(2)) != NULL)
			ia->raise_irq(2, reraise);
#endif

		}
		else if ((ia = hwirq_get_ioapic(irq)) != NULL)
		    ia->raise_irq(irq, reraise);
		else
		{
		    con << "No IOAPIC for " << irq << "\n";
		    DEBUGGER_ENTER(0);
		}
	    }
#endif	
	    if (pic_deliver)
	    {
		if( irq < 8)
		    master.raise_irq(irq, 0);
		else if (irq < 16)
		    slave.raise_irq(irq, 8);
	    }
	}
    
    bool maybe_pending_vector()
	{ return vector_cluster != 0; }

    bool pending_vector( word_t &vector, word_t &irq )
	{
	    
	    if (vector_cluster)
	    {
#if defined(CONFIG_DEVICE_APIC)
		    local_apic_t *la = get_cpu().get_lapic();
		    ASSERT(la);
		    if (la->is_enabled() && la->pending_vector(vector, irq))
			return true;
#endif	    
		if( master.irq_request && master.pending_vector(vector, irq, 0) || 
		    slave.irq_request && slave.pending_vector(vector, irq, 8))
			return true;
	    }
	    return false;
	}
    
#if defined(CONFIG_DEVICE_APIC)    	    
    void hwirq_register_ioapic(word_t hwirq, i82093_t *i)
	{
	    ASSERT(hwirq < INTLOGIC_MAX_HWIRQS);
	    ASSERT(i->is_valid_ioapic());
	    pin_to_ioapic[hwirq] = i;
	}
    i82093_t *hwirq_get_ioapic(word_t hwirq)
	{
	    ASSERT(hwirq < INTLOGIC_MAX_HWIRQS);
	    i82093_t *ret = pin_to_ioapic[hwirq];
	    ASSERT(ret == NULL || ret->is_valid_ioapic());
	    return ret;
	}
#endif
    
    
    void raise_synchronous_irq( word_t irq )
	{
	    raise_irq(irq);
	    deliver_synchronous_irq();
	}
 
    bool deliver_synchronous_irq();

};

INLINE intlogic_t & get_intlogic()
{
    extern intlogic_t intlogic;
    return intlogic;
}

#endif	/* !ASSEMBLY */

#endif	/* __AFTERBURN_WEDGE__INCLUDE__IA32__INTLOGIC_H__ */
