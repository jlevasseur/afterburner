/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/vm.cc
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
 * $Id: vm.cc,v 1.15 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include "burnxen.h"
#include <elfsimple.h>
#include <bind.h>
#include <dom0_ops.h>

unsigned long *pgd;

unsigned long end_of_memory;
unsigned long memory_size;

bool debug_set_pte = false;
static bool debug_pf_count = false;
//static bool debug_exit_hook = false;
static bool debug_pte_clear = false;
int spte_count, irq_count, irq_0_count, pf_count, iret_count;

#define PD_BITS 22
#define PT_BITS 12
#define PT_MASK 0x3ff
typedef unsigned long long cycle_t;

static inline cycle_t get_cycles(void)
{
	cycle_t cycles;
	__asm__ __volatile__("rdtsc" : "=A" (cycles));
	return cycles;
}

void
vm_unmap_region(unsigned long start, unsigned long len)
{
	for(unsigned long i = start >> 12; i < (start + len) >> 12; i++) {
		int ret = HYPERVISOR_update_va_mapping(i, 0, 0);
		ASSERT(ret == 0);
	}
}

inline word_t *
get_pdir(void)
{
	return get_vcpu().get_pdir();
}

inline unsigned long *
get_pde(unsigned long *pgd_, unsigned long v_offset, unsigned long addr)
{
	unsigned long pde_phys = (mfn_to_pfn(pgd_[addr >> PD_BITS] >> 12) << 12);
	return (unsigned long *) (pde_phys + v_offset);
}

inline unsigned long *
_get_pte(unsigned long *pgd_, unsigned long v_offset, unsigned long addr)
{
	unsigned long *pt = get_pde(pgd_, v_offset, addr);
	return &pt[(addr >> PT_BITS) & PT_MASK];
}

inline unsigned long *
get_pte(unsigned long addr)
{
	return _get_pte((unsigned long *) get_pdir(), (unsigned long) get_vcpu().get_kernel_vaddr(), addr);
}

inline unsigned long
get_pte_m(unsigned long addr)
{
	unsigned long virt_pn = addr >> 12;
	return (((get_pdir()[virt_pn >> 10] >> 12) << 12) + ((virt_pn & 0x3ff) * 4));
}
inline unsigned long *
mach_to_virt(unsigned long mach)
{
	unsigned long mfn = mach >> PAGE_BITS;
	return (unsigned long*)phys_to_virt(mfn_to_pfn(mfn) << PAGE_BITS | (mach & 0xfff));
}


inline void get_pte_update(unsigned long addr, unsigned long *val, unsigned long *ptr)
{
	*ptr = get_pte_m(addr);
	*val = *mach_to_virt(*ptr);
	*ptr |= MMU_NORMAL_PT_UPDATE;
}


inline void make_readonly(unsigned long addr)
{
	unsigned long val, ptr;
	get_pte_update(addr, &val, &ptr);
	val &= (~2);
	queue_update(ptr, val);
}

inline unsigned long
get_mfn(unsigned long *pgd_, unsigned long v_offset, unsigned long addr)
{
	unsigned long *pte = _get_pte(pgd_, v_offset, addr);
	return *(pte) >> PT_BITS;
}

inline unsigned long
phys_to_mach(unsigned long phys)
{
	unsigned long pfn = phys >> PAGE_BITS;
	ASSERT(pfn == mfn_to_pfn(pfn_to_mfn(pfn)));

	return (pfn_to_mfn(pfn) << PAGE_BITS | (phys & 0xfff));
}


inline unsigned long
virt_to_mach(unsigned long virt)
{
	return phys_to_mach(virt_to_phys(virt));
}

