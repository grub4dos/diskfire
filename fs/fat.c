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

#include "compat.h"
#include "disk.h"
#include "fs.h"
#include "file.h"
#include "charset.h"
#include "datetime.h"
#include "fshelp.h"
#include "fat.h"

// fuck you microsoft
#pragma warning(disable:4706)

enum
{
	GRUB_FAT_ATTR_READ_ONLY = 0x01,
	GRUB_FAT_ATTR_HIDDEN = 0x02,
	GRUB_FAT_ATTR_SYSTEM = 0x04,
	GRUB_FAT_ATTR_VOLUME_ID = 0x08,
	GRUB_FAT_ATTR_DIRECTORY = 0x10,
	GRUB_FAT_ATTR_ARCHIVE = 0x20,

	GRUB_FAT_ATTR_LONG_NAME = (GRUB_FAT_ATTR_READ_ONLY
	| GRUB_FAT_ATTR_HIDDEN
		| GRUB_FAT_ATTR_SYSTEM
		| GRUB_FAT_ATTR_VOLUME_ID),

	GRUB_FAT_ATTR_VALID = (GRUB_FAT_ATTR_READ_ONLY
		| GRUB_FAT_ATTR_HIDDEN
		| GRUB_FAT_ATTR_SYSTEM
		| GRUB_FAT_ATTR_DIRECTORY
		| GRUB_FAT_ATTR_ARCHIVE
		| GRUB_FAT_ATTR_VOLUME_ID
		)
};

typedef struct grub_fat_bpb grub_current_fat_bpb_t;

PRAGMA_BEGIN_PACKED
struct grub_fat_dir_entry
{
	grub_uint8_t name[11];
	grub_uint8_t attr;
	grub_uint8_t nt_reserved;
	grub_uint8_t c_time_tenth;
	grub_uint16_t c_time;
	grub_uint16_t c_date;
	grub_uint16_t a_date;
	grub_uint16_t first_cluster_high;
	grub_uint16_t w_time;
	grub_uint16_t w_date;
	grub_uint16_t first_cluster_low;
	grub_uint32_t file_size;
};

struct grub_fat_long_name_entry
{
	grub_uint8_t id;
	grub_uint16_t name1[5];
	grub_uint8_t attr;
	grub_uint8_t reserved;
	grub_uint8_t checksum;
	grub_uint16_t name2[6];
	grub_uint16_t first_cluster;
	grub_uint16_t name3[2];
};
PRAGMA_END_PACKED
typedef struct grub_fat_dir_entry grub_fat_dir_node_t;

struct grub_fat_data
{
	int logical_sector_bits;
	grub_uint32_t num_sectors;

	grub_uint32_t fat_sector;
	grub_uint32_t sectors_per_fat;
	int fat_size;

	grub_uint32_t root_cluster;

	grub_uint32_t root_sector;
	grub_uint32_t num_root_sectors;

	int cluster_bits;
	grub_uint32_t cluster_eof_mark;
	grub_uint32_t cluster_sector;
	grub_uint32_t num_clusters;

	grub_uint32_t uuid;
};

struct grub_fshelp_node
{
	grub_disk_t disk;
	struct grub_fat_data* data;

	grub_uint8_t attr;

	grub_uint32_t file_size;

	grub_uint32_t file_cluster;
	grub_uint32_t cur_cluster_num;
	grub_uint32_t cur_cluster;
};

static int
fat_log2(unsigned x)
{
	int i;

	if (x == 0)
		return -1;

	for (i = 0; (x & 1) == 0; i++)
		x >>= 1;

	if (x != 1)
		return -1;

	return i;
}

