// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "file.h"
#include "br.h"

#include "mbr_wee.h"
#include "mbr_nt6.h"

static int wee_identify(grub_uint8_t* sector)
{
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1b1, (const grub_uint8_t*)"wee", 3);
}

static grub_err_t wee_install(grub_disk_t disk, void* options)
{
	const char* wee_menu = options;
	grub_file_t wee_menu_file = 0;
	char* wee_menu_buf = NULL;
	grub_disk_addr_t lba = 0, count = 0;
	count = grub_br_get_reserved_sectors(disk, &lba);
	if (lba != 1 || count < grub_mbr_wee.reserved_sectors)
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "unsupported reserved sectors");
	if (wee_menu)
	{
		wee_menu_file = grub_file_open(wee_menu, GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
		if (!wee_menu_file)
			return grub_error(GRUB_ERR_BAD_FILENAME, "bad wee menu");
		if (wee_menu_file->size > (63ULL << GRUB_DISK_SECTOR_BITS) - 0x7829)
		{
			grub_error(GRUB_ERR_OUT_OF_RANGE, "wee menu too large");
			goto fail;
		}
	}
	/* mbr boot code */
	grub_disk_write(disk, 0, 0, 440, wee_mbr);
	/* previous mbr (nt6) */
	grub_disk_write(disk, 1, 0, 440, nt6_mbr);
	grub_disk_write(disk, 1, 510, 2, wee_mbr + 510);
	/* wee sector 3 */
	grub_disk_write(disk, 2, 0, sizeof(wee_mbr) - (2 * GRUB_DISK_SECTOR_SIZE), wee_mbr + 2 * GRUB_DISK_SECTOR_SIZE);
	/* wee menu */
	if (wee_menu_file)
	{
		wee_menu_buf = grub_zalloc(wee_menu_file->size + 1);
		if (!wee_menu_buf)
			goto fail;
		grub_file_read(wee_menu_file, wee_menu_buf, wee_menu_file->size);
		grub_disk_write(disk, 0, 0x7828, wee_menu_file->size + 1, wee_menu_buf);
	}
fail:
	if (wee_menu_file)
		grub_file_close(wee_menu_file);
	if (wee_menu_buf)
		grub_free(wee_menu_buf);
	return grub_errno;
}

struct grub_br grub_mbr_wee = 
{
	.name = "WEE",
	.desc = "WEE",
	.code = wee_mbr,
	.code_size = sizeof(wee_mbr),
	.reserved_sectors = (sizeof(wee_mbr) - 1) >> GRUB_DISK_SECTOR_BITS,
	.identify = wee_identify,
	.install = wee_install,
	.next = 0,
};
