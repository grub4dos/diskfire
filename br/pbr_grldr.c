// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "pbr_grldr.h"

#define GRLDR_PBR_MAGIC "No GRLDR"

static int grldr_identify(grub_uint8_t* sector)
{
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1c0, (grub_uint8_t*)"No ", 3))
		return 1;
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1bd, (grub_uint8_t*)"No ", 3)) // 0.4.5c
		return 1;
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1e0, (grub_uint8_t *)"No ", 3);
}

// FAT12 or FAT16
static grub_err_t fat16_install(grub_disk_t disk, grub_uint8_t options[5])
{
	grub_uint8_t* grldr_fat16 = grldr_pbr + 0x200;
	grub_disk_write(disk, 0, 0, 3, grldr_fat16); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x3e, 0x200 - 0x3e, grldr_fat16 + 0x3e);
	grub_disk_write(disk, 0, 0x1e3, 5, options);
	return grub_errno;
}

static grub_err_t fat32_install(grub_disk_t disk, grub_uint8_t options[5])
{
	grub_uint8_t* grldr_fat32 = grldr_pbr;
	grub_disk_write(disk, 0, 0, 3, grldr_fat32); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x5a, 0x200 - 0x5a, grldr_fat32 + 0x5a);
	grub_disk_write(disk, 0, 0x1e3, 5, options);
	return grub_errno;
}

static grub_err_t exfat_install(grub_disk_t disk, grub_uint8_t options[5])
{
	grub_uint8_t* grldr_exfat = grldr_pbr + 0x800;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 2)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, grldr_exfat); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x78, 0x400 - 0x78, grldr_exfat + 0x78);
	grub_disk_write(disk, 0, 0x1e3, 5, options);
	grub_br_write_exfat_checksum(disk);
	return grub_errno;
}

static grub_err_t ntfs_install(grub_disk_t disk, grub_uint8_t options[5])
{
	grub_uint8_t* grldr_ntfs = grldr_pbr + 0xc00;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 4)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, grldr_ntfs); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x54, 0x800 - 0x54, grldr_ntfs + 0x54);
	grub_disk_write(disk, 0, 0x1e3, 5, options);
	return grub_errno;
}

// TODO: EXT2/3/4 and UDF

static grub_err_t grldr_install(grub_disk_t disk, void* options)
{
	char* loader = options;
	grub_uint8_t grldr[5] = { 'G', 'R', 'L', 'D', 'R' };
	const char* fs_name = grub_br_get_fs_type(disk);
	if (loader)
	{
		if (grub_strlen(loader) != 5)
			return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "unsupported loader name %s", loader);
		grub_memcpy(grldr, loader, 5);
	}
	if (grub_strcmp(fs_name, "fat12") == 0 || grub_strcmp(fs_name, "fat16") == 0)
		return fat16_install(disk, grldr);
	else if (grub_strcmp(fs_name, "fat32") == 0)
		return fat32_install(disk, grldr);
	else if (grub_strcmp(fs_name, "exfat") == 0)
		return exfat_install(disk, grldr);
	else if (grub_strcmp(fs_name, "ntfs") == 0)
		return ntfs_install(disk, grldr);
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "unsupported filesystem %s", fs_name);
}

struct grub_br grub_pbr_grldr = 
{
	.name = "PBR_GRUB4DOS",
	.desc = "GRUB4DOS PBR",
	.code = 0,
	.code_size = 0,
	.reserved_sectors = 1,
	.identify = grldr_identify,
	.install = grldr_install,
	.next = 0,
};