struct grub_fat_data*
grub_fat_mount(grub_disk_t disk)
{
	grub_current_fat_bpb_t bpb;
	struct grub_fat_data* data = 0;
	grub_uint32_t first_fat, magic;

	if (!disk)
		goto fail;

	data = (struct grub_fat_data*)grub_malloc(sizeof(*data));
	if (!data)
		goto fail;

	/* Read the BPB.  */
	if (grub_disk_read(disk, 0, 0, sizeof(bpb), &bpb))
		goto fail;

	/* Get the sizes of logical sectors and clusters.  */
	data->logical_sector_bits =
		fat_log2(grub_le_to_cpu16(bpb.bytes_per_sector));

	if (data->logical_sector_bits < GRUB_DISK_SECTOR_BITS
		|| data->logical_sector_bits >= 16)
		goto fail;
	data->logical_sector_bits -= GRUB_DISK_SECTOR_BITS;

	data->cluster_bits = fat_log2(bpb.sectors_per_cluster);

	if (data->cluster_bits < 0 || data->cluster_bits > 25)
		goto fail;
	data->cluster_bits += data->logical_sector_bits;

	/* Get information about FATs.  */
	data->fat_sector = (grub_le_to_cpu16(bpb.num_reserved_sectors)
		<< data->logical_sector_bits);
	if (data->fat_sector == 0)
		goto fail;

	data->sectors_per_fat = ((bpb.sectors_per_fat_16
		? grub_le_to_cpu16(bpb.sectors_per_fat_16)
		: grub_le_to_cpu32(bpb.version_specific.fat32.sectors_per_fat_32))
		<< data->logical_sector_bits);

	if (data->sectors_per_fat == 0)
		goto fail;

	/* Get the number of sectors in this volume.  */
	data->num_sectors = ((bpb.num_total_sectors_16
		? grub_le_to_cpu16(bpb.num_total_sectors_16)
		: grub_le_to_cpu32(bpb.num_total_sectors_32))
		<< data->logical_sector_bits);

	if (data->num_sectors == 0)
		goto fail;

	/* Get information about the root directory.  */
	if (bpb.num_fats == 0)
		goto fail;

	data->root_sector = data->fat_sector + bpb.num_fats * data->sectors_per_fat;
	data->num_root_sectors
		= ((((grub_uint32_t)grub_le_to_cpu16(bpb.num_root_entries)
			* sizeof(struct grub_fat_dir_entry)
			+ grub_le_to_cpu16(bpb.bytes_per_sector) - 1)
			>> (data->logical_sector_bits + GRUB_DISK_SECTOR_BITS))
			<< (data->logical_sector_bits));

	data->cluster_sector = data->root_sector + data->num_root_sectors;
	data->num_clusters = (((data->num_sectors - data->cluster_sector)
		>> data->cluster_bits)
		+ 2);

	if (data->num_clusters <= 2)
		goto fail;

	if (!bpb.sectors_per_fat_16)
	{
		/* FAT32.  */
		grub_uint16_t flags = grub_le_to_cpu16(
			bpb.version_specific.fat32.extended_flags);

		data->root_cluster = grub_le_to_cpu32(bpb.version_specific.fat32.root_cluster);
		data->fat_size = 32;
		data->cluster_eof_mark = 0x0ffffff8;

		if (flags & 0x80)
		{
			/* Get an active FAT.  */
			unsigned active_fat = flags & 0xf;

			if (active_fat > bpb.num_fats)
				goto fail;

			data->fat_sector += active_fat * data->sectors_per_fat;
		}

		if (bpb.num_root_entries != 0 || bpb.version_specific.fat32.fs_version != 0)
			goto fail;
	}
	else
	{
		/* FAT12 or FAT16.  */
		data->root_cluster = ~0U;

		if (data->num_clusters <= 4085 + 2)
		{
			/* FAT12.  */
			data->fat_size = 12;
			data->cluster_eof_mark = 0x0ff8;
		}
		else
		{
			/* FAT16.  */
			data->fat_size = 16;
			data->cluster_eof_mark = 0xfff8;
		}
	}

	/* More sanity checks.  */
	if (data->num_sectors <= data->fat_sector)
		goto fail;

	if (grub_disk_read(disk,
		data->fat_sector,
		0,
		sizeof(first_fat),
		&first_fat))
		goto fail;

	first_fat = grub_le_to_cpu32(first_fat);

	if (data->fat_size == 32)
	{
		first_fat &= 0x0fffffff;
		magic = 0x0fffff00;
	}
	else if (data->fat_size == 16)
	{
		first_fat &= 0x0000ffff;
		magic = 0xff00;
	}
	else
	{
		first_fat &= 0x00000fff;
		magic = 0x0f00;
	}

	/* Serial number.  */
	if (bpb.sectors_per_fat_16)
		data->uuid = grub_le_to_cpu32(bpb.version_specific.fat12_or_fat16.num_serial);
	else
		data->uuid = grub_le_to_cpu32(bpb.version_specific.fat32.num_serial);

	/* Ignore the 3rd bit, because some BIOSes assigns 0xF0 to the media
	   descriptor, even if it is a so-called superfloppy (e.g. an USB key).
	   The check may be too strict for this kind of stupid BIOSes, as
	   they overwrite the media descriptor.  */
	if ((first_fat | 0x8) != (magic | bpb.media | 0x8))
		goto fail;

	return data;

fail:

	grub_free(data);
	grub_error(GRUB_ERR_BAD_FS, "not a FAT filesystem");
	return 0;
}

