// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _FILE_HEADER
#define _FILE_HEADER 1

#include "compat.h"
#include "fs.h"

enum grub_file_type
{
	GRUB_FILE_TYPE_NONE = 0,
	GRUB_FILE_TYPE_GET_SIZE,
	GRUB_FILE_TYPE_CAT,
	GRUB_FILE_TYPE_EXTRACT,
	GRUB_FILE_TYPE_LOOPBACK,

	GRUB_FILE_TYPE_MASK = 0xffff,

	GRUB_FILE_TYPE_SKIP_SIGNATURE = 0x10000,
	GRUB_FILE_TYPE_NO_DECOMPRESS = 0x20000
};

/* File description.  */
struct grub_file
{
	/* File name.  */
	char* name;

	/* The underlying device.  */
	grub_disk_t disk;

	/* The underlying filesystem.  */
	grub_fs_t fs;

	/* The current offset.  */
	grub_off_t offset;

	/* The file size.  */
	grub_off_t size;

	/* If file is not easily seekable. Should be set by underlying layer.  */
	int not_easily_seekable;

	/* Filesystem-specific data.  */
	void* data;

	/* This is called when a sector is read. Used only for a disk device.  */
	grub_disk_read_hook_t read_hook;

	/* Caller-specific data passed to the read hook.  */
	void* read_hook_data;
};
typedef struct grub_file* grub_file_t;

/* Filters with lower ID are executed first.  */
typedef enum grub_file_filter_id
{
	GRUB_FILE_FILTER_VERIFY,
	GRUB_FILE_FILTER_GZIO,
	GRUB_FILE_FILTER_XZIO,
	GRUB_FILE_FILTER_LZOPIO,
	GRUB_FILE_FILTER_MAX,
	GRUB_FILE_FILTER_COMPRESSION_FIRST = GRUB_FILE_FILTER_GZIO,
	GRUB_FILE_FILTER_COMPRESSION_LAST = GRUB_FILE_FILTER_LZOPIO,
} grub_file_filter_id_t;

typedef grub_file_t(*grub_file_filter_t) (grub_file_t in,
	enum grub_file_type type);

extern grub_file_filter_t grub_file_filters[GRUB_FILE_FILTER_MAX];

static inline void
grub_file_filter_register(grub_file_filter_id_t id, grub_file_filter_t filter)
{
	grub_file_filters[id] = filter;
}

static inline void
grub_file_filter_unregister(grub_file_filter_id_t id)
{
	grub_file_filters[id] = 0;
}

/* Get a device name from NAME.  */
char* grub_file_get_device_name (const char* name);

grub_file_t grub_file_open (const char* name, enum grub_file_type type);
grub_ssize_t grub_file_read (grub_file_t file, void* buf, grub_size_t len);
grub_off_t grub_file_seek (grub_file_t file, grub_off_t offset);
grub_err_t grub_file_close (grub_file_t file);

/* Return value of grub_file_size() in case file size is unknown. */
#define GRUB_FILE_SIZE_UNKNOWN	 0xffffffffffffffffULL

static inline grub_off_t
grub_file_size(const grub_file_t file)
{
	return file->size;
}

static inline grub_off_t
grub_file_tell(const grub_file_t file)
{
	return file->offset;
}

static inline int
grub_file_seekable(const grub_file_t file)
{
	return !file->not_easily_seekable;
}

void
grub_file_filter_init(void);

grub_file_t
grub_gzio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_xzio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_lzopio_open(grub_file_t io, enum grub_file_type type);

#endif
