/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup ppc32mm	
 * @{
 */
/** @file
 */

#include <arch/boot/boot.h>
#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <align.h>
#include <macros.h>
#include <print.h>

uintptr_t last_frame = 0;
uintptr_t end_frame = 0;

void physmem_print(void)
{
	unsigned int i;
	
	printf("Base       Size\n");
	printf("---------- ----------\n");
		
	for (i = 0; i < bootinfo.memmap.count; i++) {
		printf("%#10x %#10x\n", bootinfo.memmap.zones[i].start,
			bootinfo.memmap.zones[i].size);
	}
}

void frame_arch_init(void)
{
	pfn_t minconf = 2;
	count_t i;
	pfn_t start, conf;
	size_t size;
	
	for (i = 0; i < bootinfo.memmap.count; i++) {
		start = ADDR2PFN(ALIGN_UP(bootinfo.memmap.zones[i].start, FRAME_SIZE));
		size = SIZE2FRAMES(ALIGN_DOWN(bootinfo.memmap.zones[i].size, FRAME_SIZE));
		
		if ((minconf < start) || (minconf >= start + size))
			conf = start;
		else
			conf = minconf;
		
		zone_create(start, size, conf, 0);
		if (last_frame < ALIGN_UP(bootinfo.memmap.zones[i].start + bootinfo.memmap.zones[i].size, FRAME_SIZE))
			last_frame = ALIGN_UP(bootinfo.memmap.zones[i].start + bootinfo.memmap.zones[i].size, FRAME_SIZE);
	}
	
	end_frame = last_frame;
	
	/* First is exception vector, second is 'implementation specific',
	   third and fourth is reserved, other contain real mode code */
	frame_mark_unavailable(0, 8);
	
	/* Mark the Page Hash Table frames as unavailable */
	uint32_t sdr1;
	asm volatile (
		"mfsdr1 %0\n"
		: "=r" (sdr1)
	);
	frame_mark_unavailable(ADDR2PFN(sdr1 & 0xffff000), 16); // FIXME
}

/** @}
 */
