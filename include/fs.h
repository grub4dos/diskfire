// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _FS_HEADER
#define _FS_HEADER 1

#include "compat.h"
#include "disk.h"

/* Forward declaration is required, because of mutual reference.  */
struct grub_file;

struct grub_dirhook_info
{
	unsigned dir : 1;
	unsigned mtimeset : 1;
	unsigned case_insensitive : 1;
	unsigned inodeset : 1;
	grub_int64_t mtime;
	grub_uint64_t inode;
};

typedef int (*grub_fs_dir_hook_t) (const char* filename,
	const struct grub_dirhook_info* info,
	void* data);

/* Filesystem descriptor.  */
struct grub_fs
{
	/* The next filesystem.  */
	struct grub_fs* next;
	struct grub_fs** prev;

	/* My name.  */
	const char* name;

	/* Call HOOK with each file under DIR.  */
	grub_err_t(*fs_dir) (grub_disk_t disk, const char* path,
		grub_fs_dir_hook_t hook, void* hook_data);

	/* Open a file named NAME and initialize FILE.  */
	grub_err_t(*fs_open) (struct grub_file* file, const char* name);

	/* Read LEN bytes data from FILE into BUF.  */
	grub_ssize_t(*fs_read) (struct grub_file* file, char* buf, grub_size_t len);

	/* Close the file FILE.  */
	grub_err_t(*fs_close) (struct grub_file* file);

	/* Return the label of the device DEVICE in LABEL.  The label is
	   returned in a grub_malloc'ed buffer and should be freed by the
	   caller.  */
	grub_err_t(*fs_label) (grub_disk_t disk, char** label);

	/* Return the uuid of the device DEVICE in UUID.  The uuid is
	   returned in a grub_malloc'ed buffer and should be freed by the
	   caller.  */
	grub_err_t(*fs_uuid) (grub_disk_t disk, char** uuid);

	/* Get writing time of filesystem. */
	grub_err_t(*fs_mtime) (grub_disk_t disk, grub_int64_t* timebuf);
};
typedef struct grub_fs* grub_fs_t;

/* This is special, because block lists are not files in usual sense.  */
extern struct grub_fs grub_fs_blocklist;

extern grub_fs_t grub_fs_list;

static inline void
grub_fs_register(grub_fs_t fs)
{
	grub_list_push(GRUB_AS_LIST_P(&grub_fs_list), GRUB_AS_LIST(fs));
}

static inline void
grub_fs_unregister(grub_fs_t fs)
{
	grub_list_remove(GRUB_AS_LIST(fs));
}

#define FOR_FILESYSTEMS(var) FOR_LIST_ELEMENTS((var), (grub_fs_list))

grub_fs_t grub_fs_probe (grub_disk_t disk);

extern struct grub_fs grub_fat_fs;
extern struct grub_fs grub_exfat_fs;
extern struct grub_fs grub_ntfs_fs;
extern struct grub_fs grub_fbfs_fs;
extern struct grub_fs grub_ext2_fs;
extern struct grub_fs grub_udf_fs;
extern struct grub_fs grub_iso9660_fs;
extern struct grub_fs grub_cpio_be_fs;
extern struct grub_fs grub_cpio_fs;
extern struct grub_fs grub_newc_fs;

void grub_fs_init(void);

#endif
