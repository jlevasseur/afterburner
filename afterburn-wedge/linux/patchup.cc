#include <stdint.h>

extern "C" {
#include <elf/elf.h>
}

#include INC_WEDGE(debug.h)


static void *curr_virt_addr = (void*) 0x80000000;

#define OP_2BYTE 0x0f

#define OP_PUSHF 0x9c
#define OP_POPF 0x9d

#define OP_LLTL 0x00
#define OP_LDTL 0x01

#define OP_MOV_IMM32 0xb8
#define OP_MOV_MEM32 0x89
#define OP_MOV_FROM_MEM32 0x8b
#define OP_MOV_TOSEG 0x8e
#define OP_MOV_FROMSEG 0x8c
#define OP_MOV_TOCREG 0x22

#define OP_MOV_FROMSEG 0x8c
#define OP_MOV_TODREG 0x23

#define OP_MOV_FROMCREG 0x20
#define OP_MOV_FROMDREG 0x21

#define OP_CLTS 0x06
#define OP_STI  0xfb
#define OP_CLI  0xfa

#define OP_CALL_REL32 0xe8
#define OP_JMP_REL32 0xe9
#define OP_JMP_FAR 0xea

#define OP_PUSH_REG 0x50
#define OP_PUSH_IMM8 0x6a
#define OP_PUSH_IMM32 0x68

#define OP_CPUID 0xa2

#define OP_IRET 0xcf
#define OP_INT 0xcd

#define OP_RDMSR 0x32
#define OP_WRMSR 0x30

#define OP_HLT 0xf4

#define OP_LSS 0xb2

#define OP_OUT 0xe7
#define OP_OUTB 0xe6

#define OP_OUT_DX 0xef
#define OP_OUTB_DX 0xee

#define OP_SIZE_OVERRIDE 0x66

#define OP_IN 0xe5
#define OP_INB 0xe4

#define OP_INB_DX 0xec
#define OP_IN_DX 0xed

#define OP_REG_EAX 0
#define OP_REG_ECX 1
#define OP_REG_EDX 2
#define OP_REG_EBX 3
#define OP_REG_ESP 4
#define OP_REG_EBP 5
#define OP_REG_ESI 6
#define OP_REG_EDI 7


extern "C" void burn_wrmsr(void);
extern "C" void burn_rdmsr(void);
extern "C" void burn_hlt(void);
extern "C" void burn_out(void);
extern "C" void burn_cpuid(void);
extern "C" void burn_in(void);
extern "C" void burn_int(void);
extern "C" void burn_iret(void);
extern "C" void burn_idt(void);
extern "C" void burn_gdt(void);
extern "C" void burn_invlpg(void);
extern "C" void burn_lldt(void);
extern "C" void burn_ltr(void);
extern "C" void burn_clts(void);
extern "C" void burn_cli(void);
extern "C" void burn_sti(void);
extern "C" void burn_popf(void);
extern "C" void burn_pushf(void);
extern "C" void burn_mov(void);
extern "C" void burn_mov_to_eax(void);
extern "C" void burn_mov_to_ecx(void);
extern "C" void burn_mov_to_edx(void);
extern "C" void burn_mov_to_ebx(void);
extern "C" void burn_mov_to_esi(void);

extern "C" void burn_farjmp(void);

extern uintptr_t end_stack;

static uint8_t *
push_byte(uint8_t *opstream, uint8_t value) 
{
	opstream[0] = OP_PUSH_IMM8;
	opstream[1] = value;
	return &opstream[2];
}

static uint8_t *
push_word(uint8_t *opstream, uint32_t value) 
{
	opstream[0] = OP_PUSH_IMM32;
	* ((uint32_t*)&opstream[1]) = value;
	return &opstream[5];
}

static uint8_t *
push_reg(uint8_t *opstream, uint8_t regname) 
{
	opstream[0] = OP_PUSH_REG + regname;
	return &opstream[1];
}

static uint8_t *
op_jmp(uint8_t *opstream, void *func)
{
	opstream[0] = OP_JMP_REL32;
	* ((int32_t*)&opstream[1]) = ((int32_t) func - (((int32_t) &opstream[5] - (int32_t) curr_virt_addr)));
	return &opstream[5];
}

