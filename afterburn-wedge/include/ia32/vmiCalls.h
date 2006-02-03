/* **********************************************************
 * Copyright 2004, 2005 VMware, Inc.  All rights reserved.
 * Copyright 2005, Volkmar Uhlig
 * **********************************************************/

/*
 * vmiCalls.h
 *
 * The VMI ROM layout is defined by this header file.  Each entry represents
 * a single slot in the VROM, which is 32 bytes in size.  This file mayy be
 * pre-processed to generate inline function prototypes required to call these
 * functions from GNU C.  There are three types of definitions:
 *
 *   VMI_SPACE - a reserved space in the VROM table;
 *   VMI_FUNC  - a function which returns a single (possibly 64-bit) value;
 *   VMI_PROC  - defines a function which returns no value;
 *   VMI_JUMP  - defines a function which does not return;
 *
 *   All functions have the same general macro layout which easily converts to
 *   inline assembler.
 *
 *      TYPEDEF (return type, if applicable),
 *      ARGS (C function arguments),
 *      OUTPUT (how values are returned),
 *      INPUT (how values are passed in),
 *      CLOBBER (what if anything is clobbered),
 *      ATTRIBUTES (special attributes of the function)
 *
 *   Types are defined via abstract type definitions so that they may be
 *   redefined local as convenient.
 *
 *   These function definitions are defined to be C-compatible with regparm(3)
 *   argument passing wherever possible, but some of these functions must be
 *   available to assembler code, and emulate hardware instructions with fixed
 *   register input.  In these cases, an argument convention that mimics the
 *   hardware instruction has been chosen.  In addition, the amount of state
 *   which is clobbered by any VMI function is extremely minimal.  To allow
 *   for conditional branch logic in the implementation of a VMI function,
 *   clobbering of flags is almost universally allowed (VMI_ASM_CLOBBER), but
 *   for most cases, clobbering of registers is not allowed.  This allows the
 *   compiler to optimize code around VMI callouts almost identically to the
 *   way it would optimize for native hardware.
 *
 *   While the implementation of these functions is relocatable code, most
 *   functions require flat segments, which are zero based, maximal limit
 *   segments for CS, DS, ES, and SS.  The flatness requirements are tagged
 *   with attributes.  Flatness is never assumed for FS and GS segments.
 *
 *      VMI_FLAT     - CS, DS, ES, SS must be flat
 *      VMI_FLAT_SS  - CS, SS must be flat
 *      VMI_NOT_FLAT - no assumptions are made
 *
 * Zachary Amsden <zach@vmware.com>
 *
 * VU: Extended the interface with additional VMI calls to support DMA
 * and ptab value translations.  Ptab reads are required when the
 * guest and host pagetables are shared.  The read value may contain
 * incorrect addresses and valid bits (although, a better solution is
 * support of a read function).
 *
 * Volkmar Uhlig <volkmar@volkmaruhlig.com> 
 *
 *
 */

#define VMI_CALLS \
/*                                                                                               \
 * Space for the ROM and VROM headers and 16-bit init code.                                      \
 * PnP and PCI header are stored after ROM code segment                                          \
 */                                                                                              \
VMI_SPACE( ROM_HEADER0 )                                                                         \
VMI_SPACE( ROM_HEADER1 )                                                                         \
                                                                                                 \
/*                                                                                               \
 * Space for the bare ELF header and interpreter.                                                \
 * Program header and section header stored after ROM code segment                               \
 */                                                                                              \
VMI_SPACE( ELF_HEADER0 )                                                                         \
VMI_SPACE( ELF_HEADER1 )                                                                         \
                                                                                                 \
/* Initialize the per-cpu VMI data area and enter the CPU into VMI mode */                       \
VMI_FUNC( Init,                                                                                  \
          VMI_UINT32, VMI_ARGS(void *dataArea),                                                  \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (dataArea)),                                                             \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary CPUID equivalent - ebx, ecx, edx returned via variables, eax live */                   \
