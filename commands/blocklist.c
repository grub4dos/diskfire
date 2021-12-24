// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "file.h"
#include "fs.h"
#include "partition.h"
#include "command.h"

static void
print_blocklist(struct grub_fs_block* blocks, grub_off_t file_size, grub_disk_addr_t part_start)
{
	struct grub_fs_block* p;
	grub_off_t offset = 0;
	grub_uint64_t frags = 0;
	grub_disk_addr_t start_lba, len_lba, start_left, len_left;
	for (p = blocks; p->length && offset < file_size; p++)
	{
		if (frags)
			grub_printf(",");
		start_lba = (p->offset >> GRUB_DISK_SECTOR_BITS);
		start_left = p->offset - (start_lba << GRUB_DISK_SECTOR_BITS);
		len_lba = p->length >> GRUB_DISK_SECTOR_BITS;
		len_left = p->length - (len_lba << GRUB_DISK_SECTOR_BITS);
		grub_printf("%llu", start_lba + part_start);
		if (len_lba)
			grub_printf("+%llu", len_lba);
		if (start_left || len_left)
			grub_printf("[%llu-%llu]", start_left, start_left + len_left);
		frags++;
		offset += p->length;
	}
}

static grub_err_t
cmd_blocklist(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	grub_file_t file = 0;
	if (argc < 1)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing file name");
		goto fail;
	}
	file = grub_file_open(argv[0], GRUB_FILE_TYPE_BLOCKLIST | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
	{
		grub_error(GRUB_ERR_BAD_FILENAME, "bad file name");
		goto fail;
	}
	if (grub_blocklist_convert(file) == 0)
	{
		grub_error(GRUB_ERR_BAD_DEVICE, "no blocklist");
		goto fail;
	}
	grub_printf("(%s)", file->disk->name);
	print_blocklist(file->data, file->size, grub_partition_get_start(file->disk->partition));
	grub_printf("\n");
fail:
	if (file)
		grub_file_close(file);
	return grub_errno;
}

static void
help_blocklist(struct grub_command* cmd)
{
	grub_printf("%s FILE\n", cmd->name);
	grub_printf("Print a block list.\n");
}

struct grub_command grub_cmd_blocklist =
{
	.name = "blocklist",
	.func = cmd_blocklist,
	.help = help_blocklist,
	.next = 0,
};
