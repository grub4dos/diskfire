// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "fs.h"
#include "file.h"
#include "commands.h"

static int probe_partmap_hook(struct grub_disk* disk, const grub_partition_t partition, void* data)
{
	(void)disk;
	(void)partition;
	(void)data;
	return 1;
}

static grub_err_t probe_partmap(grub_disk_t disk)
{
	grub_disk_t parent = 0;
	grub_partition_map_t partmap;
	if (disk->partition && disk->partition->partmap->name)
	{
		grub_printf("%s\n", disk->partition->partmap->name);
		return GRUB_ERR_NONE;
	}
	parent = grub_disk_open(disk->name);
	if (!parent)
		return grub_errno;
	FOR_PARTITION_MAPS(partmap)
	{
		if (partmap->iterate(parent, probe_partmap_hook, NULL) == GRUB_ERR_NONE)
		{
			grub_printf("%s\n", partmap->name);
			goto out;
		}
		grub_errno = GRUB_ERR_NONE;
	}
	grub_printf("none\n");
out:
	grub_errno = GRUB_ERR_NONE;
	grub_disk_close(parent);
	return grub_errno;
}

static grub_err_t probe_fs(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->name)
		grub_printf("%s\n", fs->name);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_fsuuid(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->fs_uuid)
	{
		char* uuid;
		(fs->fs_uuid) (disk, &uuid);
		if (grub_errno == GRUB_ERR_NONE && uuid)
		{
			if (grub_strlen(uuid))
				grub_printf("%s\n", uuid);
			grub_free(uuid);
			grub_errno = GRUB_ERR_NONE;
		}
	}
	return grub_errno;
}

static grub_err_t probe_label(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->fs_label)
	{
		char* label;
		(fs->fs_label) (disk, &label);
		if (grub_errno == GRUB_ERR_NONE && label)
		{
			if (grub_strlen(label))
				grub_printf("%s\n", label);
			grub_free(label);
			grub_errno = GRUB_ERR_NONE;
		}
	}
	return grub_errno;
}

static grub_err_t probe_size(grub_disk_t disk)
{
	grub_printf("%llu\n", grub_disk_native_sectors(disk) << GRUB_DISK_SECTOR_BITS);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_startlba(grub_disk_t disk)
{
	grub_printf("%llu\n", disk->partition ? grub_partition_get_start(disk->partition) : 0);
	return GRUB_ERR_NONE;
}

static grub_err_t
parse_probe_opt(const char* arg, grub_disk_t disk)
{
	if (grub_strcmp(arg, "--partmap") == 0)
		return probe_partmap(disk);
	if (grub_strcmp(arg, "--fs") == 0)
		return probe_fs(disk);
	if (grub_strcmp(arg, "--fsuuid") == 0)
		return probe_fsuuid(disk);
	if (grub_strcmp(arg, "--label") == 0)
		return probe_label(disk);
	if (grub_strcmp(arg, "--size") == 0)
		return probe_size(disk);
	if (grub_strcmp(arg, "--startlba") == 0)
		return probe_startlba(disk);

	return grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid option %s\n", arg);
}

grub_err_t
cmd_probe(int argc, char* argv[])
{
	char* ptr;
	grub_disk_t disk = 0;
	if (argc < 2)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing arguments");
		goto fail;
	}
	ptr = argv[1] + grub_strlen(argv[1]) - 1;
	if (argv[1][0] == '(' && *ptr == ')')
	{
		*ptr = 0;
		disk = grub_disk_open(argv[1] + 1);
		*ptr = ')';
	}
	else
		disk = grub_disk_open(argv[1]);
	if (!disk)
		goto fail;
	parse_probe_opt(argv[0], disk);
fail:
	if (disk)
		grub_disk_close(disk);
	if (grub_error)
		grub_print_error();
	return 0;
}
