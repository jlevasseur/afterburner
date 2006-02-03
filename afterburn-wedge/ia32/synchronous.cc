/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe and
 *                      National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/ia32/synchronous.cc
 * Description:   CPU emulation executed synchronously to the 
 *                instruction stream.
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
 * $Id: synchronous.cc,v 1.4 2005/04/13 14:27:10 joshua Exp $
 *
 ********************************************************************/

#include INC_ARCH(page.h)
#include INC_ARCH(cpu.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)

#include <memory.h>
#include <templates.h>

static const bool debug_user_pfault=0;
static const bool debug_user_except=0;
static const bool debug_user_int=1;
static const bool debug_kernel_sync_vector=0;
static const bool debug_superpages=0;


static void NORETURN
deliver_ia32_user_vector( cpu_t &cpu, word_t vector, 
	bool use_error_code, word_t error_code, word_t ip )
{
    ASSERT( vector <= cpu.idtr.get_total_gates() );
    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    flags_t old_flags = cpu.flags;
    old_flags.x.fields.fi = 1;  // Interrupts are usually enabled at user.
    cpu.flags.prepare_for_interrupt();
    // Note: we leave interrupts disabled.

    tss_t *tss = cpu.get_tss();
    if( debug_user_pfault )
	con << "tss esp0 " << (void *)tss->esp0
	    << ", tss ss0 " << tss->ss0 << '\n';

    u16_t old_cs = cpu.cs;
    u16_t old_ss = cpu.ss;
    cpu.cs = gate.get_segment_selector();
    cpu.ss = tss->ss0;

    if( gate.is_trap() )
	cpu.restore_interrupts( true );

    if( use_error_code ) 
    {
	__asm__ __volatile__ (
    		"movl	%6, %%esp ;" // Switch stack
    		"pushl	%5 ;"	// User ss
    		"pushl	$0 ;"	// User sp
    		"pushl	%0 ;"	// Flags
    		"pushl	%1 ;"	// CS
    		"pushl	%2 ;"	// User ip
    		"pushl	%3 ;"	// Error code
    		"jmp	*%4 ;"	// Activate gate
    		:
    		: "r"(old_flags.x.raw), "r"((u32_t)old_cs), "r"(ip),
  		  "r"(error_code), "r"(gate.get_offset()),
  		  "r"((u32_t)old_ss), "r"(tss->esp0) );
    }
    else {
    	__asm__ __volatile__ (
    		"movl	%5, %%esp ;" // Switch stack
    		"pushl	%4 ;"	// User ss
    		"pushl	$0 ;"	// User sp
    		"pushl	%0 ;"	// Flags
    		"pushl	%1 ;"	// CS
    		"pushl	%2 ;"	// User ip
    		"jmp	*%3 ;"	// Activate gate
    		:
    		: "r"(old_flags.x.raw), "r"((u32_t)old_cs), "r"(ip),
  		  "r"(gate.get_offset()),
  		  "r"((u32_t)old_ss), "r"(tss->esp0) );
    }

    panic();
}

static void NORETURN
deliver_ia32_user_vector( word_t vector, thread_info_t *thread_info )
{
    cpu_t &cpu = get_cpu();
    tss_t *tss = cpu.get_tss();

    ASSERT( vector <= cpu.idtr.get_total_gates() );
    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    flags_t old_flags = cpu.flags;
    old_flags.x.fields.fi = 1;  // Interrupts are usually enabled at user.
    cpu.flags.prepare_for_interrupt();
    // Note: we leave interrupts disabled.

    u16_t old_cs = cpu.cs;
    u16_t old_ss = cpu.ss;
    cpu.cs = gate.get_segment_selector();
    cpu.ss = tss->ss0;
    /*
    thread_info->mr_save.exc_msg.eflags &= flags_user_mask;
    thread_info->mr_save.exc_msg.eflags |= (old_flags.x.raw & ~flags_user_mask);
    */
    thread_info->update_eflags(old_flags);
    if( gate.is_trap() )
	cpu.restore_interrupts( true );

    __asm__ __volatile__ (
	    "movl	%4, %%esp ;"	// Switch stack
	    "pushl	%3 ;"		// User ss
	    "pushl	" ESP_OFFSET "(%%eax) ;"	// User sp
	    "pushl	" EFLAGS_OFFSET "(%%eax) ;"	// Flags
	    "pushl	%1 ;"		// CS
	    "pushl	" EIP_OFFSET "(%%eax) ;"	// User ip
	    "pushl	%2 ;"		// Activation point
	    "movl	" EDI_OFFSET "(%%eax), %%edi ;"
	    "movl	" ESI_OFFSET "(%%eax), %%esi ;"
	    "movl	" EBP_OFFSET "(%%eax), %%ebp ;"
	    "movl	" EBX_OFFSET "(%%eax), %%ebx ;"
	    "movl	" EDX_OFFSET "(%%eax), %%edx ;"
	    "movl	" ECX_OFFSET "(%%eax), %%ecx ;"
	    "movl	" EAX_OFFSET "(%%eax), %%eax ;"
	    "ret ;"	// Activate gate
	    :
	    : "A"(thread_info->save_base()), "r"((u32_t)old_cs), 
	      "r"(gate.get_offset()), "r"((u32_t)old_ss), "r"(tss->esp0) );

    panic();
}

