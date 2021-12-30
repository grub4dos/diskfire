// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "file.h"
#include "command.h"
#include "br.h"
#include "misc.h"

static void
mbr_print_info(grub_disk_t disk)
{
	grub_disk_addr_t lba = 0, count = 0;
	grub_br_t br = NULL;
	grub_printf("%s\n", disk->name);
	count = grub_br_get_reserved_sectors(disk, &lba);
	grub_printf("Reserved sectors: %llu+%llu\n", lba, count);
	br = grub_br_probe(disk);
	grub_printf("MBR: %s\n", br ? br->desc : "UNKNOWN");
}

static grub_err_t
mbr_install(grub_disk_t disk, const char* mbr, char *options)
{
	grub_br_t br = NULL;
	br = grub_br_find(mbr);
	if (!br)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "unknown mbr type");
	return br->install(disk, options);		
}

static grub_err_t
mbr_backup(grub_disk_t disk, const char* dest, grub_disk_addr_t sectors)
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
mbr_restore(grub_disk_t disk, const char* src, int keep)
{
	grub_disk_addr_t i;
	grub_file_t file = 0;
	grub_uint8_t buf[GRUB_DISK_SECTOR_SIZE];
	file = grub_file_open(src, GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
		return grub_error(GRUB_ERR_BAD_FILENAME, "can't open %s", src);
	for (i = 0; i << GRUB_DISK_SECTOR_BITS < file->size; i++)
	{
		grub_file_read(file, buf, GRUB_DISK_SECTOR_SIZE);
		if (grub_errno)
			break;
		if (i == 0 && keep)
			grub_disk_read(disk, 0, 0x1b8, 0x1fe - 0x1b8, buf + 0x1b8);
		grub_disk_write(disk, i, 0, GRUB_DISK_SECTOR_SIZE, buf);
		if (grub_errno)
			break;
	}
	grub_file_close(file);
	return grub_errno;
}

static grub_err_t cmd_mbr(struct grub_command* cmd, int argc, char* argv[])
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
	int keep_part_table = 1;
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
			keep_part_table = 0;
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
	if (disk->partition)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "target disk is partition");
		goto fail;
	}
	if (install)
		mbr_install(disk, install, extra);
	else if (backup)
		mbr_backup(disk, backup, backup_sectors);
	else if (restore)
		mbr_restore(disk, restore, keep_part_table);
	else
		mbr_print_info(disk);
fail:
	if (disk)
		grub_disk_close(disk);
	return grub_errno;
}

static void
help_mbr(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] DISK\n", cmd->name);
	grub_printf("MBR tool.\nOPTIONS:\n");
	grub_printf("  -i=TYPE      Install MBR to disk.\n");
	grub_printf("    -o=CMD     Specify extra options for MBR.\n");
	grub_printf("  TYPES:\n");
	grub_printf("    EMPTY      Empty MBR\n");
	grub_printf("    GRUB4DOS   GRUB4DOS MBR\n");
	//grub_printf("    GRUB2      GRUB2 MBR\n");
	grub_printf("    NT5        Windows NT5 MBR\n");
	grub_printf("    NT6        Windows NT6+ MBR\n");
	grub_printf("    RUFUS      Rufus MBR\n");
	grub_printf("    ULTRAISO   UltraISO USB-HDD+ MBR\n");
	grub_printf("    SYSLINUX   SYSLINUX 6.02 MBR\n");
	grub_printf("    WEE        WEE63 MBR, extra options:\n");
	grub_printf("      <MENU>     Import custom WEE menu.\n");
	grub_printf("  -b=FILE      Backup MBR to FILE.\n");
	grub_printf("    -s=N       Specify number of sectors to backup.\n");
	grub_printf("  -r=FILE      Restore MBR from FILE.\n");
	grub_printf("    -x         Do NOT keep original disk signature and partition table.\n");
}

struct grub_command grub_cmd_mbr =
{
	.name = "mbr",
	.func = cmd_mbr,
	.help = help_mbr,
	.next = 0,
};
