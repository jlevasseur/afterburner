#if defined(AFTERBURN_USER)
#include "afterburn-instr-user.S"
#elif defined(CONFIG_AFTERBURN_STATIC)
#include "afterburn-instr-kernel.S"
#elif defined(CONFIG_AFTERBURN_VMI)
#include "afterburn-instr-kernel.vmi.S"
#else
#include "afterburn-instr-kernel.patchup.S"
#endif

