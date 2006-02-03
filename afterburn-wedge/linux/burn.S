.data
.align 0x1000
.globl end_stack
.type end_stack,@object;
start_stack:	
.space 0x1000, 0
end_stack:

.macro func name
.text
.globl \name
.type \name,@function
\name:
.endm
	
.macro return_code
	movl	(end_stack - 4), %esp
	movl	(end_stack - 8), %eax
	movl	(end_stack - 12), %ecx
	movl	(end_stack - 16), %edx

	jmp	*(end_stack - 20)
.endm

.macro start_code
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	popl	%eax
	movl	%eax, (end_stack - 20)
.endm

.macro __afterburn_save_clobbers
	pushl	%eax
	pushl	%ecx
	pushl	%edx
.endm

.macro __afterburn_restore_clobbers drop
.if \drop
	addl	$(\drop), %esp
.endif
	popl	%edx
	popl	%ecx
	popl	%eax
.endm

.macro __afterburn_save_all
	/* The order matches that of frame_t in include/ia32/cpu.h. */
	/* TODO: consider using pusha and popa, to use a single instruction. */
	!pushfl
	pushl	%edi
	pushl	%esi
	pushl	%ebp
	pushl	%esp
	pushl	%ebx
	pushl	%edx
	pushl	%ecx
	pushl	%eax
.endm

.macro __afterburn_restore_all drop
.if \drop
	addl	$(\drop), %esp
.endif
	popl	%eax
	popl	%ecx
	popl	%edx
	popl	%ebx
	addl	$4, %esp
	popl	%ebp
	popl	%esi
	popl	%edi
	!popfl
.endm

func burn_gdt
	start_code
	call	afterburn_cpu_write_gdt32
	return_code

func burn_idt
	start_code
	call	afterburn_cpu_write_idt32
	return_code

func burn_invlpg
	start_code
	call	afterburn_cpu_invlpg
	return_code

func burn_lldt
	start_code
	call	afterburn_cpu_lldt
	return_code
		
func burn_ltr
	start_code	
	call	afterburn_cpu_ltr
	return_code
		
func burn_clts
	start_code
	call	afterburn_cpu_clts
	return_code
	
func burn_cli
	start_code
	call	afterburn_cpu_cli
	return_code

func burn_rdmsr
	movl	%ecx, (end_stack - 12) 
	popl	%eax
	movl	%eax, (end_stack - 20)

	subl	$8, %esp	/* allocate space for eax and edx output */
	pushl   $423
	pushl   $422
	pushl   %ecx
	call	afterburn_cpu_read_msr
	addl	$4, %esp	/* drop the input parameter */

	popl	%eax		/* pop the eax output parameter */
	popl	%edx		/* pop the edx output parameter */

	movl	(end_stack - 12), %ecx
	movl	(end_stack - 4), %esp

	jmp	*(end_stack - 20)

func burn_wrmsr
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)

	popl	%eax
	movl	%eax, (end_stack - 20)
	movl	(end_stack - 8), %eax
	
	pushl	%edx
	pushl	%eax
	pushl	%ecx

	
	call	afterburn_cpu_write_msr

	return_code

func burn_hlt
	start_code
	call	afterburn_cpu_hlt
	return_code

func burn_iret
	subl	$12, %esp	/* Allocate space for redirect frame. */
	__afterburn_save_all
	movl	%esp, %eax
	addl	$(12+36), %eax	/* 1st parameter: location of iret info. */
	movl	%esp, %edx	/* 2nd parameter: location of register frame. */
	movl	%edx, %ecx
	addl	$36, %ecx	/* 3rd parameter: location of redirect frame. */
	call	afterburn_cpu_iret
	__afterburn_restore_all 0

	testl	$-1, 0(%esp)	/* Do we have a redirect? */
	jnz	1f		/* Jump if we have a redirect. */
	addl	$12, %esp	/* Remove the redirect frame. */
1:
	!iret	/* Jump to the new code location. */

func burn_int
	!pushf			/* Flags for saved-frame. */
	pushl	%eax		/* Allocate space for saved-frame cs. */
	pushl	$99f		/* Saved-frame return ip address. */
	pushl	%ebx		/* Allocate space for int-frame flags. */
	pushl	%eax		/* Allocate space for int-frame cs. */
	pushl	$99f		/* Allocate space for int-frame ip; also gives a sane value in case something goes wrong. */
	__afterburn_save_clobbers
	movl	%esp, %eax
	addl	$12, %eax	/* Calculate location of int frame. */
	pushl	%eax		/* 3rd parameter: location of int-frame. */
	addl	$12, %eax	/* Calculate location of save-frame. */
	pushl	%eax		/* 2nd parameter: location of save-frame. */
	pushl	$0x80		/* 1st parameter: int vector. */
	call	afterburn_cpu_int
	__afterburn_restore_clobbers 12

	!iret			/* Invoke the interrupt handler and restore */
				/* the stack. */

	jmp	1b		/* Restart the int handler.  Must be a */
				/* 2-byte instruction. */