static uint8_t *
op_call(uint8_t *opstream, void *func)
{
	opstream[0] = OP_CALL_REL32;
	* ((int32_t*)&opstream[1]) = ((int32_t) func - (((int32_t) &opstream[5] - (int32_t) curr_virt_addr)));
	return &opstream[5];
}


static uint8_t *
set_reg(uint8_t *opstream, int reg, uint32_t value)
{
	opstream[0] = OP_MOV_IMM32 + reg;
	* ((uint32_t*)&opstream[1]) = (uint32_t) value;
	return &opstream[5];
}

static uint8_t *
set_reg_indirect(uint8_t *opstream, int reg, uint32_t value)
{
	opstream[0] = OP_MOV_FROM_MEM32;
	opstream[1] = reg << 3 | 5;
	* ((uint32_t*)&opstream[2]) = value;
	return &opstream[6];
}

static uint8_t *
switch_stack(uint8_t *opstream) 
{
	/* Save current stack */
	opstream[0] = OP_MOV_MEM32;
	opstream[1] = OP_REG_ESP << 3 | 5;
	* ((uint32_t*)&opstream[2]) = (uint32_t) (&end_stack - 1);

	return set_reg(&opstream[6], OP_REG_ESP, (uint32_t) (&end_stack - 5));
}

struct relocation {
	uint32_t r_offset;
	uint32_t r_info;
};
static const uint8_t R_386_32 = 1;
static const uint8_t R_386_PC32 = 2;


/* FIXME: Check things that may need to be movzx'ed */

void
apply_patchup(void *elf)
{
	int burn_section = elf_getSectionNamed(elf, ".afterburn");
	ASSERT(burn_section != -1);

	uintptr_t *burn_addr;
	uintptr_t value;
	int len;
	int i;

	len = elf_getSectionSize(elf, burn_section) / 4;

	burn_addr = (uintptr_t*) ((uintptr_t) elf_getSectionOffset(elf, burn_section) + (uintptr_t) elf);

	for (i = 0; i < len; i++) {
		uint8_t *opstream = (uint8_t*)burn_addr[i];
		int sreg;
		int reg;
		int rm;
		int mod;
		uint32_t jmp_target;
		uint16_t segment;
		/* Instruction decoding */
		switch(opstream[0]) {
		case OP_HLT:
			opstream = switch_stack(opstream);
			opstream = op_call(opstream, (void*) burn_hlt);
			break;
		case OP_INT:
			opstream = op_call(opstream, (void*) burn_int);
			break;
		case OP_IRET:
			opstream = op_jmp(opstream, (void*) burn_iret);
			break;
		case OP_JMP_FAR:
			jmp_target = *((uint32_t*)&opstream[1]);
			segment = *((uint16_t*)&opstream[5]);
			opstream = push_word(opstream, segment);
			opstream = push_word(opstream, jmp_target);
			opstream = op_jmp(opstream, (void*) burn_farjmp);
			/* We make the hellish assumption that now we are no longer in physical mode! */
			curr_virt_addr = (void*) 0x0;
			break;
		case OP_OUT:
			value = opstream[1];
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 32);
			opstream = push_word(opstream, value);
			opstream = op_call(opstream, (void*) burn_out);
			break;
		case OP_OUT_DX:
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 32);
			opstream = push_reg(opstream, OP_REG_EDX);
			opstream = op_call(opstream, (void*) burn_out);
			break;
		case OP_OUTB:
			value = opstream[1];
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 0x8);
			opstream = push_word(opstream, value);
			opstream = op_call(opstream, (void*) burn_out);
			break;
		case OP_OUTB_DX:
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 0x8);
			opstream = push_reg(opstream, OP_REG_EDX);
			opstream = op_call(opstream, (void*) burn_out);
			break;
		case OP_IN:
			value = opstream[1];
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 32);
			opstream = push_word(opstream, value);
			opstream = op_call(opstream, (void*) burn_in);
			break;
		case OP_IN_DX:
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 32);
			opstream = push_reg(opstream, OP_REG_EDX);
			opstream = op_call(opstream, (void*) burn_in);
			break;
		case OP_INB:
			value = opstream[1];
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 0x8);
			opstream = push_word(opstream, value);
			opstream = op_call(opstream, (void*) burn_in);
			break;
		case OP_INB_DX:
			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, 0x8);
			opstream = push_reg(opstream, OP_REG_EDX);
			opstream = op_call(opstream, (void*) burn_in);
			break;

		case OP_SIZE_OVERRIDE:
			/* Currently we only handle the out things that are overridden */
			/* In this case overriden means that we have 16 not 32 bit in/out
			   Strictly this depends on a bit in the CS(?) (i think.. read the docs!)
			   But we assume 32bit mode as stadnard, so switches mean go to 16bit */
			/* This is going to get ugly  -- this code needs refactoring! */
			switch(opstream[1]) {
			case OP_OUT:
				con << "Out on port: " << (void*) (uintptr_t) opstream[1] << " \n";
				break;
			case OP_OUT_DX:
				opstream = switch_stack(opstream);
				opstream = push_byte(opstream, 16);
				opstream = push_reg(opstream, OP_REG_EDX);
				opstream = op_call(opstream, (void*) burn_out);
				break;
			case OP_IN:
				con << "In on port: " << (void*) (uintptr_t) opstream[1] << " \n";
				break;
			case OP_IN_DX:
				opstream = switch_stack(opstream);
				opstream = push_byte(opstream, 16);
				opstream = push_reg(opstream, OP_REG_EDX);
				opstream = op_call(opstream, (void*) burn_in);
				break;
			default:
				ASSERT(!"badness");
			}
			break;

		case OP_PUSHF:
			/* We don't switch stacks yet since we must have a stack for a pop/pushf */
			opstream = op_call(opstream, (void*) burn_pushf);
			break;
		case OP_POPF:
			opstream = op_call(opstream, (void*) burn_popf);
			break;

		case OP_MOV_TOSEG:
			sreg = (opstream[1] >> 3) & 7;
			reg = opstream[1] & 7;
			
			opstream = switch_stack(opstream);
			opstream = push_reg(opstream, reg);
			opstream = push_byte(opstream, sreg | 0x80);
			opstream = op_call(opstream, (void*) burn_mov);

			break;
		case OP_MOV_FROMSEG:
			break;
