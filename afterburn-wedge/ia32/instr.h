#if defined(AFTERBURN_USER)
# include "afterburn-instr-user.h"
#elif defined(CONFIG_AFTERBURN_STATIC)
# include "afterburn-instr-kernel.h"
#elif defined(CONFIG_AFTERBURN_VMI)
# include "afterburn-instr-kernel.vmi.h"
#else
# include "afterburn-instr-kernel.patchup.h"
#endif

