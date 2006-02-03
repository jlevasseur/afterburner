/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/bind.h
 * Description:   Declarations for the high-level guest hooks.
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
 * $Id: bind.h,v 1.21 2005/12/16 12:05:44 joshua Exp $
 *
 ********************************************************************/

#ifndef __AFTERBURN_WEDGE__INCLUDE__BIND_H__
#define __AFTERBURN_WEDGE__INCLUDE__BIND_H__

#include INC_ARCH(cpu.h)

#if defined(CONFIG_WEDGE_STATIC)

extern "C" void * afterburn_thread_get_handle( void );
extern "C" void afterburn_thread_assign_handle( void * handle );

extern "C" void (*afterburn_exit_hook)( void *handle );
#if defined(CONFIG_GUEST_PTE_HOOK)
extern "C" void (*afterburn_set_pte_hook)( pgent_t *oldpte, pgent_t newpte, int level );
#endif
#if defined(CONFIG_GUEST_UACCESS_HOOK)
extern "C" word_t (*afterburn_get_user_hook)( void *to, const void *from, word_t n );
extern "C" word_t (*afterburn_put_user_hook)( void *to, const void *from, word_t n );
extern "C" word_t (*afterburn_copy_from_user_hook)( void *to, const void *from, word_t n );
extern "C" word_t (*afterburn_copy_to_user_hook)( void *to, const void *from, word_t n );
#endif

#else	/* !CONFIG_WEDGE_STATIC */

extern void * (*afterburn_thread_get_handle)(void);
extern void (*afterburn_thread_assign_handle)(void *handle);

enum bind_from_guest_e {
    import_thread_get_handle=0, import_thread_set_handle=1,
};

enum bind_to_guest_e {
    import_exit_hook=0, import_set_pte_hook=1, import_get_user_hook=2,
    import_put_user_hook=3, import_copy_from_user_hook=4,
    import_copy_to_user_hook=5, import_clear_user_hook=6,
    import_strnlen_user_hook=7, import_strncpy_from_user_hook=8,
    import_read_pte_hook=9, import_pte_test_and_clear_hook=10,
    import_pte_get_and_clear_hook=11, import_phys_to_dma_hook=12,
    import_dma_to_phys_hook=13, import_free_pgd_hook=14,
    import_module_rewrite_hook=15, import_signal_hook=16,
    import_esp0_sync=17,
};
#endif

struct elf_bind_t
{
    word_t type;
    word_t location;
};

struct patchup_info_t
{
    word_t start;
    word_t end;
};

extern bool arch_bind_from_guest( elf_bind_t *guest_exports, word_t count );
extern bool arch_bind_to_guest( elf_bind_t *guest_imports, word_t count );
extern bool arch_apply_patchups( patchup_info_t *patchups, word_t total,
	word_t vaddr_offset );
extern bool arch_apply_device_patchups( patchup_info_t *patchups, word_t total,
	word_t vaddr_offset, word_t read_func=0, word_t write_func=0,
	word_t xchg_func=0, word_t or_func=0, word_t and_func=0 );
extern bool arch_apply_bitop_patchups( patchup_info_t *patchups, word_t total,
	word_t vaddr_offset, word_t clear_bit_func, word_t set_bit_func );

#endif /* __AFTERBURN_WEDGE__INCLUDE__BIND_H__ */
