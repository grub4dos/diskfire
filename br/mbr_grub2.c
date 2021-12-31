// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#define GRUB2_MAGIC "GRUB "

static int grub2_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x180, (const grub_uint8_t*)GRUB2_MAGIC, grub_strlen(GRUB2_MAGIC));
}

static grub_err_t grub2_install(grub_disk_t disk, void* options)
{
	(void)options;
	(void)disk;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented yet");
}

struct grub_br grub_mbr_grub2 = 
{
	.name = "GRUB2",
	.desc = "GNU GRUB 2 MBR",
	.code = NULL,
	.reserved_sectors = 62,
	.identify = grub2_identify,
	.install = grub2_install,
	.next = 0,
};
