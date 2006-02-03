/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/device/portio.cc
 * Description:   Direct PC99 port accesses to
 *                their device models, or directly to the devices if so
 *                configured.
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
 * $Id: portio.cc,v 1.15 2005/11/18 11:29:46 joshua Exp $
 *
 ********************************************************************/

#include <device/portio.h>
#include INC_WEDGE(console.h)
#include INC_WEDGE(backend.h)

/* To see a list of the fixed I/O ports, see section 6.3.1 in the 
 * Intel 82801BA ICH2 and 82801BAM ICH2-M Datasheet.
 */

#if defined(CONFIG_DEVICE_PASSTHRU)
static bool do_passthru_portio( u16_t port, u32_t &value, bool read, u32_t bit_width )
{
    if( read )
	value = 0xffffffff;

    switch( port )
    {
	case 0x1:
	    if( !read )
		con << "debug: " << (void *)value << '\n';
	    return true;

	case 0x20 ... 0x21:
	case 0xa0 ... 0xa1:
	    i8259a_portio( port, value, read );
	    return true;
#if !defined(CONFIG_WEDGE_L4KA) && !defined(CONFIG_XEN_2_0)
	case 0x40 ... 0x43:
	    i8253_portio( port, value, read );
	    return true;
#endif
	case 0x3f8 ... 0x3ff:
	case 0x2f8 ... 0x2ff:
        case 0x3e8 ... 0x3ef:
	case 0x2e8 ... 0x2ef:
	    serial8250_portio( port, value, read );
	    return true;
    }
    if( read ) {
	u32_t tmp;
	switch( bit_width ) {
	    case 8:
		__asm__ __volatile__ ("inb %1, %b0\n" 
			: "=a"(tmp) : "dN"(port) );
		value = tmp;
		return true;
	    case 16:
		__asm__ __volatile__ ("inw %1, %w0\n" 
			: "=a"(tmp) : "dN"(port) );
		value = tmp;
		return true;
	    case 32:
		__asm__ __volatile__ ("inl %1, %0\n" 
			: "=a"(tmp) : "dN"(port) );
		value = tmp;
		return true;
	}
    }
    else {
	switch( bit_width ) {
	    case 8:
		__asm__ __volatile__ ("outb %b0, %1\n" 
			: : "a"(value), "dN"(port) );
		return true;
	    case 16:
		__asm__ __volatile__ ("outw %w0, %1\n" 
			: : "a"(value), "dN"(port) );
		return true;
	    case 32:
		__asm__ __volatile__ ("outl %0, %1\n" 
			: : "a"(value), "dN"(port) );
		return true;
	}
    }

    return false;
}
#endif
#ifndef CONFIG_DEVICE_PASSTHRU
static bool do_portio( u16_t port, u32_t &value, bool read )
{
    if( read )
	value = 0xffffffff;

    switch( port )
    {
	case 0x80:
	    // Often used as the debug port.  Linux uses it for a delay.
	    break;

	case 0x20 ... 0x21:
	case 0xa0 ... 0xa1:
	    i8259a_portio( port, value, read );
	    break;

	case 0x40 ... 0x43:
	    i8253_portio( port, value, read );
	    break;

	case 0x61: // NMI status and control register.
	    legacy_0x61( port, value, read );
	    break;

	case 0x60:
	case 0x64: // keyboard
	    i8042( port, value, read );
	    break;

	case 0x70 ... 0x7f: // RTC
	    mc146818rtc_portio( port, value, read );
	    break;

	case 0x170 ... 0x177: // IDE
	case 0x376:
	    return false;

	case 0x3c0 ... 0x3df: // VGA
	    break;

	case 0x3f8 ... 0x3ff:
	case 0x2f8 ... 0x2ff:
	case 0x3e8 ... 0x3ef:
	case 0x2e8 ... 0x2ef:
	    serial8250_portio( port, value, read );
	    break;

	case 0xcf8 ... 0xcff: // PCI
	    return false;

	default:
	    return false;
    }

    return true;
}
#endif
bool portio_read( u16_t port, u32_t &value, u32_t bit_width )
{
    value = ~0;
#if defined(CONFIG_DEVICE_PASSTHRU)
    return do_passthru_portio( port, value, true, bit_width );
#else
    switch( port ) {
	case 0xcf8: pci_config_address_read( value, bit_width ); return true;
	case 0xcfc: pci_config_data_read( value, bit_width, 0 ); return true;
	case 0xcfd: pci_config_data_read( value, bit_width, 8 ); return true;
	case 0xcfe: pci_config_data_read( value, bit_width, 16 ); return true;
	case 0xcff: pci_config_data_read( value, bit_width, 24 ); return true;
    }

    return do_portio( port, value, true );
#endif
}

bool portio_write( u16_t port, u32_t value, u32_t bit_width )
{
#if defined(CONFIG_DEVICE_PASSTHRU)
    return do_passthru_portio( port, value, false, bit_width );
#else
    switch( port ) {
	case 0xcf8: pci_config_address_write( value, bit_width ); return true;
	case 0xcfc: pci_config_data_write( value, bit_width, 0 ); return true;
	case 0xcfd: pci_config_data_write( value, bit_width, 8 ); return true;
	case 0xcfe: pci_config_data_write( value, bit_width, 16 ); return true;
	case 0xcff: pci_config_data_write( value, bit_width, 24 ); return true;
    }

    return do_portio( port, value, false );
#endif
}