void NORETURN
backend_handle_user_vector( word_t vector )
{
    deliver_ia32_user_vector( get_cpu(), vector, false, 0, 0 );
}

pgent_t *
backend_resolve_addr( word_t user_vaddr, word_t &kernel_vaddr )
{
    vcpu_t &vcpu = get_vcpu();
    word_t kernel_start = vcpu.get_kernel_vaddr();

    pgent_t *pdir = (pgent_t *)(vcpu.cpu.cr3.get_pdir_addr() + kernel_start);
    pdir = &pdir[ pgent_t::get_pdir_idx(user_vaddr) ];
    if( !pdir->is_valid() )
	return NULL;
    if( pdir->is_superpage() && vcpu.cpu.cr4.is_pse_enabled() ) {
	kernel_vaddr = (pdir->get_address() & SUPERPAGE_MASK)
	    + (user_vaddr & ~SUPERPAGE_MASK) + kernel_start;
	return pdir;
    }

    pgent_t *ptab = (pgent_t *)(pdir->get_address() + kernel_start);
    pgent_t *pgent = &ptab[ pgent_t::get_ptab_idx(user_vaddr) ];
    if( !pgent->is_valid() )
	return NULL;
    kernel_vaddr = pgent->get_address() + (user_vaddr & ~PAGE_MASK) 
	+ kernel_start;
    return pgent;
}


void NORETURN
backend_handle_user_exception( thread_info_t *thread_info )
{
    if( debug_user_except )
	con << "User exception at ip " 
	    << (void *)thread_info->get_eip()
	    << ", sp " << (void *)thread_info->get_esp() << '\n';

    word_t instr_addr;
    pgent_t *pgent;
    pgent = backend_resolve_addr( thread_info->get_eip(), instr_addr);
    if( !pgent || pgent->is_kernel() )
    {
	con << "Unresolved user address " 
	    << (void *)thread_info->get_eip() << '\n';
	panic();
    }

    u8_t *instr = (u8_t *)instr_addr;
    if( instr[0] == 0xcd && instr[1] >= 32 )
    {
	if( debug_user_int ) {
#if 0
	    if( thread_info->mr_save.exc_msg.eax == 3 )
		con << "> read " << thread_info->mr_save.exc_msg.ebx
		    << " " << (void *)thread_info->mr_save.exc_msg.ecx
		    << " " << thread_info->mr_save.exc_msg.edx << '\n';
	    else if( thread_info->mr_save.exc_msg.eax == 4 )
		con << "> write " << thread_info->mr_save.exc_msg.ebx
		    << " " << (void *)thread_info->mr_save.exc_msg.ecx
		    << " " << thread_info->mr_save.exc_msg.edx << '\n';
	    else if( thread_info->mr_save.exc_msg.eax == 5 )
		con << "> open " << (void *)thread_info->mr_save.exc_msg.ebx
		    << '\n';
	    else if( thread_info->mr_save.exc_msg.eax == 90 ) {
		word_t *args = (word_t *)thread_info->mr_save.exc_msg.ebx;
		con << "> mmap fd " << args[4] << ", len " << args[1]
		    << ", addr " << args[0] << ", offset " << args[5] << '\n';
	    }
	    else if( thread_info->mr_save.exc_msg.eax == 91 )
		L4_KDB_Enter("munmap");
	    else  if( thread_info->mr_save.exc_msg.eax == 2 ) {
		con << "> fork\n";
	    }
	    else if( thread_info->mr_save.exc_msg.eax == 120 ) {
		con << "> clone\n";
	    }
#endif
#if 0
	    //else
	    con << "syscall " << (void *)thread_info->mr_save.exc_msg.eax
		<< ", ebx " << (void *)thread_info->mr_save.exc_msg.ebx
		<< ", ecx " << (void *)thread_info->mr_save.exc_msg.ecx
		<< ", edx " << (void *)thread_info->mr_save.exc_msg.edx << '\n';
#endif
	}
	thread_info->incr_eip(2); // next instruction
	deliver_ia32_user_vector( instr[1], thread_info );
    }
    else
	con << "Unsupported exception from user-level"
	    << ", user ip " << (void *)thread_info->get_eip()
	    << ", TID " << thread_info->get_id() << '\n';

    panic();
}

