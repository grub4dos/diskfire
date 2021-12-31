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

#ifndef _PARTITION_HEADER
#define _PARTITION_HEADER 1

#include "compat.h"

struct grub_disk;

typedef struct grub_partition* grub_partition_t;

typedef int (*grub_partition_iterate_hook_t) (struct grub_disk* disk, const grub_partition_t partition, void* data);

/* Partition map type.  */
struct grub_partition_map
{
	/* The next partition map type.  */
	struct grub_partition_map* next;
	struct grub_partition_map** prev;
	/* The name of the partition map type.  */
	const char* name;
	/* Call HOOK with each partition, until HOOK returns non-zero.  */
	grub_err_t (*iterate) (struct grub_disk* disk,
		grub_partition_iterate_hook_t hook, void* hook_data);
};
typedef struct grub_partition_map* grub_partition_map_t;

/* Partition description.  */
struct grub_partition
{
	/* The partition number.  */
	int number;
	/* The start sector (relative to parent).  */
	grub_disk_addr_t start;
	/* The length in sector units.  */
	grub_uint64_t len;
	/* The offset of the partition table.  */
	grub_disk_addr_t offset;
	/* The index of this partition in the partition table.  */
	int index;
	/* Parent partition (physically contains this partition).  */
	struct grub_partition* parent;
	/* The type partition map.  */
	grub_partition_map_t partmap;
	/* The type of partition when it's on MSDOS. */
	grub_uint8_t msdostype;
	/* The type of partition when it's on GPT. */
	grub_packed_guid_t gpttype;
	/* Partition flag */
	grub_uint64_t flag;
	/* First usable LBA for partitions */
	grub_disk_addr_t firstlba;
};

grub_partition_t grub_partition_probe(struct grub_disk* disk, const char* str);

int grub_partition_iterate(struct grub_disk* disk, grub_partition_iterate_hook_t hook, void* hook_data);

char* grub_partition_get_name(const grub_partition_t partition);

extern grub_partition_map_t grub_partition_map_list;

static inline void
grub_partition_map_register(grub_partition_map_t partmap)
{
	grub_list_push(GRUB_AS_LIST_P(&grub_partition_map_list),
		GRUB_AS_LIST(partmap));
}

static inline void
grub_partition_map_unregister(grub_partition_map_t partmap)
{
	grub_list_remove(GRUB_AS_LIST(partmap));
}

#define FOR_PARTITION_MAPS(var) FOR_LIST_ELEMENTS((var), (grub_partition_map_list))

static inline grub_disk_addr_t
grub_partition_get_start(const grub_partition_t p)
{
	grub_partition_t part;
	grub_uint64_t part_start = 0;
	for (part = p; part; part = part->parent)
		part_start += part->start;
	return part_start;
}

static inline grub_uint64_t
grub_partition_get_len(const grub_partition_t p)
{
	return p->len;
}

extern struct grub_partition_map grub_msdos_partition_map;
extern struct grub_partition_map grub_gpt_partition_map;
extern struct grub_partition_map grub_apple_partition_map;
extern struct grub_partition_map grub_bsdlabel_partition_map;
extern struct grub_partition_map grub_netbsdlabel_partition_map;
extern struct grub_partition_map grub_openbsdlabel_partition_map;
extern struct grub_partition_map grub_dfly_partition_map;

grub_partition_map_t
grub_partmap_probe(grub_disk_t disk);

void grub_partmap_init(void);

#endif
