#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>           /* For O_* constants */
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <linux/user.h>
#include <assert.h>
#include <linux/unistd.h>
#include <termios.h>
#include <stdint.h>
#include <linux/ptrace.h>
#include <sys/io.h>

char buffer[1024];

char *
ptrace_get_string(pid_t pid, long addr)
{
	char value;
	int i;
	for(i=0; i < 1024; i++) {
		value = ptrace(PTRACE_PEEKDATA, pid, addr + i, NULL);
		buffer[i] = value;
		if (value == 0) {
			break;
		}
	}
	return buffer;
}


/*
 * File header 
 */
struct Elf32_Header {
	unsigned char   e_ident[16];
	uint16_t        e_type;	/* Relocatable=1, Executable=2 (+ some
				 * more ..) */
	uint16_t        e_machine;	/* Target architecture: MIPS=8 */
	uint32_t        e_version;	/* Elf version (should be 1) */
	uint32_t        e_entry;	/* Code entry point */
	uint32_t        e_phoff;	/* Program header table */
	uint32_t        e_shoff;	/* Section header table */
	uint32_t        e_flags;	/* Flags */
	uint16_t        e_ehsize;	/* ELF header size */
	uint16_t        e_phentsize;	/* Size of one program segment
					 * header */
	uint16_t        e_phnum;	/* Number of program segment
					 * headers */
	uint16_t        e_shentsize;	/* Size of one section header */
	uint16_t        e_shnum;	/* Number of section headers */
	uint16_t        e_shstrndx;	/* Section header index of the
					 * string table for section header 
					 * * names */
};

/*
 * Section header 
 */
struct Elf32_Shdr {
	uint32_t        sh_name;
	uint32_t        sh_type;
	uint32_t        sh_flags;
	uint32_t        sh_addr;
	uint32_t        sh_offset;
	uint32_t        sh_size;
	uint32_t        sh_link;
	uint32_t        sh_info;
	uint32_t        sh_addralign;
	uint32_t        sh_entsize;
};

/*
 * Program header 
 */
struct Elf32_Phdr {
	uint32_t p_type;	/* Segment type: Loadable segment = 1 */
	uint32_t p_offset;	/* Offset of segment in file */
	uint32_t p_vaddr;	/* Reqd virtual address of segment 
					 * when loading */
	uint32_t p_paddr;	/* Reqd physical address of
					 * segment (ignore) */
	uint32_t p_filesz;	/* How many bytes this segment
					 * occupies in file */
	uint32_t p_memsz;	/* How many bytes this segment
					 * should occupy in * memory (when 
					 * * loading, expand the segment
					 * by * concatenating enough zero
					 * bytes to it) */
	uint32_t p_flags;	/* Flags: logical "or" of PF_
					 * constants below */
	uint32_t p_align;	/* Reqd alignment of segment in
					 * memory */
};

int elf32_checkFile(struct Elf32_Header *file);
struct Elf32_Phdr * elf32_getProgramSegmentTable(struct Elf32_Header *file);
unsigned elf32_getNumSections(struct Elf32_Header *file);
char * elf32_getStringTable(struct Elf32_Header *file);
char * elf32_getSegmentStringTable(struct Elf32_Header *file);

static inline struct Elf32_Shdr *
elf32_getSectionTable(struct Elf32_Header *file)
{
	/* Cast heaven! */
	return (struct Elf32_Shdr*) (uintptr_t) (((uintptr_t) file) + file->e_shoff);
}

/* accessor functions */
static inline uint32_t 
elf32_getSectionType(struct Elf32_Header *file, uint16_t s)
{
	return elf32_getSectionTable(file)[s].sh_type;
}

static inline uint32_t 
elf32_getSectionFlags(struct Elf32_Header *file, uint16_t s)
{
	return elf32_getSectionTable(file)[s].sh_flags;
}

char * elf32_getSectionName(struct Elf32_Header *file, int i);
uint32_t elf32_getSectionSize(struct Elf32_Header *file, int i);
uint32_t elf32_getSectionAddr(struct Elf32_Header *elfFile, int i);
uint32_t elf32_getSectionOffset(struct Elf32_Header *elfFile, int i);
void * elf32_getSection(struct Elf32_Header *file, int i);
int elf32_getSectionNamed(struct Elf32_Header *file, char *str);
int elf32_getSegmentType (struct Elf32_Header *file, int segment);
void elf32_getSegmentInfo(struct Elf32_Header *file, int segment, uint64_t *p_vaddr, 
			  uint64_t *p_paddr, uint64_t *p_filesz, 
			  uint64_t *p_offset, uint64_t *p_memsz);



void elf32_fprintf(FILE *f, struct Elf32_Header *file, int size, char *name, int flags);
uint32_t elf32_getEntryPoint (struct Elf32_Header *file);

/* Program header functions */
uint16_t elf32_getNumProgramHeaders(struct Elf32_Header *file);

static inline struct Elf32_Phdr *
elf32_getProgramHeaderTable(struct Elf32_Header *file)
{
	/* Cast heaven! */
	return (struct Elf32_Phdr*) (uintptr_t) (((uintptr_t) file) + file->e_phoff);
}

/* accessor functions */
static inline uint32_t 
elf32_getProgramHeaderFlags(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_flags;
}

static inline uint32_t 
elf32_getProgramHeaderType(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_type;
}

static inline uint32_t 
elf32_getProgramHeaderFileSize(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_filesz;
}

static inline uint32_t
elf32_getProgramHeaderMemorySize(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_memsz;
}

static inline uint32_t
elf32_getProgramHeaderVaddr(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_vaddr;
}

static inline uint32_t
elf32_getProgramHeaderPaddr(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_paddr;
}

static inline uint32_t
elf32_getProgramHeaderOffset(struct Elf32_Header *file, uint16_t ph)
{
	return elf32_getProgramHeaderTable(file)[ph].p_offset;
}

/*
 * constants for Elf32_Phdr.p_flags 
 */
#define PF_X		1	/* readable segment */
#define PF_W		2	/* writeable segment */
#define PF_R		4	/* executable segment */

/*
 * constants for indexing into Elf64_Header_t.e_ident 
 */
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6

#define ELFMAG0         '\177'
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

#define ELFCLASS32      1
#define ELFCLASS64      2

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4

#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

/* Section Header type bits */
#define SHT_PROGBITS 1
#define	SHT_NOBITS 8

/* Section Header flag bits */
#define SHF_WRITE 1
#define SHF_ALLOC 2
#define SHF_EXECINSTR  4

/**/
#define ELF_PRINT_PROGRAM_HEADERS 1
#define ELF_PRINT_SECTIONS 2

/*
 * functions provided by elf.c 
 */

/**
 * Checks that elfFile points to a valid elf file. 
 *
 * @param elfFile Potential ELF file to check
 *
 * \return 0 on success. -1 if not and elf, -2 if not 32 bit.
 */
int elf_checkFile(void *elfFile);

/**
 * Determine number of sections in an ELF file.
 *
 * @param elfFile Pointer to a valid ELF header.
 *
 * \return Number of sections in the ELF file.
 */
unsigned elf_getNumSections(void *elfFile);

/**
 * Determine number of program headers in an ELF file.
 *
 * @param elfFile Pointer to a valid ELF header.
 *
 * \return Number of program headers in the ELF file.
 */
uint16_t elf_getNumProgramHeaders(void *elfFile);

/**
 * Return the base physical address of given program header in an ELF file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The memory size of the specified program header
 */
uint64_t elf_getProgramHeaderPaddr(void *elfFile, uint16_t ph);

/**
 * Return the base virtual address of given program header in an ELF file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The memory size of the specified program header
 */
uint64_t elf_getProgramHeaderVaddr(void *elfFile, uint16_t ph);

/**
 * Return the memory size of a given program header in an ELF file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The memory size of the specified program header
 */
uint64_t elf_getProgramHeaderMemorySize(void *elfFile, uint16_t ph);

