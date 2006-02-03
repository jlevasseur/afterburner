/******************************************************************************
 * hypervisor.h
 * 
 * Linux-specific hypervisor handling.
 * 
 * Copyright (c) 2002, K A Fraser
 */

#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include INC_WEDGE(types.h)
extern "C" {
#include <xen.h>
#include <io/domain_controller.h>
#include <event_channel.h>
#include <physdev.h>
#include <dom0_ops.h>

/* hypervisor.c */
void do_hypervisor_callback(struct pt_regs *regs);
void enable_hypervisor_event(unsigned int ev);
void disable_hypervisor_event(unsigned int ev);
void ack_hypervisor_event(unsigned int ev);

#define HYPERVISOR_VIRT_START (0xFC000000UL)
#ifndef machine_to_phys_mapping
#define machine_to_phys_mapping ((unsigned long *)HYPERVISOR_VIRT_START)
#endif

extern unsigned long *phys_to_machine_mapping;

#define PTCOUNT_BITS (20)
#define PTCOUNT_MASK (0xFFF << PTCOUNT_BITS)
#define PTCOUNT_INC (1 << 20)

static inline unsigned long mfn_to_pfn(unsigned long mfn)
{
	return machine_to_phys_mapping[mfn];
}

static inline unsigned long pfn_to_mfn(unsigned long pfn)
{
	return phys_to_machine_mapping[pfn] & (~PTCOUNT_MASK);
}

static inline unsigned long pfn_pt_count(unsigned long pfn)
{
	return phys_to_machine_mapping[pfn] >> PTCOUNT_BITS;
}

static inline void pfn_pt_inc(unsigned long pfn)
{
	phys_to_machine_mapping[pfn] += PTCOUNT_INC;
}

static inline void pfn_pt_dec(unsigned long pfn)
{
	phys_to_machine_mapping[pfn] -= PTCOUNT_INC;
}

/*
 * Assembler stubs for hyper-calls.
 */

static __inline__ int HYPERVISOR_set_trap_table(trap_info_t *table)
{
    int ret;
    __asm__ __volatile__ (
        TRAP_INSTR
        : "=a" (ret) : "0" (__HYPERVISOR_set_trap_table),
        "b" (table) : "memory" );

    return ret;
}

static __inline__ int HYPERVISOR_mmu_update(mmu_update_t *req, 
                                            int count, 
                                            int *success_count)
{
	int ret, d1, d2, d3;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=c" (d2), "=d" (d3)
			      : "0" (__HYPERVISOR_mmu_update), "1" (req), "2" (count), "3" (success_count)  
			      : "memory" );
	
	return ret;
}

static __inline__ int xen_tlb_flush(void)
{
	int suc;
	mmu_update_t req;
	req.ptr = MMU_EXTENDED_COMMAND;
	req.val = MMUEXT_TLB_FLUSH;
	HYPERVISOR_mmu_update(&req, 1, &suc);
	return (suc != 1);
}


static __inline__ int HYPERVISOR_set_gdt(unsigned long *frame_list, int entries)
{
	int ret, d1, d2;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1), "=c" (d2)
			      : "0" (__HYPERVISOR_set_gdt), 
				"1" (frame_list), "2" (entries) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_stack_switch(unsigned long ss, unsigned long esp)
{
	int ret, d1, d2;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=c" (d2)
			      : "0" (__HYPERVISOR_stack_switch),
				"1" (ss), "2" (esp) 
			      : "memory" );

    return ret;
}

static __inline__ int HYPERVISOR_set_callbacks(
    unsigned long event_selector, unsigned long event_address,
    unsigned long failsafe_selector, unsigned long failsafe_address)
{
	int ret, d1, d2, d3;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=c" (d2), "=d" (d2), "=S" (d3)
			      : "0" (__HYPERVISOR_set_callbacks),
				"1" (event_selector), "2" (event_address), 
				"3" (failsafe_selector), "4" (failsafe_address) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_fpu_taskswitch(void)
{
	int ret;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) 
			      : "0" (__HYPERVISOR_fpu_taskswitch) 
			      : "memory" );

	return ret;
}

static __inline__ int HYPERVISOR_yield(void)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_sched_op),
				"1" (SCHEDOP_yield) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_block(void)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_sched_op),
				"1" (SCHEDOP_block) 
			      : "memory" );
	
	return ret;
}

