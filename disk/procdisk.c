// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"

static int
procdisk_iterate(grub_disk_iterate_hook_t hook, void* hook_data)
{
	return hook("proc", hook_data);
}

static grub_err_t
procdisk_open(const char* name, grub_disk_t disk)
{
	if (grub_strcmp(name, "proc"))
		return grub_error(GRUB_ERR_UNKNOWN_DEVICE, "not a procfs disk");

	disk->total_sectors = GRUB_DISK_SIZE_UNKNOWN;
	disk->max_agglomerate = GRUB_DISK_MAX_MAX_AGGLOMERATE;
	disk->id = 0;

	disk->data = 0;

	return GRUB_ERR_NONE;
}

static void
procdisk_close(grub_disk_t disk)
{
	(void)disk;
}

static grub_err_t
procdisk_read(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, char* buf)
{
	(void)disk;
	(void)sector;
	grub_memset(buf, 0, size << GRUB_DISK_SECTOR_BITS);
	return 0;
}

static grub_err_t
procdisk_write(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, const char* buf)
{
	(void)disk;
	(void)sector;
	(void)size;
	(void)buf;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "procdisk write is not supported");
}

struct grub_disk_dev grub_procdisk_dev =
{
	.name = "proc",
	.id = GRUB_DISK_PROC_ID,
	.disk_iterate = procdisk_iterate,
	.disk_open = procdisk_open,
	.disk_close = procdisk_close,
	.disk_read = procdisk_read,
	.disk_write = procdisk_write,
	.next = 0
};
