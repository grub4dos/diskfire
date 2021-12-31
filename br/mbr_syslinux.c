// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"
#include "partition.h"

#include "mbr_syslinux.h"
#include "mbr_syslinux_gpt.h"

static int syslinux_identify(grub_uint8_t* sector)
{
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, syslinux_mbr, 440))
		return 1;
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0, syslinux_gpt_mbr, 440);
}

static grub_err_t syslinux_install(grub_disk_t disk, void* options)
{
	(void)options;
	grub_partition_map_t partmap = NULL;
	partmap = grub_partmap_probe(disk);
	if (!partmap)
		goto fail;
	if (grub_strcmp(partmap->name, "msdos") == 0)
		return grub_disk_write(disk, 0, 0, 440, syslinux_mbr);
	if (grub_strcmp(partmap->name, "gpt") == 0)
		return grub_disk_write(disk, 0, 0, 440, syslinux_gpt_mbr);
fail:
	return grub_error(GRUB_ERR_BAD_PART_TABLE, "unsupported partition table");
}

struct grub_br grub_mbr_syslinux = 
{
	.name = "SYSLINUX",
	.desc = "Syslinux 6.02 MBR",
	.code = syslinux_mbr,
	.code_size = sizeof(syslinux_mbr),
	.reserved_sectors = 0,
	.identify = syslinux_identify,
	.install = syslinux_install,
	.next = 0,
};
