// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "mbr_grldr.h"

static grub_uint8_t grldr_magic[] = { 0x47, 0x52, 0x55, 0xaa };

static int grldr_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0xa7, grldr_magic, sizeof(grldr_magic));
}

static grub_err_t grldr_install(grub_disk_t disk, void* options)
{
	(void)options;
	grub_disk_addr_t lba = 0, count = 0;
	count = grub_br_get_reserved_sectors(disk, &lba);
	if (lba != 1 || count < grub_mbr_nt6.reserved_sectors)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 440, grldr_mbr);
	grub_disk_write(disk, 1, 0, sizeof(grldr_mbr) - GRUB_DISK_SECTOR_SIZE, grldr_mbr + GRUB_DISK_SECTOR_SIZE);
	return grub_errno;
}

struct grub_br grub_mbr_grldr = 
{
	.name = "GRUB4DOS",
	.desc = "GRUB4DOS MBR",
	.bootstrap_code = grldr_mbr,
	.reserved_sectors = (sizeof(grldr_mbr) - 1) >> GRUB_DISK_SECTOR_BITS,
	.identify = grldr_identify,
	.install = grldr_install,
	.next = 0,
};
