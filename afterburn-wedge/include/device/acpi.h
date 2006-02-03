/*********************************************************************
 *                
 * Copyright (C) 2002-2006,  Karlsruhe University
 *                
 * File path:     device/acpi.h
 * Description:   ACPI structures for running on L4
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
 *                
 ********************************************************************/

#ifndef __L4KA__ACPI_H__
#define __L4KA__ACPI_H__

#if defined(CONFIG_DEVICE_APIC)

#include INC_ARCH(page.h)
#include INC_ARCH(types.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)
 
#define ACPI_MEM_SPACE	0
#define ACPI_IO_SPACE	1

#define ACPI20_PC99_RSDP_START	     0x0e0000
#define ACPI20_PC99_RSDP_END	     0x100000
#define ACPI20_PC99_RSDP_SIZELOG     (5 + PAGE_BITS)


class acpi_gas_t {
public:
    u8_t	id;
    u8_t	width;
    u8_t	offset;
private:
    u8_t	_rsvd_3;
    /* the 64-bit address is only 32-bit aligned */
    u32_t       addrlo;
    u32_t       addrhi;

public:
    u64_t address (void) { return (((u64_t) addrhi) << 32) + addrlo; }
} __attribute__((packed));


class acpi_thead_t {
public:
    char	sig[4];
    u32_t	len;
    u8_t	rev;
    u8_t	csum;
    char	oem_id[6];
    char	oem_tid[8];
    u32_t	oem_rev;
    u32_t	creator_id;
    u32_t	creator_rev;
} __attribute__((packed));

class acpi_madt_hdr_t {
public:
    u8_t	type;
    u8_t	len;
} __attribute__((packed));

class acpi_madt_lapic_t {
    acpi_madt_hdr_t	header;
public:
    u8_t		apic_processor_id;
    u8_t		id;
    struct {
	u32_t enabled	:  1;
	u32_t		: 31;
    } flags;
} __attribute__((packed));

class acpi_madt_ioapic_t {
    acpi_madt_hdr_t	header;
public:
    u8_t	id;	  /* APIC id			*/
private:
    u8_t	_rsvd_3;
public:
    u32_t	address;  /* physical address		*/
    u32_t	irq_base; /* global irq number base	*/
} __attribute__((packed));


class acpi_madt_lsapic_t {
    acpi_madt_hdr_t	header;
public:
    u8_t		apic_processor_id;
    u8_t		id;
    u8_t		eid;
private:
    u8_t		__reserved[3];
public:
    struct {
	u32_t enabled	:  1;
	u32_t		: 31;
    } flags;
} __attribute__((packed));


class acpi_madt_iosapic_t {
    acpi_madt_hdr_t	header;
public:
    u8_t	id;	  /* APIC id			*/
private:
    u8_t	__reserved;
public:
    u32_t	irq_base; /* global irq number base	*/
    u64_t	address;  /* physical address		*/
} __attribute__((packed));


class acpi_madt_irq_t {
    acpi_madt_hdr_t	header;
public:
    u8_t	src_bus;	/* source bus, fixed 0=ISA	*/
    u8_t	src_irq;	/* source bus irq		*/
    u32_t	dest;		/* global irq number		*/
    union {
	u16_t	flags;		/* irq flags */
	struct {
	    u16_t polarity	: 2;
	    u16_t trigger_mode	: 2;
	    u16_t reserved	: 12;
	} x;
    };

public:
    enum polarity_t {
	conform_polarity = 0,
	active_high = 1,
	reserved_polarity = 2,
	active_low = 3
    };

    enum trigger_mode_t {
	conform_trigger = 0,
	edge = 1,
	reserved_trigger = 2,
	level = 3
    };

    polarity_t get_polarity()
	{ return (polarity_t) x.polarity; }
    trigger_mode_t get_trigger_mode()
	{ return (trigger_mode_t) x.trigger_mode; }

} __attribute__((packed));


class acpi_madt_nmi_t
{
    acpi_madt_hdr_t	header;
public:
    union {
	u16_t		flags;
	struct {
	    u16_t polarity	: 2;
	    u16_t trigger_mode	: 2;
	    u16_t reserved	: 12;
	} x;
    };
    u32_t		irq;

    enum polarity_t {
	conform_polarity	= 0,
	active_high		= 1,
	active_low		= 3
    };

    enum trigger_mode_t {
	conform_trigger		= 0,
	edge			= 1,
	level			= 3
    };
    
    polarity_t get_polarity (void)
	{ return (polarity_t) x.polarity; }

