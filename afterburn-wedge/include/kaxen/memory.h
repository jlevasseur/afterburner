/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 * Copyright (C) 2005,  Volkmar Uhlig
 *
 * File path:     afterburn-wedge/include/kaxen/memory.h
 * Description:   Memory management specific to Xen.
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
#ifndef __AFTERBURN_WEDGE__INCLUDE__KAXEN__MEMORY_H__
#define __AFTERBURN_WEDGE__INCLUDE__KAXEN__MEMORY_H__

#include INC_ARCH(bitops.h)
#include INC_ARCH(cpu.h)

#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(debug.h)
#include INC_WEDGE(xen_hypervisor.h)
#include INC_WEDGE(segment.h)

#if defined(CONFIG_KAXEN_GLOBAL_PAGES)
# define ON_KAXEN_GLOBAL_PAGES(a) do { a ; } while(0)
#else
# define ON_KAXEN_GLOBAL_PAGES(a)
#endif

struct mach_page_t
{
    enum mach_type_e {mach_normal=0, mach_pgtable=1};
    union {
	struct {
#if defined(CONFIG_KAXEN_UNLINK_AGGRESSIVE)
	    word_t pdir_entry : 10;
#elif defined(CONFIG_KAXEN_WRITABLE_PGTAB)
	    word_t inuse  :1;
	    word_t multi  :1;
	    word_t unused :8;
#else
	    word_t unused :10;
#endif
	    word_t type   :1;
	    word_t pinned :1;
	    word_t page   :20;
	} fields;
#if defined(CONFIG_KAXEN_PGD_COUNTING)
	struct {
	    word_t count  :10;
	    word_t type   :1;
	    word_t pinned :1;
	    word_t page   :20;
	} pgd_fields;
#endif
	word_t raw;
    } x;

    void set_normal()
        {
    	    x.fields.type = mach_normal; 
	    x.fields.pinned = false; 
#if defined(CONFIG_KAXEN_WRITABLE_PGTAB)
	    x.fields.inuse = x.fields.multi = false; 
#endif
	}
    void set_address( word_t mach_address )
	{ x.fields.page = mach_address >> PAGE_BITS; }
    void set_pgtable()
	{ x.fields.type = mach_pgtable; }
    void set_pinned()
	{ x.fields.pinned = true; }
    void set_unpinned()
	{ x.fields.pinned = false; }

#if defined(CONFIG_KAXEN_UNLINK_AGGRESSIVE)
    void set_pdir_addr( word_t vaddr )
	{ x.fields.pdir_entry = pgent_t::get_pdir_idx(vaddr); }
    void set_pdir_entry( word_t pdir_entry )
	{ x.fields.pdir_entry = pdir_entry; }
    word_t get_pdir_entry()
	{ return x.fields.pdir_entry; }
#elif defined(CONFIG_KAXEN_WRITABLE_PGTAB)
    bool is_multi()
	{ return x.fields.multi; }
    void set_multi()
	{ x.fields.multi = true; }
    void link_inc()
	{ x.fields.multi |= x.fields.inuse; x.fields.inuse = true; /*con << "pdir link: " << (void *)get_address() << '\n';*/ }
    void link_dec()
	{ x.fields.inuse = false; }
    bool is_linked()
	{ return x.fields.inuse || x.fields.multi; }
    bool is_unlinked()
	{ return !is_linked(); }
    bool is_single_link()
	{ return x.fields.inuse && !x.fields.multi; }
#endif

#if defined(CONFIG_KAXEN_PGD_COUNTING)
    void clear_pgds()
	{ x.pgd_fields.count = 0; }
    void inc_pgds()
	{ x.pgd_fields.count++; }
    void dec_pgds()
	{ x.pgd_fields.count--; }
    bool is_empty_pgds()
	{ return x.pgd_fields.count == 0; }
#endif

    word_t get_address()
	{ return x.fields.page << PAGE_BITS; }
    word_t get_raw()
	{ return x.raw; }
    bool is_pgtable()
	{ return x.fields.type == mach_pgtable; }
    bool is_pinned()
	{ return x.fields.pinned; }
};

/* pgtab_region is declared in the linker script, and has no physical
 * backing.  It is mapped to all pages that back the page directory and
 * page table, by recursively mapping the page directory as a pgdir entry
 * at the address pgtab_region.
 */
extern pgent_t pgtab_region[];
/* pdir_region is declared in the linker script, and has no physical 
 * backing.  It is mapped to the current page directory.
 */
extern pgent_t pdir_region[];
/* xen_p2m_region is declared in the linker script, and 
 * has no physical backing.  It is allocated physical backing at runtime.
 */
extern mach_page_t xen_p2m_region[];
/* tmp_region is declared in the linker script, and has no physical
 * backing.  It is used for temporary mappings.
 */
extern word_t tmp_region[];

class xen_memory_t {
public:
    static const word_t max_mach_pages = 1024*1024*1024/PAGE_SIZE*4;

