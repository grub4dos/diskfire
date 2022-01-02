// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

#include "pbr_ntldr.h"

static int ntldr_identify(grub_uint8_t* sector)
{
	grub_uint8_t ntldr_magic[5] = { 'N', 'T', 'L', 'D', 'R' };
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1ae, ntldr_magic, 5)) // FAT16
		return 1;
	if (grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1ae, ntldr_magic, 5)) // FAT32
		return 1;
	return grub_br_check_data(sector, GRUB_DISK_SECTOR_SIZE, 0x1a2, ntldr_magic, 5); // NTFS
}

static grub_err_t fat16_install(grub_disk_t disk)
{
	grub_uint8_t* ntldr_fat16 = ntldr_pbr;
	grub_disk_write(disk, 0, 0, 3, ntldr_fat16); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x3e, 0x200 - 0x3e, ntldr_fat16 + 0x3e);
	return grub_errno;
}

static grub_err_t fat32_install(grub_disk_t disk)
{
	grub_uint8_t* ntldr_fat32 = ntldr_pbr + 0x200;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 2)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, ntldr_fat32); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x5a, 0x400 - 0x5a, ntldr_fat32 + 0x5a);
	return grub_errno;
}

static grub_err_t ntfs_install(grub_disk_t disk)
{
	grub_uint8_t* ntldr_ntfs = ntldr_pbr + 0x600;
	grub_disk_addr_t reserved = grub_br_get_fs_reserved_sectors(disk);
	if (reserved < 7)
		return grub_error(GRUB_ERR_BAD_DEVICE, "unsupported reserved sectors");
	grub_disk_write(disk, 0, 0, 3, ntldr_ntfs); // jmp_boot[3]
	grub_disk_write(disk, 0, 0x54, 0xe00 - 0x54, ntldr_ntfs + 0x54);
	return grub_errno;
}

static grub_err_t ntldr_install(grub_disk_t disk, void* options)
{
	(void)options;
	const char* fs_name = grub_br_get_fs_type(disk);
	if (grub_strcmp(fs_name, "fat12") == 0 || grub_strcmp(fs_name, "fat16") == 0)
		return fat16_install(disk);
	else if (grub_strcmp(fs_name, "fat32") == 0)
		return fat32_install(disk);
	else if (grub_strcmp(fs_name, "ntfs") == 0)
		return ntfs_install(disk);
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "unsupported filesystem %s", fs_name);
}

struct grub_br grub_pbr_ntldr = 
{
	.name = "PBR_NT5",
	.desc = "NTLDR (NT5) PBR",
	.code = 0,
	.code_size = 0,
	.reserved_sectors = 1,
	.identify = ntldr_identify,
	.install = ntldr_install,
	.next = 0,
};