static grub_ssize_t
grub_fat_read_data(grub_disk_t disk, grub_fshelp_node_t node,
	grub_disk_read_hook_t read_hook, void* read_hook_data,
	grub_off_t offset, grub_size_t len, char* buf)
{
	grub_size_t size;
	grub_uint32_t logical_cluster;
	unsigned logical_cluster_bits;
	grub_ssize_t ret = 0;
	unsigned long sector;

	/* This is a special case. FAT12 and FAT16 doesn't have the root directory
	   in clusters.  */
	if (node->file_cluster == ~0U)
	{
		size = (((grub_uint64_t)node->data->num_root_sectors) << GRUB_DISK_SECTOR_BITS) - offset;
		if (size > len)
			size = len;

		if (grub_disk_read(disk, node->data->root_sector, offset, size, buf))
			return -1;

		return size;
	}

	/* Calculate the logical cluster number and offset.  */
	logical_cluster_bits = (node->data->cluster_bits
		+ GRUB_DISK_SECTOR_BITS);
	logical_cluster = (grub_uint32_t)(offset >> logical_cluster_bits);
	offset &= (1ULL << logical_cluster_bits) - 1;

	if (logical_cluster < node->cur_cluster_num)
	{
		node->cur_cluster_num = 0;
		node->cur_cluster = node->file_cluster;
	}

	while (len)
	{
		while (logical_cluster > node->cur_cluster_num)
		{
			/* Find next cluster.  */
			grub_uint32_t next_cluster;
			grub_uint32_t fat_offset;

			switch (node->data->fat_size)
			{
			case 32:
				fat_offset = node->cur_cluster << 2;
				break;
			case 16:
				fat_offset = node->cur_cluster << 1;
				break;
			default:
				/* case 12: */
				fat_offset = node->cur_cluster + (node->cur_cluster >> 1);
				break;
			}

			/* Read the FAT.  */
			if (grub_disk_read(disk, node->data->fat_sector, fat_offset,
				(7ULL + node->data->fat_size) >> 3,
				(char*)&next_cluster))
				return -1;

			next_cluster = grub_le_to_cpu32(next_cluster);
			switch (node->data->fat_size)
			{
			case 16:
				next_cluster &= 0xFFFF;
				break;
			case 12:
				if (node->cur_cluster & 1)
					next_cluster >>= 4;

				next_cluster &= 0x0FFF;
				break;
			}

			grub_dprintf("fat", "fat_size=%d, next_cluster=%u\n",
				node->data->fat_size, next_cluster);

			/* Check the end.  */
			if (next_cluster >= node->data->cluster_eof_mark)
				return ret;

			if (next_cluster < 2 || next_cluster >= node->data->num_clusters)
			{
				grub_error(GRUB_ERR_BAD_FS, "invalid cluster %u",
					next_cluster);
				return -1;
			}

			node->cur_cluster = next_cluster;
			node->cur_cluster_num++;
		}

		/* Read the data here.  */
		sector = (node->data->cluster_sector
			+ ((node->cur_cluster - 2)
				<< node->data->cluster_bits));
		size = (1ULL << logical_cluster_bits) - offset;
		if (size > len)
			size = len;

		disk->read_hook = read_hook;
		disk->read_hook_data = read_hook_data;
		grub_disk_read(disk, sector, offset, size, buf);
		disk->read_hook = 0;
		if (grub_errno)
			return -1;

		len -= size;
		buf += size;
		ret += size;
		logical_cluster++;
		offset = 0;
	}

	return ret;
}

struct grub_fat_iterate_context
{
	struct grub_fat_dir_entry dir;
	char* filename;
	grub_uint16_t* unibuf;
	grub_ssize_t offset;
};

