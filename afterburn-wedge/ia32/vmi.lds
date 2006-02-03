#ifdef CONFIG_VMI_SUPPORT
#include "ia32/vmiCalls.h"
#define VMI_SPACE(name, ...) \
        . = __VMI_NEXT; \
        __VMI_NEXT = . + 32; \
        .vmi.VMI_##name : { *(.vmi.VMI_##name) }
                         
#define VMI_FUNC(name, ...) \
        . = __VMI_NEXT; \
        __VMI_NEXT = . + 32; \
        .vmi.VMI_##name : { *(.vmi.VMI_##name) }
                         
#define VMI_PROC(name, ...) \
        . = __VMI_NEXT; \
        __VMI_NEXT = . + 32; \
        .vmi.VMI_##name : { *(.vmi.VMI_##name) }
                         
#define VMI_JUMP(name, ...) \
        . = __VMI_NEXT; \
        __VMI_NEXT = . + 32; \
        .vmi.VMI_##name : { *(.vmi.VMI_##name) }

#define SECTION_VMI \
. = ALIGN(4096); \
__VMI_START = .; \
__VMI_NEXT = .;  \
__VMI_END = . + 16384; \
VMI_CALLS \
. = __VMI_END; \
. = ALIGN(4096);
#else
#define SECTION_VMI
#endif

