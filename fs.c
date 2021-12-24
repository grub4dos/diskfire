// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "fs.h"
#include "file.h"
#include "misc.h"
#include <windows.h>

grub_fs_t grub_fs_list = 0;

/* Helper for grub_fs_probe.  */
static int
probe_dummy_iter(const char* filename,
	const struct grub_dirhook_info* info,
	void* data)
{
	(void)filename;
	(void)info;
	(void)data;
	return 1;
}

grub_fs_t
grub_fs_probe(grub_disk_t disk)
{
	grub_fs_t p;

	for (p = grub_fs_list; p; p = p->next)
	{
		grub_dprintf("fs", "Detecting %s...\n", p->name);

		(p->fs_dir) (disk, "/", probe_dummy_iter, NULL);
		if (grub_errno == GRUB_ERR_NONE)
			return p;

		grub_error_push();
		grub_dprintf("fs", "%s detection failed.\n", p->name);
		grub_error_pop();

		if (grub_errno != GRUB_ERR_BAD_FS && grub_errno != GRUB_ERR_OUT_OF_RANGE)
			return 0;

		grub_errno = GRUB_ERR_NONE;
	}

	grub_error(GRUB_ERR_UNKNOWN_FS, N_("unknown filesystem"));
	return 0;
}

void
grub_fs_init(void)
{
	grub_fs_register(&grub_fat_fs);
	grub_fs_register(&grub_exfat_fs);
	grub_fs_register(&grub_ntfs_fs);
	grub_fs_register(&grub_fbfs_fs);
	grub_fs_register(&grub_ext2_fs);
	grub_fs_register(&grub_udf_fs);
	grub_fs_register(&grub_iso9660_fs);
	grub_fs_register(&grub_cpio_be_fs);
	grub_fs_register(&grub_cpio_fs);
	grub_fs_register(&grub_newc_fs);
	grub_fs_register(&grub_tar_fs);
	grub_fs_register(&grub_squash_fs);
	grub_fs_register(&grub_f2fs_fs);
	grub_fs_register(&grub_xfs_fs);
	grub_fs_register(&grub_jfs_fs);
	grub_fs_register(&grub_hfs_fs);
	grub_fs_register(&grub_hfsplus_fs);
	grub_fs_register(&grub_btrfs_fs);
	grub_fs_register(&grub_reiserfs_fs);
}

/* Block list support routines.  */

static grub_err_t
grub_fs_blocklist_open(grub_file_t file, const char* name)
{
	const char* p = name;
	unsigned num = 0;
	unsigned i;
	grub_disk_t disk = file->disk;
	struct grub_fs_block* blocks;
	grub_disk_addr_t part_sectors = 0;
	grub_disk_addr_t max_sectors;

	/* First, count the number of blocks.  */
	do
	{
		num++;
		p = grub_strchr(p, ',');
		if (p)
			p++;
	} while (p);

	/* Allocate a block list.  */
	blocks = grub_calloc(1ULL + num, sizeof(struct grub_fs_block));
	if (!blocks)
		return 0;

	file->size = 0;
	max_sectors = grub_disk_from_native_sector(disk, disk->total_sectors);
	p = (char*)name;
	if (!*p)
	{
		blocks[0].offset = 0;
		if (disk->partition)
		{
			part_sectors = grub_disk_from_native_sector(disk, disk->partition->len);
			blocks[0].length = part_sectors << GRUB_DISK_SECTOR_BITS;
		}
		else
			blocks[0].length = max_sectors << GRUB_DISK_SECTOR_BITS;
		file->size = blocks[0].length;
	}
	else
	{
		for (i = 0; i < num; i++)
		{
			if (*p != '+')
			{
				blocks[i].offset = grub_strtoull(p, &p, 0);
				if (grub_errno != GRUB_ERR_NONE || *p != '+')
				{
					grub_error(GRUB_ERR_BAD_FILENAME,
						N_("invalid file name `%s'"), name);
					goto fail;
				}
			}

			p++;
			if (*p == '\0' || *p == ',')
				blocks[i].length = max_sectors - blocks[i].offset;
			else
				blocks[i].length = grub_strtoull(p, &p, 0);

			if (grub_errno != GRUB_ERR_NONE
				|| blocks[i].length == 0
				|| (*p && *p != ',' && !grub_isspace(*p)))
			{
				grub_error(GRUB_ERR_BAD_FILENAME,
					"invalid file name %s", name);
				goto fail;
			}

			if (max_sectors < blocks[i].offset + blocks[i].length)
			{
				grub_error(GRUB_ERR_BAD_FILENAME, "beyond the total sectors");
				goto fail;
			}

			file->size += (blocks[i].length << GRUB_DISK_SECTOR_BITS);
			p++;
		}
	}

	file->data = blocks;

	return GRUB_ERR_NONE;

fail:
	grub_free(blocks);
	return grub_errno;
}

static grub_ssize_t
grub_fs_blocklist_rw(grub_file_t file, char* buf, grub_size_t len, int write)
{
	struct grub_fs_block* p;
	grub_disk_addr_t sector;
	grub_off_t offset;
	grub_ssize_t ret = 0;

	if (len > file->size - file->offset)
		len = file->size - file->offset;

	sector = (file->offset >> GRUB_DISK_SECTOR_BITS);
	offset = (file->offset & (GRUB_DISK_SECTOR_SIZE - 1));
	for (p = file->data; p->length && len > 0; p++)
	{
		if (sector < p->length)
		{
			grub_size_t size;

			size = len;
			if (((size + offset + GRUB_DISK_SECTOR_SIZE - 1)
				>> GRUB_DISK_SECTOR_BITS) > p->length - sector)
				size = ((p->length - sector) << GRUB_DISK_SECTOR_BITS) - offset;

			if ((write ?
				grub_disk_write(file->disk, 0, p->offset + offset, size, buf) :
				grub_disk_read(file->disk, p->offset + sector, offset, size, buf)) != GRUB_ERR_NONE)
				return -1;

			ret += size;
			len -= size;
			sector -= ((size + offset) >> GRUB_DISK_SECTOR_BITS);
			offset = ((size + offset) & (GRUB_DISK_SECTOR_SIZE - 1));
		}
		else
			sector -= p->length;
	}

	return ret;
}

