// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "mbr_rufus.h"

static int rufus_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, rufus_mbr, 440);
}

static grub_err_t rufus_install(grub_disk_t disk, void* options)
{
	(void)options;
	return grub_disk_write(disk, 0, 0, 440, rufus_mbr);
}

struct grub_br grub_mbr_rufus = 
{
	.name = "RUFUS",
	.desc = "Rufus Custom MBR",
	.code = rufus_mbr,
	.code_size = sizeof(rufus_mbr),
	.reserved_sectors = 0,
	.identify = rufus_identify,
	.install = rufus_install,
	.next = 0,
};