#if 0
			/* FIXME */
			sreg = (opstream[1] >> 3) & 7;
			reg = opstream[1] & 7;
			sreg |= 0x80;

			opstream = switch_stack(opstream);
			opstream = push_byte(opstream, sreg);
			/* FIXME: Doesn't handle address mode at all! */
			switch(reg) {
			case OP_REG_EAX:
				opstream = op_call(opstream, (void*) burn_mov_to_eax);
				break;
			case OP_REG_ECX:
				opstream = op_call(opstream, (void*) burn_mov_to_ecx);
				break;
			case OP_REG_EDX:
				opstream = op_call(opstream, (void*) burn_mov_to_edx);
				break;
			case OP_REG_EBX:
				opstream = op_call(opstream, (void*) burn_mov_to_ebx);
				break;
			case OP_REG_ESI:
				opstream = op_call(opstream, (void*) burn_mov_to_esi);
				break;
			default:
				con << "unknown register " << reg << ' ' << opstream << '\n';
				ASSERT(!"move to unknown\n");
				break;
			}
#endif
			break;
		case OP_CLI:
			opstream = switch_stack(opstream);
			opstream = op_call(opstream, (void*) burn_cli);
			break;
		case OP_STI:
			opstream = switch_stack(opstream);
			opstream = op_call(opstream, (void*) burn_sti);
			break;
		case OP_2BYTE:
			switch(opstream[1]) {
			case OP_CPUID:
				opstream = switch_stack(opstream);
				opstream = op_call(opstream, (void*) burn_cpuid);

				break;
			case OP_LSS:
				reg = (opstream[2] >> 3) & 7;
				value = *((uint32_t*) &opstream[3]);
				sreg = 0x2;
				opstream = switch_stack(opstream);
				opstream = push_word(opstream, value); /* push word indirect ? */
				opstream = push_byte(opstream, sreg | 0x80);
				opstream = op_call(opstream, (void*) burn_mov);
				opstream = set_reg_indirect(opstream, reg, value);
				break;
			case OP_MOV_FROMCREG:
			case OP_MOV_FROMDREG:
				sreg = (opstream[2] >> 3) & 7;
				if (opstream[1] == OP_MOV_FROMDREG)
					sreg |= 0x40;
				reg = opstream[2] & 7;


				opstream = switch_stack(opstream);
				opstream = push_byte(opstream, sreg);
				switch(reg) {
				case OP_REG_EAX:
					opstream = op_call(opstream, (void*) burn_mov_to_eax);
					break;
				case OP_REG_ECX:
					opstream = op_call(opstream, (void*) burn_mov_to_ecx);
					break;
				case OP_REG_EDX:
					opstream = op_call(opstream, (void*) burn_mov_to_edx);
					break;
				case OP_REG_EBX:
					opstream = op_call(opstream, (void*) burn_mov_to_ebx);
					break;
				case OP_REG_ESI:
					opstream = op_call(opstream, (void*) burn_mov_to_esi);
					break;
				default:
					con << "unknown register " << reg << ' ' << opstream << '\n';
					ASSERT(!"move to unknown\n");
					break;
				}
				break;
			case OP_MOV_TOCREG:
			case OP_MOV_TODREG:
				sreg = (opstream[2] >> 3) & 7;
				if (opstream[1] == OP_MOV_TODREG)
					sreg |= 0x40;
				reg = opstream[2] & 7;
				
				opstream = switch_stack(opstream);
				opstream = push_reg(opstream, reg);
				opstream = push_byte(opstream, sreg);
				opstream = op_call(opstream, (void*) burn_mov);
				break;
			case OP_CLTS:
				opstream = switch_stack(opstream);
				opstream = op_call(opstream, (void*) burn_clts);
				break;
			case OP_LLTL:
				sreg = (opstream[2] >> 3) & 7;
				reg = opstream[2] & 7;
				opstream = switch_stack(opstream);
				opstream = push_reg(opstream, reg);
				switch(sreg) {
				case 3:
					opstream = op_call(opstream, (void*) burn_ltr);
					break;
				case 2:
					opstream = op_call(opstream, (void*) burn_lldt);
					break;
				default:
					con << "unknown sub type " << sreg << "\n";
					ASSERT(!"don't go on");
				}
				break;
			case OP_LDTL:
				reg = (opstream[2] >> 3) & 7;
				mod = (opstream[2] >> 6);
				rm = opstream[2] & 7;
				value = opstream[3];
				switch(mod) {
				case 0:
					switch(rm) {
					case 5:
						value = *((uint32_t*) &opstream[3]);
						opstream = switch_stack(opstream);
						opstream = push_word(opstream, value);
						break;
					case 4:
						/* FIXME: this is broken!  */
						value = *((uint32_t*) &opstream[4]);
						opstream = switch_stack(opstream);
						opstream = push_word(opstream, value);
						break;
					default:
						/* register value */
						opstream = switch_stack(opstream);
						opstream = push_reg(opstream, rm);
						break;
					} 
					break;
				case 1:
					/* Disp8 */
					if (opstream[3] == 0) {
						opstream = push_reg(opstream, rm);
						break;
					} else {
						ASSERT(!"ERROR, can't rewrite offset");
					}
				default:
					ASSERT(!"EREROR!");
				}
				
				switch(reg) {
				case 7:
					opstream = op_call(opstream, (void*) burn_invlpg);
					break;
				case 3:
					opstream = op_call(opstream, (void*) burn_idt);
					break;
				case 2:
					opstream = op_call(opstream, (void*) burn_gdt);
					break;
				default:
					con << "unknown sub " << reg << "\n";
					ASSERT(!"don't go on");
				}
				break;
			case OP_WRMSR:
				opstream = switch_stack(opstream);
				opstream = op_call(opstream, (void*) burn_wrmsr);
				break;
			case OP_RDMSR:
				opstream = switch_stack(opstream);
				opstream = op_call(opstream, (void*) burn_rdmsr);
				break;
			default:
				con << "unknown " << (void*) (uintptr_t) opstream[1] << "\n";
				ASSERT(!"Unknown 2 byte opcode");
			}
			break;
		default:
			con << "unknown " << (void*) (uintptr_t) opstream[0] << " " << opstream <<"\n";
			ASSERT(!"Unknown opcode");
		}
	}
}
