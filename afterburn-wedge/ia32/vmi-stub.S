/*
 * Copyright (c) 2005, VMware, Inc.
 * Copyright (c) 2005, Volkmar Uhlig
 * Copyright (c) 2005, University of Karlsruhe
 * All rights reserved.
 */

#if !defined(CONFIG_VMI_LINKER_SCRIPT)

/**********************************************************************
 *
 *  Define the ROM layout: The offsets for all of the entries.
 *
 **********************************************************************/
	.section .vmi_rom, "ax"
	.global __VMI_START
	.balign 32
__VMI_START:

	.set which, 0
#define VMI_SPACE(name, ...) .set vmi_rom_offset_##name## , which * 32 ; .set which , which + 1 ;
#define VMI_FUNC(name, ...) .set vmi_rom_offset_##name## , which * 32 ; .set which , which + 1 ;
#define VMI_PROC(name, ...) .set vmi_rom_offset_##name## , which * 32 ; .set which , which + 1 ;
#define VMI_JUMP(name, ...) .set vmi_rom_offset_##name## , which * 32 ; .set which , which + 1 ;

/* Alternative approach for defining the ROM contents, but which requires
 * quite a bit more support macros:
#define VMI_SPACE(name, ...) .global vmi_rom_##name ; vmi_rom_##name##: ; .space 32 - (. - vmi_rom_##name ) ;
#define VMI_FUNC(name, ...) .global vmi_rom_##name ; vmi_rom_##name##: ; .space 32 - (. - vmi_rom_##name ) ;
#define VMI_PROC(name, ...) .global vmi_rom_##name ; vmi_rom_##name##: ; .space 32 - (. - vmi_rom_##name ) ;
#define VMI_JUMP(name, ...) .global vmi_rom_##name ; vmi_rom_##name##: ; .space 32 - (. - vmi_rom_##name ) ;
*/

/* Include the ROM definition header file, and instantiate it.
 */

#include INC_ARCH(vmiCalls.h)

VMI_CALLS

#endif	/* CONFIG_VMI_LINKER_SCRIPT */

/**********************************************************************
 *
 *  Define the ROM entries.  We define them in ascending order of
 *  their ROM locations, since the assembler can't produce code 
 *  out-of-order.
 *
 **********************************************************************/

#include INC_ARCH(frontend.S)
#include INC_WEDGE(vcpu.h)

vmi_stub_absolute Init, vmi_init_ext


/* The CPUID handler doesn't fit within the ROM, so split it across multiple
 * sites.
 */
vmi_stub_begin CPUID
	__afterburn_save_all
	pushl	%esp		/* Push the bottom of the parameter block. */
	subl	$8, 0(%esp)	/* Adjust to point at the call frame. */
	jmp	*(1f)
	1: .long vmi_cpuid_finish
vmi_stub_end

	.text
vmi_cpuid_finish:
	call	afterburn_cpu_read_cpuid_ext
	__afterburn_restore_all 4
	ret

vmi_stub_simple WRMSR, afterburn_cpu_write_msr_ext
vmi_stub_simple RDMSR, afterburn_cpu_read_msr_ext

vmi_stub_simple SetGDT, afterburn_cpu_write_gdt32_ext
vmi_stub_simple SetIDT, afterburn_cpu_write_idt32_ext
vmi_stub_simple SetLDT, afterburn_cpu_lldt_ext
vmi_stub_simple SetTR, afterburn_cpu_ltr_ext

vmi_stub_simple GetGDT, afterburn_cpu_unimplemented_ext
vmi_stub_simple GetIDT, afterburn_cpu_unimplemented_ext
vmi_stub_simple GetLDT, afterburn_cpu_lldt_ext
vmi_stub_simple GetTR, afterburn_cpu_str_ext

vmi_stub_nop UpdateGDT
vmi_stub_nop UpdateIDT
vmi_stub_nop UpdateLDT
vmi_stub_simple UpdateKernelStack, vmi_esp0_update_ext

vmi_stub_simple SetCR0, afterburn_cpu_write_cr0_regs
vmi_stub_simple SetCR2, afterburn_cpu_write_cr2_regs
vmi_stub_simple SetCR3, afterburn_cpu_write_cr3_regs
vmi_stub_simple SetCR4, afterburn_cpu_write_cr4_regs

vmi_stub_simple GetCR0, afterburn_cpu_read_cr0_ext
vmi_stub_simple GetCR2, afterburn_cpu_read_cr2_ext
vmi_stub_simple GetCR3, afterburn_cpu_read_cr3_ext
vmi_stub_simple GetCR4, afterburn_cpu_read_cr4_ext

vmi_stub_simple LMSW, afterburn_cpu_unimplemented_ext
vmi_stub_simple SMSW, afterburn_cpu_unimplemented_ext

vmi_stub_simple CLTS, afterburn_cpu_clts
vmi_stub_simple STTS, vmi_cpu_stts

vmi_stub_simple SetDR, afterburn_cpu_write_dr_regs
vmi_stub_simple GetDR, afterburn_cpu_read_dr_regs

