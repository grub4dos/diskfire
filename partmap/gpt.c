// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "msdos_partition.h"
#include "gpt_partition.h"

static grub_uint8_t grub_gpt_magic[8] =
{
	0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
};

static const grub_gpt_part_guid_t grub_gpt_partition_type_empty =
GRUB_GPT_PARTITION_TYPE_EMPTY;

/* 512 << 7 = 65536 byte sectors.  */
#define MAX_SECTOR_LOG 7

grub_err_t
grub_gpt_partition_map_iterate(grub_disk_t disk,
	grub_partition_iterate_hook_t hook,
	void* hook_data)
{
	struct grub_partition part = { 0 };
	struct grub_gpt_header gpt;
	struct grub_gpt_partentry entry;
	struct grub_msdos_partition_mbr mbr;
	grub_uint64_t entries;
	unsigned int i;
	int last_offset = 0;
	int sector_log = 0;

	/* Read the protective MBR.  */
	if (grub_disk_read(disk, 0, 0, sizeof(mbr), &mbr))
		return grub_errno;

	/* Check if it is valid.  */
	if (mbr.signature != grub_cpu_to_le16_compile_time(GRUB_PC_PARTITION_SIGNATURE))
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no signature");

	/* Make sure the MBR is a protective MBR and not a normal MBR.  */
	for (i = 0; i < 4; i++)
		if (mbr.entries[i].type == GRUB_PC_PARTITION_TYPE_GPT_DISK)
			break;
	if (i == 4)
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no GPT partition map found");

	/* Read the GPT header.  */
	for (sector_log = 0; sector_log < MAX_SECTOR_LOG; sector_log++)
	{
		if (grub_disk_read(disk, 1ULL << sector_log, 0, sizeof(gpt), &gpt))
			return grub_errno;

		if (grub_memcmp(gpt.magic, grub_gpt_magic, sizeof(grub_gpt_magic)) == 0)
			break;
	}
	if (sector_log == MAX_SECTOR_LOG)
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no valid GPT header");

	grub_dprintf("gpt", "Read a valid GPT header\n");
	part.firstlba = gpt.start;

	entries = grub_le_to_cpu64(gpt.partitions) << sector_log;
	for (i = 0; i < grub_le_to_cpu32(gpt.maxpart); i++)
	{
		if (grub_disk_read(disk, entries, last_offset,
			sizeof(entry), &entry))
			return grub_errno;

		if (grub_memcmp(&grub_gpt_partition_type_empty, &entry.type,
			sizeof(grub_gpt_partition_type_empty)))
		{
			/* Calculate the first block and the size of the partition.  */
			part.start = grub_le_to_cpu64(entry.start) << sector_log;
			part.len = (grub_le_to_cpu64(entry.end)
				- grub_le_to_cpu64(entry.start) + 1) << sector_log;
			part.offset = entries;
			part.number = i;
			part.index = last_offset;
			part.partmap = &grub_gpt_partition_map;
			part.parent = disk->partition;
			part.flag = entry.attrib;
			grub_memcpy(&part.gpttype, &entry.type, sizeof(grub_packed_guid_t));

			grub_dprintf("gpt", "GPT entry %u: start=%llu, length=%llu\n", i,
				(unsigned long long) part.start,
				(unsigned long long) part.len);

			if (hook(disk, &part, hook_data))
				return grub_errno;
		}

		last_offset += grub_le_to_cpu32(gpt.partentry_size);
		if (last_offset == GRUB_DISK_SECTOR_SIZE)
		{
			last_offset = 0;
			entries++;
		}
	}

	return GRUB_ERR_NONE;
}

/* Partition map type.  */
struct grub_partition_map grub_gpt_partition_map =
{
  .name = "gpt",
  .iterate = grub_gpt_partition_map_iterate,
};
