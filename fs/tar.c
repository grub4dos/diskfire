// SPDX-License-Identifier: GPL-3.0-or-later

#include "compat.h"
#include "disk.h"
#include "fs.h"
#include "file.h"
#include "archelp.h"

/* tar support */
#define MAGIC	"ustar"
PRAGMA_BEGIN_PACKED
struct head
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};
PRAGMA_END_PACKED

static inline unsigned long long
read_number(const char* str, grub_size_t size)
{
	unsigned long long ret = 0;
	while (size-- && *str >= '0' && *str <= '7')
		ret = (ret << 3) | (*str++ & 0xf);
	return ret;
}

struct grub_archelp_data
{
	grub_disk_t disk;
	grub_off_t hofs, next_hofs;
	grub_off_t dofs;
	grub_off_t size;
	char* linkname;
	grub_size_t linkname_alloc;
};

static grub_err_t
grub_tar_find_file(struct grub_archelp_data* data, char** name,
	grub_int32_t* mtime,
	grub_uint32_t* mode)
{
	struct head hd;
	int reread = 0, have_longname = 0, have_longlink = 0;

	data->hofs = data->next_hofs;

	for (reread = 0; reread < 3; reread++)
	{
		if (grub_disk_read(data->disk, 0, data->hofs, sizeof(hd), &hd))
			return grub_errno;

		if (!hd.name[0] && !hd.prefix[0])
		{
			*mode = (grub_uint32_t)GRUB_ARCHELP_ATTR_END;
			return GRUB_ERR_NONE;
		}

		if (grub_memcmp(hd.magic, MAGIC, sizeof(MAGIC) - 1))
			return grub_error(GRUB_ERR_BAD_FS, "invalid tar archive");

		if (hd.typeflag == 'L')
		{
			grub_err_t err;
			grub_size_t namesize = read_number(hd.size, sizeof(hd.size));
			*name = grub_malloc(namesize + 1);
			if (*name == NULL)
				return grub_errno;
			err = grub_disk_read(data->disk, 0,
				data->hofs + GRUB_DISK_SECTOR_SIZE, namesize,
				*name);
			(*name)[namesize] = 0;
			if (err)
				return err;
			data->hofs += GRUB_DISK_SECTOR_SIZE
				+ ((namesize + GRUB_DISK_SECTOR_SIZE - 1) &
					~(GRUB_DISK_SECTOR_SIZE - 1));
			have_longname = 1;
			continue;
		}

		if (hd.typeflag == 'K')
		{
			grub_err_t err;
			grub_size_t linksize = read_number(hd.size, sizeof(hd.size));
			if (data->linkname_alloc < linksize + 1)
			{
				char* n;
				n = grub_calloc(2, linksize + 1);
				if (!n)
					return grub_errno;
				grub_free(data->linkname);
				data->linkname = n;
				data->linkname_alloc = 2 * (linksize + 1);
			}

			err = grub_disk_read(data->disk, 0,
				data->hofs + GRUB_DISK_SECTOR_SIZE, linksize,
				data->linkname);
			if (err)
				return err;
			data->linkname[linksize] = 0;
			data->hofs += GRUB_DISK_SECTOR_SIZE
				+ ((linksize + GRUB_DISK_SECTOR_SIZE - 1) &
					~(GRUB_DISK_SECTOR_SIZE - 1));
			have_longlink = 1;
			continue;
		}

		if (!have_longname)
		{
			grub_size_t extra_size = 0;

			while (extra_size < sizeof(hd.prefix)
				&& hd.prefix[extra_size])
				extra_size++;
			*name = grub_malloc(sizeof(hd.name) + extra_size + 2);
			if (*name == NULL)
				return grub_errno;
			if (hd.prefix[0])
			{
				grub_memcpy(*name, hd.prefix, extra_size);
				(*name)[extra_size++] = '/';
			}
			grub_memcpy(*name + extra_size, hd.name, sizeof(hd.name));
			(*name)[extra_size + sizeof(hd.name)] = 0;
		}

		data->size = read_number(hd.size, sizeof(hd.size));
		data->dofs = data->hofs + GRUB_DISK_SECTOR_SIZE;
		data->next_hofs = data->dofs + ((data->size + GRUB_DISK_SECTOR_SIZE - 1) &
			~(GRUB_DISK_SECTOR_SIZE - 1));
		if (mtime)
			*mtime = (grub_int32_t)read_number(hd.mtime, sizeof(hd.mtime));
		if (mode)
		{
			*mode = (grub_uint32_t)read_number(hd.mode, sizeof(hd.mode));
			switch (hd.typeflag)
			{
				/* Hardlink.  */
			case '1':
				/* Symlink.  */
			case '2':
				*mode |= GRUB_ARCHELP_ATTR_LNK;
				break;
			case '0':
				*mode |= GRUB_ARCHELP_ATTR_FILE;
				break;
			case '5':
				*mode |= GRUB_ARCHELP_ATTR_DIR;
				break;
			}
		}
		if (!have_longlink)
		{
			if (data->linkname_alloc < 101)
			{
				char* n;
				n = grub_malloc(101);
				if (!n)
					return grub_errno;
				grub_free(data->linkname);
				data->linkname = n;
				data->linkname_alloc = 101;
			}
			grub_memcpy(data->linkname, hd.linkname, sizeof(hd.linkname));
			data->linkname[100] = 0;
		}
		return GRUB_ERR_NONE;
	}
	return GRUB_ERR_NONE;
}

