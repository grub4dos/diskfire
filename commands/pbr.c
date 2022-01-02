// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "file.h"
#include "command.h"
#include "br.h"
#include "misc.h"
#include "fat.h"
#include "exfat.h"
#include "ntfs.h"
#include "ext2.h"
#include "misc.h"

static const char*
oem_to_str(grub_uint8_t oem_name[8])
{
	static char oem[9];
	grub_memcpy(oem, oem_name, 8);
	oem[8] = 0;
	return oem;
}

static void
pbr_print_info(grub_disk_t disk)
{
	//grub_br_t br = NULL;
	grub_uint8_t vbr[GRUB_DISK_SECTOR_SIZE];
	const char* fs_name = grub_br_get_fs_type(disk);
	grub_errno = GRUB_ERR_NONE;
	if (grub_disk_read(disk, 0, 0, GRUB_DISK_SECTOR_SIZE, vbr))
		return;
	grub_printf("FS: %s\n", fs_name);
	if (grub_strncmp(fs_name, "fat", 3) == 0)
	{
		struct grub_fat_bpb* bpb = (void*)vbr;
		grub_printf("OEM name: %s\n", oem_to_str(bpb->oem_name));
		grub_printf("Reserved sectors: %u\n", bpb->num_reserved_sectors);
	}
	else if (grub_strcmp(fs_name, "exfat") == 0)
	{
		struct grub_exfat_bpb* bpb = (void*)vbr;
		grub_printf("OEM name: %s\n", oem_to_str(bpb->oem_name));
		grub_printf("Reserved sectors: %u\n", bpb->num_reserved_sectors);
	}
	else if (grub_strcmp(fs_name, "ntfs") == 0)
	{
		struct grub_ntfs_bpb* bpb = (void*)vbr;
		grub_printf("OEM name: %s\n", oem_to_str(bpb->oem_name));
		grub_printf("Reserved sectors: %llu\n", grub_br_get_fs_reserved_sectors(disk));
		grub_printf("$MFT cluster number: %llu\n", bpb->mft_lcn);
	}
	else if (grub_strcmp(fs_name, "ext") == 0)
	{
		struct grub_ext2_sblock sb;
		grub_disk_read(disk, 1 * 2, 0, sizeof(struct grub_ext2_sblock), &sb);
		grub_printf("Reserved blocks: %u\n", sb.reserved_blocks);
		grub_printf("First data block: %u\n", sb.first_data_block);
		grub_printf("LOG2 block size: %u\n", sb.log2_block_size);
	}
}

static grub_err_t
pbr_install(grub_disk_t disk, const char* pbr, char *options)
{
	HANDLE* hVolList = NULL;
	grub_br_t br = NULL;
	grub_size_t br_len = grub_strlen(pbr) + 5; // PBR_XXX
	char* br_name = grub_malloc(br_len);
	if (!br_name)
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	grub_snprintf(br_name, br_len, "PBR_%s", pbr);
	br = grub_br_find(br_name);
	grub_free(br_name);
	if (!br)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "unknown pbr type");
	if (disk->dev->id == GRUB_DISK_WINDISK_ID)
		hVolList = LockDriveById(disk->id);
	br->install(disk, options);		
	if (disk->dev->id == GRUB_DISK_WINDISK_ID)
		UnlockDrive(hVolList);
	return grub_errno;
}