static grub_ssize_t
grub_fs_blocklist_read(grub_file_t file, char* buf, grub_size_t len)
{
	grub_ssize_t ret;
	file->disk->read_hook = file->read_hook;
	file->disk->read_hook_data = file->read_hook_data;
	ret = grub_fs_blocklist_rw(file, buf, len, 0);
	file->disk->read_hook = 0;
	return ret;
}

struct grub_fs grub_fs_blocklist =
{
  .name = "blocklist",
  .fs_dir = 0,
  .fs_open = grub_fs_blocklist_open,
  .fs_read = grub_fs_blocklist_read,
  .fs_close = 0,
  .next = 0
};

static grub_err_t
grub_fs_winfile_open(grub_file_t file, const char* name)
{
	LARGE_INTEGER li;
	HANDLE fd = INVALID_HANDLE_VALUE;
	grub_dprintf("fs", "winfile %s\n", name);
	fd = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (fd == INVALID_HANDLE_VALUE)
		return grub_error(GRUB_ERR_BAD_FILENAME, "invalid winfile %s", name);
	if (GetFileSizeEx(fd, &li))
		file->size = li.QuadPart;
	else
		file->size = GRUB_FILE_SIZE_UNKNOWN;
	grub_dprintf("fs", "winfile size %llu\n", file->size);
	file->data = fd;
	return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_fs_winfile_read(grub_file_t file, char* buf, grub_size_t len)
{
	grub_ssize_t ret = -1;
	LARGE_INTEGER li;
	HANDLE fd = file->data;
	DWORD dwsize = (DWORD)len;
	li.QuadPart = file->offset;
	li.LowPart = SetFilePointer(fd, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return -1;
	if (ReadFile(fd, buf, dwsize, &dwsize, NULL))
		ret = (grub_ssize_t)dwsize;
	return ret;
}

static grub_err_t
grub_fs_winfile_close(struct grub_file* file)
{
	HANDLE fd = file->data;
	CHECK_CLOSE_HANDLE(fd);
	return GRUB_ERR_NONE;
}

struct grub_fs grub_fs_winfile =
{
  .name = "winfile",
  .fs_dir = 0,
  .fs_open = grub_fs_winfile_open,
  .fs_read = grub_fs_winfile_read,
  .fs_close = grub_fs_winfile_close,
  .next = 0
};

#define BLOCKLIST_INC_STEP	8

struct read_blocklist_ctx
{
	grub_uint64_t num;
	struct grub_fs_block* blocks;
	grub_off_t total_size;
	grub_disk_addr_t part_start;
};

static void
read_blocklist(grub_disk_addr_t sector, unsigned offset,
	unsigned length, void* ctx)
{
	struct read_blocklist_ctx* c = ctx;

	sector = ((sector - c->part_start) << GRUB_DISK_SECTOR_BITS) + offset;

	if ((c->num) &&
		(c->blocks[c->num - 1].offset + c->blocks[c->num - 1].length == sector))
	{
		c->blocks[c->num - 1].length += length;
		goto quit;
	}

	if ((c->num & (BLOCKLIST_INC_STEP - 1)) == 0)
	{
		c->blocks = grub_realloc(c->blocks, (c->num + BLOCKLIST_INC_STEP) * sizeof(struct grub_fs_block));
		if (!c->blocks)
			return;
	}

	c->blocks[c->num].offset = sector;
	c->blocks[c->num].length = length;
	c->num++;

quit:
	c->total_size += length;
}

static grub_uint64_t
blocklist_count_frags(struct grub_fs_block* blocks, grub_off_t file_size)
{
	struct grub_fs_block* p;
	grub_off_t offset = 0;
	grub_uint64_t frags = 0;

	for (p = blocks; p->length && offset < file_size; p++)
	{
		frags++;
		offset += p->length;
	}

	return frags;
}

grub_uint64_t
grub_blocklist_convert(grub_file_t file)
{
	struct read_blocklist_ctx c;
	char buf[4 * GRUB_DISK_SECTOR_SIZE];

	if (file->fs == &grub_fs_blocklist)
		return blocklist_count_frags(file->data, file->size);

	if (!file->disk || !file->size)
		return 0;

	file->offset = 0;

	c.num = 0;
	c.blocks = 0;
	c.total_size = 0;
	c.part_start = grub_partition_get_start(file->disk->partition);
	file->read_hook = read_blocklist;
	file->read_hook_data = &c;
	while (grub_file_read(file, buf, sizeof(buf)) > 0)
		;
	file->read_hook = 0;
	if ((grub_errno) || (c.total_size != file->size))
	{
		grub_errno = 0;
		c.num = 0;
		grub_free(c.blocks);
	}
	else
	{
		if (file->fs->fs_close)
			(file->fs->fs_close) (file);
		file->fs = &grub_fs_blocklist;
		file->data = c.blocks;
	}

	file->offset = 0;
	return c.num;
}

grub_ssize_t
grub_blocklist_write(grub_file_t file, const char* buf, grub_size_t len)
{
	return (file->fs != &grub_fs_blocklist) ? -1 : grub_fs_blocklist_rw(file, (char*)buf, len, 1);
}