99:
	ret
		

func burn_cpuid
	popl	%ecx
	movl	%ecx, (end_stack - 20)
	
	__afterburn_save_all
	pushl	%esp	/* The register save frame. */

	call	afterburn_cpu_read_cpuid

	__afterburn_restore_all 4

	mov	(end_stack - 4), %esp
	jmp	*(end_stack - 20)
	

func burn_pushf
	/* Save clobbers */	
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	pop     %edx
	movl	%edx, (end_stack - 20)

	/* Switch stacks */
	mov     %esp, end_stack - 4
	lea	end_stack - 24, %esp

	!pushfl  /* Actually push the flags */
	
	call	afterburn_cpu_read_flags

	/* Restore clobbers */
	movl	(end_stack - 4), %esp
	pushl	%eax
	movl	(end_stack - 8), %eax
	movl	(end_stack - 12), %ecx
	movl	(end_stack - 16), %edx

	jmp	*(end_stack - 20)

func burn_popf
	/* Save clobbers */	
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	pop     %eax
	movl	%eax, (end_stack - 20)
	pop     %eax
	
	/* Switch stacks */
	mov     %esp, (end_stack - 4)
	lea	end_stack - 24, %esp

	push    %eax

	call	afterburn_cpu_write_flags
	pushl	%eax
	popf

	/* Restore clobbers */
	movl	(end_stack - 4), %esp
	movl	(end_stack - 8), %eax
	movl	(end_stack - 12), %ecx
	movl	(end_stack - 16), %edx
	jmp	*(end_stack - 20)

		
func burn_sti
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	popl	%eax
	movl	%eax, (end_stack - 20)
	
	call	afterburn_cpu_sti

	return_code
	
func burn_farjmp
	start_code
	push    $~0x7e /* 0x81 but won't get sign extended this way */ /* CS */
	call	afterburn_cpu_burn_mov_dispatch
	return_code

func burn_mov
	start_code
	call	afterburn_cpu_burn_mov_dispatch
	return_code
	
func burn_mov_to_eax
	/*movl	%eax, (end_stack - 8)  No need to save eax here */
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	popl	%eax
	movl	%eax, (end_stack - 20)
	
	call	afterburn_cpu_burn_mov_to_dispatch

	mov	(end_stack - 4), %esp
	/* movl	(end_stack - 8), %eax No need to restore eax here */
	movl	(end_stack - 12), %ecx
	movl	(end_stack - 16), %edx

	jmp	*(end_stack - 20)


func burn_mov_to_ecx
	movl	%eax, (end_stack - 8)
	/* movl	%ecx, (end_stack - 12)  No need to save exc here */
	movl	%edx, (end_stack - 16)
	popl	%eax
	movl	%eax, (end_stack - 20)
	
	call	afterburn_cpu_burn_mov_to_dispatch

	mov	%eax, %ecx
	mov	(end_stack - 4), %esp
	movl	(end_stack - 8), %eax
	/* movl	(end_stack - 12), %ecx */
	movl	(end_stack - 16), %edx

	jmp	*(end_stack - 20)


func burn_mov_to_edx
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	/*movl	%edx, (end_stack - 16)*/
	popl	%eax
	movl	%eax, (end_stack - 20)
	
	call	afterburn_cpu_burn_mov_to_dispatch

	mov	%eax, %edx
	mov	(end_stack - 4), %esp
	movl	(end_stack - 8), %eax
	movl	(end_stack - 12), %ecx
	/*movl	(end_stack - 16), %edx */

	jmp	*(end_stack - 20)

	
func burn_mov_to_esi
	start_code
	call	afterburn_cpu_burn_mov_to_dispatch
	mov	%eax, %esi
	return_code

	
func burn_mov_to_ebx
	start_code	
	call	afterburn_cpu_burn_mov_to_dispatch
	mov	%eax, %ebx
	return_code
	
func burn_out
	movl	%eax, (end_stack - 8)
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	popl	%ecx
	movl	%ecx, (end_stack - 20)

	pushl	%eax

	call	afterburn_cpu_out_port

	return_code
	
func burn_in
	movl	%ecx, (end_stack - 12) 
	movl	%edx, (end_stack - 16)
	popl	%ecx
	movl	%ecx, (end_stack - 20)

	pushl   %eax
	call	afterburn_cpu_in_port
	
	movl	(end_stack - 4), %esp
	movl	(end_stack - 12), %ecx
	movl	(end_stack - 16), %edx

	jmp	*(end_stack - 20)

func int_fixup
	pushfl
	/* push old_cs */
	pushl	irq_old_cs
	/* push old ip */
	pushl	irq_old_ip
	/* jump */
	jmp	*(irq_handler)

	