/**
 * Return the file size of a given program header in an ELF file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The file size of the specified program header
 */
uint64_t elf_getProgramHeaderFileSize(void *elfFile, uint16_t ph);

/**
 * Return the start offset of he file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The offset of this program header with relation to the start
 * of the elfFile.
 */
uint64_t elf_getProgramHeaderOffset(void *elfFile, uint16_t ph);

/**
 * Return the flags for a given program header
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The flags of a given program header
 */
uint32_t elf_getProgramHeaderFlags(void *elfFile, uint16_t ph);

/**
 * Return the type for a given program header
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the program header
 *
 * \return The type of a given program header
 */
uint32_t elf_getProgramHeaderType(void *elfFile, uint16_t ph);

/**
 * Return the physical translation of a physical address, with respect
 * to a given program header
 *
 */
uint64_t elf_vtopProgramHeader(void *elfFile, uint16_t ph, uint64_t vaddr);


/**
 * 
 * \return true if the address in in this program header
 */
bool elf_vaddrInProgramHeader(void *elfFile, uint16_t ph, uint64_t vaddr);

/**
 * Determine the memory bounds of an ELF file
 *
 * @param elfFile Pointer to a valid ELF header
 * @param phys If true return bounds of physical memory, otherwise return
 *   bounds of virtual memory
 * @param min Pointer to return value of the minimum
 * @param max Pointer to return value of the maximum
 *
 * \return true on success. false on failure, if for example, it is an invalid ELF file 
 */
bool elf_getMemoryBounds(void *elfFile, bool phys, uint64_t *min, uint64_t *max);

/**
 * Find the entry point of an ELF file.
 *
 * @param elfFile Pointer to a valid ELF header
 *
 * \return The entry point address as a 64-bit integer.
 */
uint64_t elf_getEntryPoint(void *elfFile);

/**
 * Load an ELF file into memory
 *
 * @param elfFile Pointer to a valid ELF file
 * @param phys If true load using the physical address, otherwise using the virtual addresses
 *
 * \return true on success, false on failure.
 *
 * The function assumes that the ELF file is loaded in memory at some
 * address different to the target address at which it will be loaded.
 * It also assumes direct access to the source and destination address, i.e:
 * Memory must be ale to me loaded with a simple memcpy.
 *
 * Obviously this also means that if we are loading a 64bit ELF on a 32bit
 * platform, we assume that any memory address are within the first 4GB.
 *
 */
bool elf_loadFile(void *elfFile, bool phys);

char *elf_getStringTable(void *elfFile, int string_segment);
char *elf_getSegmentStringTable(void *elfFile);
int elf_getSectionNamed(void *elfFile, char *str);
char *elf_getSectionName(void *elfFile, int i);
uint64_t elf_getSectionSize(void *elfFile, int i);
uint64_t elf_getSectionAddr(void *elfFile, int i);
uint64_t elf_getSectionOffset(void *elfFile, int i);

/**
 * Return the flags for a given sections
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the sections
 *
 * \return The flags of a given section
 */
uint32_t elf_getSectionFlags(void *elfFile, int i);

/**
 * Return the type for a given sections
 *
 * @param elfFile Pointer to a valid ELF header
 * @param ph Index of the sections
 *
 * \return The type of a given section
 */
uint32_t elf_getSectionType(void *elfFile, int i);

void *elf_getSection(void *elfFile, int i);
void elf_getProgramHeaderInfo(void *elfFile, uint16_t ph, uint64_t *p_vaddr, 
			      uint64_t *p_paddr, uint64_t *p_filesz, 
			      uint64_t *p_offset, uint64_t *p_memsz);


/**
 * output the details of an ELF file to the stream f
 */
void elf_fprintf(FILE *f, void *elfFile, int size, char *name, int flags);

#if 0
/*
 * Returns a pointer to the program segment table, which is an array of
 * ELF64_Phdr_t structs.  The size of the array can be found by calling
 * getNumProgramSegments. 
 */
struct Elf32_Phdr *elf_getProgramSegmentTable(void *elfFile);
#endif
#if 0
/**
 * Returns a pointer to the program segment table, which is an array of
 * ELF64_Phdr_t structs.  The size of the array can be found by calling
 * getNumProgramSegments. 
 */
struct Elf32_Shdr *elf_getSectionTable(void *elfFile);
#endif


int
elf32_checkFile(struct Elf32_Header *file)
{
	if (file->e_ident[EI_MAG0] != ELFMAG0
	    || file->e_ident[EI_MAG1] != ELFMAG1
	    || file->e_ident[EI_MAG2] != ELFMAG2
	    || file->e_ident[EI_MAG3] != ELFMAG3)
		return -1;	/* not an elf file */
	if (file->e_ident[EI_CLASS] != ELFCLASS32)
		return -2;	/* not 32-bit file */
	return 0;		/* elf file looks OK */
}

/*
 * Returns the number of program segments in this elf file. 
 */
unsigned
elf32_getNumSections(struct Elf32_Header *elfFile)
{
	return elfFile->e_shnum;
}

char *
elf32_getStringTable(struct Elf32_Header *elfFile)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	return (char *)elfFile + sections[elfFile->e_shstrndx].sh_offset;
}

/* Returns a pointer to the program segment table, which is an array of
 * ELF32_Phdr_t structs.  The size of the array can be found by calling
 * getNumProgramSegments. */
struct Elf32_Phdr *
elf32_getProgramSegmentTable(struct Elf32_Header *elfFile)
{
	struct Elf32_Header *fileHdr = elfFile;
	return (struct Elf32_Phdr *) (fileHdr->e_phoff + (long) elfFile);
}

/* Returns the number of program segments in this elf file. */
uint16_t
elf32_getNumProgramHeaders(struct Elf32_Header *elfFile)
{
	struct Elf32_Header *fileHdr = elfFile;
	return fileHdr->e_phnum;
}

char *
elf32_getSectionName(struct Elf32_Header *elfFile, int i)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	char *str_table = elf32_getSegmentStringTable(elfFile);
	if (str_table == NULL) {
		return "<corrupted>";
	} else {
		return str_table + sections[i].sh_name;
	}
}

uint32_t
elf32_getSectionSize(struct Elf32_Header *elfFile, int i)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	return sections[i].sh_size;
}

uint32_t
elf32_getSectionAddr(struct Elf32_Header *elfFile, int i)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	return sections[i].sh_addr;
}

uint32_t
elf32_getSectionOffset(struct Elf32_Header *elfFile, int i)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	return sections[i].sh_offset;
}
	
void *
elf32_getSection(struct Elf32_Header *elfFile, int i)
{
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	return (char *)elfFile + sections[i].sh_offset;
}

int
elf32_getSectionNamed(struct Elf32_Header *elfFile, char *str)
{
	int numSections = elf32_getNumSections(elfFile);
	int i;
	for (i = 0; i < numSections; i++) {
		if (strcmp(str, elf32_getSectionName(elfFile, i)) == 0) {
			return i;
		}
	}
	return -1;
}

char *
elf32_getSegmentStringTable(struct Elf32_Header *elfFile)
{
	struct Elf32_Header *fileHdr = (struct Elf32_Header *) elfFile;
	if (fileHdr->e_shstrndx == 0) {
		return NULL;
	} else {
		return elf32_getStringTable(elfFile);
	}
}

#ifdef ELF_DEBUG
void
elf32_printStringTable(struct Elf32_Header *elfFile)
{
	int counter;
	struct Elf32_Shdr *sections = elf32_getSectionTable(elfFile);
	char * stringTable;

	if (!sections) {
		printf("No sections.\n");
		return;
	}
	
	stringTable = ((void *)elfFile) + sections[elfFile->e_shstrndx].sh_offset;
	
	printf("File is %p; sections is %p; string table is %p\n", elfFile, sections, stringTable);

	for (counter=0; counter < sections[elfFile->e_shstrndx].sh_size; counter++) {
		printf("%02x %c ", stringTable[counter], 
				stringTable[counter] >= 0x20 ? stringTable[counter] : '.');
	}
}
#endif