VMI_FUNC( CPUID,                                                                                 \
          VMI_UINT32, VMI_ARGS(VMI_UINT32 func, VMI_UINT32 *b, VMI_UINT32 *c, VMI_UINT32 *d),    \
          VMI_OUTPUT("=a"(ret),"=b"(*b),"=c"(*c),"=d"(*c)),                                      \
          VMI_INPUT("0" (func)),                                                                 \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary WRMSR equivalent */                                                                    \
VMI_PROC( WRMSR,                                                                                 \
          VMI_ARGS(VMI_UINT32 msr, VMI_UINT64 value),                                            \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("A" (value), "c" (msr)),                                                     \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary RDMSR equivalent */                                                                    \
VMI_FUNC( RDMSR,                                                                                 \
          VMI_UINT64, VMI_ARGS(VMI_UINT32 msr),                                                  \
          VMI_OUTPUT("=A" (ret)),                                                                \
          VMI_INPUT("c" (msr)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary LGDT equivalent */                                                                     \
VMI_PROC( SetGDT,                                                                                \
          VMI_ARGS(VMI_DTR *dtr),                                                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dtr)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary LIDT equivalent */                                                                     \
VMI_PROC( SetIDT,                                                                                \
          VMI_ARGS(VMI_DTR *dtr),                                                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dtr)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary LLDT equivalent */                                                                     \
VMI_PROC( SetLDT,                                                                                \
          VMI_ARGS(VMI_SELECTOR sel),                                                            \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (sel)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary LTR equivalent */                                                                      \
VMI_PROC( SetTR,                                                                                 \
          VMI_ARGS(VMI_SELECTOR sel),                                                            \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (sel)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary SGDT equivalent */                                                                     \
VMI_PROC( GetGDT,                                                                                \
          VMI_ARGS(VMI_DTR *dtr),                                                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dtr)),                                                                  \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary SIDT equivalent */                                                                     \
VMI_PROC( GetIDT,                                                                                \
          VMI_ARGS(VMI_DTR *dtr),                                                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dtr)),                                                                  \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary SLDT equivalent */                                                                     \
VMI_FUNC( GetLDT,                                                                                \
          VMI_SELECTOR, VMI_ARGS(void),                                                          \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary STR equivalent */                                                                      \
VMI_FUNC( GetTR,                                                                                 \
          VMI_SELECTOR, VMI_ARGS(void),                                                          \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* GDT update notification */                                                                    \
VMI_PROC( UpdateGDT,                                                                             \
          VMI_ARGS(VMI_UINT32 first, VMI_UINT32 last),                                           \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (first), "d" (last)),                                                    \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* IDT update notification */                                                                    \
VMI_PROC( UpdateIDT,                                                                             \
          VMI_ARGS(VMI_UINT32 first, VMI_UINT32 last),                                           \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (first), "d" (last)),                                                    \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* LDT update notification */                                                                    \
VMI_PROC( UpdateLDT,                                                                             \
          VMI_ARGS(VMI_UINT32 first, VMI_UINT32 last),                                           \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (first), "d" (last)),                                                    \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* TSS ss0 / esp0 update notification */                                                         \
VMI_PROC( UpdateKernelStack,                                                                     \
          VMI_ARGS(VMI_SELECTOR ss0, char *esp0),                                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (ss0), "d" (esp0)),                                                      \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %eax, %cr0 equivalent */                                                           \
VMI_PROC( SetCR0,                                                                                \
          VMI_ARGS(VMI_UINT32 val),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %eax, %cr2 equivalent */                                                           \
VMI_PROC( SetCR2,                                                                                \
          VMI_ARGS(VMI_UINT32 val),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %eax, %cr3 equivalent */                                                           \
VMI_PROC( SetCR3,                                                                                \
          VMI_ARGS(VMI_UINT32 val),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %eax, %cr4 equivalent */                                                           \
VMI_PROC( SetCR4,                                                                                \
          VMI_ARGS(VMI_UINT32 val),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val)),                                                                  \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %cr0, %eax equivalent */                                                           \
VMI_FUNC( GetCR0,                                                                                \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %cr2, %eax equivalent */                                                           \
VMI_FUNC( GetCR2,                                                                                \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %cr3, %eax equivalent */                                                           \
VMI_FUNC( GetCR3,                                                                                \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary mov %cr4, %eax equivalent */                                                           \
VMI_FUNC( GetCR4,                                                                                \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary LMSW equivalent */                                                                     \
VMI_PROC( LMSW,                                                                                  \
          VMI_ARGS(VMI_UINT16 word),                                                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (word)),                                                                 \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary SMSW equivalent */                                                                     \
VMI_FUNC( SMSW,                                                                                  \
          VMI_UINT16, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary CLTS equivalent */                                                                     \
VMI_PROC( CLTS,                                                                                  \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set TS bit in CR0 */                                                                          \
VMI_PROC( STTS,                                                                                  \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set debug register */                                                                         \
VMI_PROC( SetDR,                                                                                 \
          VMI_ARGS(VMI_UINT32 num, VMI_UINT32 val),                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (num), "d" (val)),                                                       \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Get debug register */                                                                         \
VMI_FUNC( GetDR,                                                                                 \
          VMI_UINT32, VMI_ARGS(VMI_UINT32 num),                                                  \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (num)),                                                                  \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary RDPMC equivalent */                                                                    \
VMI_FUNC( RDPMC,                                                                                 \
          VMI_UINT64, VMI_ARGS(VMI_UINT32 num),                                                  \
          VMI_OUTPUT("=A" (ret)),                                                                \
          VMI_INPUT("c" (num)),                                                                  \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary RDTSC equivalent */                                                                    \
VMI_FUNC( RDTSC,                                                                                 \
          VMI_UINT64, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=A" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INVD equivalent */                                                                     \
VMI_PROC( INVD,                                                                                  \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary WBINVD equivalent */                                                                   \
VMI_PROC( WBINVD,                                                                                \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INB equivalent */                                                                      \
VMI_FUNC( INB,                                                                                   \
          VMI_UINT8,  VMI_ARGS(VMI_UINT32 port),                                                 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INW equivalent */                                                                      \
VMI_FUNC( INW,                                                                                   \
          VMI_UINT16,  VMI_ARGS(VMI_UINT32 port),                                                \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INL equivalent */                                                                      \
VMI_FUNC( INL,                                                                                   \
          VMI_UINT32,  VMI_ARGS(VMI_UINT32 port),                                                \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTB equivalent */                                                                     \
VMI_PROC( OUTB,                                                                                  \
          VMI_ARGS(VMI_UINT8 val, VMI_UINT32 port),                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val), "d" (port)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTW equivalent */                                                                     \
VMI_PROC( OUTW,                                                                                  \
          VMI_ARGS(VMI_UINT16 val, VMI_UINT32 port),                                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val), "d" (port)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTL equivalent */                                                                     \
VMI_PROC( OUTL,                                                                                  \
          VMI_ARGS(VMI_UINT32 val, VMI_UINT32 port),                                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (val), "d" (port)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INSB equivalent */                                                                     \
VMI_PROC( INSB,                                                                                  \
          VMI_ARGS(VMI_UINT8 *addr, VMI_UINT32 port, VMI_UINT32 count),                          \
          VMI_OUTPUT("+D" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INSW equivalent */                                                                     \
VMI_PROC( INSW,                                                                                  \
          VMI_ARGS(VMI_UINT16 *addr, VMI_UINT32 port, VMI_UINT32 count),                         \
          VMI_OUTPUT("+D" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INSL equivalent */                                                                     \
VMI_PROC( INSL,                                                                                  \
          VMI_ARGS(VMI_UINT32 *addr, VMI_UINT32 port, VMI_UINT32 count),                         \
          VMI_OUTPUT("+D" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTSB equivalent */                                                                    \
VMI_PROC( OUTSB,                                                                                 \
          VMI_ARGS(VMI_UINT8 *addr, VMI_UINT32 port, VMI_UINT32 count),                          \
          VMI_OUTPUT("+S" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTSW equivalent */                                                                    \
VMI_PROC( OUTSW,                                                                                 \
          VMI_ARGS(VMI_UINT16 *addr, VMI_UINT32 port, VMI_UINT32 count),                         \
          VMI_OUTPUT("+S" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary OUTSL equivalent */                                                                    \
VMI_PROC( OUTSL,                                                                                 \
          VMI_ARGS(VMI_UINT32 *addr, VMI_UINT32 port, VMI_UINT32 count),                         \
          VMI_OUTPUT("+S" (addr), "+c" (count)),                                                 \
          VMI_INPUT("d" (port)),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set current IOPL mask */                                                                      \
VMI_PROC( SetIOPLMask,                                                                           \
          VMI_ARGS(VMI_UINT32 mask),                                                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (mask)),                                                                 \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Get currnet IOPL mask */                                                                      \
VMI_FUNC( GetIOPLMask,                                                                           \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Notify hypervisor of changes to IO bitmap */                                                  \
VMI_PROC( UpdateIOBitmap,                                                                        \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Notify hypervisor of changes to VME interrupt redirection bitmap */                           \
VMI_PROC( UpdateInterruptBitmap,                                                                 \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set all EFLAGS except VIF, VIP, VM */                                                         \
VMI_PROC( SetEFLAGS,                                                                             \
          VMI_ARGS(VMI_UINT32 flags),                                                            \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (flags)),                                                                \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Get all EFLAGS */                                                                             \
VMI_FUNC( GetEFLAGS,                                                                             \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Clear EFLAGS_IF, return old value */                                                          \
VMI_FUNC( SaveAndDisableIRQs,                                                                    \
          VMI_UINT32, VMI_ARGS(void),                                                            \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Restore EFLAGS_IF from saved value */                                                         \
VMI_PROC( RestoreIRQs,                                                                           \
          VMI_ARGS(VMI_UINT32 mask),                                                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (mask)),                                                                 \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary STI equivalent */                                                                      \
VMI_PROC( STI,                                                                                   \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Binary CLI equivalent */                                                                      \
VMI_PROC( CLI,                                                                                   \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Perform an IRET; may not change IOPL, but EFLAGS_IF restored */                               \
VMI_JUMP( IRET,                                                                                  \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Perform an IRET as above, but on a 16-bit stack */                                            \
VMI_JUMP( IRET16,                                                                                \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_NOT_FLAT, VMI_VOLATILE))                                            \
                                                                                                 \
/* Binary SYSEXIT equivalent */                                                                  \
VMI_JUMP( SysExit,                                                                               \
          VMI_ARGS(VMI_UINT32 eip, VMI_UINT32 esp),                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("d" (eip), "c" (esp)),                                                       \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Enable interrupts and holdoff until SYSEXIT completes */                                      \
VMI_JUMP( StiSysexit,                                                                            \
          VMI_ARGS(VMI_UINT32 eip, VMI_UINT32 esp),                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("d" (eip), "c" (esp)),                                                       \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Binary SYSRET equivalent */                                                                   \
VMI_JUMP( SysRet,                                                                                \
          VMI_ARGS(VMI_UINT32 eip),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("c" (eip)),                                                                  \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT_SS, VMI_VOLATILE))                                             \
                                                                                                 \
/* Enable interrupts and halt until interrupted */                                               \
VMI_PROC( SafeHalt,                                                                              \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Halt until interrupted */                                                                     \
VMI_PROC( Halt,                                                                                  \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary PAUSE equivalent */                                                                    \
VMI_PROC( Pause,                                                                                 \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Shutdown this processor */                                                                    \
VMI_PROC( Shutdown,                                                                              \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Reboot the machine */                                                                         \
VMI_PROC( Reboot,                                                                                \
          VMI_ARGS(VMI_UINT32 how),                                                              \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Translates the value of a pte */                                                              \
VMI_FUNC( GetPteVal,                                                                             \
          VMI_UINT32, VMI_ARGS(VMI_PTE pte),							 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (pte)),			                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
												 \
VMI_FUNC( GetPdeVal,                                                                             \
          VMI_UINT32, VMI_ARGS(VMI_PTE pde),							 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (pde)),			                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
												 \
/* Set a non-PAE P{T|D}E */                                                                      \
VMI_PROC( SetPte,                                                                                \
          VMI_ARGS(VMI_PTE pte, VMI_PTE *ptep, VMI_UINT32 level),                                \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (pte), "d" (ptep), "c" (level)),                                         \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically swap a non-PAE PTE */                                                              \
VMI_FUNC( SwapPte,                                                                               \
          VMI_PTE, VMI_ARGS(VMI_PTE pte, VMI_PTE *ptep),                                         \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (pte), "d" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically test and set a bit in a PTE */                                                     \
VMI_FUNC( TestAndSetPteBit,                                                                      \
          VMI_BOOL, VMI_ARGS(VMI_INT bit, VMI_PTE *ptep),                                        \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (bit), "d" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically test and clear a bit in a PTE */                                                   \
VMI_FUNC( TestAndClearPteBit,                                                                    \
          VMI_BOOL, VMI_ARGS(VMI_INT bit, VMI_PTE *ptep),                                        \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (bit), "d" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set a PAE P{T|D|PD}E */                                                                       \
VMI_PROC( SetPteLong,                                                                            \
          VMI_ARGS(VMI_PAE_PTE pte, VMI_PAE_PTE *ptep),                                          \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("A" (pte), "c" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically swap a PAE PTE */                                                                  \
VMI_FUNC( SwapPteLong,                                                                           \
          VMI_PAE_PTE, VMI_ARGS(VMI_UINT64 pte, VMI_PAE_PTE *ptep),                              \
          VMI_OUTPUT("=A" (ret)),                                                                \
          VMI_INPUT("b" ((VMI_UINT32)(pte)), "c" ((VMI_UINT32)((pte)>>32)), "D" (ptep)),         \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically test and set a bit in a PAE PTE */                                                 \
VMI_FUNC( TestAndSetPteBitLong,                                                                  \
          VMI_BOOL, VMI_ARGS(VMI_INT bit, VMI_PAE_PTE *ptep),                                    \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (bit), "d" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Atomically test and clear a bit in a PAE PTE */                                               \
VMI_FUNC( TestAndClearPteBitLong,                                                                \
          VMI_BOOL, VMI_ARGS(VMI_INT bit, VMI_PAE_PTE *ptep),                                    \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (bit), "d" (ptep)),                                                      \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Clone a new PT from an existing page */                                                       \
VMI_PROC( ClonePageTable,                                                                        \
          VMI_ARGS(VMI_UINT32 dstPPN, VMI_UINT32 srcPPN, VMI_UINT32 flags),                      \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dstPPN), "d" (srcPPN), "c" (flags)),                                    \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Clone a new PD from an existing page */                                                       \
VMI_PROC( ClonePageDirectory,                                                                    \
          VMI_ARGS(VMI_UINT32 dstPPN, VMI_UINT32 srcPPN, VMI_UINT32 flags),                      \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (dstPPN), "d" (srcPPN), "c" (flags)),                                    \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Notify hypervisor about maps of page primaries */                                             \
VMI_PROC( RegisterPageMapping,                                                                   \
          VMI_ARGS(VMI_UINT32 va, VMI_UINT32 ppn, VMI_UINT32 pages),                             \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (va), "d" (ppn), "c" (pages)),                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Notify hypervisor how a page is being used (PT, PD, GDT, etc.) */                             \
VMI_PROC( RegisterPageUsage,                                                                     \
          VMI_ARGS(VMI_UINT32 ppn, int flags),                                                   \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (ppn), "d" (flags)),                                                     \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Notify hypervisor a page is no longer being used */                                           \
VMI_PROC( ReleasePage,                                                                           \
          VMI_ARGS(VMI_UINT32 ppn, int flags),                                                   \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (ppn), "d" (flags)),                                                     \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Binary INVLPG equivalent */                                                                   \
VMI_PROC( InvalPage,                                                                             \
          VMI_ARGS(VMI_UINT32 va),                                                               \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT("a" (va)),                                                                   \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Flush local TLB */                                                                            \
VMI_PROC( FlushTLB,                                                                              \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Flush local TLB, including global entries */                                                  \
VMI_PROC( FlushTLBAll,                                                                           \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Set new lazy update mode */                                                                   \
VMI_PROC( SetDeferredMode,                                                                       \
          VMI_ARGS(VMI_UINT32 deferBits),                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
                                                                                                 \
/* Flush all pending CPU & MMU state updates */                                                  \
VMI_PROC( FlushDeferredCalls,                                                                    \
          VMI_ARGS(void),                                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_ASM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
												 \
/* Translate guest physical to machine physical */                                               \
VMI_FUNC( PhysToMachine,                                                                         \
          VMI_UINT32, VMI_ARGS(VMI_UINT32 addr),						 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (addr)),		                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
												 \
/* Translate machine physical to guest physical */                                               \
VMI_FUNC( MachineToPhys,                                                                         \
          VMI_UINT32, VMI_ARGS(VMI_UINT32 addr),						 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT("0" (addr)),		                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \

#if 0

/* Infrastructure to support direct access to the Xt-PIC. */

VMI_PROC( OUT_20,                                                                       \
          VMI_ARGS(VMI_UINT8 val),                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_PROC( OUT_21,                                                                       \
          VMI_ARGS(VMI_UINT8 val),                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_PROC( OUT_a0,                                                                       \
          VMI_ARGS(VMI_UINT8 val),                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_PROC( OUT_a1,                                                                       \
          VMI_ARGS(VMI_UINT8 val),                                                        \
          VMI_OUTPUT(),                                                                          \
          VMI_INPUT(),                                                                           \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_FUNC( IN_20,                                                                                   \
          VMI_UINT8,  VMI_ARGS(void),                                                 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_FUNC( IN_21,                                                                                   \
          VMI_UINT8,  VMI_ARGS(void),                                                 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_FUNC( IN_a0,                                                                                   \
          VMI_UINT8,  VMI_ARGS(void),                                                 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \
VMI_FUNC( IN_a1,                                                                                   \
          VMI_UINT8,  VMI_ARGS(void),                                                 \
          VMI_OUTPUT("=a" (ret)),                                                                \
          VMI_INPUT(),                                                                 \
          VMI_MEM_CLOBBER,                                                                       \
          VMI_ATTRIBUTES(VMI_FLAT, VMI_VOLATILE))                                                \

#endif