    trigger_mode_t get_trigger_mode (void)
	{ return (trigger_mode_t) x.trigger_mode; }
};


class acpi_madt_t {
    acpi_thead_t header;
public:
    u32_t	local_apic_addr;
    u32_t	apic_flags;
private:
    u8_t	data[0];
public:
    acpi_madt_hdr_t* find(u8_t type, int index) 
	{
	    for (word_t i = 0; i < (header.len-sizeof(acpi_madt_t));)
	    {
		acpi_madt_hdr_t* h = (acpi_madt_hdr_t*) &data[i];
		if (h->type == type)
		{
		    if (index == 0)
			return h;
		    index--;
		}
		i += h->len;
	    };
	    return NULL;
	}
    word_t len() { return header.len; }
	
public:
    acpi_madt_lapic_t*  lapic(int index) { return (acpi_madt_lapic_t*) find(0, index); };
    acpi_madt_ioapic_t* ioapic(int index) { return (acpi_madt_ioapic_t*) find(1, index); };
    acpi_madt_lsapic_t* lsapic(int index) { return (acpi_madt_lsapic_t*) find(7, index); };
    acpi_madt_iosapic_t* iosapic(int index) { return (acpi_madt_iosapic_t*) find (6, index); };
    acpi_madt_irq_t* irq(int index) { return (acpi_madt_irq_t*) find(2, index); };
    acpi_madt_nmi_t* nmi(int index) { return (acpi_madt_nmi_t*) find(3, index); };

} __attribute__((packed));




/*
  RSDT and XSDT differ in their pointer size only
  rsdt: 32bit, xsdt: 64bit
*/
    template<typename T> class acpi__sdt_t {
	acpi_thead_t	header;
	T		ptrs[0];
    public:
	/* find table with a given signature */
	acpi_thead_t* find(const char sig[4]) {
	    for (word_t i = 0; i < ((header.len-sizeof(header))/sizeof(ptrs[0])); i++)
	    {
		word_t rwx = 7;

		if (((word_t) this & PAGE_MASK) != ((word_t) ptrs[i] & PAGE_MASK))
		    backend_request_device_mem(ptrs[i], PAGE_SIZE, rwx);
		
		acpi_thead_t* t= (acpi_thead_t*)((word_t)ptrs[i]);
		if (t->sig[0] == sig[0] && 
		    t->sig[1] == sig[1] && 
		    t->sig[2] == sig[2] && 
		    t->sig[3] == sig[3])
		    return t;
		
		if (((word_t) this & PAGE_MASK) !=  ((word_t) ptrs[i] & PAGE_MASK))
		    backend_unmap_device_mem(ptrs[i], PAGE_SIZE, rwx);

	    };
	    return NULL;
	};
	void list() {
#if 0
	    for (word_t i = 0; i < ((header.len-sizeof(header))/sizeof(ptrs[0])); i++)
	    {
		acpi_thead_t* t= (acpi_thead_t*)((word_t)ptrs[i]);
		printf("%c%c%c%c @ %p\n",
		       t->sig[0], t->sig[1], t->sig[2], t->sig[3], t);
	    };
#endif
	};
	word_t len() { return header.len; };

	friend class kdb_t;
    } __attribute__((packed));

    typedef acpi__sdt_t<u32_t> acpi_rsdt_t;
    typedef acpi__sdt_t<u64_t> acpi_xsdt_t;

    class acpi_rsdp_t {
   
	char	sig[8];
	u8_t	csum;
	char	oemid[6];
	u8_t	rev;
	u32_t	rsdt_ptr;
	u32_t	rsdp_len;
	u64_t	xsdt_ptr;
	u8_t	xcsum;
	u8_t	_rsvd_33[3];
    private:
    public:
	static acpi_rsdp_t* locate();
   
	acpi_rsdt_t* rsdt() {
	    /* verify checksum */
	    u8_t c = 0;
	    for (int i = 0; i < 20; i++)
		c += ((char*)this)[i];
	    if (c != 0)
		return NULL;
	    return (acpi_rsdt_t*) (word_t)rsdt_ptr;
	};
	acpi_xsdt_t* xsdt() {
	    /* check version - only ACPI 2.0 knows about an XSDT */
	    if (rev != 2)
		return NULL;
	    /* verify checksum
	       hopefully it's wrong if there's no xsdt pointer*/
	    u8_t c = 0;
	    for (int i = 0; i < 36; i++)
		c += ((char*)this)[i];
	    if (c != 0)
		return NULL;
	    return (acpi_xsdt_t*) (word_t)xsdt_ptr;
	};
	word_t len() { return rsdp_len; };
	
    } __attribute__((packed));