void
mach_swap(long frame, long phys)
{
	int suc = 0;
	mmu_update_t req[2];
	unsigned long old = phys_to_machine_mapping[phys];

	req[0].ptr = frame << 12 | MMU_MACHPHYS_UPDATE;
	req[0].val = phys;

	req[1].ptr = old << 12 | MMU_MACHPHYS_UPDATE;
	req[1].val = machine_to_phys_mapping[frame];

	phys_to_machine_mapping[phys] = frame;
	phys_to_machine_mapping[machine_to_phys_mapping[frame]] = old;

	HYPERVISOR_mmu_update(req, 2, &suc);
	ASSERT(suc == 2);
}

word_t backend_phys_to_dma_hook( word_t phys )
{
    // VU: FIXME!!!!!
    //con << "backend_phys_to_dmahook: " << (void*)phys << " " << (void*)phys_to_mach(phys) << " \n";
    return phys_to_mach(phys);
}

word_t backend_dma_to_phys_hook( word_t dma )
{
	//con << "backenddmatophyshook\n";
    //con << "backend_dma_to_phys_hook: " << (void*)dma << " \n";
    return 0;
}




#define INCREMENT(pfn, pdir)                                                 \
    do {                                                                \
	    if (debug_pf_count) debug_vm << "INC " << (void*) (pfn << 12) << " " << pfn_pt_count(pfn) \
					 << " PDIR: " << (void*) pdir << " File: " __FILE__ \
            << ':' << __LINE__ << " Func: " << __func__  << '\n';      \
    } while(0)


#define DECREMENT(pfn, pdir)                                                 \
    do {                                                                \
	    if (debug_pf_count) debug_vm << "DEC " << (void*) (pfn << 12) << " " << pfn_pt_count(pfn) \
					 << " PDIR: " << (void*) pdir << " File: " __FILE__ \
            << ':' << __LINE__ << " Func: " << __func__  << '\n';      \
    } while(0)

void 
vm_init(unsigned long end_of_mem)
{
	mmu_update_t req;
	int suc;
	unsigned long *pde_mach;
	unsigned long cur_page; /* Simple page allocator */
	unsigned long virt_start = (unsigned long) &__executable_start;
	

	end_of_memory = align_up(end_of_mem, MB(4));
	/* Initialise phys->machine mapping table */
	phys_to_machine_mapping = (unsigned long*) start_info.mfn_list;

	/* Initialise startup page directory */
	pgd = (unsigned long*) start_info.pt_base;

	/* And the machine address for it too */
	pde_mach = (unsigned long *) (pfn_to_mfn(( ((unsigned long) pgd) - ((unsigned long) virt_start)) >> 12) << 12);

	/* Step 1 
	   move all the current memory out of the way so 
	   that we can make sure when we load the elf file
	   we don't overlap anything */

	/* We have a simple allocator to allocate from the top
	   of physical memory, since the OS is going to expect memory 
	   to start at zero */
	cur_page = start_info.nr_pages;

	/* Ensure we only have 4MB of VM, currently we can't handle
	   greater than 4MB */
	ASSERT(end_of_memory - virt_start <= MB(4));

	/* Find out where the page table for our 4MB region resides */
	unsigned long *pt = _get_pte(pgd, virt_start, virt_start);

	/* Now we change our physical mapping so that we reside in the top of
	   physical memory, not the bottom of physical memory */

	/* The reason we can't currently use the generic functions here is because we
	   screw up the phys->virtual offset thing. */
	for (unsigned long addr = (virt_start >> 12); addr < (end_of_memory >> 12); addr++) {
		unsigned long mach = pt[addr & 0x3ff] >> 12;
		mach_swap(mach, --cur_page);
	}

	/* 
	   Now lets 4MB P:0x0 -> V:0x0 -- we assume that the loaded
	   ELF takes up < 4MB this can be fixed later. For now we want
	   to make it easy so we only map one 4MB section. Mapping greater
	   than this would require some real code.
	 */
	/* First we install a pagetable entry for [0->4MB) */
	{
		unsigned long *pte = &pde_mach[0x00000000 >> PD_BITS];
		unsigned long new_frame = pfn_to_mfn(--cur_page) << 12;
		req.ptr = ((unsigned long) pte) | MMU_NORMAL_PT_UPDATE;
		req.val = new_frame | 3;
		HYPERVISOR_mmu_update(&req, 1, &suc);

		/* Then we install the page table entries */
		for (int i = 0; i < 1024; i++) {
			req.ptr = (new_frame + (4*i))| MMU_NORMAL_PT_UPDATE;
			req.val = (pfn_to_mfn(i) << 12) | 3;
			HYPERVISOR_mmu_update(&req, 1, &suc);
			ASSERT(suc == 1);
		}
	}


	{
		/* Verify that we have correctly mapped the region */
		char *load_address = (char*) 0x00000000;
		for (int i = 0; i < MB(4); i++) 
			load_address[i] = 'a';
		for (int i = 0; i < MB(4); i++)
			ASSERT (load_address[i] == 'a');
	}
	memory_size = (cur_page - 10) * 0x1000; /* 10 gives us 10 pages to use if needed */
}

