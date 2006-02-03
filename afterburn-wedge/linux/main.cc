#include INC_ARCH(cpu.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(comms.h)

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <getopt.h>
#include <elfsimple.h>

extern "C" {
#include <elf/elf.h>
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/io.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bind.h>

bool debug_kernel_fault = false;
bool debug_page_fault = false;

extern intlogic_t intlogic;
hiostream_linux_t con_driver;

extern char __executable_start;
uintptr_t executable_start = (uintptr_t) &__executable_start;

hconsole_t con;
vcpu_t the_vcpu;
struct comm_buffer *comm_buffer = NULL;

static int MB = (1024 * 1024);

static char *os_binary = NULL;
static char *default_os_binary = "vmlinux";

static char *command_line = NULL;
static char *default_command_line = "";

static char *ramdisk = NULL;
static char *default_ramdisk = "initrd";

long memory_size = 0;
static long default_memory_size = (64 * 1024 * 1024); /* 64MB */


static void *shared_memory_area = ((void*) 0xbfffd000);
static const void *base_virt_addr = (void*) 0x80000000;
//static const void *default_ramdisk_offset = (void*) (4 * 1024 * 1024); /* 4MB */
static void *phys_mem;
static void *virt_mem;

int physmem_handle;

extern void child_start(void *entry, char *command_line);

static void
setup_ioperm(void)
{
	/* Currently we allow the kernel to access some device
	   ports directly */
	int r;

	r = ioperm(0x60, 0xf, 1);
	ASSERT(r == 0);

	r = ioperm(0x70, 0xf, 1);
	ASSERT(r == 0);

	ioperm(0x80, 0x3, 1);
	ASSERT(r == 0);
}

static int
setup_physmem(int size)
{
	int r;
	int shmmem;
	char filename[PATH_MAX];

	/*
	  FIXME: We can only have one copy running since we share the memory. We
	  should create a temp file, and register an at_exit() to delete it, and
	  use O_EXCL
	*/
	r = snprintf(filename, PATH_MAX, "afterburn.%05d", getpid());
	if (r < 0) {
		perror("setup_physmem-sprintf");
		exit(1);
	}

	shmmem = shm_open(filename, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);

	if (shmmem == -1) {
		perror("setup_physmem-shm_open");
		exit(1);
	}

	ASSERT(shmmem >= 0);

	/* 
	   Hooray for POSIX. This is a race here. If we are killed, then
	   we leave a file lying around until it is deleted or the machine
	   is rebooted. This sucks, but there is not way to solve it.
	 */
	
	r = shm_unlink(filename);
	
	if (r == -1) {
		perror("setup_physmem-shm_unlink");
		exit(1);
	}

	ASSERT(r == 0);

	r = ftruncate(shmmem, size);

	if (r == -1) {
		perror("setup_physmem-ftruncate");
		exit(1);
	}

	ASSERT(r == 0);

	phys_mem = mmap((void*)0, size, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_FIXED | MAP_SHARED, shmmem, 0);

	if (phys_mem == MAP_FAILED) {
		perror("setup_physmem-mmap");
		exit(1);
	}

	ASSERT(phys_mem == 0);

	return shmmem;
}

static void
setup_virtmem(int mem_handle, int size)
{
	virt_mem = mmap((void*)base_virt_addr, size, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_FIXED | MAP_SHARED, mem_handle, 0);

	ASSERT(virt_mem == base_virt_addr);
}

static void
teardown_virtmem(int size)
{
	int r;
	r = munmap(virt_mem, size);
	ASSERT(r == 0);
}

static void
init_cpu(void)
{
	void *area;
	bool r = false;
	area = mmap(shared_memory_area, 0x1000, 
		    PROT_READ | PROT_WRITE, 
		    MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS, 
		    0, 0);
	ASSERT(area == shared_memory_area);

	comm_buffer = (struct comm_buffer *) area;

	r = frontend_init(&get_cpu());
	ASSERT(r == true);
}

void apply_relocation(void *elf, intptr_t offest, uintptr_t size);

static bool
apply_patchup(void *elf_file)
{
	elf_ehdr_t *elf = elf_is_valid((word_t) elf_file);
	// Patchup the binary.
	elf_shdr_t *elf_patchup = elf->get_section(".afterburn");
	if( !elf_patchup ) {
		con << "Missing patchup section.\n";
		return false;
	}
	patchup_info_t *patchups = 
		(patchup_info_t *)elf_patchup->get_file_data(elf);
	word_t count = elf_patchup->size / sizeof(patchup_info_t);
	if( !arch_apply_patchups(patchups, count, (unsigned int) -1) )
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
load_os(char *filename)
{
	int f, r;
	void *elf;
	struct stat buf;
	uintptr_t entry_point;
	f = open(filename, O_RDONLY);
	ASSERT(f >= 0);

	r = fstat(f, &buf);
	ASSERT(r == 0);

	elf = mmap(0, buf.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, f, 0);
	ASSERT(elf != MAP_FAILED);
	ASSERT(elf_checkFile(elf) == 0);

#ifdef DEBUG_ELFFILE
	elf_fprintf(stdout, elf, buf.st_size, filename, 
		ELF_PRINT_PROGRAM_HEADERS|ELF_PRINT_SECTIONS);
#endif

	//apply_relocation(elf, -0x40000000, buf.st_size);
	r = elf_loadFile(elf, 0, 0); //-0x40000000);
	ASSERT(r == true);
	con << "load and relocated...\n";
	get_vcpu().set_kernel_vaddr(0x80000000);
	apply_patchup(elf);
	entry_point = (uintptr_t) elf_getEntryPoint(elf) - (uintptr_t) base_virt_addr;
	//uintptr_t *vaddr_end_ptr = (uintptr_t*) 0x801f27d4;
	//*vaddr_end_ptr = 0xbf000000;
	r = close(f);
	ASSERT(r == 0);

	r = munmap(elf, buf.st_size);
	ASSERT(r == 0);

	return (void*) (entry_point); // - 0x40000000);
}

static void
setup_ramdisk(char *file)
{
	int f, r, i;
	ssize_t amount_read = 0;
	char *data = (char*) (4 * 1024 * 1024);
	struct stat buf;
	con << "File: " << file << "\n";
	f = open(file, O_RDONLY);
	ASSERT(f >= 0);
	r = fstat(f, &buf);
	ASSERT(r == 0);
	
	for (i = buf.st_size; i > 0; i -= amount_read, data+=amount_read) {
		con << "Read into: " << (void*) data << '\n';
		amount_read = read(f, data, buf.st_size);
		if (amount_read <= 0) {
			con << "ERrror\n";
			while(1);
		}
		ASSERT(amount_read > 0);
	}
	con << "Amount read: " << amount_read << " " << buf.st_size << "\n";
	//ASSERT(amount_read == buf.st_size);
}

void
print_usage(void)
{
	printf("Usage: \n");
}

void
parse_args(int argc, char **argv)
{
	int c;
	int option_index = 0;
	int error = 0;

	while (1) {
		static struct option long_options[] = {
			{"kernel",  1, 0, 'k'},
			{"command", 1, 0, 'c'},
			{"ramdisk",  1, 0, 'r'},
			{"memory",  1, 0, 'm'},
			{"debug",  1, 0, 'd'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "k:c:r:d:m:",
				 long_options, &option_index);
		if (c == -1)
			break;
		
		switch (c) {
		case 'k':
			if (os_binary != NULL) {
				error = 1;
				break;
			}
			os_binary = optarg;
			break;
		case 'c':
			if (command_line != NULL) {
				error = 1;
				break;
			}
			command_line = optarg;
			break;
		case 'r':
			if (ramdisk != NULL) {
				error = 1;
				break;
			}
			ramdisk = optarg;
			break;
		case 'm':
			if (memory_size != 0) {
				error = 1;
				break;
			}
			memory_size = strtol(optarg, NULL, 0) * (1024 * 1024);
			break;
		case 'd':
			con << "debug - " << optarg << "\n";
			break;
		default:
			con << "?? getopt returned character code " <<  c << "\n";
		}
	}

	/* Kernel */
	if (optind < argc) {
		if (os_binary != NULL) {
			error = 1;
		}
		os_binary = argv[optind];
		optind++;
	}
	/* Command line */
	if (optind < argc) {
		if (command_line != NULL) {
			error = 1;
		}
		command_line = argv[optind];
		optind++;
	}
	/* Ramdisk */
	if (optind < argc) {
		if (ramdisk != NULL) {
			error = 1;
		}
		ramdisk = argv[optind];
		optind++;
	}

	if (error == 1) {
		print_usage();
		exit(1);
	}

	if (os_binary == NULL) 
		os_binary = default_os_binary;

	if (command_line == NULL) 
		command_line = default_command_line;

	if (ramdisk == NULL) 
		ramdisk = default_ramdisk;

	if (memory_size == 0)
		memory_size = default_memory_size;
}

void
validate_args(void)
{
	/* Validate memory size */
	if ((uintptr_t) base_virt_addr + memory_size > (uintptr_t) &__executable_start) {
		con << "Specified " << (memory_size / MB) << "MB of memory -- Maximum allowed: " << 
			((uintptr_t) &__executable_start - (uintptr_t) base_virt_addr) / MB << "\n";
		exit(1);
	}
	/* Should also validate the other bits */
}

extern "C" int
main(int argc, char **argv)
{
	void *entry_pt;

	con_driver.init();
	con.init( &con_driver, "\e[31mafterburn:\e[0m ");

	parse_args(argc, argv);

	validate_args();

	con << "Loading " << os_binary << " with command line \"" << command_line << "\"\n";

	if (geteuid() == 0)
		setup_ioperm();

	init_cpu();

	physmem_handle = setup_physmem(memory_size);
	/* We now have `physical' memory mapped at 0x0 */

	setup_virtmem(physmem_handle, memory_size);
	/* We now have the physical memory aliased at the 'virtual' address */
	
	entry_pt = load_os(os_binary);

	setup_ramdisk(ramdisk);

	/* After the OS is laoded, the virtual alias is no longer intersting, so
	   we  tear it down */
	teardown_virtmem(memory_size);

	child_start(entry_pt, command_line);
	return 0;
}

NORETURN void
panic(void)
{
	if (errno) {
		con << "PANIC: errno: " << strerror(errno) << "\n";
	}
	abort();
}
