/*
 * iMX boot ROM plugin.
 *
 * Author: Anti Sullin <anti.sullin@artecdesign.ee>
 * Copyright (C) 2016 Artec Design LLC
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stddef.h>

#include "serial.h"
#include "board.h"

/* iMX boot ROM looks for iMX header at this offset */
#define FLASH_OFFSET				0x400

#define __REG(x)     (*((volatile uint32_t *)(x)))
#define __stringify_1(x...)				#x
#define __stringify(x...)				__stringify_1(x)

int plugin_download(void **start, uint32_t *bytes, uint32_t *ivt_offset);

///////////////////////////////////////////////////////////////////////////////
/* Plugin metadata from linker script
 * The value of a variable from linker script is the POINTER of a variable in C! */
extern uint8_t _plugin_start, _plugin_end, _plugin_size;

///////////////////////////////////////////////////////////////////////////////
struct ivt_header {
	uint8_t tag;
	uint8_t length[2];	/* big endian! */
	uint8_t version;
} __attribute__((packed));

struct boot_data {
	void* start;
	uint32_t size;
	uint32_t plugin;
} __attribute__((packed));

struct ivt {
	struct ivt_header header;
	void* entry;
	uint32_t reserved1;
	void* dcd_ptr;
	void* boot_data_ptr;
	void* self;
	uint32_t csf;
	uint32_t reserved2;
} __attribute__((packed));

struct flash_header {
	struct ivt ivt;
	struct boot_data boot;
};

/* iMX header of this plugin. The image shall be written at FLASH_OFFSET. */
__attribute__ ((used, section(".flash_header")))
		struct flash_header header = {
	.ivt = {
		.header = {
			.tag = 0xD1,
			.length = { 0x00, 0x20 },
			.version = 0x40,
		},
		.entry = plugin_download,
		.dcd_ptr = NULL,
		.boot_data_ptr = &header.boot,
		.self = &header.ivt,
	},
	.boot = {
		/* The flash read starts always at 0. Offset the dest pointer so that
		 * the data will end up at the right place. */
		.start = (&_plugin_start) - FLASH_OFFSET,
		/* Load extra 512 bytes to include the uboot flash header, which must be
		 * concatenated after this plugin image.
		 * Keep the size multiple of 512 bytes or the ROM will mess up the data! */
		.size = (uint32_t)(&_plugin_size) + FLASH_OFFSET + 512,
		.plugin = 1,
	},
};

void gpio(int on)
{
*((uint32_t *)0x20e01e0) = 5;
*((uint32_t *)0x20A8004) = 0x100000;
*((uint32_t *)0x20A8000) = on ? 0x100000 : 0;
}

/* Entry point from iMX boot ROM */
int plugin_download(void **start, uint32_t *bytes, uint32_t *ivt_offset) {
 board_early_init_hw();
 dbg_init();
 dbg_str("SPL loaded\r\nEcho mode enabled\r\n");
 int on = 1;
 while(1) {
   gpio(on);
   on = on ? 0 : 1;
   dbg_putc(dbg_getc());
 }
}
