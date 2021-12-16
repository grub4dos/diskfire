// SPDX-License-Identifier: GPL-3.0-or-later

#include "compat.h"
#include "disk.h"
#include "fs.h"

#define ALIGN_CPIO(x) (ALIGN_UP ((x), 2))
#define	MAGIC       "\x71\xc7"

PRAGMA_BEGIN_PACKED
struct head
{
	grub_uint16_t magic[1];
	grub_uint16_t dev;
	grub_uint16_t ino;
	grub_uint16_t mode[1];
	grub_uint16_t uid;
	grub_uint16_t gid;
	grub_uint16_t nlink;
	grub_uint16_t rdev;
	grub_uint16_t mtime[2];
	grub_uint16_t namesize[1];
	grub_uint16_t filesize[2];
};
PRAGMA_END_PACKED

static inline unsigned long long
read_number(const grub_uint16_t* arr, grub_size_t size)
{
	long long ret = 0;
	while (size--)
		ret = (ret << 16) | grub_be_to_cpu16(*arr++);
	return ret;
}

#define FSNAME "cpiofs_be"

#include "cpio_common.c"

struct grub_fs grub_cpio_be_fs =
{
	.name = FSNAME,
	.fs_dir = grub_cpio_dir,
	.fs_open = grub_cpio_open,
	.fs_read = grub_cpio_read,
	.fs_close = grub_cpio_close,
};