bool 
backend_handle_user_pagefault(
	word_t page_dir_paddr,
	word_t fault_addr, word_t fault_ip, word_t fault_rwx,
	word_t & map_addr, word_t & map_bits, word_t & map_rwx )
{
    vcpu_t &vcpu = get_vcpu();
    cpu_t &cpu = vcpu.cpu;
    word_t link_addr = vcpu.get_kernel_vaddr();

    map_rwx = 7;

    pgent_t *pdir = (pgent_t *)(page_dir_paddr + link_addr);
    pdir = &pdir[ pgent_t::get_pdir_idx(fault_addr) ];
    if( !pdir->is_valid() ) {
	if( debug_user_pfault )
	    con << "user pdir not present\n";
	goto not_present;
    }

    if( pdir->is_superpage() && cpu.cr4.is_pse_enabled() ) {
	map_addr = (pdir->get_address() & SUPERPAGE_MASK) + 
	    (fault_addr & ~SUPERPAGE_MASK);
	if( !pdir->is_writable() )
	    map_rwx = 5;
	map_bits = SUPERPAGE_BITS;
	if( debug_superpages )
	    con << "user super page\n";
    }
    else 
    {
	pgent_t *ptab = (pgent_t *)(pdir->get_address() + link_addr);
	pgent_t *pgent = &ptab[ pgent_t::get_ptab_idx(fault_addr) ];
	if( !pgent->is_valid() )
	    goto not_present;
	map_addr = pgent->get_address() + (fault_addr & ~PAGE_MASK);
	if( !pgent->is_writable() )
	    map_rwx = 5;
	map_bits = PAGE_BITS;
    }

    // TODO: figure out whether the mapping given to user actuall exists
    // in kernel space.  If not, we have to grant the page.
    if( map_addr < link_addr )
	map_addr += link_addr;

    if( (map_rwx & fault_rwx) != fault_rwx )
	goto permissions_fault;

    // TODO: only do this if the mapping exists in kernel space too.
    *(volatile word_t *)map_addr;
    return true;

not_present:
    if( page_dir_paddr != cpu.cr3.get_pdir_addr() )
	return false;	// We have to delay fault delivery.
    cpu.cr2 = fault_addr;
    if( debug_user_pfault )
	con << "page not present, fault addr " << (void *)fault_addr
	    << ", ip " << (void *)fault_ip << '\n';
    deliver_ia32_user_vector( cpu, 14, true, 4 | ((fault_rwx & 2) | 0), fault_ip );
    goto unhandled;

permissions_fault:
    if( page_dir_paddr != cpu.cr3.get_pdir_addr() )
	return false;	// We have to delay fault delivery.
    cpu.cr2 = fault_addr;
    if( debug_user_pfault )
	con << "Delivering page fault for addr " << (void *)fault_addr
	    << ", permissions " << fault_rwx 
	    << ", ip " << (void *)fault_ip << '\n';
    deliver_ia32_user_vector( cpu, 14, true, 4 | ((fault_rwx & 2) | 1), fault_ip );
    goto unhandled;

unhandled:
    con << "Unhandled page permissions fault, fault addr " << (void *)fault_addr
	<< ", ip " << (void *)fault_ip << ", fault rwx " << fault_rwx << '\n';
    panic();
    return false;
}


void backend_sync_deliver_vector( word_t vector, bool old_int_state, bool use_error_code, word_t error_code )
    // Must be called with interrupts disabled.
{
    cpu_t &cpu = get_cpu();

    if( vector > cpu.idtr.get_total_gates() ) {
	con << "No IDT entry for vector " << vector << '\n';
	return;
    }

    gate_t *idt = cpu.idtr.get_gate_table();
    gate_t &gate = idt[ vector ];

    ASSERT( gate.is_trap() || gate.is_interrupt() );
    ASSERT( gate.is_present() );
    ASSERT( gate.is_32bit() );

    if( debug_kernel_sync_vector )
	con << "Delivering vector " << vector
	    << ", handler ip " << (void *)gate.get_offset() << '\n';

    flags_t old_flags = cpu.flags;
    old_flags.x.fields.fi = old_int_state;
    cpu.flags.prepare_for_interrupt();
    // Note: we leave interrupts disabled.

    u16_t old_cs = cpu.cs;
    cpu.cs = gate.get_segment_selector();

    if( use_error_code )
    {
    	__asm__ __volatile__ (
    		"pushl	%0 ;"
    		"pushl	%1 ;"
    		"pushl	$1f ;"
		"pushl	%3 ;"
    		"jmp	*%2 ;"
    		"1:"
    		:
    		: "r"(old_flags.x.raw), "r"((u32_t)old_cs),
		  "r"(gate.get_offset()), "r"(error_code)
    		: "flags", "memory" );
    }
    else
    {
	__asm__ __volatile__ (
    		"pushl	%0 ;"
    		"pushl	%1 ;"
    		"pushl	$1f ;"
    		"jmp	*%2 ;"
    		"1:"
    		:
    		: "r"(old_flags.x.raw), "r"((u32_t)old_cs),
		  "r"(gate.get_offset())
    		: "flags", "memory" );
    }

    if( debug_kernel_sync_vector )
	con << "Finished synchronous vector delivery.\n";
}

