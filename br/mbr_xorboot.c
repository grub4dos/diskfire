// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#define XORBOOT_MAGIC "XORBOOT"

static int xorboot_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x110,
		(const grub_uint8_t*)XORBOOT_MAGIC, grub_strlen(XORBOOT_MAGIC));
}

static grub_err_t xorboot_install(grub_disk_t disk, void* options)
{
	(void)options;
	(void)disk;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented yet");
}

struct grub_br grub_mbr_xorboot = 
{
	.name = "XORBOOT",
	.desc = "XorBoot",
	.bootstrap_code = NULL,
	.reserved_sectors = 62,
	.identify = xorboot_identify,
	.install = xorboot_install,
	.next = 0,
};
