/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup ia64mm	
 * @{
 */
/** @file
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <config.h>
#include <panic.h>
#include <arch/bootinfo.h>
#include <align.h>
#include <macros.h>

#define KERNEL_RESERVED_AREA_BASE 	(0x4400000)
#define KERNEL_RESERVED_AREA_SIZE 	(16 * 1024 * 1024)

#define ROM_BASE	0xa0000               /* for simulators */
#define ROM_SIZE	(384 * 1024)          /* for simulators */

#define MIN_ZONE_SIZE	(64 * 1024)

#define MINCONF 1

uintptr_t last_frame = 0;
uintptr_t end_frame = 0;

void frame_arch_init(void)
{
	if (config.cpu_active == 1) {
		unsigned int i;
		for (i = 0; i < bootinfo->memmap_items; i++) {
			if (bootinfo->memmap[i].type == EFI_MEMMAP_FREE_MEM) {
				uint64_t base = bootinfo->memmap[i].base;
				uint64_t size = bootinfo->memmap[i].size;
				uint64_t abase = ALIGN_UP(base, FRAME_SIZE);

				if (size > FRAME_SIZE)
					size -= abase - base;

				if (size > MIN_ZONE_SIZE) {
					zone_create(abase >> FRAME_WIDTH,
					    size >> FRAME_WIDTH,
					    max(MINCONF, abase >> FRAME_WIDTH),
					    0);
				}
				if (abase + size > last_frame)
					last_frame = abase + size;
			}
		}
		
		/*
		 * Blacklist ROM regions.
		 */
		frame_mark_unavailable(ADDR2PFN(ROM_BASE),
		    SIZE2FRAMES(ROM_SIZE));

		frame_mark_unavailable(ADDR2PFN(KERNEL_RESERVED_AREA_BASE),
		    SIZE2FRAMES(KERNEL_RESERVED_AREA_SIZE));
	}	
}

/** @}
 */
