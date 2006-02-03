/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/device/i82093
 * Description:   Front-end emulation for the IO APIC 82093.
 *
 * Re istribution and use in source and binary forms, with or without
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
 * $Id: i82093.cc,v 1.3 2006-01-11 17:41:46 stoess Exp $
 *
 ********************************************************************/

#include <device/portio.h>
#include INC_ARCH(intlogic.h)
#include INC_ARCH(bitops.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(backend.h)

#ifdef CONFIG_WEDGE_XEN
#include INC_WEDGE(hypervisor.h)
#endif

const char ioapic_virt_magic[8] = { 'V', 'I', 'O', 'A', 'P', 'I', 'C', '0' };
const char *ioapic_delmode[8] = { "fixed", "lowest priority", "SMI", "reserved",
				  "NMI", "INT", "reserved", "extINT" };


extern "C" void __attribute__((regparm(2)))
ioapic_write_patch( word_t value, word_t addr )
{
    i82093_t *ioapic = i82093_t::addr_to_ioapic(addr);
    
    if( debug_ioapic_regsel)
    {
	word_t dummy, paddr = 0;
	pgent_t *pgent = backend_resolve_addr(addr, dummy);
	
	if( !pgent || !pgent->is_kernel() )
	{
	    con << "Inconsistent guest pagetable entries for IO-APIC mapping @ " 
		<< (void *) addr << ", pgent " << (void *) pgent << "\n"; 
	    panic();
	}

	if (!ioapic->is_valid_virtual_ioapic())
	    con << "BUG no sane softIOAPIC @" << (void *) ioapic
		<< ", phys " << (void *) pgent->get_address() 
		<< ", pgent " << (void *) pgent << "\n"; 
	
	paddr = pgent->get_address() + (addr & ~PAGE_MASK);
	
	con << "IOAPIC " << ioapic->get_id() << " write  @ " << (void *)addr << " (" << (void *) paddr << "), ip "
	    <<  __builtin_return_address(0) << ", value  " << (void *) value << "\n" ;
	
    }
    ioapic->write(value, ioapic->addr_to_reg(addr));
   
}

extern "C" word_t __attribute__((regparm(1)))
ioapic_read_patch( word_t addr )
{
    word_t ret = * (word_t *) addr;

    if( debug_ioapic_regsel )
    {
	word_t dummy, paddr = 0;
	pgent_t *pgent = backend_resolve_addr(addr, dummy);
	
	if( !pgent || !pgent->is_kernel() )
	{
	    con << "Inconsistent guest pagetable entries for IOAPIC mapping @" 
		<< (void *) addr << ", pgent " << (void *) pgent << "\n"; 
	    panic();
	}

	i82093_t *ioapic = (i82093_t *) (addr & PAGE_MASK);
	if (!ioapic->is_valid_virtual_ioapic())
	    con << "BUG no sane softIOAPIC @" << (void *) ioapic 
		<< ", phys " << (void *) pgent->get_address() 
		<< ", pgent " << (void *) pgent << "\n"; 

	paddr = pgent->get_address() + (addr & ~PAGE_MASK);

	if (addr != (word_t) &ioapic->fields.mm_regs.regwin)
	    con << "IOAPIC strange read on non-regwin addr " << addr		
		<< " (" << (void *) paddr 
		<< "), ip " <<  __builtin_return_address(0) << "\n";
	
	con << "IOAPIC " << ioapic->get_id() << "read  reg " 
	    << ioapic->fields.mm_regs.regsel 
	    << " (" << (void *) paddr 
	    << "), ip " <<  __builtin_return_address(0) 
	    << ", return " << (void *) ret << "\n" ;
    }
  
    return ret;

}

void i82093_t::eoi(word_t hwirq)
{
    word_t entry = hwirq - fields.irq_base;
    ASSERT(entry < IOAPIC_MAX_REDIR_ENTRIES);

    // always clear IRR (don't check for edge/level)
    bit_clear_atomic(14, fields.io_regs.x.redtbl[entry].raw[0]);
    
    intlogic_t &intlogic = get_intlogic();

    if (intlogic.test_and_clear_hwirq_mask(hwirq) && 
	    !intlogic.is_hwirq_squashed(hwirq))
    {
	if(intlogic.is_irq_traced(hwirq))
	    con << "IOAPIC " << get_id() << " eoi irq " << hwirq << ", unmask\n";			
	backend_unmask_device_interrupt( hwirq );
    }
    
    if (bit_test_and_clear_atomic(17, fields.io_regs.x.redtbl[entry].raw[0]))
    {
	if(intlogic.is_irq_traced(hwirq))
	    con << "IOAPIC " << get_id() << " reraise pending irq" << hwirq << "untested\n";
	DEBUGGER_ENTER(0);
	raise_irq(hwirq, true);
    }

}
    

