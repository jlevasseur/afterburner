#include "burnxen.h"
#include <string.h>
#include INC_ARCH(cpu.h)
#include INC_WEDGE(tlocal.h)
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(debug.h)

extern long memory_size;

/*
 * At boot, cs:2 is the setup header.  We must initialize the
 * setup header, and then jump to startup_32.  It is located at
 * 0x9020:2.  But we can safely locate it anywhere between the link address
 * and the start of the text segment.
 *
 * Additionally, esi must point at the structure.
 *
 * See arch/i386/boot/setup.S
 */

static const unsigned linux_boot_param_addr = 0x9022;
static const unsigned linux_boot_param_size = 2048;
static const unsigned linux_cmdline_addr = 0x10000;
static const unsigned linux_cmdline_size = 256;
static const unsigned linux_e820map_max_entries = 32;

typedef enum {
    ofs_screen_info=0, ofs_cmdline=0x228,
    ofs_e820map=0x2d0, ofs_e820map_nr=0x1e8,
    ofs_initrd_start=0x218, ofs_initrd_size=0x21c,
    ofs_loader_type=0x210,
} boot_offset_e;

struct screen_info_t
{
    u8_t  orig_x;		/* 0x00 */
    u8_t  orig_y;		/* 0x01 */
    u16_t dontuse1;		/* 0x02 -- EXT_MEM_K sits here */
    u16_t orig_video_page;	/* 0x04 */
    u8_t  orig_video_mode;	/* 0x06 */
    u8_t  orig_video_cols;	/* 0x07 */
    u16_t unused2;		/* 0x08 */
    u16_t orig_video_ega_bx;	/* 0x0a */
    u16_t unused3;		/* 0x0c */
    u8_t  orig_video_lines;	/* 0x0e */
    u8_t  orig_video_isVGA;	/* 0x0f */
    u16_t orig_video_points;	/* 0x10 */
};

struct e820_entry_t
{
    enum e820_type_e {e820_ram=1, e820_reserved=2, e820_acpi=3, e820_nvs=4};

    u64_t addr;
    u64_t size;
    u32_t type;
};

static void e820_init(unsigned long size)
    // http://www.acpi.info/
{
    e820_entry_t *entries = (e820_entry_t *)
	(linux_boot_param_addr + ofs_e820map);
    u8_t *nr_entries = (u8_t *)(linux_boot_param_addr + ofs_e820map_nr);

    //if( resourcemon_shared.phys_size <= MB(1) )
    //return;

    *nr_entries = 3;

    entries[0].addr = 0;
    entries[0].size = 640 * 1024;
    entries[0].type = e820_entry_t::e820_ram;

    entries[1].addr = entries[0].addr + entries[0].size;
    entries[1].size = (1 * 1024 * 1024) - entries[1].addr;
    entries[1].type = e820_entry_t::e820_reserved;

    entries[2].addr = entries[1].addr + entries[1].size;
    entries[2].size = size - entries[2].addr;
    entries[2].type = e820_entry_t::e820_ram;
}

void ramdisk_init( void )
{
    word_t *start = (word_t *)(linux_boot_param_addr + ofs_initrd_start);
    word_t *size  = (word_t *)(linux_boot_param_addr + ofs_initrd_size);
    *start = 4 * 1024 * 1024;
    *size = 4 * 1024 * 1024;
    // Linux wants a physical start address.
#if 0
    *start = resourcemon_shared.ramdisk_start;
    if( resourcemon_shared.ramdisk_start >= resourcemon_shared.link_vaddr ) {
	// The resource monitor configured the ramdisk with a virtual addr.
	*start -= resourcemon_shared.link_vaddr;
    }

    *size = resourcemon_shared.ramdisk_size;
#endif
}

bool
backend_preboot(void *entry, void *entry_sp, char *command_line)
{
    get_cpu().cr0.x.fields.pe = 1;	// Enable protected mode.
    // Zero the parameter block.
    u8_t *param_block = (u8_t *)linux_boot_param_addr;
    for( unsigned i = 0; i < linux_boot_param_size; i++ )
	param_block[i] = 0;

    // Choose an arbitrary loader identifier.
    *(u8_t *)(linux_boot_param_addr + ofs_loader_type) = 0x14;

    // Init the screen info.
    screen_info_t *scr = (screen_info_t *)(param_block + ofs_screen_info);
    scr->orig_x = 0;
    scr->orig_y = 0;
    scr->orig_video_cols = 80;
    scr->orig_video_lines = 25;
    scr->orig_video_points = 16;
    scr->orig_video_mode = 3;
    scr->orig_video_isVGA = 1;
    scr->orig_video_ega_bx = 1;

    // Install the command line.
    ASSERT( linux_cmdline_addr > 
	    linux_boot_param_addr + linux_boot_param_size );
    char *cmdline = (char *)linux_cmdline_addr;
    //unsigned cmdline_len = 1 + strlen( resourcemon_shared.cmdline );
    //if( cmdline_len > linux_cmdline_size )
    //	cmdline_len = linux_cmdline_size;

    strcpy(cmdline, command_line);
    *(char **)(param_block + ofs_cmdline) = cmdline;

    e820_init(memory_size);
    ramdisk_init();

    con << "About to start at " << (void*) entry << " esp: " << (void*) entry_sp << "\n";
    // Start executing the binary.
    __asm__ __volatile__ (
	    "movl %0, %%esp ;"
	    "jmp *%1 ;"
	    : /* outputs */
	    : /* inputs */
	      "a"(entry_sp), "b"(entry),
	      "S"(param_block)
	    );

    return true;
}