void
sync_deliver_page_not_present( word_t fault_addr, word_t fault_rwx )
{
    cpu_t &cpu = get_cpu();
    bool int_save = cpu.disable_interrupts();
    cpu.cr2 = fault_addr;
    backend_sync_deliver_vector( 14, int_save, 
	    true, 4 | ((fault_rwx & 2) | 0) );
    cpu.restore_interrupts( int_save, true );
}

void
sync_deliver_page_permissions_fault( word_t fault_addr, word_t fault_rwx )
{
    cpu_t &cpu = get_cpu();
    bool int_save = cpu.disable_interrupts();
    cpu.cr2 = fault_addr;
    backend_sync_deliver_vector( 14, int_save, 
	    true, 4 | ((fault_rwx & 2) | 1) );
    cpu.restore_interrupts( int_save, true );
}


INLINE u32_t word_text( char a, char b, char c, char d )
{
    return (a)<<0 | (b)<<8 | (c)<<16 | (d)<<24;
}


void backend_cpuid_override( 
	u32_t func, u32_t max_basic, u32_t max_extended,
	frame_t *regs )
{
    if( (func > max_basic) && (func < 0x80000000) )
	func = max_basic;
    if( func > max_extended )
	func = max_extended;

    if( func <= max_basic )
    {
	switch( func )
	{
	    case 1:
		bit_clear( 3, regs->x.fields.ecx ); // Disable monitor/mwait.
		if (! get_vcpu().pse_supported()) 
			bit_clear( 3, regs->x.fields.edx ); // Disable pse
		if (! get_vcpu().pge_supported()) 
			bit_clear( 13, regs->x.fields.edx ); // Disable pge
		break;
	}
    }
    else {
	switch( func )
	{
	    case 0x80000002:
		regs->x.fields.eax = word_text('L','4','K','a');
		regs->x.fields.ebx = word_text(':',':','P','i');
		regs->x.fields.ecx = word_text('s','t','a','c');
		regs->x.fields.edx = word_text('h','i','o',' ');
		break;
	    case 0x80000003:
		regs->x.fields.eax = word_text('(','h','t','t');
		regs->x.fields.ebx = word_text('p',':','/','/');
		regs->x.fields.ecx = word_text('l','4','k','a');
		regs->x.fields.edx = word_text('.','o','r','g');
		break;
	    case 0x80000004:
		regs->x.fields.eax = word_text(')',0,0,0);
		break;
	}
    }
}

#ifndef CONFIG_WEDGE_XEN
static word_t
flush_page( vcpu_t &vcpu, word_t addr, word_t bits, word_t permissions=Burn_FullyAccessible )
{
#if defined(CONFIG_WEDGE_STATIC)
    // TODO: remove when pistachio stops unmapping the UTCB
    if( (L4_MyLocalId().raw >= addr) &&
	    (L4_MyLocalId().raw < (addr + PAGEDIR_SIZE)) )
	return 0;
#endif

    bool int_save = false;
    int_save = vcpu.cpu.disable_interrupts();
    word_t result = backend_sync_flush_page(addr, bits, permissions); //L4_Fpage_t result = L4_Flush( L4_FpageLog2(addr, bits) + permissions );
    vcpu.cpu.restore_interrupts( int_save, true );

    return result; //L4_Rights(result);
}
#endif
static word_t
unmap_page( vcpu_t &vcpu, word_t addr, word_t bits, word_t permissions )
{
    bool int_save = false;
    int_save = vcpu.cpu.disable_interrupts();
    word_t result = backend_sync_unmap_page(addr, bits, permissions); //L4_Fpage_t result = L4_Flush( L4_FpageLog2(addr, bits) + permissions );
    //    L4_Fpage_t result = L4_UnmapFpage( L4_FpageLog2(addr, bits) + permissions );
    vcpu.cpu.restore_interrupts( int_save, true );

    return result; //L4_Rights(result);
}

