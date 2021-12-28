// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#define FB_MAGIC "FBBF"

static int fbinst_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1b4, (const grub_uint8_t*)FB_MAGIC, grub_strlen(FB_MAGIC));
}

static grub_err_t fbinst_install(grub_disk_t disk, void* options)
{
	(void)options;
	(void)disk;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented yet");
}

struct grub_br grub_mbr_fbinst =
{
	.name = "FBINST",
	.desc = "Fbinst",
	.bootstrap_code = NULL,
	.reserved_sectors = 0,
	.identify = fbinst_identify,
	.install = fbinst_install,
	.next = 0,
};
