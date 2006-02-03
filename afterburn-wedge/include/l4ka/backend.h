/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/l4ka/backend.h
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
 * $Id: backend.h,v 1.40 2006-01-11 18:32:54 stoess Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__

#include INC_ARCH(cpu.h)
#include INC_ARCH(intlogic.h)
#include INC_WEDGE(tlocal.h)

#include <elfsimple.h>

extern void backend_handle_pagefault( 
	word_t fault_addr, word_t fault_ip, 
	word_t & map_addr, word_t & map_bits, word_t & map_rwx );
extern bool backend_handle_user_pagefault(
	word_t page_dir_paddr,
	word_t fault_addr, word_t fault_ip, word_t fault_rwx,
	word_t & map_addr, word_t & map_bits, word_t & map_rwx );

extern void backend_sync_deliver_vector( L4_Word_t vector, bool old_int_state, bool use_error_code, L4_Word_t error_code );
extern void backend_async_irq_deliver( intlogic_t &intlogic );

extern void NORETURN backend_handle_user_vector( word_t vector );

extern void backend_interruptible_idle( burn_redirect_frame_t *frame );
extern void backend_activate_user( user_frame_t *user_frame );

extern void backend_invalidate_tlb( void );
extern void backend_enable_paging( word_t *ret_address );
extern void backend_flush_vaddr( word_t vaddr );
extern void backend_flush_user( void );
extern void backend_flush_page( u32_t paddr );
extern void backend_flush_superpage( u32_t paddr );
extern pgent_t * backend_resolve_addr( word_t user_vaddr, word_t &kernel_vaddr);

INLINE void backend_write_msr( word_t msr_addr, word_t lower, word_t upper )
{
    con << "Unhandled msr write.\n";
}

INLINE void backend_protect_fpu()
{
}

INLINE word_t backend_phys_to_virt( word_t paddr )
{
    return paddr + get_vcpu().get_kernel_vaddr();
}

extern bool backend_enable_device_interrupt( u32_t interrupt );
extern bool backend_unmask_device_interrupt( u32_t interrupt );
extern u32_t backend_get_nr_device_interrupts( void );

struct backend_vm_init_t
{
    L4_Word_t entry_sp;
    L4_Word_t entry_ip;
};

extern bool backend_load_vm( backend_vm_init_t *init_info );
extern bool backend_preboot( backend_vm_init_t *init_info );

extern void backend_reboot( void );

extern void backend_exit_hook( void *handle );
extern int  backend_signal_hook( void *handle );

extern "C" word_t __attribute__((regparm(1))) backend_pte_read_patch( pgent_t *pgent );
extern "C" word_t __attribute__((regparm(1))) backend_pgd_read_patch( pgent_t *pgent );
extern "C" void __attribute__((regparm(2))) backend_pte_write_patch( pgent_t new_val, pgent_t *old_pgent );
extern "C" void __attribute__((regparm(2))) backend_pte_or_patch( word_t bits, pgent_t *old_pgent );
extern "C" void __attribute__((regparm(2))) backend_pte_and_patch( word_t bits, pgent_t *old_pgent );
extern "C" void __attribute__((regparm(2))) backend_pgd_write_patch( pgent_t new_val, pgent_t *old_pgent );
extern "C" word_t __attribute__((regparm(2))) backend_pte_test_clear_patch( word_t bit, pgent_t *pgent );
extern "C" word_t __attribute__((regparm(2))) backend_pte_xchg_patch( pgent_t new_val, pgent_t *pgent );


#if defined(CONFIG_GUEST_PTE_HOOK) || defined(CONFIG_VMI_SUPPORT)
extern void backend_set_pte_hook( pgent_t *old_pte, pgent_t new_pte, int level);
extern word_t backend_pte_test_and_clear_hook( pgent_t *pgent, word_t bit );
extern pgent_t backend_pte_get_and_clear_hook( pgent_t *pgent );
#endif
#if defined(CONFIG_GUEST_UACCESS_HOOK)
extern word_t backend_get_user_hook( void *to, const void *from, word_t n );
extern word_t backend_put_user_hook( void *to, const void *from, word_t n );
extern word_t backend_copy_from_user_hook( void *to, const void *from, word_t n );
extern word_t backend_copy_to_user_hook( void *to, const void *from, word_t n );
extern word_t backend_clear_user_hook( void *to, word_t n );
extern word_t backend_strnlen_user_hook( const char *s, word_t n );
extern word_t backend_strncpy_from_user_hook( char *dst, const char *src, word_t count, word_t *success );
#endif
#if defined(CONFIG_GUEST_DMA_HOOK)
extern word_t backend_phys_to_dma_hook( word_t phys );
extern word_t backend_dma_to_phys_hook( word_t dma );
#endif
#if defined(CONFIG_GUEST_MODULE_HOOK)
extern bool backend_module_rewrite_hook( elf_ehdr_t *ehdr );
#endif

extern bool backend_request_device_mem( word_t base, word_t size, word_t rwx, bool boot=false);
extern bool backend_unmap_device_mem( word_t base, word_t size, word_t &rwx, bool boot=false);
    
// ia32 specific, TODO: relocate
extern void backend_cpuid_override( u32_t func, u32_t max_basic, 
	u32_t max_extended, frame_t *regs );
extern void backend_flush_old_pdir( u32_t new_pdir, u32_t old_pdir );

#include <l4-common/user.h>
extern void NORETURN 
backend_handle_user_exception( thread_info_t *thread_info );

#endif /* __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__ */
