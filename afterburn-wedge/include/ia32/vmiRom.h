/*
 * vmiRom.h --
 *
 *      Header file for paravirtualization interface and definitions
 *      for the hypervisor option ROM tables.
 *
 *      Copyright (C) 2005, VMWare, Inc.
 *
 */

#ifndef __AFTERBURN_WEDGE__INCLUDE__IA32__VMIROM_H__
#define __AFTERBURN_WEDGE__INCLUDE__IA32__VMIROM_H__

#include INC_ARCH(vmiCalls.h)

/*
 *---------------------------------------------------------------------
 *
 *  VMI Option ROM API
 *
 *---------------------------------------------------------------------
 */

#define VMI_SPACE(call, ...)
#define VMI_FUNC(call, ...) VMI_CALL_##call,
#define VMI_PROC(call, ...) VMI_CALL_##call,
#define VMI_JUMP(call, ...) VMI_CALL_##call,

typedef enum VMICall {
	VMI_CALLS
} VMICall;

#undef VMI_SPACE
#undef VMI_FUNC
#undef VMI_PROC
#undef VMI_JUMP

#define VMI_SIGNATURE 0x696d5663   /* "cVmi" */

#define VMI_API_REV_MAJOR          6
#define VMI_API_REV_MINOR          1

/* Flags used by VMI_Reboot call */
#define VMI_REBOOT_SOFT          0x0
#define VMI_REBOOT_HARD          0x1

/* Flags used by VMI_RegisterPageMapping call */
#define VMI_PAGE_PT              0x00
#define VMI_PAGE_PD              0x01
#define VMI_PAGE_PAE_PT          0x04
#define VMI_PAGE_PAE_PD          0x05
#define VMI_PAGE_PDP             0x06
#define VMI_PAGE_PML4            0x07

/* Flags for VMI_ClonePage{Table|Directory} call */
#define VMI_MKCLONE(start, count) (((start) << 16) | (count))

/* Flags used by VMI_SetDeferredMode call */
#define VMI_DEFER_NONE		0x00
#define VMI_DEFER_MMU		0x01
#define VMI_DEFER_CPU		0x02
#define VMI_DEFER_DT		0x04

/*
 *---------------------------------------------------------------------
 *
 *  Generic VROM structures and definitions
 *
 *---------------------------------------------------------------------
 */

/* VROM call table definitions */
#define VROM_CALL_LEN             32

typedef struct VROMCallEntry {
   char f[VROM_CALL_LEN];
} VROMCallEntry;

typedef struct VROMHeader {
   VMI_UINT16          romSignature;             // option ROM signature
   VMI_INT8            romLength;                // ROM length in 512 byte chunks
   unsigned char       romEntry[4];              // 16-bit code entry point
   VMI_UINT8           romPad0;                  // 4-byte align pad
   VMI_UINT32          vRomSignature;            // VROM identification signature
   VMI_UINT8           APIVersionMinor;          // Minor version of API
   VMI_UINT8           APIVersionMajor;          // Major version of API
   VMI_UINT8           dataPages;                // Number of pages for data area
   VMI_UINT8           reserved0;                // Reserved for expansion
   VMI_UINT32          reserved1;                // Reserved for expansion
   VMI_UINT32          reserved2;                // Reserved for private use
   VMI_UINT16          pciHeaderOffset;          // Offset to PCI OPROM header
   VMI_UINT16          pnpHeaderOffset;          // Offset to PnP OPROM header
   VMI_UINT32          romPad3;                  // PnP reserverd / VMI reserved
   char                reserved[32];             // Reserved for future expansion
   char                elfHeader[64];            // Reserved for ELF header
   VROMCallEntry       vromCall[124];            // @ 0x80: ROM calls 4-127
} VROMHeader;

#endif	/* __AFTERBURN_WEDGE__INCLUDE__IA32__VMIROM_H__ */