void i82093_t::raise_irq (word_t irq, bool reraise)
{
    word_t entry = irq - fields.irq_base;
    ASSERT(entry < IOAPIC_MAX_REDIR_ENTRIES);
    
    intlogic_t &intlogic = get_intlogic();
    
    if(intlogic.is_irq_traced(irq))
	con << "IOAPIC " << get_id() << " IRQ " 
	    << irq << " entry " 
	    << entry << " fields " 
	    << (void *) (word_t) fields.io_regs.x.redtbl[entry].raw[0] 
	    << "\n"; 
    
    if (fields.io_regs.x.redtbl[entry].x.vec <= 15)
	return;
    
    if (fields.io_regs.x.redtbl[entry].x.msk == 0)
    {
    
	// By design, LAPIC IDs are identical to VCPU IDs
	// make sure that dest ID == VCPU 
	local_apic_t *la = get_cpu().get_lapic();
	ASSERT(la);
	
	if (fields.io_regs.x.redtbl[entry].x.dstm == 0)
	    ASSERT(fields.io_regs.x.redtbl[entry].x.dest.phys.pdst == la->get_id());
	else
	    ASSERT(fields.io_regs.x.redtbl[entry].x.dest.log.ldst == la->get_logical_dest());
	
	word_t vector = fields.io_regs.x.redtbl[entry].x.vec;

	if (fields.io_regs.x.redtbl[entry].x.trg == 1) 
	{
	    // if level triggered, set IRR 
	    bit_set_atomic(14, fields.io_regs.x.redtbl[entry].raw[0]);
	    intlogic.set_hwirq_mask(irq);
	    
	    // Only fixed (000) and lowest 1priority (001) IRQs need eoi 
	    bool from_eoi = (fields.io_regs.x.redtbl[entry].x.del <= 1);
	    la->raise_vector(vector, irq, this, from_eoi, reraise);
	    
	}
	else {
	    // if edge triggered, EOI not needed 
	    la->raise_vector(vector, irq, this, false, reraise);

	    if (!intlogic.is_hwirq_squashed(irq) &&
		intlogic.test_and_clear_hwirq_mask(irq))
	    {
		if(intlogic.is_irq_traced(irq))
		    con << "Unmask edge triggered IRQ " << irq << "\n";

		cpu_t &cpu = get_cpu();
		word_t int_save = cpu.disable_interrupts();
		backend_unmask_device_interrupt( irq );
		cpu.restore_interrupts( int_save, false );
	    }
	}

    }
    // If masked level irq mark pending, abuse bit 17 for this purpose
    else if (fields.io_regs.x.redtbl[entry].x.trg == 1) 
    {
	if(intlogic.is_irq_traced(irq))
	    con << "IOAPIC " << get_id() << " mark masked level irq " << irq << " pending\n";
	bit_set_atomic(17, fields.io_regs.x.redtbl[entry].raw[0]);
	intlogic.set_hwirq_mask(irq);
    }
    else 
    {
	if(intlogic.is_irq_traced(irq))
	    con << "IOAPIC" << get_id() << " ack and ignore masked edge irq " << irq << "\n";
	
	if (!intlogic.is_hwirq_squashed(irq) &&
	    intlogic.test_and_clear_hwirq_mask(irq))
	{
	    if(intlogic.is_irq_traced(irq))
		con << "IOAPIC " << get_id() << " unmask masked edge irq " << irq << "\n";
	    cpu_t &cpu = get_cpu();
	    word_t int_save = cpu.disable_interrupts();
	    backend_unmask_device_interrupt( irq );
	    cpu.restore_interrupts( int_save, false );
	}
    }
}	    


