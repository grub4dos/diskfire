// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "mbr_nt6.h"

static int nt6_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, nt6_mbr, 440);
}

static grub_err_t nt6_install(grub_disk_t disk, void* options)
{
	(void)options;
	return grub_disk_write(disk, 0, 0, 440, nt6_mbr);
}

struct grub_br grub_mbr_nt6 = 
{
	.name = "NT6",
	.desc = "Windows NT 6.x MBR",
	.code = nt6_mbr,
	.code_size = sizeof(nt6_mbr),
	.reserved_sectors = 0,
	.identify = nt6_identify,
	.install = nt6_install,
	.next = 0,
};
