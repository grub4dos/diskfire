// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#define PLOP_MAGIC "PLoP Boot Manager"

static int plop_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x5d, (const grub_uint8_t*)PLOP_MAGIC, grub_strlen(PLOP_MAGIC));
}

static grub_err_t plop_install(grub_disk_t disk, void* options)
{
	(void)options;
	(void)disk;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented yet");
}

struct grub_br grub_mbr_plop =
{
	.name = "PLOP",
	.desc = "PLoP Boot Manager",
	.code = NULL,
	.reserved_sectors = 62,
	.identify = plop_identify,
	.install = plop_install,
	.next = 0,
};
