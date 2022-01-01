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

#include "exfat.h"

 // fuck you microsoft
#pragma warning(disable:4706)

enum
{
	GRUB_FAT_ATTR_READ_ONLY = 0x01,
	GRUB_FAT_ATTR_HIDDEN = 0x02,
	GRUB_FAT_ATTR_SYSTEM = 0x04,

	GRUB_FAT_ATTR_DIRECTORY = 0x10,
	GRUB_FAT_ATTR_ARCHIVE = 0x20,

	GRUB_FAT_ATTR_VALID = (GRUB_FAT_ATTR_READ_ONLY
		| GRUB_FAT_ATTR_HIDDEN
		| GRUB_FAT_ATTR_SYSTEM
		| GRUB_FAT_ATTR_DIRECTORY
		| GRUB_FAT_ATTR_ARCHIVE
		)
};

typedef struct grub_exfat_bpb grub_current_fat_bpb_t;

enum
{
	FLAG_CONTIGUOUS = 2
};
PRAGMA_BEGIN_PACKED
struct grub_fat_dir_entry
{
	grub_uint8_t entry_type;
	union
	{
		grub_uint8_t placeholder[31];
		struct
		{
			grub_uint8_t secondary_count;
			grub_uint16_t checksum;
			grub_uint16_t attr;
			grub_uint16_t reserved1;
			grub_uint32_t c_time;
			grub_uint32_t m_time;
			grub_uint32_t a_time;
			grub_uint8_t c_time_tenth;
			grub_uint8_t m_time_tenth;
			grub_uint8_t a_time_tenth;
			grub_uint8_t reserved2[9];
		} file;
		struct
		{
			grub_uint8_t flags;
			grub_uint8_t reserved1;
			grub_uint8_t name_length;
			grub_uint16_t name_hash;
			grub_uint16_t reserved2;
			grub_uint64_t valid_size;
			grub_uint32_t reserved3;
			grub_uint32_t first_cluster;
			grub_uint64_t file_size;
		} stream_extension;
		struct
		{
			grub_uint8_t flags;
			grub_uint16_t str[15];
		} file_name;
		struct
		{
			grub_uint8_t character_count;
			grub_uint16_t str[15];
		} volume_label;
	} type_specific;
};
PRAGMA_END_PACKED
struct grub_fat_dir_node
{
	grub_uint32_t attr;
	grub_uint32_t first_cluster;
	grub_uint64_t file_size;
	grub_uint64_t valid_size;
	int have_stream;
	int is_contiguous;
};

typedef struct grub_fat_dir_node grub_fat_dir_node_t;

struct grub_fat_data
{
	int logical_sector_bits;
	grub_uint32_t num_sectors;

	grub_uint32_t fat_sector;
	grub_uint32_t sectors_per_fat;
	int fat_size;

	grub_uint32_t root_cluster;

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

	grub_uint64_t file_size;

	grub_uint32_t file_cluster;
	grub_uint32_t cur_cluster_num;
	grub_uint32_t cur_cluster;

	int is_contiguous;
};

static struct grub_fat_data*
grub_exfat_mount(grub_disk_t disk)
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

	if (grub_memcmp((const char*)bpb.oem_name, "EXFAT   ",
		sizeof(bpb.oem_name)) != 0)
		goto fail;

	/* Get the sizes of logical sectors and clusters.  */
	data->logical_sector_bits = bpb.bytes_per_sector_shift;

	if (data->logical_sector_bits < GRUB_DISK_SECTOR_BITS
		|| data->logical_sector_bits >= 16)
		goto fail;
	data->logical_sector_bits -= GRUB_DISK_SECTOR_BITS;

	data->cluster_bits = bpb.sectors_per_cluster_shift;

	if (data->cluster_bits < 0 || data->cluster_bits > 25)
		goto fail;
	data->cluster_bits += data->logical_sector_bits;

	/* Get information about FATs.  */
	data->fat_sector = (grub_le_to_cpu32(bpb.num_reserved_sectors)
		<< data->logical_sector_bits);

	if (data->fat_sector == 0)
		goto fail;

	data->sectors_per_fat = (grub_le_to_cpu32(bpb.sectors_per_fat)
		<< data->logical_sector_bits);

	if (data->sectors_per_fat == 0)
		goto fail;

	/* Get the number of sectors in this volume.  */
	data->num_sectors = (grub_uint32_t)((grub_le_to_cpu64(bpb.num_total_sectors))
		<< data->logical_sector_bits);

	if (data->num_sectors == 0)
		goto fail;

	/* Get information about the root directory.  */
	if (bpb.num_fats == 0)
		goto fail;

	data->cluster_sector = (grub_le_to_cpu32(bpb.cluster_offset)
		<< data->logical_sector_bits);
	data->num_clusters = (grub_le_to_cpu32(bpb.cluster_count)
		<< data->logical_sector_bits);

	if (data->num_clusters <= 2)
		goto fail;

	/* exFAT.  */
	data->root_cluster = grub_le_to_cpu32(bpb.root_cluster);
	data->fat_size = 32;
	data->cluster_eof_mark = 0xffffffff;

	if ((bpb.volume_flags & grub_cpu_to_le16_compile_time(0x1))
		&& bpb.num_fats > 1)
		data->fat_sector += data->sectors_per_fat;

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
	data->uuid = grub_le_to_cpu32(bpb.num_serial);

	(void)magic;

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

	if (node->is_contiguous)
	{
		/* Read the data here.  */
		sector = (node->data->cluster_sector
			+ ((node->file_cluster - 2)
				<< node->data->cluster_bits));

		disk->read_hook = read_hook;
		disk->read_hook_data = read_hook_data;
		grub_disk_read(disk, sector + (offset >> GRUB_DISK_SECTOR_BITS),
			offset & (GRUB_DISK_SECTOR_SIZE - 1), len, buf);
		disk->read_hook = 0;
		if (grub_errno)
			return -1;

		return len;
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
	struct grub_fat_dir_node dir;
	struct grub_fat_dir_entry entry;

	char* filename;
	grub_uint16_t* unibuf;
	grub_ssize_t offset;
};

