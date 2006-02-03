/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/device/i82093.h
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
 * $Id: lapic.cc,v 1.3 2006-01-11 17:42:18 stoess Exp $
 *
 ********************************************************************/
#include INC_ARCH(bitops.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(tlocal.h)
#include <device/lapic.h>

const char *lapic_delmode[8] =
{ "fixed", "reserved", "SMI", "reserved", 
  "NMI", "INIT", "reserved", "ExtINT" };


extern "C" void __attribute__((regparm(2))) lapic_write_patch( word_t value, word_t addr )
{
    if( debug_lapic )
    {
	word_t dummy, paddr = 0;
	pgent_t *pgent = backend_resolve_addr(addr, dummy);
	
	if( !pgent || !pgent->is_kernel() )
	{
	    con << "Inconsistent guest pagetable entries for local APIC mapping @ "
		<< (void *) addr << ", pgent " << (void *) pgent << "\n"; 
	    panic();
	}

	local_apic_t *lapic = (local_apic_t *) (addr & PAGE_MASK);
	
	if (!lapic->is_valid_lapic())
	    con << "BUG no sane softLAPIC @" << (void *) lapic 
		<< ", phys " << (void *) pgent->get_address() 
		<< ", pgent " << (void *) pgent << "\n"; 
	
	paddr = pgent->get_address() + (addr & ~PAGE_MASK);

	//con << "LAPIC  write  @ " << (void *)addr << " (" << (void *) paddr << "), ip "
	//<<  __builtin_return_address(0) << ", value  " << (void *) value << "\n" ;
	
    }
    
    
    local_apic_t *lapic = (local_apic_t *) (addr & PAGE_MASK);
    lapic->write(value, lapic->addr_to_reg(addr));
    //L4_KDB_Enter("LAPIC write");
}

extern "C" word_t __attribute__((regparm(1))) lapic_read_patch( word_t addr )
{
#warning js: reading TPR and TIMER must be computed!!!
    
    if ( (addr & PAGE_MASK) == APIC_TASK_PRIO)
    {
	con << "LAPIC reading TPR UNIMPLEMENTED -- must be computed!";
	DEBUGGER_ENTER(0);
    }
    if ( (addr & PAGE_MASK) == APIC_TIMER_COUNT)
    {
	con << "LAPIC reading TIMER COUNT UNIMPLEMENTED -- must be computed!";
	DEBUGGER_ENTER(0);
    }

    word_t ret = * (word_t *) addr;
    
   
    if( debug_lapic )
    {
	word_t dummy, paddr = 0;
	pgent_t *pgent = backend_resolve_addr(addr, dummy);
	
	if( !pgent || !pgent->is_kernel() )
	{
	    con << "Inconsistent guest pagetable entries for local APIC mapping @" 
		<< (void *) addr << ", pgent " << (void *) pgent << "\n"; 
	    panic();
	}

	local_apic_t *lapic = (local_apic_t *) (addr & PAGE_MASK);

	if (!lapic->is_valid_lapic())
	    con << "BUG no sane softLAPIC @" << (void *) lapic 
		<< ", phys " << (void *) pgent->get_address() 
		<< ", pgent " << (void *) pgent << "\n"; 
	
	paddr = pgent->get_address() + (addr & ~PAGE_MASK);
        
	if (debug_lapic)
	    con << "LAPIC  read   @ " << (void *)addr << " (" << (void *) paddr << "), ip "
		<<  __builtin_return_address(0) << ", return " << (void *) ret << "\n" ;
	
    }
    
    return ret;

}

extern "C" word_t __attribute__((regparm(2))) lapic_xchg_patch( word_t value, word_t addr )
{
    // TODO: confirm that the address maps to our bus address.  And determine
    // the dp83820 device from the address.
    if( debug_lapic )
	con << "LAPIC write @ " << (void *)addr 
	    << ", value " << (void *)value 
	    << ", ip " << __builtin_return_address(0) << '\n';
    
    return 0;

}

