// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

static grub_uint8_t empty_mbr[446] = { 0 };

static int empty_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, empty_mbr, 440);
}

static grub_err_t empty_install(grub_disk_t disk, void* options)
{
	(void)options;
	return grub_disk_write(disk, 0, 0, 446, empty_mbr);
}

struct grub_br grub_mbr_empty = 
{
	.name = "EMPTY",
	.desc = "EMPTY MBR",
	.code = empty_mbr,
	.code_size = sizeof(empty_mbr),
	.reserved_sectors = 0,
	.identify = empty_identify,
	.install = empty_install,
	.next = 0,
};