vmi_stub_ident RDPMC, rdpmc
vmi_stub_simple RDTSC, vmi_rewrite_rdtsc_ext

vmi_stub_simple INVD, afterburn_cpu_unimplemented_ext
vmi_stub_simple WBINVD, afterburn_cpu_unimplemented_ext

vmi_stub_simple INB, afterburn_cpu_read_port8_regs
vmi_stub_simple INW, afterburn_cpu_read_port16_regs
vmi_stub_simple INL, afterburn_cpu_read_port32_regs
vmi_stub_simple OUTB, afterburn_cpu_write_port8_regs
vmi_stub_simple OUTW, afterburn_cpu_write_port16_regs
vmi_stub_simple OUTL, afterburn_cpu_write_port32_regs

.macro vmi_stub_outs type bitwidth
	vmi_stub_begin OUTS\type
	push	%esi
	__afterburn_save_clobbers
	push	%esp
	subl	$8, 0(%esp)
	vmi_call_relocatable vmi_cpu_write_port_string_\bitwidth
	__afterburn_restore_clobbers 4
	pop	%esi
	ret
	vmi_stub_end
.endm

.macro vmi_stub_ins type bitwidth
	vmi_stub_begin INS\type
	push	%edi
	__afterburn_save_clobbers
	push	%esp
	subl	$8, 0(%esp)
	vmi_call_relocatable vmi_cpu_read_port_string_\bitwidth
	__afterburn_restore_clobbers 4
	pop	%edi
	ret
	vmi_stub_end
.endm

vmi_stub_ins B, 8
vmi_stub_ins W, 16
vmi_stub_ins L, 32

vmi_stub_outs B, 8
vmi_stub_outs W, 16
vmi_stub_outs L, 32
vmi_stub_begin SetIOPLMask
	pushf
	andb $0xcf, 1(%esp)
	orb  %ah, 1(%esp)	/* User error could turn on NT - not really an issue */
	popf
	ret
vmi_stub_end

vmi_stub_begin GetIOPLMask
	pushf
	pop %eax
	andl $0x3000, %eax
	ret
vmi_stub_end

vmi_stub_nop UpdateIOBitmap
vmi_stub_nop UpdateInterruptBitmap

#if defined(CONFIG_SMP)
#error SetEFLAGS and GetEFLAGS aren't SMP safe.
#endif

vmi_stub_begin SetEFLAGS
	mov	%eax, OFS_CPU_FLAGS + vcpu
	ret
vmi_stub_end

vmi_stub_begin GetEFLAGS
	mov	OFS_CPU_FLAGS + vcpu, %eax
	ret
vmi_stub_end


#if defined(CONFIG_SMP)
#error SaveAndDisableIRQs isn't SMP safe.
#endif
vmi_stub_begin SaveAndDisableIRQs
	mov	OFS_CPU_FLAGS + vcpu, %eax
	btr	$9, OFS_CPU_FLAGS + vcpu
	ret
vmi_stub_end

vmi_stub_simple RestoreIRQs, afterburn_cpu_unimplemented_ext

#if defined(CONFIG_SMP) || defined(CONFIG_IA32_STRICT_IRQ)
vmi_stub_simple STI, afterburn_cpu_sti
vmi_stub_simple CLI, afterburn_cpu_cli
#else
vmi_stub_begin STI
	bts	$9, OFS_CPU_FLAGS + vcpu
	ret
vmi_stub_end
vmi_stub_begin CLI
	btr	$9, OFS_CPU_FLAGS + vcpu
	ret
vmi_stub_end
#endif

vmi_stub_begin IRET
#if defined(CONFIG_IA32_FAST_VECTOR)
	/* We have come here via a jmp. */
	vmi_jmp_relocatable	burn_iret
#else
	/* We have come here via a jmp. */
	subl	$12, %esp	/* Allocate space for redirect frame. */
	__afterburn_save_all
	movl	%esp, %eax	/* 1st parameter: iret_handler_frame_t */
	vmi_jmp_relocatable	afterburn_cpu_iret
#endif
vmi_stub_end

vmi_stub_simple IRET16, afterburn_cpu_unimplemented_ext

vmi_stub_simple SysExit, afterburn_cpu_unimplemented_ext
vmi_stub_simple StiSysexit, afterburn_cpu_unimplemented_ext
vmi_stub_simple SysRet,  afterburn_cpu_unimplemented_ext

vmi_stub_begin SafeHalt
	bts	$9, OFS_CPU_FLAGS + vcpu
	vmi_jmp_relocatable burn_interruptible_hlt
vmi_stub_end

vmi_stub_begin Halt
	vmi_jmp_relocatable burn_interruptible_hlt
vmi_stub_end

vmi_stub_ident Pause, pause
vmi_stub_simple Shutdown, vmi_cpu_shutdown
vmi_stub_simple Reboot, vmi_cpu_reboot

/* Page table and mem translation */