static grub_err_t
grub_fat_iterate_init(struct grub_fat_iterate_context* ctxt)
{
	ctxt->offset = -(grub_ssize_t)sizeof(struct grub_fat_dir_entry);

	ctxt->unibuf = grub_malloc(15 * 256 * 2);
	ctxt->filename = grub_malloc(15 * 256 * GRUB_MAX_UTF8_PER_UTF16 + 1);

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
	grub_memset(&ctxt->dir, 0, sizeof(ctxt->dir));
	while (1)
	{
		struct grub_fat_dir_entry* dir = &ctxt->entry;

		ctxt->offset += sizeof(*dir);

		if (grub_fat_read_data(node->disk, node, 0, 0, ctxt->offset, sizeof(*dir),
			(char*)dir)
			!= sizeof(*dir))
			break;

		if (dir->entry_type == 0)
			break;
		if (!(dir->entry_type & 0x80))
			continue;

		if (dir->entry_type == 0x85)
		{
			unsigned i, nsec, slots = 0;

			nsec = dir->type_specific.file.secondary_count;

			ctxt->dir.attr = grub_cpu_to_le16(dir->type_specific.file.attr);
			ctxt->dir.have_stream = 0;
			for (i = 0; i < nsec; i++)
			{
				struct grub_fat_dir_entry sec;
				ctxt->offset += sizeof(sec);
				if (grub_fat_read_data(node->disk, node, 0, 0,
					ctxt->offset, sizeof(sec), (char*)&sec)
					!= sizeof(sec))
					break;
				if (!(sec.entry_type & 0x80))
					continue;
				if (!(sec.entry_type & 0x40))
					break;
				switch (sec.entry_type)
				{
				case 0xc0:
					ctxt->dir.first_cluster = grub_cpu_to_le32(
						sec.type_specific.stream_extension.first_cluster);
					ctxt->dir.valid_size
						= grub_cpu_to_le64(sec.type_specific.stream_extension.valid_size);
					ctxt->dir.file_size
						= grub_cpu_to_le64(sec.type_specific.stream_extension.file_size);
					ctxt->dir.have_stream = 1;
					ctxt->dir.is_contiguous = !!(sec.type_specific.stream_extension.flags
						& grub_cpu_to_le16_compile_time(FLAG_CONTIGUOUS));
					break;
				case 0xc1:
				{
					int j;
					for (j = 0; j < 15; j++)
						ctxt->unibuf[slots * 15 + j]
						= grub_le_to_cpu16(sec.type_specific.file_name.str[j]);
					slots++;
				}
				break;
				default:
					grub_dprintf("exfat", "unknown secondary type 0x%02x\n",
						sec.entry_type);
				}
			}

			if (i != nsec)
			{
				ctxt->offset -= sizeof(*dir);
				continue;
			}

			*grub_utf16_to_utf8((grub_uint8_t*)ctxt->filename, ctxt->unibuf,
				15ULL * slots) = '\0';

			return 0;
		}
		/* Allocation bitmap. */
		if (dir->entry_type == 0x81)
			continue;
		/* Upcase table. */
		if (dir->entry_type == 0x82)
			continue;
		/* Volume label. */
		if (dir->entry_type == 0x83)
			continue;
		grub_dprintf("exfat", "unknown primary type 0x%02x\n",
			dir->entry_type);
	}
	return grub_errno ? grub_errno : GRUB_ERR_EOF;
}