#if 0
static void
flush_ptab( vcpu_t &vcpu, pgent_t *ptab )
{
    word_t old_mapping;
    for( word_t idx = 0; idx < (PAGE_SIZE/sizeof(word_t)); idx++ )
    {
	pgent_t &ptab_entry = ptab[idx];
	if( !ptab_entry.is_valid() )
	    continue;
	old_mapping = ptab_entry.get_address() + vcpu.get_kernel_vaddr();
	flush_page( vcpu, old_mapping, PAGE_BITS );
    }
}
#endif

#ifndef CONFIG_WEDGE_XEN
static void
unmap_ptab( vcpu_t &vcpu, pgent_t *ptab )
{
    word_t old_mapping;
    for( word_t idx = 0; idx < (PAGE_SIZE/sizeof(word_t)); idx++ )
    {
	pgent_t &ptab_entry = ptab[idx];
	if( !ptab_entry.is_valid() )
	    continue;
	old_mapping = ptab_entry.get_address() + vcpu.get_kernel_vaddr();
	unmap_page( vcpu, old_mapping, PAGE_BITS, Burn_FullyAccessible );
    }
}
#endif
#if defined(CONFIG_GUEST_PTE_HOOK) 
#ifndef CONFIG_WEDGE_XEN
void backend_set_pte_hook( pgent_t *old_pgent, pgent_t new_val, int level )
{
    vcpu_t &vcpu = get_vcpu();
    word_t old_mapping;

    if( old_pgent->is_valid() ) {
	if( level ) {
	    // Unmap old ptab mapping.
	    old_mapping = old_pgent->get_address() + vcpu.get_kernel_vaddr();
	    unmap_page( vcpu, old_mapping, PAGE_BITS, Burn_FullyAccessible );
	}
	else if( old_pgent->is_superpage() && vcpu.cpu.cr4.is_pse_enabled() ) {
	    // Unmap super page.
	    old_mapping = old_pgent->get_address() + vcpu.get_kernel_vaddr();
    	    unmap_page( vcpu, old_mapping, SUPERPAGE_BITS, Burn_FullyAccessible );
	    if( debug_superpages )
		con << "flush super page\n";
	}
	else {
	    // Unmap entire page table.
	    pgent_t *ptab = (pgent_t *)
		(old_pgent->get_address() + vcpu.get_kernel_vaddr());
	    unmap_ptab( vcpu, ptab );
	}
    }

    *old_pgent = new_val;
}
#endif
#ifndef CONFIG_WEDGE_XEN
void
backend_pte_wrprotect_hook( pgent_t *pgent )
{
    if( !pgent->is_valid() )
	return;

    vcpu_t &vcpu = get_vcpu();
    word_t mapping = pgent->get_address() + vcpu.get_kernel_vaddr();

    // Assuming a 4KB page entry.
    word_t rwx = unmap_page( vcpu, mapping, PAGE_BITS, Burn_Writable );

    if( rwx & Burn_Writable )
	pgent->set_dirty(1);
    if( rwx & (Burn_Readable | Burn_eXecutable) )
	pgent->set_accessed(1);
}
#endif /*CONFIG_WEDGE_XEN*/

void
backend_pte_access_check_hook( pgent_t *pgent )
{
    if( !pgent->is_valid() )
	return;

    vcpu_t &vcpu = get_vcpu();
    word_t mapping = pgent->get_address() + vcpu.get_kernel_vaddr();

    // Assuming a 4KB page entry.
    word_t rwx = unmap_page( vcpu, mapping, PAGE_BITS, Burn_NoAccess );

    if( rwx & Burn_Writable )
	pgent->set_dirty(1);
    if( rwx & (Burn_Readable | Burn_eXecutable) )
	pgent->set_accessed(1);
}

#ifndef CONFIG_WEDGE_XEN
pgent_t
backend_pte_get_and_clear_hook( pgent_t *pgent )
{
    pgent_t old_pgent = *pgent; // Make a copy.
    pgent->clear();		// Remove from page table.

    if( !old_pgent.is_valid() )
	return old_pgent;

    // Perform the unmap + status bit check.
    vcpu_t &vcpu = get_vcpu();
    word_t mapping = old_pgent.get_address() + vcpu.get_kernel_vaddr();
    word_t rwx = unmap_page( vcpu, mapping, PAGE_BITS, Burn_FullyAccessible );

    // Update the status bits in the copy.
    if( rwx & Burn_Writable )
	old_pgent.set_dirty(1);
    if( rwx & (Burn_Readable | Burn_eXecutable) )
	old_pgent.set_accessed(1);

    return old_pgent;
}
#endif
#endif

