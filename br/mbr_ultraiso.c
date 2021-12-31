// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "mbr_ultraiso.h"

static int ultraiso_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, ultraiso_hdd, 440);
}

static grub_err_t ultraiso_install(grub_disk_t disk, void* options)
{
	(void)options;
	return grub_disk_write(disk, 0, 0, 440, ultraiso_hdd);
}

struct grub_br grub_mbr_ultraiso = 
{
	.name = "ULTRAISO",
	.desc = "UltraISO USB-HDD+",
	.code = ultraiso_hdd,
	.code_size = sizeof(ultraiso_hdd),
	.reserved_sectors = 0,
	.identify = ultraiso_identify,
	.install = ultraiso_install,
	.next = 0,
};