static grub_err_t
pbr_backup(grub_disk_t disk, const char* dest, grub_disk_addr_t sectors)
{
	grub_disk_addr_t i;
	HANDLE fd = INVALID_HANDLE_VALUE;
	grub_uint8_t buf[GRUB_DISK_SECTOR_SIZE];
	fd = CreateFileA(dest, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (!fd || fd == INVALID_HANDLE_VALUE)
		return grub_error(GRUB_ERR_BAD_FILENAME, "can't create file %s", dest);
	for (i = 0; i < sectors; i++)
	{
		if (grub_disk_read(disk, i, 0, GRUB_DISK_SECTOR_SIZE, buf))
			break;
		if (!WriteFile(fd, buf, GRUB_DISK_SECTOR_SIZE, NULL, NULL))
		{
			grub_error(GRUB_ERR_WRITE_ERROR, "write failed");
			break;
		}
	}

	CHECK_CLOSE_HANDLE(fd);
	return grub_errno;
}

static grub_err_t
pbr_restore(grub_disk_t disk, const char* src, int keep)
{
	grub_disk_addr_t i;
	grub_file_t file = 0;
	grub_uint8_t buf[GRUB_DISK_SECTOR_SIZE];
	const char* fs_name = grub_br_get_fs_type(disk);
	grub_off_t bpb_length = 0;
	HANDLE* hVolList = NULL;

	if (grub_strcmp(fs_name, "fat12") == 0 || grub_strcmp(fs_name, "fat16") == 0)
		bpb_length = 0x3e;
	else if (grub_strcmp(fs_name, "fat32") == 0)
		bpb_length = 0x5a;
	else if (grub_strcmp(fs_name, "exfat") == 0)
		bpb_length = 0x78;
	else if (grub_strcmp(fs_name, "ntfs") == 0)
		bpb_length = 0x54;
	else if (keep)
		return grub_error(GRUB_ERR_BAD_FS, "unsupported fs %s", fs_name);

	file = grub_file_open(src, GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
		return grub_error(GRUB_ERR_BAD_FILENAME, "can't open %s", src);
	if (disk->dev->id == GRUB_DISK_WINDISK_ID)
		hVolList = LockDriveById(disk->id);
	for (i = 0; i << GRUB_DISK_SECTOR_BITS < file->size; i++)
	{
		grub_file_read(file, buf, GRUB_DISK_SECTOR_SIZE);
		if (grub_errno)
			break;
		if (i == 0 && keep)
			grub_disk_read(disk, 0, 0, bpb_length, buf);
		grub_disk_write(disk, i, 0, GRUB_DISK_SECTOR_SIZE, buf);
		if (grub_errno)
			break;
	}
	grub_file_close(file);
	if (disk->dev->id == GRUB_DISK_WINDISK_ID)
		UnlockDrive(hVolList);
	return grub_errno;
}

static grub_err_t cmd_pbr(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	char* diskname = NULL;
	grub_disk_t disk = 0;
	const char* install = NULL;
	const char* backup = NULL;
	const char* restore = NULL;
	char* extra = NULL;
	grub_disk_addr_t backup_sectors = 1;
	int keep_bpb = 1;
	for (i = 0; i < argc; i++)
	{
		if (argv[i][0] == '(')
		{
			grub_size_t pos = grub_strlen(argv[i]) - 1;
			if (argv[i][pos] != ')')
				break;
			argv[i][pos] = 0;
			diskname = &argv[i][1];
		}
		else if (grub_strncmp(argv[i], "-i=", 3) == 0)
		{
			install = &argv[i][3];
		}
		else if (grub_strncmp(argv[i], "-o=", 3) == 0)
		{
			extra = &argv[i][3];
		}
		else if (grub_strncmp(argv[i], "-b=", 3) == 0)
		{
			backup = &argv[i][3];
		}
		else if (grub_strncmp(argv[i], "-s=", 3) == 0)
		{
			backup_sectors = grub_strtoull(&argv[i][3], NULL, 0);
		}
		else if (grub_strncmp(argv[i], "-r=", 3) == 0)
		{
			restore = &argv[i][3];
		}
		else if (grub_strcmp(argv[i], "-x") == 0)
		{
			keep_bpb = 0;
		}
		else if (argv[i][0] != '-')
			diskname = argv[i];
		else
		{
			grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid option");
			goto fail;
		}
	}
	if (!diskname)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing disk");
		goto fail;
	}
	disk = grub_disk_open(diskname);
	if (!disk)
		goto fail;
	if (install)
		pbr_install(disk, install, extra);
	else if (backup)
		pbr_backup(disk, backup, backup_sectors);
	else if (restore)
		pbr_restore(disk, restore, keep_bpb);
	else
		pbr_print_info(disk);
fail:
	if (disk)
		grub_disk_close(disk);
	return grub_errno;
}

static void
help_pbr(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] DISK\n", cmd->name);
	grub_printf("PBR tool.\nOPTIONS:\n");
	grub_printf("  -i=TYPE      Install PBR to disk partition.\n");
	grub_printf("    -o=CMD     Specify extra options for PBR.\n");
	grub_printf("  TYPES:\n");
	grub_printf("    GRUB4DOS   GRUB4DOS PBR, extra options:\n");
	grub_printf("      <NAME>     Rename GRLDR.\n");
	grub_printf("    NT5        Windows NT5 PBR\n");
	grub_printf("    NT6        Windows NT6+ PBR\n");
	grub_printf("  -b=FILE      Backup PBR to FILE.\n");
	grub_printf("    -s=N       Specify number of sectors to backup.\n");
	grub_printf("  -r=FILE      Restore PBR from FILE.\n");
	grub_printf("    -x         Do NOT keep original BPB (BIOS Parameter Block).\n");
}

struct grub_command grub_cmd_pbr =
{
	.name = "pbr",
	.func = cmd_pbr,
	.help = help_pbr,
	.next = 0,
};
