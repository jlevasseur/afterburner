#ifndef __AFTERBURN_WEDGE__INCLUDE__L4KA__DEBUG_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4KA__DEBUG_H__

#include INC_WEDGE(console.h)

extern NORETURN void panic( void );

#define DEBUG_STREAM hiostream_linux_t

#define ASSERT(x)			\
    do { 				\
	if(EXPECT_FALSE(!(x))) { 	\
    	    con << "Assertion: " MKSTR(x) ",\nfile " __FILE__ \
	        << ":" << __LINE__ << ",\nfunc " \
	        << __func__ << '\n';	\
	    panic();			\
	}				\
    } while(0)

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4KA__DEBUG_H__ */
