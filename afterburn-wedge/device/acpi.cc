/*********************************************************************
 *                
 * Copyright (C) 2003, 2005,  Karlsruhe University
 *                
 * File path:     acpi.cc
 * Description:   ACPI support code for IA-PCs (PC99)
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
 * $Id: acpi.cc,v 1.2 2006-01-11 17:40:40 stoess Exp $
 *                
 ********************************************************************/
#ifndef __PLATFORM__PC99__ACPI_H__
#define __PLATFORM__PC99__ACPI_H__

#include INC_WEDGE(backend.h)
#include <device/acpi.h>

/* ACPI 2.0 Specification, 5.2.4.1
   Finding the RSDP on IA-PC Systems */

acpi_rsdp_t* acpi_rsdp_t::locate()
{
    /** @todo checksum, check version */
    for (word_t page = ACPI20_PC99_RSDP_START;
	 page < ACPI20_PC99_RSDP_END;
	 page = page + 4096)
    {

	word_t rwx = 7;
	//jsXXX: check if real device mem and ignore if so
	backend_request_device_mem(page, PAGE_SIZE, rwx, true);
		
	for (word_t *p = (word_t *) page; p < (word_t *) (page + 4096) ; p+=4)
	{
	    acpi_rsdp_t* r = (acpi_rsdp_t*) p;
	    if (r->sig[0] == 'R' &&
		    r->sig[1] == 'S' &&
		    r->sig[2] == 'D' &&
		    r->sig[3] == ' ' &&
		    r->sig[4] == 'P' &&
		    r->sig[5] == 'T' &&
		    r->sig[6] == 'R' &&
		    r->sig[7] == ' ')
		return r;
	}
	backend_unmap_device_mem(page, PAGE_SIZE, rwx, true);
	
    };
    /* not found */
    return NULL;
};


#endif /* !__PLATFORM__PC99__ACPI_H__ */
