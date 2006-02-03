/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     afterburn-wedge/xen/main.cc
 * Description:   
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: main.cc,v 1.7 2005/04/13 15:07:13 joshua Exp $
 *
 ********************************************************************/

#include "burnxen.h"
#include <elfsimple.h>
#include <bind.h>
#include <dom0_ops.h>

bool backend_preboot(void *entry, void *entry_sp, char *command_line);

hiostream_xen_t con_driver;
hiostream_void_t void_driver;
hconsole_t con, debug_init, debug_vm;

start_info_t start_info;

char stack[8192];
char excpt_stack[8192];

vcpu_t the_vcpu;

unsigned long *phys_to_machine_mapping;

extern "C" void hypervisor_callback(void);

extern "C"
void
failsafe_callback(void)
{
	con << "Failsafe callback called. We are probably in trouble\n";
	while(1);
}

#if 0
void
print_pagetable(unsigned long* pdir) {
	int i=0;
	for(i= 0; i < 1024; i++) {
		if (pdir[i] != NULL) {
			con << (void*) (i << 22) << "(" << i << ")" << ": " << ((void*)(pdir[i] >> 12 << 12)) << " " << (void*) (machine_to_phys_mapping[pdir[i] >> 12] << 12) << "\n";
		}
	}
}
#endif


void
print_framelist(unsigned long* list, int n) {
	for (int i = 0; i < n; i++) {
		con << i << ": " << (void*)(list[i] << 12) << " " << (void*) machine_to_phys_mapping[list[i]] << "\n";
	}
}

static bool
apply_patchup(elf_ehdr_t *elf)
{
	// Patchup the binary.
	elf_shdr_t *elf_patchup = elf->get_section(".afterburn");
	if( !elf_patchup ) {
		con << "Missing patchup section.\n";
		return false;
	}

	patchup_info_t *patchups = 
		(patchup_info_t *)elf_patchup->get_file_data(elf);
	word_t count = elf_patchup->size / sizeof(patchup_info_t);

	if( !arch_apply_patchups(patchups, count, MB(64)-1) )
		return false;

	// Bind stuff to the guest OS.
	elf_shdr_t *elf_imports = elf->get_section(".afterburn.imports");
	if( !elf_imports ) {
		con << "Missing import section.\n";
		return false;
	}
	elf_bind_t *imports = (elf_bind_t *)elf_imports->get_file_data(elf);
	count = elf_imports->size / sizeof(elf_bind_t);
	if( !arch_bind_to_guest(imports, count) )
		return false;
	
	// Bind stuff from the guest OS.
	elf_shdr_t *elf_exports = elf->get_section(".afterburn.exports");
	if( !elf_exports ) {
		con << "Missing export section.\n";
		return false;
	}

	elf_bind_t *exports = (elf_bind_t *)elf_exports->get_file_data(elf);
	count = elf_exports->size / sizeof(elf_bind_t);
	if( !arch_bind_from_guest(exports, count) )
		return false;

	return true;
}

static void *
load_os(unsigned long offset)
{
	elf_ehdr_t *elf;

	elf = elf_is_valid(offset);
	ASSERT(elf != NULL);
	elf->load_phys(MB(64)-1);
	get_vcpu().set_kernel_vaddr(0x80000000);
	apply_patchup(elf);
	return (void*) ((MB(64)-1) & elf->entry);
}

extern char shared_info[PAGE_SIZE];

static shared_info_t *map_shared_info(unsigned long pa)
{
    if ( HYPERVISOR_update_va_mapping((unsigned long)shared_info >> 12,
                                      pa | 3, UVMF_INVLPG) )
    {
	    con << "Failed to map shared_info!!\n";
        *(int*)0=0;
    }
    con << "Shared info at: " << (void*) shared_info << "\n";
    return (shared_info_t *)shared_info;
}

/*
 * Shared page for communicating with the hypervisor.
 * Events flags go here, for example.
 */
shared_info_t *HYPERVISOR_shared_info;


extern unsigned long evtchn_to_irq[];

int bind_virq(int virq, int irq)
{
    evtchn_op_t op;
    int evtchn = 0;
    int ret;
    op.cmd              = EVTCHNOP_bind_virq;
    op.u.bind_virq.virq = virq;
    ret = HYPERVISOR_event_channel_op(&op);
    evtchn = op.u.bind_virq.port;
    con << "evtchn: " << evtchn << " " << ret << "\n";
    evtchn_to_irq[evtchn] = irq;
    ASSERT(ret == 0);
    return 0;
}

void
device_init(void)
{
	int suc;

	/* Setup interrupt handlers */
	for (int i=0; i< 1024; i++) 
		evtchn_to_irq[i] = (unsigned long) -1;

	HYPERVISOR_set_callbacks(__KERNEL_CS, (unsigned long)hypervisor_callback,
				 __KERNEL_CS, (unsigned long)failsafe_callback);

	//bind_virq(VIRQ_TIMER, 32);
	evtchn_to_irq[0] = 0;
	

	/* Setup IOPL */
	dom0_op_t dom0_op;
	dom0_op.cmd = DOM0_IOPL;
	dom0_op.interface_version = DOM0_INTERFACE_VERSION;
	dom0_op.u.iopl.domain = 0;
	dom0_op.u.iopl.iopl = 1;
	suc = xen_dom0_op(&dom0_op);
	ASSERT(suc == 0);
}

    
void
afterburn_main(start_info_t *si, unsigned long end_of_mem)
{
	/* Grab the shared_info pointer and put it in a safe place. */
	HYPERVISOR_shared_info = map_shared_info(si->shared_info);

	/* Copy start info somewhere sane */
	memcpy(&start_info, si, sizeof(start_info));

	con_driver.init();
	void_driver.init();

	con.init(&con_driver, "\e[31mafterburn:\e[0m ");
	debug_init.init(&void_driver /* &DEBUG_INIT_DRIVER*/, "\e[32mDEBUG:\e[0m ");
	debug_vm.init(/*&void_driver*/ &DEBUG_VM_DRIVER, "\e[33mDEBUG:\e[0m ");

	debug_init << "Hello World\n";

	con << "-------------------------------------\n";
	con << "Afterburner startup\n";
	con << "Start info:            " << si << "\n";
	con << "# pages:               " << si->nr_pages << "\n";
	con << "pt base:               " << (void*) si->pt_base << "\n";
	con << "shared info:           " << (void*) si->shared_info << "\n";
	con << "command line:          " << (char*) si->cmd_line  << "\n";
	con << "cpu freq:              " << (HYPERVISOR_shared_info->cpu_freq / 1000 / 1000) << "\n";
	con << "domain time:           " << HYPERVISOR_shared_info->domain_time << "\n";
	con << "-------------------------------------\n";

	debug_init << "Calling exception init\n";
	exception_init();


	debug_init << "Calling device init\n";
	device_init();

	/** Initialise memory **/
	debug_init << "Calling vm init\n";
	vm_init(end_of_mem);

	/* Now we can load Linux */
	debug_init << "Loading linux\n";
	void *entry = load_os(si->mod_start);

	debug_init << "Loaded linux\n";
	vm_unmap_region(si->mod_start, si->mod_len);

	/* Install stack */
	debug_init << "Setting initial exception stack\n";
	HYPERVISOR_stack_switch(0x0821, (unsigned long) &excpt_stack[8191]);

	debug_init << "Calling backend_preboot\n";
	backend_preboot(entry, (void*) end_of_memory, (char*) si->cmd_line);

	ASSERT(!"Should not reach here");
}
