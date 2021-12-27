// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "command.h"
#include "br.h"

struct reserved_ctx
{
	grub_disk_addr_t firstlba;
	grub_disk_addr_t partlba;
	grub_partition_map_t partmap;
};

static int first_part_hook(struct grub_disk* disk, const grub_partition_t partition, void* data)
{
	(void)disk;
	struct reserved_ctx *ctx = data;
	grub_disk_addr_t start = grub_partition_get_start(partition);
	if (start < ctx->partlba)
		ctx->partlba = start;
	if (!ctx->partmap)
		ctx->partmap = partition->partmap;
	if (!ctx->firstlba && partition->firstlba)
		ctx->firstlba = partition->firstlba;
	return 0;
}

static grub_disk_addr_t
get_reserved_sectors(grub_disk_t disk, grub_disk_addr_t* start)
{
	struct reserved_ctx ctx = { .firstlba = 0, .partlba = GRUB_DISK_SIZE_UNKNOWN, .partmap = NULL };
	grub_partition_iterate(disk, first_part_hook, &ctx);
	if (!ctx.partmap || ctx.partlba <= 1 || !ctx.firstlba)
		return 0;
	if (start)
		*start = ctx.firstlba;
	return ctx.partlba - ctx.firstlba;
}

static void
mbr_print_info(grub_disk_t disk)
{
	grub_disk_addr_t lba, count;
	grub_br_t br = NULL;
	grub_printf("%s\n", disk->name);
	count = get_reserved_sectors(disk, &lba);
	grub_printf("Reserved sectors: %llu+%llu\n", lba, count);
	br = grub_br_probe(disk);
	grub_printf("MBR: %s\n", br ? br->name : "UNKNOWN");
}

static grub_err_t cmd_mbr(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	char* diskname = NULL;
	grub_disk_t disk = 0;
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
	//grub_printf("    GRUB4DOS   GRUB4DOS MBR\n");
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