bool local_apic_t::pending_vector( word_t &vector, word_t &irq)
{
    intlogic_t &intlogic = get_intlogic();
    cpu_t &cpu = get_cpu();
    word_t int_save = cpu.disable_interrupts();

#warning jsXXX: reintroduce apic clustering logic for msb(irr)
    //word_t vector_cluster = intlogic.get_vector_cluster(false);
   
    /* 
     * Find highest bit in IRR and ISR
     */
    
    vector = msb_rr(lapic_rr_irr);
    irq = get_pin(vector);
    
    word_t vector_in_service = msb_rr(lapic_rr_isr);
    word_t ret;
    
    if ((vector >> 4) > (vector_in_service >> 4) && vector > 15)
    {    
	/*
	 * Shift to ISR
	 */
	bit_clear_atomic_rr(vector, lapic_rr_irr);
	bit_set_atomic_rr(vector, lapic_rr_isr);
	

	if(intlogic.is_irq_traced(irq))
	{
	    i82093_t *from = get_ioapic(vector);
	    con << "LAPIC: found pending vector in IRR " << vector
		<< "(ISR " << vector_in_service << ")";	    
	    if (from) 
		con << " IRQ " << irq
		    << " IO-APIC " << from->get_id();
	    con << "\n";
	}
	//con << "P" << vector << "\n";
	ret = true;
	goto out;
    }
    ret = false;
    
out:
    static void *last_addr;
    static word_t last_vector;
    if (vector == vector_in_service && vector)
    {
	con << "ISR " << vector_in_service 
	    << " this " << __builtin_return_address(0)
	    << " last " << last_addr 
	    << " vector " << last_vector 
	    << "\n";
	//L4_KDB_Enter("ISR bit set");
    }

    last_addr = __builtin_return_address(0);
    last_vector = vector;

    cpu.restore_interrupts( int_save, false );

    return ret;

}

void local_apic_t::raise_vector(word_t vector, word_t irq, i82093_t *from, bool from_eoi, bool reraise)
{
    if (vector > 15)
    {
	cpu_t &cpu = get_cpu();
	word_t int_save = cpu.disable_interrupts();

	if (reraise)
	{
	    // clear ISR/TMR bits raised in previous pending_vector
	    bit_clear_atomic_rr(vector, lapic_rr_isr);
	    bit_clear_atomic_rr(vector, lapic_rr_tmr);
	}
	
	bit_set_atomic_rr(vector, lapic_rr_irr);
	get_intlogic().set_vector_cluster(vector);

	if (from_eoi)
	{
	    set_ioapic(vector, from);
	    bit_set_atomic_rr(vector, lapic_rr_tmr);
	}
	set_pin(vector, irq);
	
	cpu.restore_interrupts( int_save, false );

	if(get_intlogic().is_irq_traced(irq))
	    con << "LAPIC raise vector " << vector 
		<< " IRQ " << irq 
		<< " from IOAPIC " << (from ? from->get_id(): 0) 
		<< " from_eoi " << from_eoi
		<< " reraise " << reraise
		<< "\n";

    }
#if 0
    if (debug_lapic) con << "LAPIC strange IRQ request with vector " 
			 << vector << " (<15) " << " IRQ " << irq << "\n";
#endif
}


