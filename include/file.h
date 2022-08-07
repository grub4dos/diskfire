// SPDX-License-Identifier: GPL-3.0-or-later
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2022  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */
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
	GRUB_FILE_TYPE_HASH,
	GRUB_FILE_TYPE_BLOCKLIST,

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
	GRUB_FILE_FILTER_LZMAIO,
	GRUB_FILE_FILTER_ZSTDIO,
	GRUB_FILE_FILTER_VHDIO,
	GRUB_FILE_FILTER_MAX,
	GRUB_FILE_FILTER_COMPRESSION_FIRST = GRUB_FILE_FILTER_GZIO,
	GRUB_FILE_FILTER_COMPRESSION_LAST = GRUB_FILE_FILTER_VHDIO,
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
void grub_file_close (grub_file_t file);

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

char* grub_file_getline(grub_file_t file);

struct grub_fs_block
{
	grub_disk_addr_t offset;
	grub_off_t length;
};

grub_uint64_t
grub_blocklist_convert(grub_file_t file);

grub_ssize_t
grub_blocklist_write(grub_file_t file, const char* buf, grub_size_t len);

void
grub_file_filter_init(void);

grub_file_t
grub_gzio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_xzio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_lzopio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_vhdio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_lzmaio_open(grub_file_t io, enum grub_file_type type);

grub_file_t
grub_zstd_open(grub_file_t io, enum grub_file_type type);

#endif
