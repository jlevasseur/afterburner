#ifndef __LINUX__COM__H__
#define __LINUX__COM__H__

#include <stdint.h>

struct comm_buffer {
	uintptr_t old_ip;
	uintptr_t old_sp;
	void *fn;
	void *addr;
	off_t offset;
	size_t length;
	int prot;
	cpu_t cpu;
};

extern struct comm_buffer *comm_buffer;

#endif /*__LINUX__COM__H__ */
