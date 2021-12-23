// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "disk.h"
#include "fs.h"
#include "file.h"
#include "datetime.h"

static void
read_block_start(grub_disk_addr_t sector, unsigned offset, unsigned length, void* data)
{
	(void)offset;
	grub_disk_addr_t* start = data;
	*start = sector + 1 - (length >> GRUB_DISK_SECTOR_BITS);
}

static grub_disk_addr_t
get_file_lba(grub_file_t file)
{
	grub_disk_addr_t start = 0;
	char buf[GRUB_DISK_SECTOR_SIZE];
	if (!file->disk)
		return GRUB_FILE_SIZE_UNKNOWN;
	file->read_hook = read_block_start;
	file->read_hook_data = &start;
	grub_file_seek(file, 0);
	grub_file_read(file, buf, sizeof(buf));
	grub_file_seek(file, 0);
	return start;
}

struct info_hook_data
{
	struct grub_dirhook_info *info;
	char* filename;
};

static int
info_hook(const char* filename, const struct grub_dirhook_info* info, void* data)
{
	struct info_hook_data* p = data;
	if (grub_strcmp(filename, p->filename) != 0)
		return 0;
	grub_memcpy(p->info, info, sizeof(struct grub_dirhook_info));
	return 1;
}

static int
get_file_info(grub_file_t file, struct grub_dirhook_info* info)
{
	char* p = NULL;
	char* dir = NULL;
	char* fn = NULL;
	struct info_hook_data ctx;
	if (!file->fs->fs_dir)
		goto fail;
	p = (file->name[0] == '(') ? grub_strchr(file->name, ')') : NULL;
	if (!p || !p[1])
		goto fail;
	p++;
	dir = grub_strdup(p);
	if (!dir)
		goto fail;
	p = grub_strrchr(dir, '/');
	if (!p || !p[1])
		goto fail;
	p++;
	fn = grub_strdup(p);
	if (!fn)
		goto fail;
	*p = 0;
	ctx.filename = fn;
	ctx.info = info;
	if (file->fs->fs_dir(file->disk, dir, info_hook, &ctx) == GRUB_ERR_NONE)
		return 1;
fail:
	grub_errno = GRUB_ERR_NONE;
	if (dir)
		grub_free(dir);
	if (fn)
		grub_free(fn);
	return 0;
}

static const char* d_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

static grub_err_t cmd_stat(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	grub_file_t file = 0;
	grub_disk_addr_t start = 0;
	struct grub_dirhook_info info = { 0 };
	if (argc < 1)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid argument");
		goto fail;
	}
	file = grub_file_open(argv[0], GRUB_FILE_TYPE_GET_SIZE | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
		goto fail;
	grub_printf("%10s: %s\n", "File", file->name);
	grub_printf("%10s: %llu (%s)\n", "Size", file->size,
		file->size == GRUB_FILE_SIZE_UNKNOWN ? "UNKNOWN" : grub_get_human_size(file->size, d_human_sizes, 1024));
	grub_printf("%10s: %s\n", "Filesystem", file->fs->name);
	if (get_file_info(file, &info))
	{
		if (info.mtimeset)
		{
			struct grub_datetime datetime;
			grub_unixtime2datetime(info.mtime, &datetime);
			grub_printf("%10s: %d-%02d-%02d %02d:%02d:%02d\n", "Modify",
				datetime.year, datetime.month, datetime.day,
				datetime.hour, datetime.minute,
				datetime.second);
		}
		if (info.inodeset)
			grub_printf("%10s: %llu\n", "Inode", info.inode);
	}
	start = get_file_lba(file);
	if (start != GRUB_FILE_SIZE_UNKNOWN)
		grub_printf("%10s: %llu\n", "LBA", start);
fail:
	if (file)
		grub_file_close(file);
	return grub_errno;
}

static void
help_stat(struct grub_command* cmd)
{
	grub_printf("%s FILE\n", cmd->name);
	grub_printf("Display file status.\n");
}

struct grub_command grub_cmd_stat =
{
	.name = "stat",
	.func = cmd_stat,
	.help = help_stat,
	.next = 0,
};
