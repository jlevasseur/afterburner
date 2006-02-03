#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__

#include INC_ARCH(cpu.h)
#include INC_ARCH(intlogic.h)
#include <linux/user.h>

#define Burn_FullyAccessible 7
#define Burn_Writable 2
#define Burn_Readable 4
#define Burn_eXecutable 1
#define Burn_NoAccess 0

extern unsigned long backend_handle_pagefault( 
	unsigned long fault_addr, unsigned long ip, unsigned long & rwx);

extern void backend_sync_deliver_vector( word_t vector, bool old_int_state, bool use_error_code, word_t error_code );
extern void backend_async_irq_deliver( intlogic_t &intlogic );

extern void backend_interruptible_idle( void );
extern void backend_activate_user( user_frame_t *user_frame, word_t ip );

extern void backend_invalidate_tlb( void );
extern void backend_enable_paging( word_t *ret_address );
extern word_t backend_sync_flush_page(word_t addr, word_t bits, word_t permissions);
extern word_t backend_sync_unmap_page(word_t addr, word_t bits, word_t permissions);
extern void backend_flush_user( void );
extern void backend_flush_page( u32_t addr );
extern void backend_flush_vaddr( word_t vaddr );

extern bool backend_enable_device_interrupt( u32_t interrupt );
extern void backend_check_pending_interrupts( void );

extern void backend_exit_hook( void *handle );

INLINE void backend_protect_fpu()
{
}

#define CONFIG_GUEST_PTE_HOOK
#if defined(CONFIG_GUEST_PTE_HOOK)
extern void backend_set_pte_hook( pgent_t *old_pte, pgent_t new_pte, int level);
extern void backend_pte_wrprotect_hook( pgent_t *pgent );
extern void backend_pte_access_check_hook( pgent_t *pgent );
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

struct backend_vm_init_t
{
    unsigned long entry_sp;
    unsigned long entry_ip;
};

extern bool backend_preboot( backend_vm_init_t *init_info );

// ia32 specific, TODO: relocate
extern void backend_cpuid_override( u32_t func, u32_t max_basic, 
	u32_t max_extended, frame_t *regs );
extern void backend_flush_old_pdir( u32_t new_pdir, u32_t old_pdir );


#define ESP_OFFSET "(0)"
#define EFLAGS_OFFSET "(0)"
#define EIP_OFFSET "(0)"
#define EDI_OFFSET "(0)"
#define ESI_OFFSET "(0)"
#define EBP_OFFSET "(0)"
#define EIP_OFFSET "(0)"
#define EBX_OFFSET "(0)"
#define EDX_OFFSET "(0)"
#define ECX_OFFSET "(0)"
#define EAX_OFFSET "(0)"

class thread_info_t
{
    pid_t tid;
    struct user_regs_struct in;

public:
    word_t get_id(void) {
	    return (word_t) tid;
    }
    void incr_eip(word_t increment) {
	    in.eip += increment;
    }

    word_t get_eip(void) {
	    return in.eip;
    }

    word_t get_esp(void) {
	    return in.esp;
    }

    void update_eflags(flags_t old_flags) {
	    in.eflags &= flags_user_mask;
	    in.eflags |= (old_flags.x.raw & ~flags_user_mask);
    }

    void *save_base(void) {
	    return (void*) &in.ebx;
    }
};

#endif /* __AFTERBURN_WEDGE__INCLUDE__L4KA__BACKEND_H__ */