class acpi_t
    {
    private: 
	acpi_rsdp_t* rsdp;
	acpi_rsdt_t* rsdt;
	acpi_xsdt_t* xsdt;
	acpi_madt_t* madt;

	word_t lapic_base;
	
	struct{
	    word_t id;
	    word_t irq_base;
	    word_t address;
	} ioapic[CONFIG_MAX_IOAPICS];
	u32_t nr_ioapics;


    public:
	void init(word_t cpu_id)
	    {
		rsdp = NULL;
		rsdt = NULL;
		xsdt = NULL;
		madt = NULL;
		
		word_t rwx = 7;
		con << "ACPI trying to find RSDP\n";
				
		if ((rsdp = acpi_rsdp_t::locate()) == NULL)
		{
		    con << "ACPI tables not found\n";
		    return;
		}


		con << "RSDP @ " << (void *) rsdp << "\n";
	
		rsdt = rsdp->rsdt();
		xsdt = rsdp->xsdt();
	
		con << "RSDT @ " << (void *) rsdt << "\n";
		con << "XSDT @ " << (void *) xsdt << "\n";

	
		if (xsdt != NULL)
		{
		    backend_request_device_mem((word_t) xsdt, PAGE_SIZE, rwx, true);
		    madt = (acpi_madt_t*) xsdt->find("APIC");
		}
		if ((madt == NULL) && (rsdt != NULL))
		{
		    backend_request_device_mem((word_t) rsdt, PAGE_SIZE, rwx, true);
		    madt = (acpi_madt_t*) rsdt->find("APIC");
		}
		
		con << "MADT @ " << madt << "\n";

		con << "LAPIC @ " << (void *) madt->local_apic_addr << "\n";
		lapic_base = madt->local_apic_addr;

		word_t i = 0;
		for (; ((madt->ioapic(i)) != NULL && i < CONFIG_MAX_IOAPICS); i++)

		{
		    ioapic[i].id = madt->ioapic(i)->id;
		    ioapic[i].irq_base = madt->ioapic(i)->irq_base;
		    ioapic[i].address = madt->ioapic(i)->address;
		    
		    con << "IOAPIC id " << ioapic[i].id
			<< " base " << ioapic[i].irq_base 
			<< " @ " << (void *) ioapic[i].address << "\n";
		
		}
		nr_ioapics = i;
		
		
		if (rsdp)
		    backend_unmap_device_mem((word_t) rsdp, PAGE_SIZE, rwx, true);
		if (xsdt)
		    backend_unmap_device_mem((word_t) xsdt, PAGE_SIZE, rwx, true);
		if (rsdt)
		    backend_unmap_device_mem((word_t) rsdt, PAGE_SIZE, rwx, true);
		if (madt)
		    backend_unmap_device_mem((word_t) madt, PAGE_SIZE, rwx, true);
		
#if 0		
		L4_Word_t lapiclow = lapic_base;
		L4_Word_t lapichigh = lapic_base + PAGE_SIZE - 1;
		for( L4_Word_t d=0; d<IHypervisor_max_devices;d++ )
		{
		    if (resourcemon_shared.devices[d].low = low &&
			    resourcemon_shared.devices[d].high == high)
			con << "Removing LAPIC from device memory list\n";
		}

#endif
		
	    }

	bool is_lapic_addr(word_t addr)
	    {
		if ((addr & PAGE_MASK) == (word_t) lapic_base)
		    return true;
		return false;
	    }

	bool is_ioapic_addr(word_t addr, word_t *id)
	    {
		for (word_t i = 0; i < nr_ioapics; i++)
		{
		    if ((addr & PAGE_MASK) == ioapic[i].address)
		    {
			*id = ioapic[i].id;
			return true;
		    }
		}
		return false;
	    }
	
	u32_t get_nr_ioapics() { return nr_ioapics; }
	
	u32_t get_ioapic_irq_base(u32_t idx) { 

	    ASSERT(idx < CONFIG_MAX_IOAPICS);
	    return ioapic[idx].irq_base;
	}

	u32_t get_ioapic_id(u32_t idx) { 

	    ASSERT(idx < CONFIG_MAX_IOAPICS);
	    return ioapic[idx].id;
	}
	
	u32_t get_ioapic_addr(u32_t idx) { 

	    ASSERT(idx < CONFIG_MAX_IOAPICS);
	    return ioapic[idx].address;
	}



};

extern acpi_t acpi;

#endif	/* CONFIG_DEVICE_APIC */

#endif /* __L4KA__ACPI_H__ */
