// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "mbr_nt5.h"

static int nt5_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, nt5_mbr, 440);
}

static grub_err_t nt5_install(grub_disk_t disk, void* options)
{
	(void)options;
	return grub_disk_write(disk, 0, 0, 440, nt5_mbr);
}

struct grub_br grub_mbr_nt5 = 
{
	.name = "NT5",
	.desc = "Windows NT 5.x MBR",
	.bootstrap_code = nt5_mbr,
	.reserved_sectors = 0,
	.identify = nt5_identify,
	.install = nt5_install,
	.next = 0,
};
