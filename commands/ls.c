// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "fs.h"
#include "file.h"
#include "command.h"
#include "datetime.h"
#include "partition.h"

static const char* d_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

static grub_err_t
ls_print_disk_info(const char* name)
{
	grub_disk_t disk;
	char* p;

	p = grub_strchr(name, ',');
	if (p)
		grub_printf("\tPartition %s: ", name);
	else
		grub_printf("Disk %s: ", name);

	disk = grub_disk_open(name);
	if (!disk)
		grub_printf("Filesystem cannot be accessed");
	else
	{
		grub_fs_t fs;
		fs = grub_fs_probe(disk);
		/* Ignore all errors.  */
		grub_errno = 0;
		if (fs)
		{
			const char* fsname = fs->name;
			grub_printf("Filesystem type %s", fsname);
			if (fs->fs_label)
			{
				char* label;
				(fs->fs_label) (disk, &label);
				if (grub_errno == GRUB_ERR_NONE)
				{
					if (label && grub_strlen(label))
						grub_printf(" - Label `%s'", label);
					grub_free(label);
				}
				grub_errno = GRUB_ERR_NONE;
			}
			if (fs->fs_uuid)
			{
				char* uuid;
				(fs->fs_uuid) (disk, &uuid);
				if (grub_errno == GRUB_ERR_NONE)
				{
					if (uuid && grub_strlen(uuid))
						grub_printf(", UUID %s", uuid);
					grub_free(uuid);
				}
				grub_errno = GRUB_ERR_NONE;
			}
		}
		else
			grub_printf("No known filesystem detected");

		if (disk->partition)
			grub_printf(" - Partition start at %s",
				grub_get_human_size(grub_partition_get_start(disk->partition) << GRUB_DISK_SECTOR_BITS, d_human_sizes, 1024));
		else
			grub_printf(" - Sector size %uB", 1ULL << disk->log_sector_size);
		if (grub_disk_native_sectors(disk) == GRUB_DISK_SIZE_UNKNOWN)
			grub_printf(" - Total size unknown");
		else
			grub_printf(" - Total size %s",
				grub_get_human_size(grub_disk_native_sectors(disk) << GRUB_DISK_SECTOR_BITS, d_human_sizes, 1024));
		grub_disk_close(disk);
	}

	grub_printf("\n");
	return grub_errno;
}

static grub_err_t
ls_print_file_info(const char* filename, const struct grub_dirhook_info* info, const char *dirname)
{
	if (!info->dir)
	{
		grub_file_t file;
		grub_size_t pathlen = grub_strlen(dirname) + grub_strlen(filename) + 2;
		char* pathname = grub_malloc(pathlen);
		if (!pathname)
			return GRUB_ERR_OUT_OF_MEMORY;

		if (dirname[grub_strlen(dirname) - 1] == '/')
			grub_snprintf(pathname, pathlen, "%s%s", dirname, filename);
		else
			grub_snprintf(pathname, pathlen, "%s/%s", dirname, filename);

		/* XXX: For ext2fs symlinks are detected as files while they
		should be reported as directories.  */
		file = grub_file_open(pathname, GRUB_FILE_TYPE_GET_SIZE | GRUB_FILE_TYPE_NO_DECOMPRESS);
		if (!file)
		{
			grub_errno = 0;
			grub_free(pathname);
			return 0;
		}

		grub_printf("%-12s", grub_get_human_size(file->size, d_human_sizes, 1024));
		grub_file_close(file);
		grub_free(pathname);
	}
	else
		grub_printf("%-12s", "DIR");

	grub_printf("%s%s\n", filename, info->dir ? "/" : "");

	return 0;
}

static int
ls_print_disk(const char* name, void* data)
{
	int* longlist = data;

	if (*longlist)
		ls_print_disk_info(name);
	else
		grub_printf("(%s) ", name);
	return 0;
}

static grub_err_t
ls_list_disks(int longlist)
{
	grub_disk_iterate(ls_print_disk, &longlist);
	grub_printf("\n");
	return 0;
}

struct grub_ls_list_files_ctx
{
	const char* dirname;
	int longlist;
};

static int
ls_print_files(const char* filename, const struct grub_dirhook_info* info,
	void* data)
{
	struct grub_ls_list_files_ctx* ctx = data;

	if (ctx->longlist)
		ls_print_file_info(filename, info, ctx->dirname);
	else
		grub_printf("%s%s ", filename, info->dir ? "/" : "");

	return 0;
}

static grub_err_t
ls_list_files(char* dirname, int longlist)
{
	char* disk_name;
	grub_fs_t fs;
	const char* path;
	grub_disk_t disk;

	disk_name = grub_file_get_device_name(dirname);
	disk = grub_disk_open(disk_name);
	if (!disk)
		goto fail;

	fs = grub_fs_probe(disk);
	path = grub_strchr(dirname, ')');
	if (!path)
		path = dirname;
	else
		path++;

	if (!path && !disk_name)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid argument");
		goto fail;
	}

	if ((!path || !*path) && disk_name)
	{
		if (grub_errno == GRUB_ERR_UNKNOWN_FS)
			grub_errno = GRUB_ERR_NONE;

		ls_print_disk_info(disk_name);
	}
	else if (fs)
	{
		struct grub_ls_list_files_ctx ctx =
		{
		  .dirname = dirname,
		  .longlist = longlist,
		};
		(fs->fs_dir) (disk, path, ls_print_files, &ctx);

		if (grub_errno == GRUB_ERR_BAD_FILE_TYPE
			&& path[grub_strlen(path) - 1] != '/')
		{
			/* PATH might be a regular file.  */
			char* p;
			grub_file_t file;
			struct grub_dirhook_info info;
			grub_errno = 0;

			file = grub_file_open(dirname, GRUB_FILE_TYPE_GET_SIZE | GRUB_FILE_TYPE_NO_DECOMPRESS);
			if (!file)
				goto fail;

			grub_file_close(file);

			p = grub_strrchr(dirname, '/') + 1;
			dirname = grub_strndup(dirname, p - dirname);
			if (!dirname)
				goto fail;

			grub_memset(&info, 0, sizeof(info));
			ctx.dirname = dirname;
			ls_print_files(p, &info, &ctx);

			grub_free(dirname);
		}

		if (grub_errno == GRUB_ERR_NONE)
			grub_printf("\n");
	}

fail:
	if (disk)
		grub_disk_close(disk);

	grub_free(disk_name);

	return 0;
}

static grub_err_t
cmd_ls(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int longlist = 0;
	if (argc > 0 && grub_strcmp(argv[0], "-l") == 0)
		longlist = 1;
	if (argc - longlist < 1)
		ls_list_disks(longlist);
	else
		ls_list_files(argv[longlist], longlist);
	return grub_errno;
}

static void
help_ls(struct grub_command* cmd)
{
	grub_printf("%s [-l] [FILE|DISK]\n", cmd->name);
	grub_printf("List disks and files.\n");
	grub_printf("  -l  Show a long list with more detailed information.\n");
}

struct grub_command grub_cmd_ls =
{
	.name = "ls",
	.func = cmd_ls,
	.help = help_ls,
	.next = 0,
};