    void init( word_t mach_mem_total );
    void init_m2p_p2m_maps();
    void remap_boot_region( word_t boot_addr, word_t page_cnt, word_t new_vaddr );
    void alloc_remaining_boot_pages();

    void dump_active_pdir( bool pdir_only=false );
    void dump_pgent( int level, word_t vaddr, pgent_t &pgent, word_t pgsize );

    word_t get_mfn_count()
	{ return xen_start_info.nr_pages; }
    word_t get_vm_size()
	{ return get_mfn_count() << PAGE_BITS; }
    word_t get_guest_size()
	{ return guest_phys_size; }
    word_t get_wedge_size()
	{ return get_vm_size() - get_guest_size(); }

    void enable_guest_paging( word_t pdir_phys );
    void switch_page_dir( word_t new_pdir_phys, word_t old_pdir_phys );

    word_t p2m( word_t phys_addr )
    { 
	if(EXPECT_TRUE(phys_addr < get_guest_size()))
	    return xen_p2m_region[phys_addr >> PAGE_BITS].get_address();

	PANIC( "Guest used an invalid physical address.\n" );
    }

    word_t m2p( word_t mach_addr )
	// machine_to_phys_mapping is imported from Xen's headers, in xen.h
    { 
#if defined(CONFIG_DEVICE_PASSTHRU)
	if( EXPECT_FALSE(mach_addr > total_mach_mem) )
	    PANIC( "Invalid machine address " << (void *)mach_addr 
		    << " from " << (void *)__builtin_return_address(0) );
#endif
	word_t phys_addr = machine_to_phys_mapping[mach_addr >> PAGE_BITS];
	ASSERT( phys_addr < get_guest_size() );
	return phys_addr;
    }

    mach_page_t & p2mpage( word_t phys_addr )
    {
	if(EXPECT_TRUE(phys_addr < get_guest_size()))
	    return xen_p2m_region[phys_addr >> PAGE_BITS];
	
	PANIC( "Guest used an invalid physical address, "
		<< (void *)phys_addr << " >= " << (void *)get_guest_size()
		<< ", called from " << (void *)__builtin_return_address(0) );
    }

    void unpin_page( mach_page_t &mpage, word_t paddr, bool commit=true );
    void pin_ptab( mach_page_t *mpage, pgent_t pgent );
    void flush_vaddr( word_t vaddr );
    void change_pgent( pgent_t *old_pgent, pgent_t new_pgent, bool leaf );
    bool resolve_page_fault( xen_frame_t *frame );
#if defined(CONFIG_KAXEN_UNLINK_HEURISTICS)
    void relink_ptab( word_t fault_vaddr );
#endif

    word_t guest_kernel_pdent()
	{ return pgent_t::get_pdir_idx(get_vcpu().get_kernel_vaddr()); }
    word_t guest_v2p( word_t vaddr )
	{ return vaddr - get_vcpu().get_kernel_vaddr(); }
    word_t guest_p2v( word_t paddr )
	{ return paddr + get_vcpu().get_kernel_vaddr(); }

    static bool is_device_overlap( word_t addr )
    {
	// Is this device memory that overlaps physical memory?
	// We need to provide this, since we will be asked to convert
	// dma to phys addresses by Linux for the legacy areas.
	return (addr >= 0xa0000) && (addr < 0x000f0000) ||
	    (addr >= 0x0e0000 && addr < 0x100000); 
    }
    
    bool is_device_memory( word_t addr )
#if defined(CONFIG_DEVICE_PASSTHRU)
    // jXXX: this won't work for IO holes, e.g., with ACPI inside VMware
	{ return is_device_overlap(addr) || (addr >= total_mach_mem); }
#else
	{ return false; }
#endif
    
    bool map_device_memory( word_t vaddr, word_t maddr, bool boot=false);
    bool unmap_device_memory( word_t vaddr, word_t maddr, bool boot=false);

    pgent_t get_pgent( word_t vaddr )
	{ return get_ptab( pgent_t::get_pdir_idx(vaddr) )[ pgent_t::get_ptab_idx(vaddr) ]; }
    pgent_t get_pdent( word_t vaddr )
	{ return get_pdir()[ pgent_t::get_pdir_idx(vaddr) ]; }

    pgent_t *get_pgent_ptr( word_t vaddr )
	{ return &get_ptab( pgent_t::get_pdir_idx(vaddr) )[ pgent_t::get_ptab_idx(vaddr) ]; }

    
private:
    word_t pdir_end_entry;
    word_t boot_pdir_start_entry;
    word_t boot_mfn_list_allocated;
    word_t boot_mfn_list_start;
    word_t guest_phys_size;
    word_t total_mach_mem;
    word_t pdir_maddr;
    word_t boot_pdir_maddr;

    void validate_boot_pdir();
    void globalize_wedge();
    void map_boot_pdir();
    void unpin_boot_pdir();
    void init_segment_descriptors();

    void alloc_boot_ptab( word_t vaddr );
    void alloc_boot_region( word_t vaddr, word_t size );

    word_t allocate_boot_page();
    void map_boot_page( word_t vaddr, word_t maddr, bool read_only=false );