#if 0
static void flush_old_ptab( vcpu_t &vcpu, pgent_t *new_ptab, pgent_t *old_ptab )
{
    for( word_t idx = 0; idx < (PAGE_SIZE/sizeof(word_t)); idx++ )
    {
	pgent_t &old_pgent = old_ptab[idx];

	if( !old_pgent.is_valid() || old_pgent.is_global() )
	    continue;

    	word_t mapping = old_pgent.get_address() + vcpu.get_kernel_vaddr();
	flush_page( vcpu, mapping, PAGE_BITS );
    }
}
#endif

#ifndef CONFIG_WEDGE_XEN
void backend_flush_old_pdir( u32_t new_pdir_paddr, u32_t old_pdir_paddr )
{
    vcpu_t &vcpu = get_vcpu();
#if defined(CONFIG_GUEST_PTE_HOOK)
    bool page_global_enabled = vcpu.cpu.cr4.is_page_global_enabled() && !vcpu.is_global_crap();
#else
    bool page_global_enabled = false;
#endif

    // TODO: optimize by batching the L4 system calls!

    if( page_global_enabled && vcpu.vaddr_global_only ) {
	// The kernel added only global pages since the last flush.  Thus
	// we don't have anything new to flush.
	return;
    }

    pgent_t *new_pdir = (pgent_t *)(new_pdir_paddr + vcpu.get_kernel_vaddr());
    pgent_t *old_pdir = (pgent_t *)(old_pdir_paddr + vcpu.get_kernel_vaddr());

    for( word_t idx = vcpu.vaddr_flush_min >> PAGEDIR_BITS;
	    idx <= vcpu.vaddr_flush_max >> PAGEDIR_BITS; idx++ )
    {
	pgent_t &new_pgent = new_pdir[idx];
	pgent_t &old_pgent = old_pdir[idx];

	if( !old_pgent.is_valid() )
	    continue;	// No mapping to flush.
	if( old_pgent.is_superpage() && vcpu.cpu.cr4.is_pse_enabled() )
	{
	    if( page_global_enabled && old_pgent.is_global() )
		continue;
	    word_t mapping = old_pgent.get_address() + vcpu.get_kernel_vaddr();
    	    if( debug_superpages )
		con << "flush super page\n";
	    flush_page( vcpu, mapping, SUPERPAGE_BITS );
	}
	else if( page_global_enabled
		&& old_pgent.get_address() == new_pgent.get_address()
		&& old_pgent.is_writable() == new_pgent.is_writable() )
	{
	    pgent_t *new_ptab = (pgent_t *)(new_pgent.get_address() + vcpu.get_kernel_vaddr());
	    pgent_t *old_ptab = (pgent_t *)(old_pgent.get_address() + vcpu.get_kernel_vaddr());
	    flush_old_ptab( vcpu, new_ptab, old_ptab );
	}
	else if( old_pgent.get_address() != old_pdir_paddr ) {
	    // Note: FreeBSD installs recursive entries, i.e., a pdir
	    // entry that points back at itself, thus establishing the
	    // pdir as a page table.  Skip recursive entries.
    	    pgent_t *ptab = (pgent_t *)
		(old_pgent.get_address() + vcpu.get_kernel_vaddr());
	    flush_ptab( vcpu, ptab );
	}
    }

    vcpu.vaddr_stats_reset();
}
#endif

#ifndef CONFIG_WEDGE_XEN
extern void backend_flush_vaddr( word_t vaddr )
    // Flush the mapping of an arbitrary virtual address.  The mapping 
    // could be of any size, so we must walk the page table to determine
    // the page size.
{
    vcpu_t &vcpu = get_vcpu();
    word_t kernel_start = vcpu.get_kernel_vaddr();

    pgent_t *pdir = (pgent_t *)(vcpu.cpu.cr3.get_pdir_addr() + kernel_start);
    pdir = &pdir[ pgent_t::get_pdir_idx(vaddr) ];
    if( !pdir->is_valid() )
	return;
    if( pdir->is_superpage() && vcpu.cpu.cr4.is_pse_enabled() ) {
	vaddr = pdir->get_address() + kernel_start;
	flush_page( vcpu, vaddr, SUPERPAGE_BITS );
	return;
    }

    pgent_t *ptab = (pgent_t *)(pdir->get_address() + kernel_start);
    pgent_t *pgent = &ptab[ pgent_t::get_ptab_idx(vaddr) ];
    if( !pgent->is_valid() )
	return;
    vaddr = pgent->get_address() + kernel_start;
    flush_page( vcpu, vaddr, PAGE_BITS );
}
#endif

#ifndef CONFIG_WEDGE_XEN
void backend_invalidate_tlb( void )
{
    get_vcpu().invalidate_globals();
}
#endif
#if defined(CONFIG_GUEST_UACCESS_HOOK)