int
elf32_getSegmentType (struct Elf32_Header *elfFile, int segment)
{
	return elf32_getProgramSegmentTable(elfFile)[segment].p_type;
}

void
elf32_getSegmentInfo(struct Elf32_Header *elfFile, int segment, uint64_t *p_vaddr, uint64_t *p_addr, uint64_t *p_filesz, uint64_t *p_offset, uint64_t *p_memsz)
{
	struct Elf32_Phdr *segments;
		
	segments = elf32_getProgramSegmentTable(elfFile);
	*p_addr = segments[segment].p_paddr;
	*p_vaddr = segments[segment].p_vaddr;
	*p_filesz = segments[segment].p_filesz;
	*p_offset = segments[segment].p_offset;
	*p_memsz = segments[segment].p_memsz;
}

uint32_t
elf32_getEntryPoint (struct Elf32_Header *elfFile)
{
	return elfFile->e_entry;
}

/*
 * Debugging functions 
 */

/*
 * prints out some details of one elf file 
 */
void
elf32_fprintf(FILE *f, struct Elf32_Header *file, int size, char *name, int flags)
{
	struct Elf32_Phdr *segments;
	unsigned numSegments;
	struct Elf32_Shdr *sections;
	unsigned numSections;
	int i, r;
	char *str_table;

	fprintf(f, "Found an elf32 file called \"%s\" located "
		"at address 0x%p\n", name, file);

	if ((r = elf32_checkFile(file)) != 0) {
		char *magic = (char*) file;
		fprintf(f, "Invalid elf file (%d)\n", r);
		fprintf(f, "Magic is: %2.2hhx %2.2hhx %2.2hhx %2.2hhx\n",
			magic[0], magic[1], magic[2], magic[3]);
		return;
	}


	/*
	 * get a pointer to the table of program segments 
	 */
	segments = elf32_getProgramHeaderTable(file);
	numSegments = elf32_getNumProgramHeaders(file);

	sections = elf32_getSectionTable(file);
	numSections = elf32_getNumSections(file);

	if ((uintptr_t) sections >  ((uintptr_t) file + size)) {
		fprintf(f, "Corrupted elfFile..\n");
		return;
	}

		/*
		 * print out info about each section 
		 */
		
	if (flags & ELF_PRINT_PROGRAM_HEADERS) {
		/*
		 * print out info about each program segment 
		 */
		fprintf(f, "Program Headers:\n");
		fprintf(f, "  Type           Offset   VirtAddr   PhysAddr   "
			"FileSiz MemSiz  Flg Align\n");
		for (i = 0; i < numSegments; i++) {
			if (segments[i].p_type != 1) {
				fprintf(f, "segment %d is not loadable, "
					"skipping\n", i);
			} else {
				fprintf(f, "  LOAD           0x%06" PRIx32 " 0x%08" PRIx32 " 0x%08" PRIx32  \
				       " 0x%05" PRIx32" 0x%05" PRIx32 " %c%c%c 0x%04" PRIx32 "\n",
				//fprintf(f, "  LOAD           0x%" PRIxPTR " 0x%" PRIxPTR " 0x%" PRIxPTR 
				//	" 0x%" PRIxPTR" 0x%" PRIxPTR " %c%c%c 0x%" PRIxPTR "\n",
					segments[i].p_offset, segments[i].p_vaddr,
					segments[i].p_paddr,
					segments[i].p_filesz, segments[i].p_memsz,
					segments[i].p_flags & PF_R ? 'R' : ' ',
					segments[i].p_flags & PF_W ? 'W' : ' ',
					segments[i].p_flags & PF_X ? 'E' : ' ',
					segments[i].p_align);
			}
		}
	}
	if (flags & ELF_PRINT_SECTIONS) {
		str_table = elf32_getSegmentStringTable(file);

		printf("Section Headers:\n");
		printf("  [Nr] Name              Type            Addr     Off\n");
		for (i = 0; i < numSections; i++) {
			//if (elf_checkSection(file, i) == 0) {
			fprintf(f, "%-17.17s %-15.15s %x %x\n", elf32_getSectionName(file, i), " ", 
				///fprintf(f, "%-17.17s %-15.15s %08x %06x\n", elf32_getSectionName(file, i), " "	/* sections[i].sh_type 
				//									 */ ,
				sections[i].sh_addr, sections[i].sh_offset);
			//}
		}
	}
}

/*
 * Checks that elfFile points to a valid elf file. Returns 0 if the elf
 * file is valid, < 0 if invalid. 
 */

#define ISELF32(elfFile) ( ((struct Elf32_Header*)elfFile)->e_ident[EI_CLASS] == ELFCLASS32 )
#define ISELF64(elfFile) ( ((struct Elf64_Header*)elfFile)->e_ident[EI_CLASS] == ELFCLASS64 )

int
elf_checkFile(void *elfFile)
{
	return ISELF32 (elfFile)
		? elf32_checkFile(elfFile)
		: 0; //elf64_checkFile(elfFile);
}

/* Program Headers Access functions */
uint16_t
elf_getNumProgramHeaders(void *elfFile)
{
	return ISELF32 (elfFile)
		? elf32_getNumProgramHeaders(elfFile)
		: 0; //elf64_getNumProgramHeaders(elfFile);
}

uint32_t
elf_getProgramHeaderFlags(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderFlags(elfFile, ph)
		: 0; //elf64_getProgramHeaderFlags(elfFile, ph);
}

uint32_t
elf_getProgramHeaderType(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderType(elfFile, ph)
		: 0; //elf64_getProgramHeaderType(elfFile, ph);
}

uint64_t
elf_getProgramHeaderPaddr(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderPaddr(elfFile, ph)
		: 0; //elf64_getProgramHeaderPaddr(elfFile, ph);
}

uint64_t
elf_getProgramHeaderVaddr(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderVaddr(elfFile, ph)
		: 0; //elf64_getProgramHeaderVaddr(elfFile, ph);
}

uint64_t
elf_getProgramHeaderMemorySize(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderMemorySize(elfFile, ph)
		: 0; //elf64_getProgramHeaderMemorySize(elfFile, ph);
}

uint64_t
elf_getProgramHeaderFileSize(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderFileSize(elfFile, ph)
		: 0; //elf64_getProgramHeaderFileSize(elfFile, ph);
}

uint64_t
elf_getProgramHeaderOffset(void *elfFile, uint16_t ph)
{
	return ISELF32 (elfFile)
		? elf32_getProgramHeaderOffset(elfFile, ph)
		: 0; //elf64_getProgramHeaderOffset(elfFile, ph);
}

char *
elf_getSegmentStringTable(void *elfFile)
{
	return ISELF32 (elfFile)
		? elf32_getSegmentStringTable(elfFile)
		: NULL; //, assert(!"don't do elf64");
}

char *
elf_getStringTable(void *elfFile, int string_segment)
{
	return ISELF32 (elfFile)
		? elf32_getStringTable(elfFile)
		: NULL; //elf64_getStringTable(elfFile, string_segment);
}


unsigned
elf_getNumSections(void *elfFile)
{
	return ISELF32 (elfFile)
		? elf32_getNumSections(elfFile)
		: 0; //elf64_getNumSections(elfFile);
}

char *
elf_getSectionName(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionName(elfFile, i)
		: NULL; //elf64_getSectionName(elfFile, i);
}

uint32_t
elf_getSectionFlags(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionFlags(elfFile, i)
		: 0; //elf64_getSectionFlags(elfFile, i);
}

uint32_t
elf_getSectionType(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionType(elfFile, i)
		: 0; //elf64_getSectionType(elfFile, i);
}

uint64_t
elf_getSectionSize(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionSize(elfFile, i)
		: 0; //elf64_getSectionSize(elfFile, i);
}

uint64_t
elf_getSectionAddr(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionAddr(elfFile, i)
		: 0; //elf64_getSectionAddr(elfFile, i);
}

