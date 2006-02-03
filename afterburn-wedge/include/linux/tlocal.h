#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__

#include INC_ARCH(cpu.h)
#include INC_WEDGE(vcpu.h)
#include <hiostream.h>

extern vcpu_t the_vcpu;

INLINE vcpu_t & get_vcpu() __attribute__((const));

INLINE vcpu_t & get_vcpu()
    // Get the thread local virtual CPU object.  Return a reference, so that by 
    // definition, we must return a valid object.
{
    return the_vcpu;
}

INLINE cpu_t & get_cpu() __attribute__((const));

INLINE cpu_t & get_cpu()
    // Get the thread local architecture CPU object.  Return a reference, so 
    // that by definition, we must return a valid object.
{
    return get_vcpu().cpu;
}

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4KA__TLOCAL_H__ */
