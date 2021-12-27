// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "command.h"
#include "br.h"

static void
mbr_print_info(grub_disk_t disk)
{
	grub_disk_addr_t lba = 0, count = 0;
	grub_br_t br = NULL;
	grub_printf("%s\n", disk->name);
	count = grub_br_get_reserved_sectors(disk, &lba);
	grub_printf("Reserved sectors: %llu+%llu\n", lba, count);
	br = grub_br_probe(disk);
	grub_printf("MBR: %s\n", br ? br->name : "UNKNOWN");
}

static grub_err_t
mbr_install(grub_disk_t disk, const char* mbr)
{
	if (grub_strcasecmp(mbr, "EMPTY") == 0)
		return grub_mbr_empty.install(disk, NULL);
	else if (grub_strcasecmp(mbr, "NT5") == 0)
		return grub_mbr_nt5.install(disk, NULL);
	else if (grub_strcasecmp(mbr, "NT6") == 0)
		return grub_mbr_nt6.install(disk, NULL);
	else if (grub_strcasecmp(mbr, "GRUB4DOS") == 0)
		return grub_mbr_grldr.install(disk, NULL);
	else
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "unknown mbr type");
}

static grub_err_t cmd_mbr(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	char* diskname = NULL;
	grub_disk_t disk = 0;
	const char* install = NULL;
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
		mbr_install(disk, install);
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
	grub_printf("  TYPES:\n");
	grub_printf("    EMPTY      Empty MBR\n");
	grub_printf("    GRUB4DOS   GRUB4DOS MBR\n");
	//grub_printf("    GRUB2      GRUB2 MBR\n");
	grub_printf("    NT5        NT5 MBR (ntldr)\n");
	grub_printf("    NT6        NT6 MBR (bootmgr)\n");
	grub_printf("  -b=FILE  Backup MBR to FILE.\n");
	grub_printf("    -s=N   Specify number of sectors to backup.\n");
	grub_printf("  -r=FILE  Restore MBR from FILE.\n");
	grub_printf("    -k     Keep original disk signature and partition table.\n");
}

struct grub_command grub_cmd_mbr =
{
	.name = "mbr",
	.func = cmd_mbr,
	.help = help_mbr,
	.next = 0,
};