    void manage_page_dir( word_t new_pdir_phys );
    void unmanage_page_dir( word_t pdir_vaddr );

    word_t unallocated_pages()
	{ return xen_start_info.nr_pages - (boot_mfn_list_allocated - boot_mfn_list_start); }

    pgent_t *get_boot_pdir()
	{ return (pgent_t *)xen_start_info.pt_base; }
    pgent_t *get_boot_ptab( word_t pdir_entry )
    	{ return (pgent_t *)(xen_start_info.pt_base + PAGE_SIZE*(pdir_entry-boot_pdir_start_entry+1)); }
    pgent_t *get_pdir()
	{ return pdir_region; }
    pgent_t *get_ptab( word_t pdir_entry )
	{ return (pgent_t *)(word_t(pgtab_region) + PAGE_SIZE*pdir_entry); }
    pgent_t *get_guest_pdir()
	{ return (pgent_t *)guest_p2v(get_cpu().cr3.get_pdir_addr()); }

    // Return the machine address of the current page directory.
    word_t get_pdir_maddr()
	{ return pdir_maddr; }
    // Return a machine address of an entry in the current page directory.
    word_t get_pdent_maddr( word_t vaddr )
	{ return pdir_maddr + pgent_t::get_pdir_idx(vaddr)*sizeof(pgent_t); }
    // Return a machine address of an entry in an active page table.
    word_t get_pgent_maddr( word_t vaddr )
    {
	pgent_t pdent = get_pdir()[ pgent_t::get_pdir_idx(vaddr) ];
	return pdent.get_address() + pgent_t::get_ptab_idx(vaddr)*sizeof(pgent_t);
    }

#if defined(CONFIG_KAXEN_UNLINK_AGGRESSIVE)
    bool unlink_pdent_heuristic( pgent_t *pdir, mach_page_t &mpage, bool cr2 );
#endif
};

class xen_mmop_queue_t
{
private:
    static const word_t queue_len = ARGS_PER_MULTICALL_ENTRY;
    word_t count;
    mmu_update_t req[queue_len];

#ifdef CONFIG_XEN_3_0
    /* Xen 3.0 split extended and MMU calls into separate syscalls.
     * We emulate the behavior by using multicalls that execute one or
     * the other syscall */
    word_t ext_count;
    static const word_t ext_queue_len = ARGS_PER_MULTICALL_ENTRY;
    struct mmuext_op ext_req[ext_queue_len];

    word_t mc_count;
    static const word_t mc_queue_len = ARGS_PER_MULTICALL_ENTRY * 2; // worst case
    multicall_entry_t multicall[mc_queue_len];
    u32 multicall_success[mc_queue_len];
#endif

    bool add( word_t type, word_t ptr, word_t val, bool sync=false );
    bool add_ext( word_t exttype, word_t ptr, bool sync=false );

public:
    bool is_pending() { return this->count > 0; }
    bool commit();

    xen_mmop_queue_t()
	{ 
	    count = 0;
#ifdef CONFIG_XEN_3_0
	    ext_count = 0;
	    mc_count = 0;
#endif
	}

public:
    // Helpers ptab
    bool ptab_update( word_t maddr, word_t val, bool sync=false ) 
	{ return add (MMU_NORMAL_PT_UPDATE, maddr, val, sync); }

    bool ptab_phys_update( word_t maddr, word_t val, bool sync=false ) 
	{ return add (MMU_MACHPHYS_UPDATE, maddr, val, sync); }

    // Helpers extended
    bool invlpg( word_t addr, bool sync=false )
	{ return add_ext( MMUEXT_INVLPG, addr, sync ); }

    bool pin_table( word_t addr, word_t level, bool sync=false )
	{ 
	    return add_ext( level == 0 ? MMUEXT_PIN_L1_TABLE :
			    level == 1 ? MMUEXT_PIN_L2_TABLE :
			    level == 2 ? MMUEXT_PIN_L3_TABLE :
					 MMUEXT_PIN_L4_TABLE,
			    addr >> XEN_EXTMMU_SHIFT, sync); 
	}

    bool unpin_table( word_t addr, bool sync=false )
	{ return add_ext( MMUEXT_UNPIN_TABLE, addr >> XEN_EXTMMU_SHIFT, sync ); }

    bool set_baseptr( word_t addr, bool sync=false )
	{ return add_ext( MMUEXT_NEW_BASEPTR, addr >> XEN_EXTMMU_SHIFT, sync); }

    bool tlb_flush ( bool sync=false)
	{ return add_ext( MMUEXT_TLB_FLUSH, 0, sync ); }

#ifdef CONFIG_XEN_3_0
    bool tlb_flush_multi( void *vcpumask, bool sync=false);

    bool set_user_baseptr( word_t addr, bool sync=false )
	{ return add_ext( MMUEXT_NEW_USER_BASEPTR, addr, sync ); }
#endif
};


extern xen_memory_t xen_memory;
extern xen_mmop_queue_t mmop_queue;

#endif	/* __AFTERBURN_WEDGE__INCLUDE__KAXEN__MEMORY_H__ */
