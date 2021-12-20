// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "fs.h"
#include "file.h"

grub_file_filter_t grub_file_filters[GRUB_FILE_FILTER_MAX];

/* Get the device part of the filename NAME. It is enclosed by parentheses.  */
char*
grub_file_get_device_name(const char* name)
{
	if (name[0] == '(')
	{
		char* p = grub_strchr(name, ')');
		char* ret;

		if (!p)
		{
			grub_error(GRUB_ERR_BAD_FILENAME, N_("missing `%c' symbol"), ')');
			return 0;
		}

		ret = (char*)grub_malloc(p - name);
		if (!ret)
			return 0;

		grub_memcpy(ret, name + 1, p - name - 1);
		ret[p - name - 1] = '\0';
		return ret;
	}

	return 0;
}

grub_file_t
grub_file_open(const char* name, enum grub_file_type type)
{
	grub_disk_t disk = 0;
	grub_file_t file = 0, last_file = 0;
	char* device_name;
	const char* file_name;
	grub_file_filter_id_t filter;

	file = (grub_file_t)grub_zalloc(sizeof(*file));
	if (!file)
		goto fail;

	/* Get the file part of NAME.  */
	file_name = (name[0] == '(') ? grub_strchr(name, ')') : NULL;
	if (file_name)
	{
		file_name++;
		device_name = grub_file_get_device_name(name);
		if (grub_errno)
			goto fail;
		disk = grub_disk_open(device_name);
		grub_free(device_name);
		if (!disk)
			goto fail;
		file->disk = disk;
	}

	if (!file_name)
	{
		file_name = name;
		file->fs = &grub_fs_winfile;
	}
	else if (file_name[0] != '/')
		/* This is a block list.  */
		file->fs = &grub_fs_blocklist;
	else
	{
		file->fs = grub_fs_probe(disk);
		if (!file->fs)
			goto fail;
	}

	if ((file->fs->fs_open) (file, file_name) != GRUB_ERR_NONE)
		goto fail;


	file->name = grub_strdup(name);
	grub_errno = GRUB_ERR_NONE;

	for (filter = 0; file && filter < ARRAY_SIZE(grub_file_filters);
		filter++)
		if (grub_file_filters[filter])
		{
			last_file = file;
			file = grub_file_filters[filter](file, type);
			if (file && file != last_file)
			{
				file->name = grub_strdup(name);
				grub_errno = GRUB_ERR_NONE;
			}
		}
	if (!file)
		grub_file_close(last_file);

	return file;

fail:
	if (disk)
		grub_disk_close(disk);

	grub_free(file);

	return 0;
}

grub_ssize_t
grub_file_read(grub_file_t file, void* buf, grub_size_t len)
{
	grub_ssize_t res;
	grub_disk_read_hook_t read_hook;
	void* read_hook_data;

	if (file->offset > file->size)
	{
		grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to read past the end of file");
		return -1;
	}

	if (len == 0)
		return 0;

	if (len > file->size - file->offset)
		len = file->size - file->offset;

	/* Prevent an overflow.  */
	if ((grub_ssize_t)len < 0)
		len >>= 1;

	if (len == 0)
		return 0;
	read_hook = file->read_hook;
	read_hook_data = file->read_hook_data;
	res = (file->fs->fs_read) (file, buf, len);
	file->read_hook = read_hook;
	file->read_hook_data = read_hook_data;
	if (res > 0)
		file->offset += res;

	return res;
}

grub_err_t
grub_file_close(grub_file_t file)
{
	if (file->fs->fs_close)
		(file->fs->fs_close) (file);

	if (file->disk)
		grub_disk_close(file->disk);
	grub_free(file->name);
	grub_free(file);
	return grub_errno;
}

grub_off_t
grub_file_seek(grub_file_t file, grub_off_t offset)
{
	grub_off_t old;

	if (offset > file->size)
	{
		grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to seek outside of the file");
		return (grub_off_t)-1;
	}

	old = file->offset;
	file->offset = offset;

	return old;
}

void
grub_file_filter_init(void)
{
	grub_file_filter_register(GRUB_FILE_FILTER_GZIO, grub_gzio_open);
}