static grub_err_t
grub_fat_iterate_init(struct grub_fat_iterate_context* ctxt)
{
	ctxt->offset = -(grub_ssize_t)sizeof(struct grub_fat_dir_entry);

	/* Allocate space enough to hold a long name.  */
	ctxt->filename = grub_malloc(0x40 * 13 * GRUB_MAX_UTF8_PER_UTF16 + 1);
	ctxt->unibuf = (grub_uint16_t*)grub_malloc(0x40 * 13 * 2);

	if (!ctxt->filename || !ctxt->unibuf)
	{
		grub_free(ctxt->filename);
		grub_free(ctxt->unibuf);
		return grub_errno;
	}
	return GRUB_ERR_NONE;
}

static void
grub_fat_iterate_fini(struct grub_fat_iterate_context* ctxt)
{
	grub_free(ctxt->filename);
	grub_free(ctxt->unibuf);
}

static grub_err_t
grub_fat_iterate_dir_next(grub_fshelp_node_t node,
	struct grub_fat_iterate_context* ctxt)
{
	char* filep = 0;
	int checksum = -1;
	int slot = -1, slots = -1;

	while (1)
	{
		unsigned i;

		/* Adjust the offset.  */
		ctxt->offset += sizeof(ctxt->dir);

		/* Read a directory entry.  */
		if (grub_fat_read_data(node->disk, node, 0, 0,
			ctxt->offset, sizeof(ctxt->dir),
			(char*)&ctxt->dir)
			!= sizeof(ctxt->dir) || ctxt->dir.name[0] == 0)
			break;

		/* Handle long name entries.  */
		if (ctxt->dir.attr == GRUB_FAT_ATTR_LONG_NAME)
		{
			struct grub_fat_long_name_entry* long_name
				= (struct grub_fat_long_name_entry*)&ctxt->dir;
			grub_uint8_t id = long_name->id;

			if (id & 0x40)
			{
				id &= 0x3f;
				slots = slot = id;
				checksum = long_name->checksum;
			}

			if (id != slot || slot == 0 || checksum != long_name->checksum)
			{
				checksum = -1;
				continue;
			}

			slot--;
			grub_memcpy(ctxt->unibuf + 13LL * slot, long_name->name1, 5 * 2);
			grub_memcpy(ctxt->unibuf + 13LL * slot + 5, long_name->name2, 6 * 2);
			grub_memcpy(ctxt->unibuf + 13LL * slot + 11, long_name->name3, 2 * 2);
			continue;
		}

		/* Check if this entry is valid.  */
		if (ctxt->dir.name[0] == 0xe5 || (ctxt->dir.attr & ~GRUB_FAT_ATTR_VALID))
			continue;

		/* This is a workaround for Japanese.  */
		if (ctxt->dir.name[0] == 0x05)
			ctxt->dir.name[0] = 0xe5;

		if (checksum != -1 && slot == 0)
		{
			grub_uint8_t sum;

			for (sum = 0, i = 0; i < sizeof(ctxt->dir.name); i++)
				sum = ((sum >> 1) | (sum << 7)) + ctxt->dir.name[i];

			if (sum == checksum)
			{
				int u;

				for (u = 0; u < slots * 13; u++)
					ctxt->unibuf[u] = grub_le_to_cpu16(ctxt->unibuf[u]);

				*grub_utf16_to_utf8((grub_uint8_t*)ctxt->filename,
					ctxt->unibuf,
					13LL * slots) = '\0';

				return GRUB_ERR_NONE;
			}

			checksum = -1;
		}

		/* Convert the 8.3 file name.  */
		filep = ctxt->filename;
		if (ctxt->dir.attr & GRUB_FAT_ATTR_VOLUME_ID)
		{
			for (i = 0; i < sizeof(ctxt->dir.name) && ctxt->dir.name[i]; i++)
				*filep++ = ctxt->dir.name[i];
			while (i > 0 && ctxt->dir.name[i - 1] == ' ')
			{
				filep--;
				i--;
			}
		}
		else
		{
			for (i = 0; i < 8 && ctxt->dir.name[i]; i++)
				*filep++ = (char)grub_tolower(ctxt->dir.name[i]);
			while (i > 0 && ctxt->dir.name[i - 1] == ' ')
			{
				filep--;
				i--;
			}

			/* XXX should we check that dir position is 0 or 1? */
			if (i > 2 || filep[0] != '.' || (i == 2 && filep[1] != '.'))
				*filep++ = '.';

			for (i = 8; i < 11 && ctxt->dir.name[i]; i++)
				*filep++ = (char)grub_tolower(ctxt->dir.name[i]);
			while (i > 8 && ctxt->dir.name[i - 1] == ' ')
			{
				filep--;
				i--;
			}

			if (i == 8)
				filep--;
		}
		*filep = '\0';
		return GRUB_ERR_NONE;
	}