void i82093_t::write(word_t value, word_t reg)
{
    switch (reg) 
    {
	case IOAPIC_SEL:
	{
	    if (debug_ioapic_regsel)
		con << "IOAPIC " << get_id() << " register select " << value << "\n";
	    fields.mm_regs.regsel = value;
		    
	    /*
	     * js: is prefetching really preferable ?
	     */
	    if (value < 16 + (2 * IOAPIC_MAX_REDIR_ENTRIES))
		fields.mm_regs.regwin = fields.io_regs.raw[value];

	    else
		con << "BUG: strange io regster selection for IOAPIC (" << value << ")\n";
	    break;
	}
	case IOAPIC_WIN:
	{
	    if (debug_ioapic)
		con << "IOAPIC " << get_id() << " write " << (void *) value 
		    << " to  reg " << fields.mm_regs.regsel << "\n";
	    
	    switch (fields.mm_regs.regsel) 
	    {
		case IOAPIC_ID:	      
		case IOAPIC_VER:
		case IOAPIC_ARB:
		{
		    if (debug_ioapic)
			con << "IOAPIC " << get_id() << "  write " << (void *) value 
			    << " to  reg " << fields.mm_regs.regsel << "UNIMPLEMENTED\n";
		    DEBUGGER_ENTER(0);
		}
	    
		case IOAPIC_RDTBL_0_0 ... IOAPIC_RDTBL_23_1:
		{
		    word_t entry = (fields.mm_regs.regsel - IOAPIC_RDTBL_0_0) >> 1;
		    bool int_save;		
		    cpu_t cpu = get_cpu();
		    i82093_redtbl_t nredtbl;
		    
		    /* 
		     * Lower half of register ?
		     */
		    if ((fields.mm_regs.regsel & 1) == 0) 
		    {
				
			word_t save_msk = fields.io_regs.x.redtbl[entry].x.msk;
			word_t hwirq = entry + fields.irq_base;
			intlogic_t &intlogic = get_intlogic();
			
			nredtbl.raw[0]  = value;			
		
			int_save = cpu.disable_interrupts();
			//jsXXX: this should be optimized
			fields.io_regs.x.redtbl[entry].x.msk  = nredtbl.x.msk;
			fields.io_regs.x.redtbl[entry].x.trg  = nredtbl.x.trg;
			fields.io_regs.x.redtbl[entry].x.pol  = nredtbl.x.pol;
			fields.io_regs.x.redtbl[entry].x.dstm = nredtbl.x.dstm;
			fields.io_regs.x.redtbl[entry].x.del  = nredtbl.x.del;
			fields.io_regs.x.redtbl[entry].x.vec  = nredtbl.x.vec;
			    
			cpu.restore_interrupts( int_save, false );

			if(debug_ioapic_rdtbl || intlogic.is_irq_traced(hwirq)) 
			{
			    con << "IOAPIC " << get_id() << " set RDTBL " << entry  << "\n\t\t"
				<< "addr " << (void *) &fields.io_regs.x.redtbl[entry] << "\n\t\t"
				<< ((nredtbl.x.msk == 1) ? "masked" : "unmasked") << "\n\t\t"
				<< ((nredtbl.x.trg == 1) ? "level" : " edge") << " triggered \n\t\t"
				<< ((nredtbl.x.pol == 1) ? "low" : " high") << " active\n\t\t" 
				<< ((nredtbl.x.dstm == 1) ? "logical" : " physical") << " destination mode\n\t\t" 
				<< "delivery mode " << ioapic_delmode[nredtbl.x.del] << "\n\t\t"
				<< "vector " << nredtbl.x.vec << "\n";
			}
			
			if (nredtbl.x.msk == 0 && save_msk == 1 &&
				!intlogic.is_hwirq_squashed(hwirq))
			{
			    if (!intlogic.test_and_set_hwirq_latch(hwirq))
			    {
				if(intlogic.is_irq_traced(hwirq))
				{
				    con << "IOAPIC " << get_id() << " enable hwirq " << hwirq << "\n";
				    DEBUGGER_ENTER(0);
				}

				if( !backend_enable_device_interrupt(hwirq) )
				    con << "Unable to enable passthru device interrupt " << hwirq << "\n";
				
			    }
			    else if (bit_test_and_clear_atomic(17, fields.io_regs.x.redtbl[entry].raw[0]))
			    {
				if(intlogic.is_irq_traced(hwirq))
				    con << "IOAPIC " << get_id() << " reraise pending irq" << hwirq << "untested\n";
				DEBUGGER_ENTER(0);
				raise_irq(hwirq, true);
			    }
				
			}
			else if (nredtbl.x.msk == 1 && save_msk == 0)
			{
			    if(intlogic.is_irq_traced(hwirq))
				con << "IOAPIC " << get_id() << " mask IRQ " << hwirq << "\n";
			}
			    

		    }
		    else {
			nredtbl.raw[1]  = value;			
			
			int_save = cpu.disable_interrupts();
			
			if (fields.io_regs.x.redtbl[entry].x.dstm == 1) 
			    fields.io_regs.x.redtbl[entry].x.dest.log.ldst = 
				nredtbl.x.dest.log.ldst;
			else 
			    fields.io_regs.x.redtbl[entry].x.dest.phys.pdst = 
				nredtbl.x.dest.phys.pdst;
			
			cpu.restore_interrupts( int_save, false );
			
			if(debug_ioapic_rdtbl) 
			    con << "IOAPIC " << get_id() << " set RDTBL " << entry 
				<< ((fields.io_regs.x.redtbl[entry].x.dstm == 1) ? " logical" : " physical" )
				<< " destination to " << nredtbl.x.dest.log.ldst << "\n";

		    }
		    break;		   
		}
		default:
		{
		    if (debug_ioapic)
			con << "IOAPIC " << get_id() << " UNIMPLEMENTED write " << (void *) value 
			    << " to  reg " << fields.mm_regs.regsel << "\n";
		    DEBUGGER_ENTER(0);
		    break;
		}
	    }
	    break;
	}
	default:
	{
	    con << "IOAPIC " << get_id() << " strange write " << (void *) value 
		<< " to non-window reg " << reg << "\n";
	    DEBUGGER_ENTER(0);
	    break;
	}
    }
}

