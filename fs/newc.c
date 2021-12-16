// SPDX-License-Identifier: GPL-3.0-or-later

#include "compat.h"
#include "disk.h"
#include "fs.h"

#define ALIGN_CPIO(x) (ALIGN_UP ((x), 4))
#define	MAGIC	"070701"
#define	MAGIC2	"070702"
PRAGMA_BEGIN_PACKED
struct head
{
	char magic[6];
	char ino[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char devmajor[8];
	char devminor[8];
	char rdevmajor[8];
	char rdevminor[8];
	char namesize[8];
	char check[8];
};
PRAGMA_END_PACKED

static inline unsigned long long
read_number(const char* str, grub_size_t size)
{
	unsigned long long ret = 0;
	while (size-- && grub_isxdigit(*str))
	{
		char dig = *str++;
		if (dig >= '0' && dig <= '9')
			dig &= 0xf;
		else if (dig >= 'a' && dig <= 'f')
			dig -= 'a' - 10;
		else
			dig -= 'A' - 10;
		ret = (ret << 4) | (dig);
	}
	return ret;
}

#define FSNAME "newc"

#include "cpio_common.c"

struct grub_fs grub_newc_fs =
{
	.name = FSNAME,
	.fs_dir = grub_cpio_dir,
	.fs_open = grub_cpio_open,
	.fs_read = grub_cpio_read,
	.fs_close = grub_cpio_close,
};