	return grub_errno ? grub_errno : GRUB_ERR_EOF;
}

/*
 * Convert a date and time in FAT format to seconds since the UNIX epoch
 * according to sections 11.3.5 and 11.3.6 in ECMA-107.
 * https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-107.pdf
 */
static int
grub_fat_timestamp(grub_uint16_t time, grub_uint16_t date, grub_int64_t* nix)
{
	struct grub_datetime datetime =
	{
		.year = (grub_uint16_t)((date >> 9) + 1980),
		.month = (grub_uint8_t)((date & 0x01E0) >> 5),
		.day = (grub_uint8_t)(date & 0x001F),
		.hour = (grub_uint8_t)(time >> 11),
		.minute = (grub_uint8_t)((time & 0x07E0) >> 5),
		.second = (grub_uint8_t)((time & 0x001F) * 2),
	};

	/* The conversion below allows seconds=60, so don't trust its validation. */
	if ((time & 0x1F) > 29)
		return 0;

	return grub_datetime2unixtime(&datetime, nix);
}

static grub_err_t lookup_file(grub_fshelp_node_t node,
	const char* name,
	grub_fshelp_node_t* foundnode,
	enum grub_fshelp_filetype* foundtype)
{
	grub_err_t err;
	struct grub_fat_iterate_context ctxt;

	err = grub_fat_iterate_init(&ctxt);
	if (err)
		return err;

	while (!(err = grub_fat_iterate_dir_next(node, &ctxt)))
	{
		if (ctxt.dir.attr & GRUB_FAT_ATTR_VOLUME_ID)
			continue;

		if (grub_strcasecmp(name, ctxt.filename) == 0)
		{
			*foundnode = grub_malloc(sizeof(struct grub_fshelp_node));
			if (!*foundnode)
				return grub_errno;
			(*foundnode)->attr = (grub_uint8_t)ctxt.dir.attr;

			(*foundnode)->file_size = grub_le_to_cpu32(ctxt.dir.file_size);
			(*foundnode)->file_cluster = ((grub_le_to_cpu16(ctxt.dir.first_cluster_high) <<
				16)
				| grub_le_to_cpu16(ctxt.dir.first_cluster_low));
			/* If directory points to root, starting cluster is 0 */
			if (!(*foundnode)->file_cluster)
				(*foundnode)->file_cluster = node->data->root_cluster;

			(*foundnode)->cur_cluster_num = ~0U;
			(*foundnode)->data = node->data;
			(*foundnode)->disk = node->disk;

			*foundtype = ((*foundnode)->attr & GRUB_FAT_ATTR_DIRECTORY) ? GRUB_FSHELP_DIR :
				GRUB_FSHELP_REG;

			grub_fat_iterate_fini(&ctxt);
			return GRUB_ERR_NONE;
		}
	}

	grub_fat_iterate_fini(&ctxt);
	if (err == GRUB_ERR_EOF)
		err = 0;

	return err;

}

static grub_err_t
grub_fat_dir(grub_disk_t disk, const char* path, grub_fs_dir_hook_t hook,
	void* hook_data)
{
	struct grub_fat_data* data = 0;
	grub_fshelp_node_t found = NULL;
	grub_err_t err;
	struct grub_fat_iterate_context ctxt;

	data = grub_fat_mount(disk);
	if (!data)
		goto fail;

	struct grub_fshelp_node root =
	{
	  .data = data,
	  .disk = disk,
	  .attr = GRUB_FAT_ATTR_DIRECTORY,
	  .file_size = 0,
	  .file_cluster = data->root_cluster,
	  .cur_cluster_num = ~0U,
	  .cur_cluster = 0,
	};

	err = grub_fshelp_find_file_lookup(path, &root, &found, lookup_file, NULL,
		GRUB_FSHELP_DIR);
	if (err)
		goto fail;

	err = grub_fat_iterate_init(&ctxt);
	if (err)
		goto fail;

	while (!(err = grub_fat_iterate_dir_next(found, &ctxt)))
	{
		struct grub_dirhook_info info;
		grub_memset(&info, 0, sizeof(info));

		info.dir = !!(ctxt.dir.attr & GRUB_FAT_ATTR_DIRECTORY);
		info.case_insensitive = 1;

		if (ctxt.dir.attr & GRUB_FAT_ATTR_VOLUME_ID)
			continue;
		info.mtimeset = grub_fat_timestamp(grub_le_to_cpu16(ctxt.dir.w_time),
			grub_le_to_cpu16(ctxt.dir.w_date),
			&info.mtime);

		if (info.mtimeset == 0)
			grub_error(GRUB_ERR_OUT_OF_RANGE,
				"invalid modification timestamp for %s", path);

		if (hook(ctxt.filename, &info, hook_data))
			break;
	}
	grub_fat_iterate_fini(&ctxt);
	if (err == GRUB_ERR_EOF)
		err = 0;

fail:
	if (found != &root)
		grub_free(found);

	grub_free(data);

	return grub_errno;
}