vmi_stub_begin GetPteVal
	testl	%eax, %eax
	jz	1f
	/* Assume: 
	 *   extern "C" word_t __attribute__((regparm(1)))
	 *   backend_pte_normalize_patch( pgent_t pgent )
	 */
	push	%edx
	push	%ecx
	vmi_call_relocatable backend_pte_normalize_patch
	pop	%ecx
	pop	%edx
1:	
	ret
vmi_stub_end

vmi_stub_begin GetPdeVal
	/* Assume: 
	 *   extern "C" word_t __attribute__((regparm(1)))
	 *   backend_pde_normalize_patch( pgent_t pgent )
	 */
	push	%edx
	push	%ecx
	vmi_call_relocatable backend_pde_normalize_patch
	pop	%ecx
	pop	%edx
	ret
vmi_stub_end


vmi_stub_simple SetPte, vmi_set_pte_ext		/* mov %eax, (%edx) */


vmi_stub_begin SwapPte
	/* Assume: 
	 *   extern "C" word_t __attribute__((regparm(2)))
	 *   backend_pte_xchg_patch( word_t new_val, pgent_t *pgent )
	 */
	push	%edx
	push	%ecx
	vmi_call_relocatable backend_pte_xchg_patch
	pop	%ecx
	pop	%edx
	ret
vmi_stub_end


vmi_stub_simple TestAndSetPteBit, vmi_test_and_set_pte_bit_ext  /* LOCK bts %eax, (%edx), sbb %eax, %eax */

vmi_stub_begin TestAndClearPteBit
	/* Assume: 
	 *   extern "C" word_t __attribute__((regparm(2)))
	 *   backend_pte_test_clear_patch( word_t bit, pgent_t *pgent )
	 */
	push	%edx
	push	%ecx
	vmi_call_relocatable backend_pte_test_clear_patch
	pop	%ecx
	pop	%edx
	ret
vmi_stub_end

/*
 * This function can not be used on an active root PDP.
 * use SwapPdpeLong instead.  The problem is SMIs can
 * arrive which cause suspend to activate, and a half
 * written value may get cached in hardware when CR3 is
 * reloaded.
 */
vmi_stub_begin SetPteLong
	int3 // catch 
	mov %edx, 4(%ecx)
	SFENCE
	mov %eax, (%ecx)
	ret
vmi_stub_end

vmi_stub_begin SwapPteLong
	int3 // catch
	mov  (%edi), %eax
	mov  4(%edi), %edx
1:	LOCK cmpxchg8b (%edi)
	jne  1b
	ret
vmi_stub_end

/*
 * The following are actually completly identical for PDE and PTE cases
 * in both PAE and non-PAE mode, but having separate functions allows the
 * functions to avoid using a double wide convention for non-PAE updates.
 */
vmi_stub_begin TestAndSetPteBitLong
	int3 // catch
	LOCK bts %eax, (%edx)
	sbb %eax, %eax
	ret
vmi_stub_end

vmi_stub_begin TestAndClearPteBitLong
	int3 // catch
	LOCK btr %eax, (%edx)
	sbb %eax, %eax
	ret
vmi_stub_end

vmi_stub_nop ClonePageTable
vmi_stub_nop ClonePageDirectory
vmi_stub_simple RegisterPageMapping, vmi_register_page_mapping_ext
vmi_stub_simple RegisterPageUsage, vmi_register_page_usage_ext
vmi_stub_simple ReleasePage, vmi_release_page_ext

vmi_stub_simple InvalPage, afterburn_cpu_invlpg_ext
vmi_stub_simple FlushTLB, vmi_flush_tlb_ext
vmi_stub_simple FlushTLBAll, vmi_flush_tlb_ext


/* The first VMI function to be executed after Linux relocates the ROM is
 * SetDeferredMode.  As a hack, we'll rewrite the ROM's relative
 * call points at this time.
 */
vmi_stub_begin SetDeferredMode
	.global _vmi_rewrite_calls_invoke, _vmi_rewrite_calls_return
_vmi_rewrite_calls_invoke:
	__afterburn_save_clobbers
	push	%esp		/* Build the burn_clobbers_frame_t. */
	subl	$8, 0(%esp)
	call	*(1f)		/* Invoke the VMI Init function. */
_vmi_rewrite_calls_return:
	__afterburn_restore_clobbers 4
	ret
1:	.long vmi_rewrite_calls_ext
vmi_stub_end


vmi_stub_nop FlushDeferredCalls

vmi_stub_simple PhysToMachine, vmi_phys_to_machine_ext
vmi_stub_simple MachineToPhys, vmi_machine_to_phys_ext




#if 0

/* Infrastructure for directly accessing the Xt-PIC. */

vmi_stub_begin OUT_20
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0x20_out
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin OUT_21
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0x21_out
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin OUT_a0
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0xa0_out
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin OUT_a1
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0xa1_out
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin IN_20
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0x20_in
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin IN_21
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0x21_in
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin IN_a0
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0xa0_in
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

vmi_stub_begin IN_a1
	push	%ecx
	push	%edx
	vmi_call_relocatable device_8259_0xa1_in
	pop	%edx
	pop	%ecx
	ret
vmi_stub_end

#endif