static __inline__ int HYPERVISOR_shutdown(void)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1) 
			      : "0" (__HYPERVISOR_sched_op),
				"1" (SCHEDOP_shutdown | (SHUTDOWN_poweroff << SCHEDOP_reasonshift))
			      : "memory" );
    
    return ret;
}

static __inline__ int HYPERVISOR_reboot(void)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_sched_op),
				"1" (SCHEDOP_shutdown | (SHUTDOWN_reboot << SCHEDOP_reasonshift))
			      : "memory" );

    return ret;
}

static __inline__ int HYPERVISOR_suspend(unsigned long srec)
{
	int ret, d1, d2;
	/* NB. On suspend, control software expects a suspend record in %esi. */
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=S" (d2)
			      : "0" (__HYPERVISOR_sched_op),
				"1" (SCHEDOP_shutdown | (SHUTDOWN_suspend << SCHEDOP_reasonshift)), 
				"2" (srec) 
			      : "memory" );
	return ret;
}

static __inline__ long HYPERVISOR_set_timer_op(void *timer_arg)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1)
			      : "0" (__HYPERVISOR_set_timer_op),
				"1" (timer_arg) 
			      : "memory" );
	return ret;
}

static __inline__ int xen_dom0_op(dom0_op_t *dom0_op)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_dom0_op),
				"1" (dom0_op) 
			      : "memory" );
	
	return ret;
}

static __inline__ int HYPERVISOR_set_debugreg(int reg, unsigned long value)
{
	int ret, d1, d2;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1), "=c" (d2)
			      : "0" (__HYPERVISOR_set_debugreg),
				"1" (reg), "2" (value) 
			      : "memory" );

    return ret;
}

static __inline__ unsigned long HYPERVISOR_get_debugreg(int reg)
{
	unsigned long ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_get_debugreg),
				"1" (reg) 
			      : "memory" );

    return ret;
}

static __inline__ int HYPERVISOR_update_descriptor(
    unsigned long pa, unsigned long word1, unsigned long word2)
{
	int ret, d1, d2, d3;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1), "=c" (d2), "=d" (d3)
			      : "0" (__HYPERVISOR_update_descriptor), 
				"1" (pa), "2" (word1), "3" (word2) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_set_fast_trap(int idx)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_set_fast_trap), 
				"1" (idx) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_dom_mem_op(void *dom_mem_op)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1) 
			      : "0" (__HYPERVISOR_dom_mem_op),
				"1" (dom_mem_op) 
			      : "memory" );

    return ret;
}

static __inline__ int HYPERVISOR_multicall(void *call_list, int nr_calls)
{
	int ret, d1, d2;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1), "=c" (d2)
			      : "0" (__HYPERVISOR_multicall),
				"1" (call_list), "2" (nr_calls) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_update_va_mapping(
    unsigned long page_nr, unsigned long new_val, unsigned long flags)
{
	int ret, d1, d2, d3;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=c" (d2), "=d" (d3)
			      : "0" (__HYPERVISOR_update_va_mapping), 
				"1" (page_nr), "2" (new_val), "3" (flags) 
			      : "memory" );

	return ret;
}

static __inline__ int HYPERVISOR_xen_version(int cmd)
{
	int ret, d1;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret) , "=b" (d1)
			      : "0" (__HYPERVISOR_xen_version), 
				"1" (cmd) 
			      : "memory" );
	return ret;
}

static __inline__ int HYPERVISOR_console_io(int cmd, int count, char *str)
{
	int ret, d1, d2, d3;
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1), "=c" (d2), "=d" (d3)
			      : "0" (__HYPERVISOR_console_io),
				"1" (cmd), "2" (count), "3" (str) \
			      : "memory" );

    return ret;
}


static __inline__ int HYPERVISOR_event_channel_op(evtchn_op_t *op)
{
	int ret, d1;
	
	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1) 
			      : "0" (__HYPERVISOR_event_channel_op), "1" (op)
			      : "memory");
	
	return ret;
}


static __inline__ int HYPERVISOR_physdev_op(physdev_op_t *op)
{
	int ret, d1;

	__asm__ __volatile__ (
			      TRAP_INSTR
			      : "=a" (ret), "=b" (d1) 
			      : "0" (__HYPERVISOR_physdev_op), "1" (op)
			      : "memory");
	return ret;
}



}
#endif /* __HYPERVISOR_H__ */