void local_apic_t::write(word_t value, word_t reg)
{
    switch (reg) {
	
	case APIC_VERSION:
	{
	    if(debug_lapic_write) con << "LAPIC write to r/o version register " << (void*) value << "\n";
	    return;
	}
	case APIC_LOGICAL_DEST:
	{
	    lapic_id_reg_t nid = { raw: value };
	    if(debug_lapic_write) con << "LAPIC set logical destination to " << (void*) nid.x.id << "\n";
	    fields.ldest.x.id = nid.x.id;
	    return;
	}
	case APIC_DEST_FORMAT:
	{
	    lapic_dest_format_reg_t ndfr = { raw : value};

	    /* we only support the flat format */
	    if (ndfr.x.model == 0xF)
	    {
		if(debug_lapic_write) con << "LAPIC set to flat model ";
		return;
	    }
	    else
	    {
		con << "LAPIC unsupported destination format " << ndfr.x.model << "\n";
		DEBUGGER_ENTER(0);
	    }
	}
	case APIC_TASK_PRIO:
	{
	    lapic_prio_reg_t ntpr = { raw: value };	

	    if (ntpr.x.prio == 0 && ntpr.x.subprio == 0)
	    {
		if(debug_lapic_write) con << "LAPIC set task prio to accept all ";
		return;
	    }
	    else
	    {
		if(debug_lapic_write) con << "LAPIC unsupported task priorities " 
				    << ntpr.x.prio << "/" << ntpr.x.subprio << "\n";
		DEBUGGER_ENTER(0);
	    }
	}
	case APIC_EOI:
	{
	    cpu_t &cpu = get_cpu();
	    word_t int_save = cpu.disable_interrupts();
		intlogic_t &intlogic = get_intlogic();
	    
	    word_t vector = msb_rr( lapic_rr_isr );
	    //con << "E" << vector << "\n";
	    if( vector > 15 && bit_test_and_clear_atomic_rr(vector, lapic_rr_isr))
	    {
		
		if (msb_rr_cluster(lapic_rr_isr, vector) == 0)
		    intlogic.clear_vector_cluster(vector);
			    
		if (bit_test_and_clear_atomic_rr(vector, lapic_rr_tmr) &&
			get_ioapic(vector))
		{
		    ASSERT(get_ioapic(vector)->is_valid_ioapic());
		    get_ioapic(vector)->eoi(get_pin(vector));
		}
		
		if (intlogic.is_irq_traced(get_pin(vector)))
		{
		    con << "LAPIC: EOI vector " << vector;
		    if (get_ioapic(vector))
			con << " IRQ " << get_pin(vector)
			    << " IO-APIC " << get_ioapic(vector)->get_id();
		    con << "\n";
		}

	    }
	    cpu.restore_interrupts(int_save);
	    get_intlogic().deliver_synchronous_irq();
	    break;
	}		    
	case APIC_SVR:
	{
	    lapic_svr_reg_t nsvr = { raw : value };

	    fields.svr.x.vector = nsvr.x.vector;
	    fields.svr.x.focus_processor = nsvr.x.focus_processor;
		
	    if(debug_lapic_write)
	    {
		con << "LAPIC set spurious vector to " << fields.svr.x.vector << "\n";
		con << "LAPIC set focus processor to " << fields.svr.x.focus_processor << "\n";
	    }
		
	    if ((fields.svr.x.enabled = nsvr.x.enabled))
	    {
		con << "LAPIC enabled\n";
		fields.enabled = true;
#if defined(CONFIG_WEDGE_L4KA) 
		/* 
		 * jsXXX: this assumes a passthrough PIT 8253 and IRQ override
		 * IRQ0 in APIC mode (see device/portio.cc)
		 */
		get_intlogic().clear_hwirq_squash(2);
#endif
	    }
	    else
	    {
		con << "LAPIC disable not supported\n";
		DEBUGGER_ENTER(0);
	    }
	    break;
	}
	case APIC_LVT_TIMER:
	{ 
	    lapic_lvt_timer_t ntimer = { raw : value};
	    
	    fields.timer.x.vec = ntimer.x.vec;
	    /* Only support periodic timers for now */
	    fields.timer.x.msk = ntimer.x.msk;
	    fields.timer.x.mod = ntimer.x.mod;

	    if(debug_lapic_write) 
		con << "LAPIC set TIMER\n\t\t"
		    << ((ntimer.x.msk == 1) ? "masked" : "unmasked") << "\n\t\t"
		    << " vector to " << ntimer.x.vec << "\n";
	    if (ntimer.x.mod == 0 && ntimer.x.msk == 0)
	    {
		con << "LAPIC unsupported timer mode (one-shot)\n";
		DEBUGGER_ENTER(0);
	    }
	    
	    break;
	}
	case APIC_LVT_LINT0:
	case APIC_LVT_PMC:
	case APIC_LVT_LINT1:
	case APIC_LVT_ERROR:
	{
	    
	    lapic_lvt_t nlint = { raw : value };
	    /* 
	     * jsXXX do this via idx
	     */
	    lapic_lvt_t *lint = 
		(reg == APIC_LVT_PMC)   ? &fields.pmc : 
		(reg == APIC_LVT_LINT0) ? &fields.lint0 :
		(reg == APIC_LVT_LINT1) ? &fields.lint1 : &fields.error;
		
	    lint->x.msk = nlint.x.msk;
	    lint->x.trg = nlint.x.trg;
	    lint->x.pol = nlint.x.pol;
	    lint->x.dlm = nlint.x.dlm;
	    lint->x.vec = nlint.x.vec;
		
	    if(debug_lapic_write) 
	    {
		const char *lintname = 
		    (reg == APIC_LVT_PMC) ? "PMC" : 
		    (reg == APIC_LVT_LINT0) ? "LINT0" : 
		    (reg == APIC_LVT_LINT1) ? "LINT1" : 
		    "ERROR";
			    
		con << "LAPIC set " << lintname << "\n\t\t" 
		    << ((lint->x.msk == 1) ? "masked" : "unmasked") << "\n\t\t"
		    << ((lint->x.pol == 1) ? "low" : " high") << " active\n\t\t" 
		    << " delivery mode to " << lapic_delmode[lint->x.dlm] << "\n\t\t"
		    << " vector to " << lint->x.vec << "\n";
		
	    }
	    break;
	}
	case APIC_ERR_STATUS:
	{
	    if(debug_lapic_write) con << "LAPIC set ESR to " << (void *) value << "\n";
	    break;
	}
	case APIC_INTR_CMDLO:
	{
	    lapic_icrlo_reg_t nicrlo = { raw : value };

	    fields.icrlo.x.vector = nicrlo.x.vector;
	    fields.icrlo.x.delivery_mode = nicrlo.x.delivery_mode;
	    fields.icrlo.x.destination_mode = nicrlo.x.destination_mode;
	    fields.icrlo.x.level = nicrlo.x.level;
	    fields.icrlo.x.trigger_mode = nicrlo.x.trigger_mode;
	    fields.icrlo.x.dest_shorthand = nicrlo.x.dest_shorthand;
		    
	    // Init level-deassert, resets ArbID -- ignore
	    if (fields.icrlo.x.level == 0 && fields.icrlo.x.delivery_mode == LAPIC_INIT)
	    {
		if (debug_lapic_write) con << "LAPIC Init Level deassert IPI, ignore\n";
		return;
	    }
	    // Test if self IRQ
	    if (fields.icrlo.x.dest_shorthand == 1)
	    {
		con << "LAPIC self IPI vector " << fields.icrlo.x.vector << "\n";
		raise_vector(fields.icrlo.x.vector, 0, 0, false, false);
		// jsXXX: implement pending bit using EOI 
		return;
	    }
	    con << "LAPIC unimplemented ISR write (lo) " << (void *) nicrlo.raw << "\n";
	    DEBUGGER_ENTER(0);
	    break;
	}
	case APIC_TIMER_DIVIDE:
	{
#warning jsXXX: implement LAPIC timer counter register
	    lapic_divide_reg_t ndiv_conf = { raw : value };
	    
	    fields.div_conf.x.div0 = ndiv_conf.x.div0;
	    fields.div_conf.x.div1 = ndiv_conf.x.div1;
	    fields.div_conf.x.div2 = ndiv_conf.x.div2;
	    
	    if (debug_lapic_write)
		con << "LAPIC setting timer divide to " 
		    << (void *) fields.div_conf.raw << "\n";
	    break;
	}
	case APIC_TIMER_COUNT:
	{
	    fields.init_count = value;
	    
	    if (debug_lapic_write)
		con << "LAPIC setting current timer value to " << value << "\n";
	    break;
	}

	    
	default:
	    con << "LAPIC write to non-emulated register " 
		<< (void *) reg << ", value " << (void*) value << "\n" ;
	    DEBUGGER_ENTER(0);
	    break;
    }
}