void
backend_set_pte_hook(pgent_t *old_pgent, pgent_t new_val, int level)
{
	mmu_update_t req;
	int suc;
	unsigned long virtaddr = 0, newentry = 0, mfn_to_undo = 0;
	bool do_vm_update = false;
	spte_count++;
	if (! new_val.is_valid() && old_pgent->get_raw()) {
		if (debug_pte_clear)
			debug_vm << "Set PTE hook we clear: " << old_pgent << 
				" oldpgent: " << (void*) old_pgent->get_raw() <<
				" oldpgent: " << (void*) (mfn_to_pfn(old_pgent->get_raw() >> 12) << 12) <<
				" rawval: " << (void*) new_val.get_raw() << 
				" rawval&(1 << 10): " << (new_val.get_raw() & (1 << 10)) << 
				" level: " << level << "\n";
	}

	if (debug_set_pte) {
		debug_vm << "Set PTE hook: " << old_pgent << 
			" oldpgent: " << (void*) old_pgent->get_raw() <<
			" rawval=" << (void*) new_val.get_raw() << 
			" rawval&(1 << 10)=" << (new_val.get_raw() & (1 << 10)) << 
			" level=" << level << "\n";
	}

	cycle_t before = get_cycles();

	/* Is the page being updated even special? */
	if (pfn_pt_count(virt_to_phys((unsigned long) old_pgent) >> PAGE_BITS) == 0) {
		/* Then we don't really need to do anything yet */
		*old_pgent = new_val;
		return;
	}

	if (level == 0) {
		unsigned long *pdir = (unsigned long*) get_pdir();
		unsigned long virt_addr = phys_to_virt(new_val.get_address());

		{
			/* Debugging and checks */
			unsigned long pidx = (((unsigned long) old_pgent - ((unsigned long) pdir))) >> 2;
			if ((pidx << 22) > 0xbf000000){
				ASSERT(!"Shouldn't do this!\n");
				panic();
			}
		}

		if (! new_val.is_valid() && old_pgent->is_valid()) {
			if (pfn_pt_count(mfn_to_pfn(old_pgent->get_raw() >> 12)) == 1) {
				do_vm_update = true;
				mfn_to_undo = old_pgent->get_raw() >> 12;
				virtaddr = (phys_to_virt(mfn_to_pfn(old_pgent->get_address() >> PAGE_BITS) << PAGE_BITS) >> PAGE_BITS);
				newentry = (old_pgent->get_address() | 3);
			}
			pfn_pt_dec(mfn_to_pfn(old_pgent->get_raw() >> 12));
			/* Make the page writable again */
			DECREMENT(mfn_to_pfn(old_pgent->get_raw() >> 12), pdir);
		} else if (new_val.is_valid()) {
			pfn_pt_inc(new_val.get_raw() >> 12);
			INCREMENT(new_val.get_raw() >> 12, pdir);

			if (pfn_pt_count(new_val.get_raw() >> 12) == 1) {
				/* We update the pagetable page to be read-only if we have to */
				pgent_t *v_pte = (pgent_t *) get_pte(virt_addr);
				
				ASSERT(! v_pte->is_global());
				ASSERT(! new_val.is_xen_machine());
				
				pgent_t tmp = *v_pte;
				tmp.set_read_only();
				
				req.ptr = get_pte_m(virt_addr) | MMU_NORMAL_PT_UPDATE; 
				req.val = tmp.get_raw();
				HYPERVISOR_mmu_update(&req, 1, &suc);
				ASSERT(suc == 1);

				req.ptr = ((unsigned long) tmp.get_address()) | MMU_EXTENDED_COMMAND;
				req.val = MMUEXT_PIN_L1_TABLE;
				HYPERVISOR_mmu_update(&req, 1, &suc);
				ASSERT(suc == 1);

			}
		} 
	} else {
		/* If the destination is a page table, we better make it
		   read only */
		if (new_val.is_valid()) {
			unsigned long fn = new_val.get_address() >> PAGE_BITS;
			
			if (new_val.is_xen_machine()) {
				fn = mfn_to_pfn(fn);
			}
			
			if (fn >= (memory_size >> PAGE_BITS))
				{
					debug_vm << "Called from: " << __builtin_return_address(0) << "\n";
					debug_vm << "page > memory_size PTE hook=" << old_pgent << 
						" rawval=" << (void*) new_val.get_raw() << 
						" rawval&(1 << 10)=" << (new_val.get_raw() & (1 << 10)) << 
						" level=" << level << "\n";
					debug_vm << "# pages=" << (memory_size >> PAGE_BITS) << " " 
						 << "fn=" << fn << " new_val.get_raw()=" << (void*) new_val.get_raw() << "\n";
					
					mmu_update_t preq[2];
					preq[0].ptr = MMU_EXTENDED_COMMAND;
					preq[0].val = MMUEXT_SET_FOREIGNDOM | (DOMID_IO << 16);
					preq[1].ptr = virt_to_mach((unsigned long) old_pgent);
					preq[1].val = new_val.get_raw();
					
					if (HYPERVISOR_mmu_update(preq, 2, NULL) < 0)
						debug_vm << "remap physical failed\n";
					return;
				}
			
			if (fn >= (memory_size >> PAGE_BITS) || new_val.is_xen_machine()) 
				{
					debug_vm << "Set PTE hook=" << old_pgent << 
						" rawval=" << (void*) new_val.get_raw() << 
						" rawval&(1 << 10)=" << (new_val.get_raw() & (1 << 10)) << 
						" level=" << level << "\n";
					debug_vm << "# pages=" << (memory_size >> PAGE_BITS) << " " 
						 << "fn=" << fn << " new_val.get_raw()=" << (void*) new_val.get_raw() << "\n";
					if (new_val.is_xen_machine()) {
						debug_vm << "mfn=" << (new_val.get_address() >> PAGE_BITS) <<  " "
							 << (void*)machine_to_phys_mapping[0] << " "
							 << (void*)machine_to_phys_mapping[1024] << " "
							 << (void*)machine_to_phys_mapping[(new_val.get_address() >> PAGE_BITS)] << " "
							 << (void*)machine_to_phys_mapping[(new_val.get_address() >> PAGE_BITS)+1] << " "
							 << (void*)machine_to_phys_mapping[(new_val.get_address() >> PAGE_BITS)-1] << "\n";
					}
				}
			ASSERT(fn == mfn_to_pfn(pfn_to_mfn(fn)));
			ASSERT(fn < (memory_size >> PAGE_BITS));
			
			if (pfn_pt_count(fn) > 0) {
				new_val.set_read_only();
			}
		}
	}

	if (new_val.get_address() > 0xa0000 && new_val.get_address() < 0x100000) {
		/* This is a *HACK* */
		new_val.set_xen_machine();
	}

	if (new_val.is_valid() && (! new_val.is_xen_machine())) {
		new_val.set_address(phys_to_mach(new_val.get_address())); 
		new_val.set_xen_machine();
	} 

	req.ptr = virt_to_mach((unsigned long) old_pgent) | MMU_NORMAL_PT_UPDATE;
	req.val = new_val.get_raw();
	HYPERVISOR_mmu_update(&req, 1, &suc);
	ASSERT(suc == 1);

	if (do_vm_update) {
		req.ptr = ((unsigned long) mfn_to_undo << 12) | MMU_EXTENDED_COMMAND;
		req.val = MMUEXT_UNPIN_TABLE;
		HYPERVISOR_mmu_update(&req, 1, &suc);
		ASSERT(suc == 1);

		suc = HYPERVISOR_update_va_mapping(virtaddr, newentry, 0);
		ASSERT(suc == 0);

	}

	cycle_t after = get_cycles();
	if (debug_set_pte)
		con << "took " << after-before << "\n";
	//if (debug_set_pte) 
	//	debug_vm << "new_val: " << (void*) new_val.get_raw() << " pgent: " << (void*) old_pgent->get_raw() << "\n";

	//debug_set_pte = false;
}