uint64_t
elf_getSectionOffset(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSectionOffset(elfFile, i)
		: 0; //elf64_getSectionAddr(elfFile, i);
}


void *
elf_getSection(void *elfFile, int i)
{
	return ISELF32 (elfFile)
		? elf32_getSection(elfFile, i)
		: NULL; //elf64_getSection(elfFile, i);
}

int
elf_getSectionNamed(void *elfFile, char *str)
{
	return ISELF32 (elfFile)
		? elf32_getSectionNamed(elfFile, str)
		: -1; //elf64_getSectionNamed(elfFile, str);
}

void
elf_getProgramHeaderInfo(void *elfFile, uint16_t ph, uint64_t *p_vaddr, 
			 uint64_t *p_paddr, uint64_t *p_filesz, uint64_t *p_offset, 
			 uint64_t *p_memsz)
{
	*p_vaddr = elf_getProgramHeaderVaddr(elfFile, ph);
	*p_paddr = elf_getProgramHeaderPaddr(elfFile, ph);
	*p_filesz = elf_getProgramHeaderFileSize(elfFile, ph);
	*p_offset = elf_getProgramHeaderOffset(elfFile, ph);
	*p_memsz = elf_getProgramHeaderMemorySize(elfFile, ph);
}

uint64_t
elf_getEntryPoint(void *elfFile)
{
	return ISELF32 (elfFile)
		? elf32_getEntryPoint (elfFile)
		: 0; //elf64_getEntryPoint (elfFile);
}

void
elf_fprintf(FILE *f, void *file, int size, char *name, int flags)
{
	elf32_fprintf(f, file, size, name, flags);

}

bool
elf_getMemoryBounds(void *elfFile, bool phys, uint64_t *min, uint64_t *max)
{
	uint64_t mem_min = UINT64_MAX;
	uint64_t mem_max = 0;
	int i;

	if (elf_checkFile(elfFile) != 0) {
		return false;
	}

	for(i=0; i < elf_getNumProgramHeaders(elfFile); i++) {
		uint64_t sect_min, sect_max;

		if (elf_getProgramHeaderMemorySize(elfFile, i) == 0) {
			continue;
		}

		if (phys) {
			sect_min = elf_getProgramHeaderPaddr(elfFile, i);
		} else {
			sect_min = elf_getProgramHeaderVaddr(elfFile, i);
		}

		sect_max = sect_min + elf_getProgramHeaderMemorySize(elfFile, i);

		if (sect_max > mem_max) {
			mem_max = sect_max;
		}
		if (sect_min < mem_min) {
			mem_min = sect_min;
		}
	}
	*min = mem_min;
	*max = mem_max;

	return true;
};

bool
elf_vaddrInProgramHeader(void *elfFile, uint16_t ph, uint64_t vaddr)
{
	uint64_t min = elf_getProgramHeaderVaddr(elfFile, ph);
	uint64_t max = min + elf_getProgramHeaderMemorySize(elfFile, ph);
	if (vaddr >= min && vaddr < max) {
		return true;
	} else {
		return false;
	}
}

uint64_t
elf_vtopProgramHeader(void *elfFile, uint16_t ph, uint64_t vaddr)
{
	uint64_t ph_phys = elf_getProgramHeaderPaddr(elfFile, ph);
	uint64_t ph_virt = elf_getProgramHeaderPaddr(elfFile, ph);
	uint64_t paddr;

	paddr = vaddr - ph_virt + ph_phys;

	return paddr;
}

int shmmem;

bool
elf_loadFile(void *elfFile, bool phys)
{
	int i;
	
	if (elf_checkFile(elfFile) != 0) {
		return false;
	}

	for(i=0; i < elf_getNumProgramHeaders(elfFile); i++) {
		/* Load that section */
		uint64_t dest, src;
		size_t len;
		void *phys_mem;
		int r;
		if (elf_getProgramHeaderType(elfFile, i) != 1) {
			continue;
		}
		if (phys) {
			dest = elf_getProgramHeaderPaddr(elfFile, i);
		} else {
			dest = elf_getProgramHeaderVaddr(elfFile, i);
		}
		printf("loading: %d\n", i);
		len = elf_getProgramHeaderFileSize(elfFile, i);
		src = (uint64_t) (uintptr_t) elfFile + elf_getProgramHeaderOffset(elfFile, i);
		phys_mem = mmap((void*)(uintptr_t) dest, elf_getProgramHeaderMemorySize(elfFile, i), 
				PROT_WRITE, MAP_FIXED | MAP_SHARED, 
				shmmem, 
				dest - 0x80000000);
		if (phys_mem != (void*)(uintptr_t)dest) {
			exit(2);
		}
		memcpy((void*) (uintptr_t) dest, (void*) (uintptr_t) src, len);
		dest += len;
		memset((void*) (uintptr_t) dest, 0, elf_getProgramHeaderMemorySize(elfFile, i) - len);
		//printf("flags: %d\n", elf_getProgramHeaderFlags(elfFile, i));
		//printf("mprotect\n");
#if 0
		if (elf_getProgramHeaderFlags(elfFile, i) == 5)
			r = mprotect(phys_mem, elf_getProgramHeaderMemorySize(elfFile, i), PROT_READ | PROT_EXEC);
		else
			r = mprotect(phys_mem, elf_getProgramHeaderMemorySize(elfFile, i), PROT_WRITE | PROT_READ | PROT_EXEC);
		//printf("protected\n");
		if (r != 0) {
			perror("mprotect");
			exit(2);
		}
#endif
	}

	return true;
}

struct mmap_stuff {
	void *old_ip;
	void *old_sp;
	void *fn;
	uintptr_t prot;
	uintptr_t addr;
	uintptr_t offset;
};

struct mmap_stuff *hack_area;

extern void (*mmap_jmp) (void);
extern int start_stack;

void
mmap_unmap(void)
{
	int r;
	//printf("mode switch unmap everything! %d\n", getpid());
	r = munmap(0x0, 0xbf000000);
	assert(r == 0);
}

void
mmap_unmap_one(void)
{
	int r;
	//printf("mode switch unmap everything! %d\n", getpid());
	r = munmap((void*)hack_area->addr, 0x1000);
	assert(r == 0);
}

void
mmap_magic(void)
{
	void *addr;
	//printf("mmap hack!\n");
	//printf("old_ip: %p\n", hack_area->old_ip);
	addr = mmap((void*) hack_area->addr, 0x1000, hack_area->prot, MAP_FIXED | MAP_SHARED, 3, hack_area->offset);
	if (addr == (void*) - 1) {
		printf("Couildn't do map\n");
	}
	assert(addr != (void*) - 1);
}

enum boot_offset {
    ofs_screen_info=0, ofs_cmdline=0x228,
    ofs_e820map=0x2d0, ofs_e820map_nr=0x1e8,
    ofs_initrd_start=0x218, ofs_initrd_size=0x21c,
    ofs_loader_type=0x210,
} ;

enum e820_type_e {e820_ram=1, e820_reserved=2, e820_acpi=3, e820_nvs=4};

struct e820_entry
{
    uint64_t addr;
    uint64_t size;
    uint32_t type;
};

static const unsigned linux_boot_param_addr = 0x9022;
static const unsigned linux_boot_param_size = 2048;
static const unsigned linux_cmdline_addr = 0x10000;
static const unsigned linux_cmdline_size = 256;

#define KB(x) ((x) * 1024)
#define MB(x) (KB(x) * 1024)

static void e820_init( void )
    // http://www.acpi.info/
{
    struct e820_entry *entries = (struct e820_entry *)
	(linux_boot_param_addr + ofs_e820map);
    uint8_t *nr_entries = (uint8_t *)(linux_boot_param_addr + ofs_e820map_nr);

    *nr_entries = 3;

    entries[0].addr = 0;
    entries[0].size = KB(640);
    entries[0].type = e820_ram;

    entries[1].addr = entries[0].addr + entries[0].size;
    entries[1].size = MB(1) - entries[1].addr;
    entries[1].type = e820_reserved;

    entries[2].addr = entries[1].addr + entries[1].size;
    entries[2].size = (64 * 1024 * 1024) - entries[2].addr;
    entries[2].type = e820_ram;
}


