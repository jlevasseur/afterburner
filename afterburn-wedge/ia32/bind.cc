/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/ia32/bind.cc
 * Description:   Bind to the high-level hooks in a guest OS.
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

#include INC_WEDGE(backend.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(tlocal.h)

#include <bind.h>

void * (*afterburn_thread_get_handle)(void) = NULL;
void (*afterburn_thread_assign_handle)(void *handle) = NULL;

struct bind_from_guest_t {
    bind_from_guest_e type;
    void **dangler;
};

struct bind_to_guest_t {
    bind_to_guest_e type;
    void *func;
    bool exported;
};

bind_from_guest_t bind_from_guest[] = {
#if defined(CONFIG_WEDGE_L4KA)
    { import_thread_get_handle, (void **)&afterburn_thread_get_handle },
    { import_thread_set_handle, (void **)&afterburn_thread_assign_handle },
#endif
};
static const word_t bind_from_guest_cnt = 
    sizeof(bind_from_guest)/sizeof(bind_from_guest[0]);


bind_to_guest_t bind_to_guest[] = {
#if defined(CONFIG_WEDGE_L4KA) || defined(CONFIG_WEDGE_LINUX)
    { import_exit_hook, (void *)backend_exit_hook, false },
    { import_signal_hook, (void *)backend_signal_hook, false },
#endif
#if defined(CONFIG_GUEST_PTE_HOOK)
    { import_set_pte_hook, (void *)backend_set_pte_hook, false },
    { import_pte_test_and_clear_hook, (void *)backend_pte_test_and_clear_hook, false },
    { import_pte_get_and_clear_hook, (void *)backend_pte_get_and_clear_hook, false },
    { import_read_pte_hook, (void *)backend_read_pte_hook, false },
#if defined(CONFIG_WEDGE_XEN)
    { import_free_pgd_hook, (void *)backend_free_pgd_hook, false },
#endif
#endif
#if defined(CONFIG_GUEST_UACCESS_HOOK)
    { import_get_user_hook, (void *)backend_get_user_hook, false },
    { import_put_user_hook, (void *)backend_put_user_hook, false },
    { import_copy_from_user_hook, (void *)backend_copy_from_user_hook, false },
    { import_copy_to_user_hook, (void *)backend_copy_to_user_hook, false },
    { import_clear_user_hook, (void *)backend_clear_user_hook, false },
    { import_strnlen_user_hook, (void *)backend_strnlen_user_hook, false },
    { import_strncpy_from_user_hook, (void *)backend_strncpy_from_user_hook, false },
#endif
#if defined(CONFIG_GUEST_DMA_HOOK)
    { import_phys_to_dma_hook, (void *)backend_phys_to_dma_hook, false },
    { import_dma_to_phys_hook, (void *)backend_dma_to_phys_hook, false },
#endif
#if defined(CONFIG_GUEST_MODULE_HOOK)
    { import_module_rewrite_hook, (void *)backend_module_rewrite_hook, false },
#endif
};
static const word_t bind_to_guest_cnt = 
    sizeof(bind_to_guest)/sizeof(bind_to_guest[0]);


bool
arch_bind_from_guest( elf_bind_t *guest_exports, word_t count )
{
    ASSERT( sizeof(elf_bind_t) == 2*sizeof(word_t) );
    word_t i, j;
    
    con << "Required from guest: " << bind_from_guest_cnt << '\n';
    con << "Total from guest: " << count << '\n';

    // Walk the list of exports from the guest OS.
    for( i = 0; i < count; i++ ) {
	bool found = false;
	// Walk the import table, to find the export's matching import.
	for( j = 0; !found && (j < bind_from_guest_cnt); j++ ) 
	{
	    if( bind_from_guest[j].type != (bind_from_guest_e)guest_exports[i].type ) 
		continue;
	    if( *bind_from_guest[j].dangler )
    		con << "Multiple guest exports of type " 
		    << guest_exports[i].type << '\n';
	    *bind_from_guest[j].dangler = (void *)guest_exports[i].location;
	    found = true;
	}

//	if( !found )
//	    con << "Unused guest export of type " 
//		<< guest_exports[i].type << '\n';
    }

    // Determine whether we had any unsatisfied symbols.
    bool complete = true;
    for( j = 0; j < bind_from_guest_cnt; j++ )
	if( !*bind_from_guest[j].dangler ) {
	    con << "Unsatisfied import from the guest OS, type "
		<< bind_from_guest[j].type << ".\n";
	    complete = false;
	}

    return complete;
}

bool
arch_bind_to_guest( elf_bind_t *guest_imports, word_t count )
{
    ASSERT( sizeof(elf_bind_t) == 2*sizeof(word_t) );
    word_t i, j;
    
    con << "Required exports to guest: " << bind_to_guest_cnt << '\n';
    con << "Total exports accepted by guest: " << count << '\n';

    // Walk the list of imports from the guest OS.
    for( i = 0; i < count; i++ ) {
	bool found = false;
	// Walk the export table, to find the import's matching import.
	for( j = 0; !found && (j < bind_to_guest_cnt); j++ ) 
	{
	    if( bind_to_guest[j].type != (bind_to_guest_e)guest_imports[i].type ) 
		continue;
	    bind_to_guest[j].exported = true;
	    void **guest_location = (void **)(guest_imports[i].location - get_vcpu().get_kernel_vaddr());
	    *guest_location = bind_to_guest[j].func;
	    found = true;
	}

//	if( !found )
//	    con << "Unused export of type " << guest_imports[i].type << '\n';
    }

    // Determine whether we had any unsatisfied symbols.
    bool complete = true;
    for( j = 0; j < bind_to_guest_cnt; j++ )
	if( !bind_to_guest[j].exported ) {
	    con << "Unsatisfied export to the guest OS, type "
		<< bind_to_guest[j].type << ".\n";
	    complete = false;
	}

    return complete;
}

