// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "pbr_bootmgr.h"

static int bootmgr_identify(grub_uint8_t* sector)
{
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1ae, (grub_uint8_t*)"BOOTMGR", 7)) // FAT16
		return 1;
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x16d, (grub_uint8_t*)"BOOTMGR", 7)) // FAT32
		return 1;
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x15e, (grub_uint8_t*)L"BOOTMGR", 14)) // EXFAT
		return 1;
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1a9, (grub_uint8_t*)"BOOTMGR", 7); // NTFS
}

static grub_err_t fat16_install(grub_disk_t disk)
{
	grub_uint8_t* bootmgr_fat16 = bootmgr_pbr;
	grub_disk_write(disk, 0, 0, 3, bootmgr_fat16); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x3e, 0x200 - 0x3e, bootmgr_fat16 + 0x3e);
	return grub_errno;
}

static grub_err_t fat32_install(grub_disk_t disk)
{
	grub_uint8_t* bootmgr_fat32 = bootmgr_pbr + 0x200;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 2)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, bootmgr_fat32); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x5a, 0x400 - 0x5a, bootmgr_fat32 + 0x5a);
	return grub_errno;
}

static grub_err_t exfat_install(grub_disk_t disk)
{
	grub_uint8_t* bootmgr_exfat = bootmgr_pbr + 0x600;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 3)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, bootmgr_exfat); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x78, 0x600 - 0x78, bootmgr_exfat + 0x78);
	return grub_errno;
}

static grub_err_t ntfs_install(grub_disk_t disk)
{
	grub_uint8_t* bootmgr_ntfs = bootmgr_pbr + 0xc00;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 10)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, bootmgr_ntfs); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x54, 0x1400 - 0x54, bootmgr_ntfs + 0x54);
	return grub_errno;
}

static grub_err_t bootmgr_install(grub_disk_t disk, void* options)
{
	(void)options;
	const char* fs_name = grub_br_get_fs_type(disk);
	if (grub_strcmp(fs_name, "fat12") == 0 || grub_strcmp(fs_name, "fat16") == 0)
		return fat16_install(disk);
	else if (grub_strcmp(fs_name, "fat32") == 0)
		return fat32_install(disk);
	else if (grub_strcmp(fs_name, "exfat") == 0)
		return exfat_install(disk);
	else if (grub_strcmp(fs_name, "ntfs") == 0)
		return ntfs_install(disk);
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "unsupported filesystem %s", fs_name);
}

struct grub_br grub_pbr_bootmgr = 
{
	.name = "PBR_NT6",
	.desc = "BOOTMGR (NT6) PBR",
	.code = 0,
	.code_size = 0,
	.reserved_sectors = 1,
	.identify = bootmgr_identify,
	.install = bootmgr_install,
	.next = 0,
};