struct screen_info
{
    uint8_t  orig_x;		/* 0x00 */
    uint8_t  orig_y;		/* 0x01 */
    uint16_t dontuse1;		/* 0x02 -- EXT_MEM_K sits here */
    uint16_t orig_video_page;	/* 0x04 */
    uint8_t  orig_video_mode;	/* 0x06 */
    uint8_t  orig_video_cols;	/* 0x07 */
    uint16_t unused2;		/* 0x08 */
    uint16_t orig_video_ega_bx;	/* 0x0a */
    uint16_t unused3;		/* 0x0c */
    uint8_t  orig_video_lines;	/* 0x0e */
    uint8_t  orig_video_isVGA;	/* 0x0f */
    uint16_t orig_video_points;	/* 0x10 */
};


uintptr_t cr[5] = {0, 0, 0, 0};
uintptr_t idt = -1;
#define interrupt_enabled ((eflags >> 9) & 1)
#define interrupt_enable (eflags = eflags | 1 << 9)
#define interrupt_disable (eflags = eflags & (~(1 << 9)))
uintptr_t eflags;
int irq_pending = 0;
uintptr_t badaddress[1025];
int bad_addres_count = 0;

int
printf_ptab(uintptr_t ptab_addr)
{
	int i=0;
	uintptr_t *ptab;
	ptab_addr &= ~ 0xfff;
	ptab = (uintptr_t*) ptab_addr;
	printf("\tptab addr: %p\n", ptab_addr);
	//badaddress[bad_addres_count++] = pdir_addr;
	for (i =0 ; i < 1024; i++) {
		if (ptab[i] != 0) {
			printf("\tEntry: %d -- %x -- %x\n", i, ptab[i], ptab[i] & ~ 0xfff);
			//badaddress[bad_addres_count++] = pdir[i] & ~ 0xfff;
		}
	}
	assert(bad_addres_count < 1025);
}

int
printf_pdir(void)
{
	int i=0;
	uintptr_t pdir_addr = cr[3] & ~ 0xfff;
	uintptr_t *pdir = (uintptr_t*) pdir_addr;
	//printf("pdir addr: %p\n", pdir_addr);
	bad_addres_count = 0;
	badaddress[bad_addres_count++] = pdir_addr;
	for (i =0 ; i < 1024; i++) {
		if (pdir[i] != 0) {
			//printf("Entry: %d -- %x -- %x -- %d\n", i, pdir[i], pdir[i] & ~ 0xfff, bad_addres_count);
			//printf_ptab(pdir[i]);
			badaddress[bad_addres_count++] = pdir[i] & ~ 0xfff;
		}
	}
}


uintptr_t * ptable_lookup(uintptr_t addr)
{
	uintptr_t pdir_addr = cr[3] & ~ 0xfff;
	uintptr_t *pdir = (uintptr_t*) pdir_addr;
	uintptr_t ptab_addr;
	uintptr_t *ptab;
	uintptr_t ptent;
	int pdir_idx = addr >> 22;
	int ptab_idx = (addr >> 12) & 0x3ff;
	//printf("pdir_idx: %d ptab_idx: %d\n", pdir_idx, ptab_idx);
	ptab_addr = pdir[pdir_idx];
	if (! (ptab_addr & 0x1)) {
		printf("pagefault!\n");
		return (void*) -1; //exit(0);
	}
	ptab = (uintptr_t*) (ptab_addr & (~0xfff));
	//printf("ptab: %p\n", ptab);
	//printf("ptent: %p %p %p\n", ptab[ptab_idx], ptab[ptab_idx-1], ptab[ptab_idx+1]);
	return &(ptab[ptab_idx]);
}

uintptr_t lookup_addr(uintptr_t addr)
{
	uintptr_t ptent;
	ptent = *ptable_lookup(addr);
	if (ptent & 0x1 == 0) {
		return -1;
	}
	return ((ptent & (~ 0xfff)) | (addr & 0xfff));

}

#if 0
void
sigalarm(int alarm)
{
	if (idt != 0) {
		printf("alrm with irq\n");
	}
	
}
#endif