static pgent_t *
uaccess_resolve_addr( word_t addr, word_t rwx, word_t & kernel_vaddr )
{
    pgent_t *pgent = backend_resolve_addr( addr, kernel_vaddr );
    if( !pgent || ((rwx & 2) && !pgent->is_writable()) )
    {
	if( !pgent )
	    sync_deliver_page_not_present( addr, rwx );
	else
	    sync_deliver_page_permissions_fault( addr, rwx );

	pgent = backend_resolve_addr( addr, kernel_vaddr );
	if( !pgent || ((rwx & 2) && !pgent->is_writable()) ) {
	    ASSERT(!"double resolve failure");
	    return NULL;
	}
    }

    return pgent;
}

INLINE word_t
copy_to_user_page( void *to, const void *from, unsigned long n )
{
    word_t kernel_vaddr;
    pgent_t * pgent = uaccess_resolve_addr( (word_t)to, 6, kernel_vaddr );
    if( !pgent )
	return 0;

    pgent->set_accessed( true );
    pgent->set_dirty( true );

    memcpy( (void *)kernel_vaddr, from, n );
    return n;
}

INLINE word_t
copy_from_user_page( void *to, const void *from, unsigned long n )
{
    word_t kernel_vaddr;
    pgent_t * pgent = uaccess_resolve_addr( (word_t)from, 4, kernel_vaddr );
    if( !pgent )
	return 0;

    pgent->set_accessed( true );

    memcpy( to, (void *)kernel_vaddr, n );
    return n;
}

static word_t
clear_user_page( void *to, unsigned long n )
{
    word_t kernel_vaddr;
    pgent_t * pgent = uaccess_resolve_addr( (word_t)to, 6, kernel_vaddr );
    if( !pgent )
	return 0;

    pgent->set_accessed( true );
    pgent->set_dirty( true );

    unsigned long i;
    if( (kernel_vaddr % sizeof(word_t)) || (n % sizeof(word_t)) ) {
	// Unaligned stuff.
	for( i = 0; i < n; i++ )
	    ((u8_t *)kernel_vaddr)[i] = 0;
    }
    else {
	// Aligned stuff.
	for( i = 0; i < n/sizeof(word_t); i++ )
	    ((word_t *)kernel_vaddr)[i] = 0;
    }
    return n;
}

static bool
strnlen_user_page( const char *s, word_t n, word_t *len )
{
    word_t kernel_vaddr;
    pgent_t *pgent = uaccess_resolve_addr( (word_t)s, 4, kernel_vaddr );
    if( !pgent ) {
	*len = 0;     // Reset the length.
	return false; // Stop searching.
    }

    word_t res, end;
    __asm__ __volatile__ (
	    "0:	repne; scasb\n"
	    "	sete %b1\n"
	    :"=c" (res), "=a" (end)
	    :"0" (n), "1"(0), "D"(kernel_vaddr)
	    );

    pgent->set_accessed( true );

    /* End is 0 -- max number of bytes searched.
     * End is > 0 -- end of string reached, res contains the number of
     *   remaining bytes.
     */
    *len += n - res;
    if( end )
	return false; // Stop searching.
    return true;
}

static word_t 
strncpy_from_user_page( char *dst, const char *src, word_t copy_size, word_t *success, bool *finished )
{
    word_t kernel_vaddr;
    pgent_t * pgent = uaccess_resolve_addr( (word_t)src, 4, kernel_vaddr );
    if( !pgent ) {
	*finished = true;
	*success = 0;
	return 0;
    }

    pgent->set_accessed( true );

    word_t remain = copy_size;
    word_t d0, d1, d2;
    __asm__ __volatile__ (
	    "	testl %0,%0\n"
	    "	jz 1f\n"
	    "0:	lodsb\n"
	    "	stosb\n"
	    "	testb %%al,%%al\n"
	    "	jz 1f\n"
	    "	decl %0\n"
	    "	jnz 0b\n"
	    "1:\n"
	    : "=c"(remain), "=&a"(d0), "=&S"(d1),
	      "=&D"(d2)
	    : "0"(remain), "2"(kernel_vaddr), "3"(dst)
	    : "memory" );

    if( remain )
	*finished = true;
    return copy_size - remain;
}

word_t
backend_get_user_hook( void *to, const void *from, word_t n )
{
    word_t kernel_vaddr;
    pgent_t *pgent = uaccess_resolve_addr( (word_t)from, 4, kernel_vaddr );
    if( !pgent )
	return 0;

    switch( n ) {
	case 1: *(u8_t *)to = *(u8_t *)kernel_vaddr; break;
	case 2: *(u16_t *)to = *(u16_t *)kernel_vaddr; break;
	case 4: *(u32_t *)to = *(u32_t *)kernel_vaddr; break;
	case 8: *(u64_t *)to = *(u64_t *)kernel_vaddr; break;
	default:
		ASSERT( !"not supported" );
    }

    pgent->set_accessed( true );

    return n;
}

