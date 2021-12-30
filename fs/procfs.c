// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "fs.h"
#include "file.h"
#include "command.h"
#include "archelp.h"

struct grub_procfs_entry
{
	struct grub_procfs_entry* next;
	struct grub_procfs_entry** prev;
	char* name;
	char* (*get_contents) (grub_size_t* sz);
};

static struct grub_procfs_entry* grub_procfs_entries;

grub_err_t
proc_add(const char* name, char* (*get_contents) (grub_size_t* sz))
{
	struct grub_procfs_entry* entry = grub_zalloc(sizeof(struct grub_procfs_entry));
	if (!entry)
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	entry->next = 0;
	entry->name = grub_strdup(name);
	if (!name)
	{
		grub_free(entry);
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	}
	entry->get_contents = get_contents;
	grub_list_push(GRUB_AS_LIST_P(&grub_procfs_entries), GRUB_AS_LIST(entry));
	return GRUB_ERR_NONE;
}

void
proc_delete(const char* name)
{
	struct grub_procfs_entry* entry = grub_named_list_find(GRUB_AS_NAMED_LIST(grub_procfs_entries), name);
	if (entry)
		grub_list_remove(GRUB_AS_LIST(entry));
}

struct grub_archelp_data
{
	struct grub_procfs_entry* entry, * next_entry;
};

static void
grub_procfs_rewind(struct grub_archelp_data* data)
{
	data->entry = NULL;
	data->next_entry = grub_procfs_entries;
}

static grub_err_t
grub_procfs_find_file(struct grub_archelp_data* data, char** name,
	grub_int32_t* mtime,
	grub_archelp_mode_t* mode)
{
	data->entry = data->next_entry;
	if (!data->entry)
	{
		*mode = GRUB_ARCHELP_ATTR_END;
		return GRUB_ERR_NONE;
	}
	data->next_entry = data->entry->next;
	*mode = GRUB_ARCHELP_ATTR_FILE | GRUB_ARCHELP_ATTR_NOTIME;
	*name = grub_strdup(data->entry->name);
	*mtime = 0;
	if (!*name)
		return grub_errno;
	return GRUB_ERR_NONE;
}

static struct grub_archelp_ops arcops =
{
	.find_file = grub_procfs_find_file,
	.rewind = grub_procfs_rewind
};

static grub_ssize_t
grub_procfs_read(grub_file_t file, char* buf, grub_size_t len)
{
	char* data = file->data;

	grub_memcpy(buf, data + file->offset, len);

	return len;
}

static grub_err_t
grub_procfs_close(grub_file_t file)
{
	char* data;

	data = file->data;
	grub_free(data);

	return GRUB_ERR_NONE;
}

static grub_err_t
grub_procfs_dir(grub_disk_t disk, const char* path,
	grub_fs_dir_hook_t hook, void* hook_data)
{
	struct grub_archelp_data data;

	/* Check if the disk is our dummy disk.  */
	if (grub_strcmp(disk->name, "proc"))
		return grub_error(GRUB_ERR_BAD_FS, "not a procfs");

	grub_procfs_rewind(&data);

	return grub_archelp_dir(&data, &arcops,
		path, hook, hook_data);
}

static grub_err_t
grub_procfs_open(struct grub_file* file, const char* path)
{
	grub_err_t err;
	struct grub_archelp_data data;
	grub_size_t sz;

	grub_procfs_rewind(&data);

	err = grub_archelp_open(&data, &arcops, path);
	if (err)
		return err;
	file->data = data.entry->get_contents(&sz);
	if (!file->data)
		return grub_errno;
	file->size = sz;
	return GRUB_ERR_NONE;
}

struct grub_fs grub_procfs_fs =
{
	.name = "procfs",
	.fs_dir = grub_procfs_dir,
	.fs_open = grub_procfs_open,
	.fs_read = grub_procfs_read,
	.fs_close = grub_procfs_close,
	.next = 0
};
