.data
.align 0x1000
.globl start_stack
.type start_stack,@object;
start_stack:	
.space 0x1000, 0
end_stack:
.text
.align 16;
.globl mmap_jmp;
.type mmap_jmp,@function;
mmap_jmp:
	movl  $end_stack - 4,%esp
	pusha
	call *0xbfffd008
	popa
	movl  0xbfffd004, %esp
	jmp  *0xbfffd000