int trace_all = 1;
int printtraps = 0;
int
main(int argc, char **argv)
{
	struct stat buf;
	int size = 64 * 1024 * 1024;
	void *phys_mem;
	int f;
	void *elf;
	pid_t child;
	void (*fn) (void);
	int r;
	struct itimerval val;
	//printf("foobar: %p ---- mmap_jmp: %p\n", &mmap_jmp, mmap_jmp);
	//printf("&start_stack: %p\n", &start_stack);

	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 100000; /* 100ms */
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 100000; /* 100ms */
	

#if 0
	r = ioperm(0x60, 0xf, 1);
	if (r != 0) {
		perror("ioperm");
		exit(0);
	}
	r = ioperm(0x70, 0xf, 1);
	if (r != 0) {
		perror("ioperm");
		exit(0);
	}
	ioperm(0x80, 0x3, 1);
	if (r != 0) {
		perror("ioperm");
		exit(0);
	}
#endif
	printf("foo\n");
	fprintf(stdout, "foop\n");
	fprintf(stderr, "fooz\n");

	start_stack = 1;
	//printf("SHM open\n");
	shmmem = shm_open("/foobar", O_RDWR | O_CREAT, S_IRWXU);
	//printf("SHM opened\n");
	if (ftruncate(shmmem, size) != 0) {
		perror("truncate");
		exit(0);
	}

	hack_area = mmap((void*) 0xbfffd000, 0x1000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	f = open(argv[1], O_RDONLY);
	fstat(f, &buf);
	//printf("size: %ld\n", buf.st_size);
	elf = mmap(0, buf.st_size, PROT_READ, MAP_PRIVATE, f, 0);
	//printf("ELF: %p\n", elf);
	//elf_fprintf(stdout, elf, buf.st_size, argv[1], ELF_PRINT_PROGRAM_HEADERS|ELF_PRINT_SECTIONS);

	printf("foo\n");
	fprintf(stdout, "foop\n");

	elf_loadFile(elf, true);
	printf("getname sect\n");
#if 0
	{
		int burn = elf_getSectionNamed(elf, ".afterburn");
		uintptr_t *burn_addr;
		int len;
		int i;
		printf("burn secttion: %d %x\n", burn, elf_getSectionOffset(elf, burn));
		len = elf_getSectionSize(elf, burn) / 4;
		burn_addr = (void*) ((uintptr_t) elf_getSectionOffset(elf, burn) + (uintptr_t) elf);
		printf("burn_addr: %p elf: %p\n", burn_addr, elf);
		for (i = 0; i < len; i++) {
			uint8_t *opstream = (uint8_t*)burn_addr[i];
#if 0
			switch(opstream[0]) {
			case 0x9c:
				//printf("push\n");
				opstream[3] = 0x9c;
				break;
			case 0x9d:
				opstream[3] = 0x9d;
				break;
			default:
				printf("unknown afterburn\n");
				exit(0);
			}
			opstream[0] = 0x0f;
			opstream[1] = 0x0b;
			opstream[2] = 0x37;
#endif
			//printf("%x -- %x -- blah\n", burn_addr[i], *((uint8_t*)burn_addr[i]));
		}
		//exit(0);
	}
#endif
	fn = (void *) ((uintptr_t) elf_getEntryPoint(elf) - 0x80000000);

	munmap(elf, buf.st_size);
	close(f);

	///printf("finish -- %p\n", fn);
	printf("forking\n");
	printf("stderr: %p, stdout: %p\n", stderr, stdout);
	fprintf(stdout, "foobaz\n");
	//fprintf(stderr, "foobaz\n");
	child = fork();
	//printf("child: %d\n", child);
	if (child == 0) {
		int r;

#if 0
		printf("setting up iterm\n");
		r = setitimer(ITIMER_REAL, &val, NULL);
		if (r != 0) {
			perror("itmr");
			exit(0);
		}
		printf("setup iterm\n");
#endif
		//printf("I am the child... abot to call mmap\n");
		r = munmap((void*)0x80000000, 16 * 1024 * 1024);
		assert(r == 0);
		phys_mem = mmap((void*)0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_SHARED, shmmem, 0);
		if (phys_mem == (void*) -1) {
			perror("badness");
		}
		//printf("I am child: %d about to call: %p\n", getpid(), fn);
		r = ptrace(PTRACE_TRACEME, getpid(), 0, 0);
		//printf("I am now ptraced! %d\n", r);
		kill(getpid(), SIGSTOP);
#if 0
		{
			volatile char blah;
			volatile char *foo = (char*) 0x9000000;
			blah = *foo;
		}
#endif
		{
			uint8_t *param_block = (uint8_t *)linux_boot_param_addr;
			struct screen_info *scr = (struct screen_info *)(param_block + ofs_screen_info);
			unsigned i;
			for(i  = 0; i < linux_boot_param_size; i++ )
				param_block[i] = 0;
			
			// Choose an arbitrary loader identifier.
			*(uint8_t *)(linux_boot_param_addr + ofs_loader_type) = 0x14;


			e820_init();
			scr->orig_x = 0;
			scr->orig_y = 0;
			scr->orig_video_cols = 80;
			scr->orig_video_lines = 25;
			scr->orig_video_points = 16;
			scr->orig_video_mode = 3;
			scr->orig_video_isVGA = 1;
			scr->orig_video_ega_bx = 1;
    
			char *cmdline = (char *)linux_cmdline_addr;
			strncpy( cmdline, "linux console=ttyS0 mem=nopentium", linux_cmdline_size );
			*(char **)(param_block + ofs_cmdline) = cmdline;
			printf("LINUX STARTING NOW!\n");
			__asm__ __volatile__ (
					      "jmp *%0 ;"
					      : /* outputs */
					      : /* inputs */
						"b"(fn),
						"S"(param_block)
					      );
		}
		assert(!"Shoudn't get here!\n");
	} else {
		int status;
		int r;
		pid_t waitee;
		int flags;
		uintptr_t last, last2, last3;
		struct user_regs_struct in, out;
		printf("mapping memory inot my as\n");
		phys_mem = mmap((void*)0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_SHARED, shmmem, 0);
		if (phys_mem == (void*) -1) {
			perror("badness");
		}
		printf("mapped\n");
		printf("Waiting for child to trace_me\n");
		fprintf(stdout, "foobar\n");
		fprintf(stderr, "foobaz\n");
		waitee = wait(&status);
		if (WIFSTOPPED(status)) {
			//printf("STOPPED : %d\n", WSTOPSIG(status));
		}
		//printf("Got child: %d -- %d\n", waitee, status);
		//printf("Now single step it\n");
		r = ptrace(PTRACE_CONT, waitee, 0, 0);
		//printf("continued.. waiting for trap: %d\n", r);
		while(1) {
			waitee = wait(&status);
			siginfo_t siginfo;
			int syscall;
			//printf("STATUS: %d\n", status);
			if (WIFSIGNALED(status)) {
				printf("Signalled: %d\n", WTERMSIG(status));
			}
			if (WIFSTOPPED(status)) {
				//printf("STOPPED : %d\n", WSTOPSIG(status));
			}
			if (WIFEXITED(status)) {
				printf("exit\n");
			}
			r = ptrace(PTRACE_GETSIGINFO, waitee, NULL, &siginfo);
			ptrace(PT_GETREGS, waitee, NULL, &in);
			if (WSTOPSIG(status) == SIGTRAP) {
				last3 = last2;
				last2 = last;
				last = in.eip;
#if 1
#define ALLOC_BOOTMEM_ADDR 0x801d3e82
#define MEMCPY_ADDR 0xbf008630 //0xbf023320 //0xbf00c010 //bf012c00 //0xbf012c2d
				if (printtraps) { // || in.eip == 0x801cc2da) {
					printtraps = 1;
					printf("trapped at: %p %x %x %p %p %p\n", in.eip, 
					       ptrace(PTRACE_PEEKDATA, waitee, in.eip, NULL),
					       ptrace(PTRACE_PEEKDATA, waitee, in.eip + 4, NULL), 
					       &hack_area,
					       &hack_area->old_ip,
					       hack_area->old_ip);
					printf("last: %x last2: %x last3: %x\n", 
					       last, last2, last3);

				}
#endif

				r = ptrace(PTRACE_SINGLESTEP, waitee, 0, 0);
				continue;
			}
			if (WSTOPSIG(status) == SIGILL) {
				if ((uint8_t)ptrace(PTRACE_PEEKDATA, waitee, in.eip+2, NULL) == 0x37) {
				} else {
					
					printf("SIGILL!\n");
					printf("Signal code: %d\n", siginfo.si_code);
					printf("Si_addr    : %p\n", siginfo.si_addr);
					printf("Addr       : %p -- %p\n", in.eip, in.eip + 0x80000000);
					printf("EAX: %x EBX: %x ECX: %x EDX: %x \n", in.eax, in.ebx, in.ecx, in.edx);
					printf("last: %x last2: %x last3: %x\n", 
					       last, last2, last3);
					if (siginfo.si_code != ILL_ILLOPN) {
						printf("NOT illopen\n");
						exit(0);
					} else {
						printf("code: %d -- %x\n", siginfo.si_code, ptrace(PTRACE_PEEKDATA, waitee, in.eip+2, NULL));
					}
					exit(0);
				}
				{
					uint8_t op = ptrace(PTRACE_PEEKDATA, waitee, in.eip+3, NULL);
					switch(op) {
					case 0x9d:
						printf("pop!\n");
						printf("in.esp: %p %p\n", in.esp, lookup_addr(in.esp));
						printf("eflags: %x\n", in.eflags);
						in.eflags = *(uintptr_t*) lookup_addr(in.esp);
						//printf("eflags: %x\n", in.eflags);

						eflags = in.eflags & (~ 0xff);
						in.esp += 4;
						in.eip += 5;
						ptrace(PT_SETREGS, waitee, NULL, &in);
						goto cont;
						break;
					case 0x9c:
						printf("push! %x\n", in.eflags | eflags);
						in.esp -= 4;
						*(uintptr_t*) lookup_addr(in.esp) = in.eflags | eflags;
						in.eip += 5;
						ptrace(PT_SETREGS, waitee, NULL, &in);
						goto cont;
						break;
					default:

						printf("SIGILL!\n");
						printf("Signal code: %d\n", siginfo.si_code);
						printf("Si_addr    : %p\n", siginfo.si_addr);
						printf("Addr       : %p -- %p\n", in.eip, in.eip + 0x80000000);
						printf("EAX: %x EBX: %x ECX: %x EDX: %x \n", in.eax, in.ebx, in.ecx, in.edx);
						printf("unknown!\n");
						exit(0);
					}
				}
				printf("illegal operand\n");
				exit(0);
			} else if (WSTOPSIG(status) == SIGALRM) {
				uintptr_t *idt_table = NULL;
				uintptr_t x, y, offset;
				uintptr_t *stack;
				//printf("got sigalarm\n");
				//printf("got sigalarm: %x %x\n", idt, idt_table);

				if (idt == -1) {
					printf("no idt table\n");
					goto cont;
				}

				idt_table = (uintptr_t*)lookup_addr(idt);
				//printf("got sigalarm: %x %x\n", idt, idt_table);

				x = idt_table[0x20*2];
				y = idt_table[(0x20*2)+1];
				offset = (x & 0xffff) | (y & 0xffff0000);
				if (offset != 0) {
					//printf("offset: %p enabled: %d %p\n", offset, interrupt_enabled, in.eip);
					if (interrupt_enabled == 0) {
						irq_pending = 1;
						goto cont;
					}
					stack = (uintptr_t*) lookup_addr(in.esp);
					printf("stack: %p\n", stack);
					stack -= 4;
					printf("stack: %p\n", stack);
					stack[3] = eflags | (in.eflags & 0xff);
					printf("interrupt_enabled: %d\n", stack[3]);
					stack[2] = 0;
					stack[1] = in.eip;
					//stack[0] = 3; /* Hack! */
					cr[2] = 0; //(uintptr_t)siginfo.si_addr;
					//printf("CR2 now: %p\n", cr[2]);
					trace_all = 1;
					in.esp -= 12;
					in.eip = offset;
					printf("## OMG SENDING INTERRUPT\n");
					ptrace(PT_SETREGS, waitee, NULL, &in);
					
					//exit(0);
					goto cont;
				}
				goto cont;
			} else if (WSTOPSIG(status) != SIGSEGV) {
				printf("Unexpected signal! %d\n", WSTOPSIG(status));
				printf("last: %x last2: %x last3: %x\n", 
				       last, last2, last3);

				exit(0);
			}
			if (siginfo.si_code == 128) {
				uint8_t value;
				/* Read first byte */
				value = ptrace(PTRACE_PEEKDATA, waitee, in.eip, NULL);
				if (value == 0xf) {
					value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 1, NULL);
					if (value == 1) {
						value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 2, NULL);
						switch((value >> 3) & 7) {
						case 2:
							printf("%p -- lgdtl (%x)\n", in.eip, value);
							in.eip += 7;
							ptrace(PT_SETREGS, waitee, NULL, &in);
							break;
						case 3:
							{
								int i;
								uintptr_t value;
								uintptr_t idt_desc;
								uint8_t *vals = (uint8_t *) &value;
								//printf("ljump\n");
								for (i=0; i < 4; i++) {
									vals[i] = ptrace(PTRACE_PEEKDATA, waitee, in.eip + i + 1, NULL);
								}

								value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 3, NULL);
								printf("%p -- lidtl (%08x)\n", in.eip, value);
								in.eip += 7;
								value += 2;
								/* Lookup */
								idt_desc = lookup_addr(value);
								if (idt_desc == -1) {
									printf("badness setting idt\n");
								}
								printf("idt_desc: %p\n", idt_desc);
								idt = *((uintptr_t*)idt_desc);
								printf("idt: %p\n", idt);
								ptrace(PT_SETREGS, waitee, NULL, &in);
								//exit(0);
							}
							break;
						case 7:
							{
								int reg = value & 0x7;
								uintptr_t value2;
								if ((value >> 7) == 1) {
									printf("dealing with not a register arg! eep, (%d)\n", reg);
									exit(0);
								}
								switch(reg) {
								case 3:
									value2 = in.ebx;
									break;
								default:
									printf("unknown invalid tlb. %d %p\n", reg, in.eip);
									exit(0);
									break;
								}
								printf("tlb invalidate: %p\n", value2);
								in.eip += 3;
								
								hack_area->old_ip = (void*) in.eip;
								hack_area->old_sp = (void*) in.esp;
								hack_area->fn = mmap_unmap_one;
								hack_area->addr = value2;
								in.eip = (uintptr_t) &mmap_jmp; //magic;
								ptrace(PT_SETREGS, waitee, NULL, &in);

							}
							break;
						default:
							printf("undecoded priv instr: %d %p\n", (value >> 3) & 7, in.eip);
							exit(0);
						}
					} else if (value == 0x0) {
						printf("lldt\n");
						in.eip += 3;
						ptrace(PT_SETREGS, waitee, NULL, &in);
					} else if (value == 0x6) {
						//printf("CLITS!\n");
						in.eip += 2;
						ptrace(PT_SETREGS, waitee, NULL, &in);
					} else if (value == 0x22) {
						int seg, src;
						uintptr_t the_val;
						value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 2, NULL);
						seg = (value >> 3) & 7;
						src = value & 7;
						the_val = 0;
						switch(src) {
						case 0:
							the_val = in.eax;
							break;
						case 1:
							the_val = in.ecx;
							break;
						case 2:
							the_val = in.edx;
							break;
						case 3:
							the_val = in.ebx;
							break;
						default:
							printf("Read from unknown reg: %d (%p)\n", src, in.eip);
							exit(0);
						}

						if (seg == 3) {
							printf("Moving a value into CR3-- exciting: %x %d %x\n", the_val, src, in.eip);
						}
						//printf("mov CR%d <- %d\n", seg, src);

						cr[seg] = the_val;
						in.eip += 3;


						if (seg == 3) {
							printf("Moving a value into CR3-- exciting: %x %d %x\n", the_val, src, in.eip);
							printf_pdir();
							hack_area->old_ip = (void*) in.eip;
							hack_area->old_sp = (void*) in.esp;
							hack_area->fn = mmap_unmap;
							in.eip = (uintptr_t) &mmap_jmp; //magic;
						}
						
						ptrace(PT_SETREGS, waitee, NULL, &in);
						//exit(0);
					} else if (value == 0x20) {
						int seg, src;
						value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 2, NULL);
						seg = (value >> 3) & 7;
						src = value & 7;
						printf("mov CR%d (%p) -> %d (%p)\n", seg, cr[seg], src, in.eip);
						switch(src) {
						case 0:
							in.eax = cr[seg];
							break;
						case 1:
							in.ecx = cr[seg];
							break;
						case 3:
							in.ebx = cr[seg];
							break;
						default:
							printf("Write from unknown reg: %d (%p)\n", src, in.eip);
							exit(0);
						}
						
						if (seg == 3) {
							printf("Moving a value out of CR3-- exciting: %d %x -- %x\n", src, in.eip, cr[3]);
						}

						in.eip += 3;
						ptrace(PT_SETREGS, waitee, NULL, &in);
						//exit(0);
					} else if (value == 0xb2) {
						/* Set SS and esp */
						uintptr_t addr;
						value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 2, NULL);
						addr = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 3, NULL);
						//printf("register: %d\n",  (value >> 3) & 7);
						printf("Load a register from address: %p\n", addr);
						in.eip += 7;
						in.esp = addr;
						{
							uintptr_t foo;
							foo = ptrace(PTRACE_PEEKDATA, waitee, addr, NULL);
							//printf("foo: %p\n", (void*) foo);
							/* NOT SURE IF THIS IS GOOD CODE! */
							in.esp = foo;
						}
						ptrace(PT_SETREGS, waitee, NULL, &in);
					} else if (value == 0x23) {
						//printf("debug register!\n");
						in.eip += 3;
						ptrace(PT_SETREGS, waitee, NULL, &in);
					} else {
						printf("Unknown bad: %x\n", value);
						exit(0);
					}
				}
				else if (value == 0xe6) {
					//printf("outb: port: %x val: %x  \n",  (uint8_t) ptrace(PTRACE_PEEKDATA, waitee, in.eip+1, NULL), in.eax);
					in.eip += 2;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xe4) {
					//printf("inb:  %p port: %x\n", in.eip, (uint8_t) ptrace(PTRACE_PEEKDATA, waitee, in.eip+1, NULL));
					in.eip += 2;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xec) {
					if ((in.edx & 0xffff) == 0x3fd) {
						in.eax = (in.eax  & ~0xff) | 0x60;
					} else {
						//printf("Stderr printing\n");
						//fprintf(stderr, "inb2:  %p port: %x\n", in.eip, in.edx & 0xffff);
					}
					in.eip += 1;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xee) {
					if ((in.edx & 0xffff) == 0x3f8) {
						putchar(in.eax & 0xff);    
					}
					//printf("outb2:  %p port: %x val: %x\n", in.eip, in.edx & 0xffff, in.eax);
					in.eip += 1;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0x66) {
					//printf("outb3:  %p port: %x val: %x\n", in.eip, in.edx & 0xffff, in.eax);
					in.eip += 2;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xea) {
					int i;
					uintptr_t value;
					uint8_t *vals = (uint8_t *) &value;
					//printf("ljump\n");
					for (i=0; i < 4; i++) {
						vals[i] = ptrace(PTRACE_PEEKDATA, waitee, in.eip + i + 1, NULL);
					}
					//printf("jump value: %p\n", value);
					in.eip = value;
					ptrace(PT_SETREGS, waitee, NULL, &in);
					//exit(0);
				}
				else if (value == 0x8e) {
					int seg, src;
					//printf("bad mov instruction?\n");
					value = ptrace(PTRACE_PEEKDATA, waitee, in.eip + 1, NULL);
					seg = (value >> 3) & 7;
					src = value & 7;
					//printf("move into seg: (%x) %d from %d\n", value, seg, src);
					switch (seg) {
					case 5:
						//printf("GS -- eax: %d\n", in.eax);
						break;
					case 4:
						//printf("FS -- eax: %d\n", in.eax);
						break;
					case 3:
						//printf("DS -- eax: %d\n", in.eax);
						break;
					case 2:
						//printf("SS -- eax: %d\n", in.eax);
						break;
					case 0:
						//printf("ES -- eax: %d\n", in.eax);
						break;
					default:
						printf("unknown\n");
						exit(0);
					}
					in.eip += 2;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xfa) {
					//printf("cli!\n");
					printf("\t --> CLI %p\n", in.eip);
					interrupt_disable;
					in.eip += 1;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xfb) {
					printf("\t --> STI\n");
					interrupt_enable;
					if (irq_pending == 1) {
						printf("pending interrupt!\n");
						irq_pending = 0;
					}
					in.eip += 1;
					ptrace(PT_SETREGS, waitee, NULL, &in);
				}
				else if (value == 0xcf) {
					uintptr_t *stack;
					printf("IRET\n");
					stack = (uintptr_t*) lookup_addr(in.esp);
					printf("stack: %p\n", stack);
					in.eip = stack[0];
					eflags = stack[2];
					printf("new eip: %x irq enabled: %d\n", in.eip, interrupt_enabled);
					//cr[2] = (uintptr_t)siginfo.si_addr;
					//printf("CR2 now: %p\n", cr[2]);
					//trace_all = 1;
					in.esp += 12;
					//in.eip = offset;
					printf("IRET return\n");
					ptrace(PT_SETREGS, waitee, NULL, &in);
					
					//exit(0);
				}
				else {
					printf("Unknown instr: %x @ %p\n", value, (void*) in.eip);
					printf("last: %x last2: %x last3: %x\n", 
					       last, last2, last3);

					exit(0);
				}
			} else {
				uintptr_t addr;
				int i;
				uintptr_t * ptent_addr;
				uintptr_t ptent;
				uintptr_t prot;
				//printf("PAGEFAULT\n");


				//printf("Signal code: %d\n", siginfo.si_code);
				//printf("Si_addr    : %p\n", siginfo.si_addr);
				//printf("Addr       : %p -- %p\n", in.eip, in.eip + 0x80000000);
				//printf("EAX: %x EBX: %x ECX: %x EDX: %x \n", in.eax, in.ebx, in.ecx, in.edx);
				//printf("ESP: %x\n", in.esp);
				addr = ((uintptr_t) siginfo.si_addr) & (~ 0xfff);
				hack_area->old_ip = (void*) in.eip;
				hack_area->old_sp = (void*) in.esp;
				hack_area->fn = mmap_magic;
				ptent_addr =  ptable_lookup(addr);
				ptent = *ptent_addr;
				hack_area->addr = addr;
				if (hack_area->offset == -1) {
						printf("Si_addr    : %p\n", siginfo.si_addr);
						printf("pf: %p -- %p -- %p\n", hack_area->addr, hack_area->offset, hack_area->old_ip);

						printf(" -- real pagefault --\n");
						while(1);

				}
				//printf("ptent_addr: %p -- ptent %p\n", ptent_addr, ptent);
				if (ptent & 1 != 1) {
					printf("BADNESS\n");
					exit(0);
				}

				//if ((ptent >> 5) & 1) {
				if (siginfo.si_code == SEGV_ACCERR) {
					prot = PROT_WRITE | PROT_EXEC | PROT_READ;
					ptent |= 1 << 6;   /* Set dirty bit */
					*ptent_addr = ptent;
				} else {
					prot = PROT_READ | PROT_EXEC;
					ptent |= 1 << 5; /* Set access bit */
					*ptent_addr = ptent;
				}
				if ((cr[0] >> 16) & 1 ) {
					//printf("WP ENABLED\n");
					if (((ptent >> 1) & 1)  == 0x0 && (prot & PROT_WRITE)) {
						uintptr_t *idt_table = (uintptr_t*)lookup_addr(idt);
						uintptr_t x, y, offset;
						uintptr_t *stack;
						printf("idt: %x\n", idt);
						if (idt == -1) {
							printf("no idT!\n");
						}
						x = idt_table[14*2];
						y = idt_table[(14*2)+1];
						offset = (x & 0xffff) | (y & 0xffff0000);
						printf("Kernel attempts writes to readonly pages! %p\n", in.eip);
						printf("Going to raise thingy at address: %p -- %p\n", idt_table, &idt_table[14*2]);
						printf("x: %p y: %p offset: %p\n", x, y, offset);
						
						stack = (uintptr_t*) lookup_addr(in.esp);
						printf("stack: %p\n", stack);
						stack -= 4;
						printf("stack: %p\n", stack);
						stack[3] = 0;
						stack[2] = 0;
						stack[1] = in.eip;
						stack[0] = 3; /* Hack! */
						cr[2] = (uintptr_t)siginfo.si_addr;
						//printf("CR2 now: %p\n", cr[2]);
						trace_all = 1;
						in.esp -= 16;
						in.eip = offset;
						ptrace(PT_SETREGS, waitee, NULL, &in);

						//exit(0);
						goto cont;
					}

				}
				in.eip = (uintptr_t) &mmap_jmp; //magic;
				//printf("EIP: %p\n", (void*) in.eip);
				ptrace(PT_SETREGS, waitee, NULL, &in);

				hack_area->offset = ptent & (~ 0xfff);
				//printf("ptent: %p -- %p\n", ptent, hack_area->offset);
				hack_area->prot = prot;
				for (i =0 ; i < bad_addres_count; i++) {
					if (hack_area->offset == badaddress[i]) {
						printf("Si_addr    : %p\n", siginfo.si_addr);
						printf("pf: %p -- %p -- %p\n", hack_area->addr, hack_area->offset, hack_area->old_ip);

						printf("WRITE/RAD TO PDIR/PTAB\n");
						//exit(0);
					}
				}

				//printf("map: %x @ %x\n", hack_area->offset, hack_area->addr);
				//printf("pf\n");
				//printf("pf: %p -- %p -- %p (%d)\n", hack_area->addr, hack_area->offset, hack_area->old_ip, hack_area->prot);
				if ((uintptr_t) hack_area->old_ip == 0x9ffff)
					printf("HACK: last: %x last2: %x last3: %x\n", 
					       last, last2, last3);

#if 0
				r = ptrace(PTRACE_SINGLESTEP, waitee, 0, 0);
				assert(r == 0);
				continue;
#endif
				//while(1);
				//exit(0);
			}
			//syscall = in.orig_eax;
			//flags = in.ecx;
			//while(1);
			//if (waitee != child) {
			//	printf("waited on: %d: %d\n", waitee, syscall);
			//}
		cont:
			//printf("Syscall: %d\n", in.orig_eax);
			if (trace_all == 1) {
				r = ptrace(PTRACE_SINGLESTEP, waitee, 0, 0);
			} else {
				r = ptrace(PTRACE_CONT, waitee, 0, 0);
			}
			assert(r == 0);
		}
	}
	while(1);
	printf("done!\n");
}