static char*
grub_tar_get_link_target(struct grub_archelp_data* data)
{
	return grub_strdup(data->linkname);
}

static void
grub_tar_rewind(struct grub_archelp_data* data)
{
	data->next_hofs = 0;
}

#pragma warning(push)
#pragma warning(disable:4028)
static struct grub_archelp_ops arcops =
{
  .find_file = grub_tar_find_file,
  .get_link_target = grub_tar_get_link_target,
  .rewind = grub_tar_rewind
};
#pragma warning(pop)

static struct grub_archelp_data*
grub_tar_mount(grub_disk_t disk)
{
	struct head hd;
	struct grub_archelp_data* data;

	if (grub_disk_read(disk, 0, 0, sizeof(hd), &hd))
		goto fail;

	if (grub_memcmp(hd.magic, MAGIC, sizeof(MAGIC) - 1))
		goto fail;

	data = (struct grub_archelp_data*)grub_zalloc(sizeof(*data));
	if (!data)
		goto fail;

	data->disk = disk;

	return data;

fail:
	grub_error(GRUB_ERR_BAD_FS, "not a tarfs filesystem");
	return 0;
}

static grub_err_t
grub_tar_dir(grub_disk_t disk, const char* path_in,
	grub_fs_dir_hook_t hook, void* hook_data)
{
	struct grub_archelp_data* data;
	grub_err_t err;

	data = grub_tar_mount(disk);
	if (!data)
		return grub_errno;

	err = grub_archelp_dir(data, &arcops,
		path_in, hook, hook_data);

	grub_free(data->linkname);
	grub_free(data);

	return err;
}

static grub_err_t
grub_tar_open(grub_file_t file, const char* name_in)
{
	struct grub_archelp_data* data;
	grub_err_t err;

	data = grub_tar_mount(file->disk);
	if (!data)
		return grub_errno;

	err = grub_archelp_open(data, &arcops, name_in);
	if (err)
	{
		grub_free(data->linkname);
		grub_free(data);
	}
	else
	{
		file->data = data;
		file->size = data->size;
	}
	return err;
}

static grub_ssize_t
grub_tar_read(grub_file_t file, char* buf, grub_size_t len)
{
	struct grub_archelp_data* data;
	grub_ssize_t ret;

	data = file->data;

	data->disk->read_hook = file->read_hook;
	data->disk->read_hook_data = file->read_hook_data;
	ret = (grub_disk_read(data->disk, 0, data->dofs + file->offset,
		len, buf)) ? -1 : (grub_ssize_t)len;
	data->disk->read_hook = 0;

	return ret;
}

static grub_err_t
grub_tar_close(grub_file_t file)
{
	struct grub_archelp_data* data;

	data = file->data;
	grub_free(data->linkname);
	grub_free(data);

	return grub_errno;
}

struct grub_fs grub_tar_fs =
{
	.name = "tarfs",
	.fs_dir = grub_tar_dir,
	.fs_open = grub_tar_open,
	.fs_read = grub_tar_read,
	.fs_close = grub_tar_close,
};
