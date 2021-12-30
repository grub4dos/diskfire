// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "br.h"

grub_br_t grub_br_list;

struct reserved_ctx
{
	grub_disk_addr_t firstlba;
	grub_disk_addr_t partlba;
	grub_partition_map_t partmap;
};

static int first_part_hook(struct grub_disk* disk, const grub_partition_t partition, void* data)
{
	(void)disk;
	struct reserved_ctx* ctx = data;
	grub_disk_addr_t start = grub_partition_get_start(partition);
	if (start < ctx->partlba)
		ctx->partlba = start;
	if (!ctx->partmap)
		ctx->partmap = partition->partmap;
	if (!ctx->firstlba && partition->firstlba)
		ctx->firstlba = partition->firstlba;
	return 0;
}

grub_disk_addr_t
grub_br_get_reserved_sectors(grub_disk_t disk, grub_disk_addr_t* start)
{
	struct reserved_ctx ctx = { .firstlba = 0, .partlba = GRUB_DISK_SIZE_UNKNOWN, .partmap = NULL };
	grub_partition_iterate(disk, first_part_hook, &ctx);
	if (!ctx.partmap || ctx.partlba <= 1 || !ctx.firstlba)
		return 0;
	if (start)
		*start = ctx.firstlba;
	return ctx.partlba - ctx.firstlba;
}

int
grub_br_check_data(const grub_uint8_t *data, grub_size_t data_len,
	grub_uint32_t offset, const grub_uint8_t *buf, grub_uint32_t buf_len)
{
	if (data_len < (grub_uint64_t)offset + buf_len)
		return 0;
	if (grub_memcmp(data + offset, buf, buf_len) == 0)
		return 1;
	return 0;
}

grub_br_t
grub_br_probe(grub_disk_t disk)
{
	grub_br_t br = NULL;
	grub_uint8_t sector[GRUB_DISK_SECTOR_SIZE];
	if (grub_disk_read(disk, 0, 0, sizeof(sector), sector))
		return NULL;
	FOR_BOOTRECORDS(br)
	{
		if (br->identify(sector))
			return br;
	}
	return NULL;
}

void grub_br_init(void)
{
	grub_br_register(&grub_mbr_nt6);
	grub_br_register(&grub_mbr_nt5);
	grub_br_register(&grub_mbr_grldr);
	grub_br_register(&grub_mbr_grub2);
	grub_br_register(&grub_mbr_xorboot);
	grub_br_register(&grub_mbr_plop);
	grub_br_register(&grub_mbr_fbinst);
	grub_br_register(&grub_mbr_ventoy);
	grub_br_register(&grub_mbr_ultraiso);
	grub_br_register(&grub_mbr_syslinux);
	grub_br_register(&grub_mbr_wee);
	grub_br_register(&grub_mbr_empty);
}
