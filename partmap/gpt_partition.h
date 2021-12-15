// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _GPT_PARTITION_HEADER
#define _GPT_PARTITION_HEADER 1

#include "compat.h"
#include "disk.h"
#include "partition.h"

PRAGMA_BEGIN_PACKED
struct grub_gpt_part_guid
{
	grub_uint32_t data1;
	grub_uint16_t data2;
	grub_uint16_t data3;
	grub_uint8_t data4[8];
};
PRAGMA_END_PACKED
typedef struct grub_gpt_part_guid grub_gpt_part_guid_t;

#define GRUB_GPT_PARTITION_TYPE_EMPTY \
	{ 0x0, 0x0, 0x0, \
		{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } \
	}

#define GRUB_GPT_PARTITION_TYPE_BIOS_BOOT \
	{ grub_cpu_to_le32_compile_time (0x21686148), \
		grub_cpu_to_le16_compile_time (0x6449), \
		grub_cpu_to_le16_compile_time (0x6e6f), \
		{ 0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } \
	}

#define GRUB_GPT_PARTITION_TYPE_LDM \
	{ grub_cpu_to_le32_compile_time (0x5808C8AAU),\
		grub_cpu_to_le16_compile_time (0x7E8F), \
		grub_cpu_to_le16_compile_time (0x42E0), \
		{ 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3 } \
	}

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

grub_err_t
grub_gpt_partition_map_iterate (grub_disk_t disk,
								grub_partition_iterate_hook_t hook,
								void *hook_data);

#endif
