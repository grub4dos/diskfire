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
#ifndef _GPT_PARTITION_HEADER
#define _GPT_PARTITION_HEADER 1

#include "compat.h"
#include "disk.h"
#include "partition.h"

typedef grub_packed_guid_t grub_gpt_part_guid_t;

#define GRUB_GPT_GUID_INIT(a, b, c, d1, d2, d3, d4, d5, d6, d7, d8)  \
	{ \
		grub_cpu_to_le32_compile_time (a), \
		grub_cpu_to_le16_compile_time (b), \
		grub_cpu_to_le16_compile_time (c), \
		{ d1, d2, d3, d4, d5, d6, d7, d8 } \
	}

#define GRUB_GPT_PARTITION_TYPE_EMPTY \
	GRUB_GPT_GUID_INIT (0x0, 0x0, 0x0,  \
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0)

#define GRUB_GPT_PARTITION_TYPE_EFI_SYSTEM \
	GRUB_GPT_GUID_INIT (0xc12a7328, 0xf81f, 0x11d2, \
		0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b)

#define GRUB_GPT_PARTITION_TYPE_BIOS_BOOT \
	GRUB_GPT_GUID_INIT (0x21686148, 0x6449, 0x6e6f, \
		0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49)

#define GRUB_GPT_PARTITION_TYPE_LEGACY_MBR \
	GRUB_GPT_GUID_INIT (0x024dee41, 0x33e7, 0x11d3, \
		0x9d, 0x69, 0x00, 0x08, 0xc7, 0x81, 0xf3, 0x9f)

#define GRUB_GPT_PARTITION_TYPE_LDM \
	GRUB_GPT_GUID_INIT (0x5808c8aa, 0x7e8f, 0x42e0, \
		0x85, 0xd2, 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3)

#define GRUB_GPT_PARTITION_TYPE_LDM_DATA \
	GRUB_GPT_GUID_INIT (0xaf9b60a0, 0x1431, 0x4f62, \
		0xbc, 0x68, 0x33, 0x11, 0x71, 0x4a, 0x69, 0xad)

#define GRUB_GPT_PARTITION_TYPE_MSR \
	GRUB_GPT_GUID_INIT (0xe3c9e316, 0x0b5c, 0x4db8, \
		0x81, 0x7d, 0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae)

#define GRUB_GPT_PARTITION_TYPE_WINRE \
	GRUB_GPT_GUID_INIT (0xde94bba4, 0x06d1, 0x4d40, \
		0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac)

PRAGMA_BEGIN_PACKED
struct grub_gpt_header
{
	grub_uint8_t magic[8];
	grub_uint32_t version;
	grub_uint32_t headersize;
	grub_uint32_t crc32;
	grub_uint32_t unused1;
	grub_uint64_t primary;
	grub_uint64_t backup;
	grub_uint64_t start;
	grub_uint64_t end;
	grub_uint8_t guid[16];
	grub_uint64_t partitions;
	grub_uint32_t maxpart;
	grub_uint32_t partentry_size;
	grub_uint32_t partentry_crc32;
};

struct grub_gpt_partentry
{
	grub_gpt_part_guid_t type;
	grub_gpt_part_guid_t guid;
	grub_uint64_t start;
	grub_uint64_t end;
	grub_uint64_t attrib;
	char name[72];
};
PRAGMA_END_PACKED

/* Standard partition attribute bits defined by UEFI.  */
#define GRUB_GPT_PART_ATTR_OFFSET_REQUIRED				0 // OEM
#define GRUB_GPT_PART_ATTR_OFFSET_NO_BLOCK_IO_PROTOCOL	1
#define GRUB_GPT_PART_ATTR_OFFSET_LEGACY_BIOS_BOOTABLE	2

/* De facto standard attribute bits defined by Microsoft and reused by
 * http://www.freedesktop.org/wiki/Specifications/DiscoverablePartitionsSpec */
#define GRUB_GPT_PART_ATTR_OFFSET_READ_ONLY				60
#define GRUB_GPT_PART_ATTR_OFFSET_SHADOW_COPY			61
#define GRUB_GPT_PART_ATTR_OFFSET_HIDDEN				62
#define GRUB_GPT_PART_ATTR_OFFSET_NO_AUTO				63

static inline grub_uint64_t
grub_gpt_entry_attribute(grub_uint64_t attr, grub_uint32_t offset)
{
	grub_uint64_t attrib = grub_le_to_cpu64(attr);
	return attrib & (1ULL << offset);
}

grub_err_t
grub_gpt_partition_map_iterate (grub_disk_t disk,
								grub_partition_iterate_hook_t hook,
								void *hook_data);

#endif
