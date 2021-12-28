// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

static grub_uint8_t vt_magic[] = { 0x56, 0x54, 0x00, 0x47, 0x65, 0x00, 0x48, 0x44, 0x00 };

static int vt_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x190, vt_magic, sizeof(vt_magic));
}

static grub_err_t vt_install(grub_disk_t disk, void* options)
{
	(void)options;
	(void)disk;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented yet");
}

struct grub_br grub_mbr_ventoy = 
{
	.name = "VENTOY",
	.desc = "Ventoy MBR",
	.bootstrap_code = NULL,
	.reserved_sectors = 62,
	.identify = vt_identify,
	.install = vt_install,
	.next = 0,
};