static grub_err_t
grub_fat_open(grub_file_t file, const char* name)
{
	struct grub_fat_data* data = 0;
	grub_fshelp_node_t found = NULL;
	grub_err_t err;
	grub_disk_t disk = file->disk;

	data = grub_fat_mount(disk);
	if (!data)
		goto fail;

	struct grub_fshelp_node root =
	{
	  .data = data,
	  .disk = disk,
	  .attr = GRUB_FAT_ATTR_DIRECTORY,
	  .file_size = 0,
	  .file_cluster = data->root_cluster,
	  .cur_cluster_num = ~0U,
	  .cur_cluster = 0,
	};

	err = grub_fshelp_find_file_lookup(name, &root, &found, lookup_file, NULL,
		GRUB_FSHELP_REG);
	if (err)
		goto fail;

	file->data = found;
	file->size = found->file_size;

	return GRUB_ERR_NONE;

fail:

	if (found != &root)
		grub_free(found);

	grub_free(data);

	return grub_errno;
}

static grub_ssize_t
grub_fat_read(grub_file_t file, char* buf, grub_size_t len)
{
	return grub_fat_read_data(file->disk, file->data,
		file->read_hook, file->read_hook_data,
		file->offset, len, buf);
}

static grub_err_t
grub_fat_close(grub_file_t file)
{
	grub_fshelp_node_t node = file->data;

	grub_free(node->data);
	grub_free(node);

	return grub_errno;
}

static grub_err_t
grub_fat_label(grub_disk_t disk, char** label)
{
	grub_err_t err;
	struct grub_fat_iterate_context ctxt;
	struct grub_fshelp_node root =
	{
	  .disk = disk,
	  .attr = GRUB_FAT_ATTR_DIRECTORY,
	  .file_size = 0,
	  .cur_cluster_num = ~0U,
	  .cur_cluster = 0,
	};

	*label = 0;

	root.data = grub_fat_mount(disk);
	if (!root.data)
		goto fail;

	root.file_cluster = root.data->root_cluster;

	err = grub_fat_iterate_init(&ctxt);
	if (err)
		goto fail;

	while (!(err = grub_fat_iterate_dir_next(&root, &ctxt)))
		if ((ctxt.dir.attr & ~GRUB_FAT_ATTR_ARCHIVE) == GRUB_FAT_ATTR_VOLUME_ID)
		{
			*label = grub_strdup(ctxt.filename);
			break;
		}

	grub_fat_iterate_fini(&ctxt);

fail:

	grub_free(root.data);

	return grub_errno;
}

static grub_err_t
grub_fat_uuid(grub_disk_t disk, char** uuid)
{
	struct grub_fat_data* data;

	data = grub_fat_mount(disk);
	if (data)
	{
		*uuid = grub_malloc(sizeof("1234-5678"));
		if (*uuid)
			grub_snprintf(*uuid, sizeof("1234-5678"), "%04X-%04X", (grub_uint16_t)(data->uuid >> 16), (grub_uint16_t)data->uuid);
	}
	else
		*uuid = NULL;

	grub_free(data);

	return grub_errno;
}

struct grub_fs grub_fat_fs =
{
  .name = "fat",
  .fs_dir = grub_fat_dir,
  .fs_open = grub_fat_open,
  .fs_read = grub_fat_read,
  .fs_close = grub_fat_close,
  .fs_label = grub_fat_label,
  .fs_uuid = grub_fat_uuid,
  .next = 0
};
