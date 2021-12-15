// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "msdos_partition.h"
#include "bsdlabel.h"

static grub_err_t
iterate_real(grub_disk_t disk, grub_disk_addr_t sector, int freebsd,
	struct grub_partition_map* pmap,
	grub_partition_iterate_hook_t hook, void* hook_data)
{
	struct grub_partition_bsd_disk_label label;
	struct grub_partition p;
	grub_disk_addr_t delta = 0;
	grub_disk_addr_t pos;

	/* Read the BSD label.  */
	if (grub_disk_read(disk, sector, 0, sizeof(label), &label))
		return grub_errno;

	/* Check if it is valid.  */
	if (label.magic != grub_cpu_to_le32_compile_time(
		GRUB_PC_PARTITION_BSD_LABEL_MAGIC))
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no signature");

	/* A kludge to determine a base of be.offset.  */
	if (GRUB_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION
		< grub_cpu_to_le16(label.num_partitions) && freebsd)
	{
		struct grub_partition_bsd_entry whole_disk_be;

		pos = sizeof(label) + sector * GRUB_DISK_SECTOR_SIZE
			+ sizeof(struct grub_partition_bsd_entry)
			* GRUB_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION;

		if (grub_disk_read(disk, pos / GRUB_DISK_SECTOR_SIZE,
			pos % GRUB_DISK_SECTOR_SIZE, sizeof(whole_disk_be),
			&whole_disk_be))
			return grub_errno;

		delta = grub_le_to_cpu32(whole_disk_be.offset);
	}

	pos = sizeof(label) + sector * GRUB_DISK_SECTOR_SIZE;

	for (p.number = 0;
		p.number < grub_cpu_to_le16(label.num_partitions);
		p.number++, pos += sizeof(struct grub_partition_bsd_entry))
	{
		struct grub_partition_bsd_entry be;

		if (p.number == GRUB_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION)
			continue;

		p.offset = pos / GRUB_DISK_SECTOR_SIZE;
		p.index = pos % GRUB_DISK_SECTOR_SIZE;

		if (grub_disk_read(disk, p.offset, p.index, sizeof(be), &be))
			return grub_errno;

		p.start = grub_le_to_cpu32(be.offset);
		p.len = grub_le_to_cpu32(be.size);
		p.partmap = pmap;

		if (p.len == 0)
			continue;

		if (p.start < delta)
		{
			continue;
		}

		p.start -= delta;

		if (hook(disk, &p, hook_data))
			return grub_errno;
	}
	return GRUB_ERR_NONE;
}

static grub_err_t
bsdlabel_partition_map_iterate(grub_disk_t disk,
	grub_partition_iterate_hook_t hook,
	void* hook_data)
{

	if (disk->partition && grub_strcmp(disk->partition->partmap->name, "msdos")
		== 0 && disk->partition->msdostype == GRUB_PC_PARTITION_TYPE_FREEBSD)
		return iterate_real(disk, GRUB_PC_PARTITION_BSD_LABEL_SECTOR, 1,
			&grub_bsdlabel_partition_map, hook, hook_data);

	if (disk->partition
		&& (grub_strcmp(disk->partition->partmap->name, "msdos") == 0
			|| disk->partition->partmap == &grub_bsdlabel_partition_map
			|| disk->partition->partmap == &grub_netbsdlabel_partition_map
			|| disk->partition->partmap == &grub_openbsdlabel_partition_map))
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no embedding supported");

	return iterate_real(disk, GRUB_PC_PARTITION_BSD_LABEL_SECTOR, 0,
		&grub_bsdlabel_partition_map, hook, hook_data);
}

/* Context for netopenbsdlabel_partition_map_iterate.  */
struct netopenbsdlabel_ctx
{
	grub_uint8_t type;
	struct grub_partition_map* pmap;
	grub_partition_iterate_hook_t hook;
	void* hook_data;
	int count;
};

/* Helper for netopenbsdlabel_partition_map_iterate.  */
static int
check_msdos(grub_disk_t dsk, const grub_partition_t partition, void* data)
{
	struct netopenbsdlabel_ctx* ctx = data;
	grub_err_t err;

	if (partition->msdostype != ctx->type)
		return 0;

	err = iterate_real(dsk, partition->start
		+ GRUB_PC_PARTITION_BSD_LABEL_SECTOR, 0, ctx->pmap,
		ctx->hook, ctx->hook_data);
	if (err == GRUB_ERR_NONE)
	{
		ctx->count++;
		return 1;
	}
	if (err == GRUB_ERR_BAD_PART_TABLE)
	{
		grub_errno = GRUB_ERR_NONE;
		return 0;
	}
	grub_print_error();
	return 0;
}

/* This is a total breakage. Even when net-/openbsd label is inside partition
   it actually describes the whole disk.
 */
static grub_err_t
netopenbsdlabel_partition_map_iterate(grub_disk_t disk, grub_uint8_t type,
	struct grub_partition_map* pmap,
	grub_partition_iterate_hook_t hook,
	void* hook_data)
{
	if (disk->partition && grub_strcmp(disk->partition->partmap->name, "msdos")
		== 0)
		return grub_error(GRUB_ERR_BAD_PART_TABLE, "no embedding supported");

	{
		struct netopenbsdlabel_ctx ctx =
		{
		  .type = type,
		  .pmap = pmap,
		  .hook = hook,
		  .hook_data = hook_data,
		  .count = 0
		};
		grub_err_t err;

		err = grub_partition_msdos_iterate(disk, check_msdos, &ctx);

		if (err)
			return err;
		if (!ctx.count)
			return grub_error(GRUB_ERR_BAD_PART_TABLE, "no bsdlabel found");
	}
	return GRUB_ERR_NONE;
}

static grub_err_t
netbsdlabel_partition_map_iterate(grub_disk_t disk,
	grub_partition_iterate_hook_t hook,
	void* hook_data)
{
	return netopenbsdlabel_partition_map_iterate(disk,
		GRUB_PC_PARTITION_TYPE_NETBSD,
		&grub_netbsdlabel_partition_map,
		hook, hook_data);
}

static grub_err_t
openbsdlabel_partition_map_iterate(grub_disk_t disk,
	grub_partition_iterate_hook_t hook,
	void* hook_data)
{
	return netopenbsdlabel_partition_map_iterate(disk,
		GRUB_PC_PARTITION_TYPE_OPENBSD,
		&grub_openbsdlabel_partition_map,
		hook, hook_data);
}


struct grub_partition_map grub_bsdlabel_partition_map =
{
  .name = "bsd",
  .iterate = bsdlabel_partition_map_iterate,
};

struct grub_partition_map grub_openbsdlabel_partition_map =
{
  .name = "openbsd",
  .iterate = openbsdlabel_partition_map_iterate,
};

struct grub_partition_map grub_netbsdlabel_partition_map =
{
  .name = "netbsd",
  .iterate = netbsdlabel_partition_map_iterate,
};