word_t backend_put_user_hook( void *to, const void *from, word_t n )
{
    word_t kernel_vaddr;
    pgent_t *pgent = uaccess_resolve_addr( (word_t)to, 6, kernel_vaddr );
    if( !pgent )
	return 0;

    switch( n ) {
	case 1: *(u8_t *)kernel_vaddr = *(u8_t *)from; break;
	case 2: *(u16_t *)kernel_vaddr = *(u16_t *)from; break;
	case 4: *(u32_t *)kernel_vaddr = *(u32_t *)from; break;
	case 8: *(u64_t *)kernel_vaddr = *(u64_t *)from; break;
	default:
		ASSERT( !"not supported" );
    }

    pgent->set_accessed( true );
    pgent->set_dirty( true );

    return n;
}

word_t backend_copy_from_user_hook( void *to, const void *from, word_t n )
{
    word_t result;
    word_t remaining = n;
    word_t copy_size = (word_t)from & ~PAGE_MASK;

    if( copy_size ) {
	copy_size = min( (word_t)PAGE_SIZE - copy_size, remaining );
	result = copy_from_user_page( to, from, copy_size );
	remaining -= result;
	if( result != copy_size )
	    return n - remaining;
    }

    while( remaining ) {
	from = (void *)(word_t(from) + copy_size);
	to = (void *)(word_t(to) + copy_size);
	copy_size = min( (word_t)PAGE_SIZE, remaining );
	result = copy_from_user_page( to, from, copy_size );
	remaining -= result;
	if( result != copy_size )
	    return n - remaining;
    }

    return n;
}

word_t backend_copy_to_user_hook( void *to, const void *from, word_t n )
{
    word_t result;
    word_t remaining = n;
    word_t copy_size = (word_t)to & ~PAGE_MASK;

    if( copy_size ) {
	copy_size = min( (word_t)PAGE_SIZE - copy_size, remaining );
	result = copy_to_user_page( to, from, copy_size );
	remaining -= result;
	if( result != copy_size )
	    return n - remaining;
    }

    while( remaining ) {
	from = (void *)(word_t(from) + copy_size);
	to = (void *)(word_t(to) + copy_size);
	copy_size = min( (word_t)PAGE_SIZE, remaining );
	result = copy_to_user_page( to, from, copy_size );
	remaining -= result;
	if( result != copy_size )
	    return n - remaining;
    }

    return n;
}

word_t
backend_clear_user_hook( void *dst, word_t n )
{
    word_t result;
    word_t remaining = n;
    word_t clear_size = (word_t)dst & ~PAGE_MASK;

    if( clear_size ) {
	clear_size = min( (word_t)PAGE_SIZE - clear_size, remaining );
	result = clear_user_page( dst, clear_size );
	remaining -= result;
	if( result != clear_size )
	    return n - remaining;
    }

    while( remaining ) {
	dst = (void *)(word_t(dst) + clear_size );
	clear_size = min( word_t(PAGE_SIZE), remaining );
	result = clear_user_page( dst, clear_size );
	remaining -= result;
	if( result != clear_size )
	    return n - remaining;
    }

    return n;
}

word_t
backend_strnlen_user_hook( const char *s, word_t n )
    // Must include the size of the terminating NULL.
{
    word_t search_size = PAGE_SIZE - ((word_t)s & ~PAGE_MASK );
    word_t len = 0;

    if( !search_size )
	search_size = PAGE_SIZE;
    if( search_size > n )
	search_size = n;

    while( n > 0 ) {
	if( !strnlen_user_page(s, search_size, &len) )
	    return len;

	s += search_size;
	n =- search_size;
	search_size = PAGE_SIZE;
	if( search_size > n )
	    search_size = n;
    }

    return len;
}

word_t
backend_strncpy_from_user_hook( char *dst, const char *src, word_t count, word_t *success )
{
    word_t result;
    word_t remaining = count;
    word_t copy_size = (word_t)src & ~PAGE_MASK;
    bool finished = false;

    *success = 1;

    if( copy_size ) {
	copy_size = min( (word_t)PAGE_SIZE - copy_size, remaining );
	result = strncpy_from_user_page( dst, src, copy_size, success, &finished );
	remaining -= result;
	if( finished )
	    return count - remaining;
    }

    while( remaining ) {
	src += copy_size;
	dst += copy_size;
	copy_size = min( (word_t)PAGE_SIZE, remaining );
	result = strncpy_from_user_page( dst, src, copy_size, success, &finished );
	remaining -= result;
	if( finished )
	    return count - remaining;
    }

    return count;
}


#endif