/*
 * Convert a timestamp in exFAT format to seconds since the UNIX epoch
 * according to sections 7.4.8 and 7.4.9 in the exFAT specification.
 * https://docs.microsoft.com/en-us/windows/win32/fileio/exfat-specification
 */
static int
grub_exfat_timestamp(grub_uint32_t field, grub_uint8_t msec, grub_int64_t* nix)
{
	struct grub_datetime datetime =
	{
		.year = (grub_uint16_t)((field >> 25) + 1980),
		.month = (grub_uint8_t)((field & 0x01E00000) >> 21),
		.day = (grub_uint8_t)((field & 0x001F0000) >> 16),
		.hour = (grub_uint8_t)((field & 0x0000F800) >> 11),
		.minute = (grub_uint8_t)((field & 0x000007E0) >> 5),
		.second = (grub_uint8_t)((field & 0x0000001F) * 2 + (msec >= 100 ? 1 : 0)),
	};

	/* The conversion below allows seconds=60, so don't trust its validation. */
	if ((field & 0x1F) > 29)
		return 0;

	/* Validate the 10-msec field even though it is rounded down to seconds. */
	if (msec > 199)
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
		if (!ctxt.dir.have_stream)
			continue;

		if (grub_strcasecmp(name, ctxt.filename) == 0)
		{
			*foundnode = grub_malloc(sizeof(struct grub_fshelp_node));
			if (!*foundnode)
				return grub_errno;
			(*foundnode)->attr = (grub_uint8_t)ctxt.dir.attr;

			(*foundnode)->file_size = ctxt.dir.file_size;
			(*foundnode)->file_cluster = ctxt.dir.first_cluster;
			(*foundnode)->is_contiguous = ctxt.dir.is_contiguous;

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

	data = grub_exfat_mount(disk);
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
	  .is_contiguous = 0,
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

		if (!ctxt.dir.have_stream)
			continue;
		info.mtimeset = grub_exfat_timestamp(grub_le_to_cpu32(
			ctxt.entry.type_specific.file.m_time),
			ctxt.entry.type_specific.file.m_time_tenth,
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

	data = grub_exfat_mount(disk);
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
	  .is_contiguous = 0,
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
	struct grub_fat_dir_entry dir;
	grub_ssize_t offset = -(grub_ssize_t)sizeof(dir);
	struct grub_fshelp_node root =
	{
	  .disk = disk,
	  .attr = GRUB_FAT_ATTR_DIRECTORY,
	  .file_size = 0,
	  .cur_cluster_num = ~0U,
	  .cur_cluster = 0,
	  .is_contiguous = 0,
	};

	root.data = grub_exfat_mount(disk);
	if (!root.data)
		return grub_errno;

	root.file_cluster = root.data->root_cluster;

	*label = NULL;

	while (1)
	{
		offset += sizeof(dir);

		if (grub_fat_read_data(disk, &root, 0, 0,
			offset, sizeof(dir), (char*)&dir)
			!= sizeof(dir))
			break;

		if (dir.entry_type == 0)
			break;
		if (!(dir.entry_type & 0x80))
			continue;

		/* Volume label. */
		if (dir.entry_type == 0x83)
		{
			grub_size_t chc;
			grub_uint16_t t[ARRAY_SIZE(dir.type_specific.volume_label.str)];
			grub_size_t i;
			*label = grub_malloc(ARRAY_SIZE(dir.type_specific.volume_label.str)
				* GRUB_MAX_UTF8_PER_UTF16 + 1);
			if (!*label)
			{
				grub_free(root.data);
				return grub_errno;
			}
			chc = dir.type_specific.volume_label.character_count;
			if (chc > ARRAY_SIZE(dir.type_specific.volume_label.str))
				chc = ARRAY_SIZE(dir.type_specific.volume_label.str);
			for (i = 0; i < chc; i++)
				t[i] = grub_le_to_cpu16(dir.type_specific.volume_label.str[i]);
			*grub_utf16_to_utf8((grub_uint8_t*)*label, t, chc) = '\0';
		}
	}

	grub_free(root.data);
	return grub_errno;
}

static grub_err_t
grub_fat_uuid(grub_disk_t disk, char** uuid)
{
	struct grub_fat_data* data;

	data = grub_exfat_mount(disk);
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

struct grub_fs grub_exfat_fs =
{
  .name = "exfat",
  .fs_dir = grub_fat_dir,
  .fs_open = grub_fat_open,
  .fs_read = grub_fat_read,
  .fs_close = grub_fat_close,
  .fs_label = grub_fat_label,
  .fs_uuid = grub_fat_uuid,
  .next = 0
};