extern bool exit_hook_called;
extern unsigned long exit_old_pdir;
extern unsigned long the_old_pdir_paddr;

unsigned long
get_pte2(unsigned long addr, unsigned long offset)
{
	unsigned long *pdir = (unsigned long *) (the_old_pdir_paddr + offset);
	unsigned long pdeval;
	unsigned long *pte;
	unsigned long pteval;
	pdeval = pdir[addr >> 22];
	pte = (unsigned long *) ((mfn_to_pfn(pdeval >> 12) << 12) + offset);
	pteval = pte[(addr >> 12) & 0xfff];
	return pteval;
}


//static int switch_count = 0;

cycle_t start_cycles, end_cycles;

void
switch_as(unsigned long offset)
{
	unsigned int i;
	//mmu_update_t req;
	//int suc;
	int the_frame;
	unsigned long *foo;
	//int count = 0;
	the_frame = get_cpu().cr3.get_pdir_addr() >> 12;
	foo = (unsigned long*) ((the_frame << 12) + offset);

#if 0
	switch_count++;
	
	if (switch_count > 2000 ) {
		end_cycles = get_cycles();
		con << "switch: " << foo << 
			" pte_count: " << spte_count <<
			" IRQ_count: " << irq_count <<
			" IRQ_count: " << irq_0_count <<
			" PF_count: " << pf_count << 
			" cycles: " << end_cycles - start_cycles << 
			" `IRET_U_count: " << iret_count << "\n";
		irq_0_count = spte_count = irq_count = pf_count = iret_count = 0;
		start_cycles = get_cycles();
		debug_set_pte = true;
	}
#endif
	if (pfn_pt_count(the_frame)) {
		queue_update(((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND,
			     MMUEXT_NEW_BASEPTR);
		
		flush_page_update_queue();
		return ;
	}
	pfn_pt_inc(the_frame);
	INCREMENT(the_frame, (the_frame << 12));

	for (i=0; i < 0xbf000000U >> 22; i++) {
		if (foo[i] & 1) {
			unsigned long foo_i = foo[i];
			if (foo[i] & (1 << 10)) {
				//con << "This is a magic thing " << i << " " << (void*) foo_i << "\n";
				foo_i = (mfn_to_pfn(foo_i >> 12) << 12) | (foo_i & 0x7ff);
			} else {
				foo[i] = (pfn_to_mfn(foo[i] >> 12) << 12) | (foo[i] & 0x7ff) | (1 << 10);
			}
			pfn_pt_inc(foo_i >> 12);
			INCREMENT(foo_i >> 12, (the_frame << 12));
			if (pfn_pt_count(foo_i >> 12) == 1) {
				unsigned long * pt = (unsigned long*) ((foo_i >> 12 << 12) + offset);
				/* Need to convert the page into a proper pagetable */
				for(int j=0; j < 1024; j++) {
					if (pt[j] & 0x1) {
						unsigned long pt_j = pt[j];
						if (pt_j & (1 << 10)) {
							//con << "Magic :(\n";
						} else {
							if (pfn_pt_count(pt_j >> 12) == 0) {
								pt[j] = 
									(pfn_to_mfn(pt_j >> 12) << 12) | (pt_j & 0x6ff) | (1 << 10);
							} else {
								pt[j] = 
									(pfn_to_mfn(pt_j >> 12) << 12) | (pt_j & 0x6fd) | (1 << 10);
							}
						}
					}
				}
				/* Pin */
				make_readonly(foo_i+offset);
				queue_update(((unsigned long) pfn_to_mfn(foo_i >> 12) << 12) | MMU_EXTENDED_COMMAND,
					     (unsigned long) MMUEXT_PIN_L1_TABLE);
			} 
		}
	}
	for (i = (0xbf000000U >> 22); i < 1024; i++) {
		foo[i] = pgd[i];
	}
	make_readonly((unsigned long)foo);
	queue_update(((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND,
		     (unsigned long) MMUEXT_PIN_L2_TABLE);
	queue_update(((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND,
		       MMUEXT_NEW_BASEPTR);


#if 0
	cycle_t before = get_cycles();
	flush_page_update_queue();
	cycle_t after = get_cycles();
	count++;
	con << "Full switch: " << (void*)(the_frame << 12) << " " << count << " " << (after - before) << "\n";
#endif		
	return;
}

void
switch_as_initial(unsigned long offset)
{
	unsigned int i;
	mmu_update_t req;
	int ret;
	int suc;
	unsigned long test_val;
	int the_frame;
	unsigned long *foo;
	int count = 0;
	the_frame = get_cpu().cr3.get_pdir_addr() >> 12;
	foo = (unsigned long*) ((the_frame << 12) + offset);

	cycle_t before = get_cycles();
	con << "Swithc as : " << offset << "\n";
	if (pfn_pt_count(the_frame)) {
		/* Now we are allowed to update it */
		req.ptr = ((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND;
		req.val = MMUEXT_NEW_BASEPTR;
		HYPERVISOR_mmu_update(&req, 1, &suc);
		ASSERT(suc == 1);
		return ;
	}

	if (offset > 0) {
		unsigned long pteval;
		pteval = get_pte2((unsigned long) foo, offset);
		//con << "pteval: " << (void*) pteval << "\n";
	}
	pfn_pt_inc(the_frame);
	INCREMENT(the_frame, (the_frame << 12));
	for (i=0; i < 0xbf000000U >> 22; i++) {
		if (foo[i] & 1) {
			unsigned long foo_i = foo[i];
			if (foo[i] & (1 << 10)) {
				//con << "This is a magic thing " << i << " " << (void*) foo_i << "\n";
				foo_i = (mfn_to_pfn(foo_i >> 12) << 12) | (foo_i & 0x7ff);
			} else {
				foo[i] = (pfn_to_mfn(foo[i] >> 12) << 12) | (foo[i] & 0x7ff) | (1 << 10);
			}
			pfn_pt_inc(foo_i >> 12);
			INCREMENT(foo_i >> 12, (the_frame << 12));
			if (pfn_pt_count(foo_i >> 12) == 1) {
				unsigned long * pt = (unsigned long*) ((foo_i >> 12 << 12) + offset);
				/* Need to convert the page into a proper pagetable */
				for(int j=0; j < 1024; j++) {
					if (pt[j] & 0x1) {
						unsigned long pt_j = pt[j];
						if (pt_j & (1 << 10)) {
							//con << "Magic :(\n";
						} else {
							if (pfn_pt_count(pt_j >> 12) == 0) {
								pt[j] = 
									(pfn_to_mfn(pt_j >> 12) << 12) | (pt_j & 0x6ff) | (1 << 10);
							} else {
								pt[j] = 
									(pfn_to_mfn(pt_j >> 12) << 12) | (pt_j & 0x6fd) | (1 << 10);
							}
						}
					}
				}
				ret = HYPERVISOR_update_va_mapping((foo_i + offset) >> 12, (pfn_to_mfn(foo_i >> 12) << 12)| 1, 0);
				queue_update(((unsigned long) pfn_to_mfn(foo_i >> 12) << 12) | MMU_EXTENDED_COMMAND,
					     (unsigned long) MMUEXT_PIN_L1_TABLE);
				flush_page_update_queue();
				ASSERT(ret == 0);
				count++;
			} else {
				//if (offset > 0)
				//	con << "No i: " << i  << " foo[i]: " << (void*)foo_i << "\n";
			}
		}
	}
	for (i = (0xbf000000U >> 22); i < 1024; i++) {
		foo[i] = pgd[i];
	}

	test_val = foo[526];


	/* Unmap it  */
	if (offset > 0) {
		//con << "??????\n";
		//con << "Updating foo bar: " << (void*) get_pte((unsigned long) foo, offset) << "\n";
		ret = HYPERVISOR_update_va_mapping(((unsigned long)foo) >> 12,  (pfn_to_mfn(the_frame) << 12) | (1), 0);
		ASSERT(ret == 0);
		count++;
	} else {
		ret = HYPERVISOR_update_va_mapping(((unsigned long)foo) >> 12, 0, 0);
		ASSERT(ret == 0);
		count++;
	}

	/* First lets unmap the first 4MB */
	if (offset == 0) {
		for (i=0; i< 1024; i++) {
			ret = HYPERVISOR_update_va_mapping(i, 0, 0);
			ASSERT(ret == 0);
			count++;
		}
	}

	/* Now we are allowed to update it */
	//cycle_t before = get_cycles();

	req.ptr = ((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND;
	req.val = MMUEXT_PIN_L2_TABLE;
	HYPERVISOR_mmu_update(&req, 1, &suc);
	ASSERT(suc == 1);

	req.ptr = ((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND;
	req.val = MMUEXT_NEW_BASEPTR;
	HYPERVISOR_mmu_update(&req, 1, &suc);
	ASSERT(suc == 1);
	cycle_t after = get_cycles();
	count++;
	con << "Full switch: " << (void*)(the_frame << 12) << " " << count << " " << (after - before) << "\n";
	return;
}



pgent_t
backend_pte_get_and_clear_hook( pgent_t *pgent )
{
	//int suc;
	//mmu_update_t req;
	pgent_t old_pgent = *pgent; // Make a copy.
	unsigned long foo = (unsigned long)pgent;
	if (debug_set_pte)
		con << "get and clear " << pgent << " " << (void*) pgent->get_raw() << "\n";
	if (pfn_pt_count(virt_to_phys(foo) >> 12) > 0) {
		queue_update(virt_to_mach((unsigned long) foo) | MMU_NORMAL_PT_UPDATE, 0);
		flush_page_update_queue();
	} else {
		pgent->clear();
	}
	return old_pgent;
}

word_t
backend_pte_test_and_clear_hook( pgent_t *pgent, word_t bits )
{
	int suc;
	mmu_update_t req;
	unsigned long foo = (unsigned long)pgent;
	word_t ret = pgent->x.raw & bits;
	if (debug_set_pte)
		con << "test and clear " << pgent << "\n";

	ASSERT(pgent);

	// quick exit if none of the bits is not set
	if (!ret)
		return 0;

	if (pfn_pt_count(virt_to_phys(foo) >> 12) > 0) {
		pgent_t new_val = *pgent;
		ASSERT(new_val.is_xen_machine());
		// clear bit
		new_val.x.raw &= ~bits;
		req.ptr = virt_to_mach((unsigned long) foo) | MMU_NORMAL_PT_UPDATE;
		req.val = new_val.get_raw();
		HYPERVISOR_mmu_update(&req, 1, &suc);
		ASSERT(suc == 1);
	} else {
		pgent->x.raw &= ~bits;
	}
	return ret;
}

unsigned long
backend_read_pte_hook( pgent_t pgent_)
{
	unsigned long pgent = pgent_.get_raw();
	//if (debug_set_pte)
	//	con << "read_pte_hook " << (void*) pgent << " " << level << "\n";
	//if (level == 0)
	if (pgent_.is_valid() && pgent_.is_xen_machine()) {
		return ((mfn_to_pfn(pgent >> 12) << 12) | (pgent & 0xfff) ) & ~ ( 1 << 10);
	} else {
		return pgent;
	}
}

void
backend_free_pgd_hook( unsigned long  free_pgd)
{
    int the_frame;
    unsigned long virtaddr = 0, newentry = 0;
    unsigned long *foo;
    unsigned long offset = 0x80000000;
    unsigned int i;
    int suc;
    mmu_update_t req;
    free_pgd -= offset;
    //con << "Free pgd hook called: " << (void*) free_pgd << "\n";
    int count = 0;
    //cycle_t before = get_cycles();
    
    the_frame = free_pgd >> 12;
    foo = (unsigned long*) ((the_frame << 12) + offset);

    /* Special! */
    for (i=0; i < 0xbf000000U >> 22; i++) {
	    if (foo[i] & 1) {
		    unsigned long foo_i = foo[i];
		    foo_i = (mfn_to_pfn(foo_i >> 12) << 12) | (foo_i & 0x7ff);
		    if (pfn_pt_count(foo_i>> 12) == 1) {

			    req.ptr = ((unsigned long) pfn_to_mfn(foo_i >> 12) << 12) | MMU_EXTENDED_COMMAND;
			    req.val = MMUEXT_UNPIN_TABLE;
			    HYPERVISOR_mmu_update(&req, 1, &suc);
			    ASSERT(suc == 1);

			    virtaddr = (phys_to_virt(foo_i) >> PAGE_BITS);
			    newentry = (pfn_to_mfn(foo_i >> PAGE_BITS) << PAGE_BITS) | 3;
			    suc = HYPERVISOR_update_va_mapping(virtaddr, newentry, 0);
			    ASSERT(suc == 0);
			    count ++;
		    }
		    pfn_pt_dec(foo_i >> 12);
		    DECREMENT(foo_i >> 12, exit_old_pdir);
	    }
    }

    if (pfn_pt_count(the_frame) == 1) {

	    req.ptr = ((unsigned long) pfn_to_mfn(the_frame) << 12) | MMU_EXTENDED_COMMAND;
	    req.val = MMUEXT_UNPIN_TABLE;
	    HYPERVISOR_mmu_update(&req, 1, &suc);
	    ASSERT(suc == 1);
	    count ++;
	    
	    virtaddr = (phys_to_virt(the_frame << PAGE_BITS) >> PAGE_BITS);
	    newentry = (pfn_to_mfn(the_frame) << PAGE_BITS) | 3;
	    suc = HYPERVISOR_update_va_mapping(virtaddr, newentry, UVMF_INVLPG);
	    ASSERT(suc == 0);
	    count ++;
    }

    pfn_pt_dec(the_frame);
    DECREMENT(the_frame, exit_old_pdir);
    //cycle_t after = get_cycles();
    //con << "free took: " << (after - before) << " " << pfn_pt_count(the_frame) << " " << (void*) foo  << "\n";
    //con << "free pgd done\n";
}
