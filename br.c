// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "br.h"
#include "command.h"
#include "file.h"
#include "fat.h"
#include "exfat.h"

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

static
grub_off_t br_proc_read(struct grub_file* file, void* data, grub_size_t sz)
{
	struct grub_procfs_entry* entry = file->data;
	grub_br_t br = entry->data;
	if (!br || !br->code || !br->code_size)
		return 0;
	if (data)
		grub_memcpy(data, br->code + file->offset, sz);
	return br->code_size;
}

void grub_br_register(grub_br_t br)
{
	char proc_name[64];
	grub_list_push(GRUB_AS_LIST_P(&grub_br_list), GRUB_AS_LIST(br));
	if (br->code && br->code_size)
	{
		grub_snprintf(proc_name, sizeof(proc_name), "%s.mbr", br->name);
		proc_add(proc_name, br, br_proc_read);
	}
}

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

const char*
grub_br_get_fs_type(grub_disk_t disk)
{
	int fat_size = 0;
	struct grub_fat_data* data;
	grub_fs_t fs = grub_fs_probe(disk);
	if (!fs)
		return "unknown";
	if (grub_strcmp(fs->name, "fat") != 0)
		return fs->name;
	data = grub_fat_mount(disk);
	if (data)
	{
		fat_size = data->fat_size;
		grub_free(data);
	}
	switch (fat_size)
	{
	case 12: return "fat12";
	case 16: return "fat16";
	case 32: return "fat32";
	}
	return "error";
}

grub_disk_addr_t
grub_br_get_fs_reserved_sectors(grub_disk_t disk)
{
	grub_disk_addr_t reserved = 0;
	grub_uint8_t vbr[GRUB_DISK_SECTOR_SIZE];
	const char* fs_name = grub_br_get_fs_type(disk);
	grub_errno = GRUB_ERR_NONE;
	if (!fs_name)
		goto out;
	if (grub_disk_read(disk, 0, 0, GRUB_DISK_SECTOR_SIZE, vbr))
		goto out;
	if (grub_strncmp(fs_name, "fat", 3) == 0)
	{
		struct grub_fat_bpb* bpb = (void*)vbr;
		reserved = bpb->num_reserved_sectors;
	}
	else if (grub_strcmp(fs_name, "exfat") == 0)
	{
		struct grub_exfat_bpb* bpb = (void*)vbr;
		reserved = bpb->num_reserved_sectors;
	}
	else if (grub_strcmp(fs_name, "ntfs") == 0)
	{
		grub_file_t file = 0;
		reserved = 16;
		file = grub_zalloc(sizeof(struct grub_file));
		if (!file)
			goto out;
		file->disk = disk;
		if (grub_ntfs_fs.fs_open(file, "/$Boot"))
			goto out;
		reserved = file->size >> GRUB_DISK_SECTOR_BITS;
		grub_ntfs_fs.fs_close(file);
		grub_free(file);
	}
	else if (grub_strcmp(fs_name, "ext") == 0)
		reserved = 2;
out:
	grub_errno = GRUB_ERR_NONE;
	return reserved;
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

	grub_br_register(&grub_pbr_grldr);
}
