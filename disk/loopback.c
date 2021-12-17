// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "file.h"
#include "commands.h"

struct grub_loopback
{
	char* devname;
	grub_file_t file;
	struct grub_loopback* next;
	unsigned long id;
};

static struct grub_loopback* loopback_list;
static unsigned long last_id = 0;

/* Delete the loopback device NAME.  */
grub_err_t loopback_delete(const char* name)
{
	struct grub_loopback* dev;
	struct grub_loopback** prev;

	/* Search for the device.  */
	for (dev = loopback_list, prev = &loopback_list;
		dev;
		prev = &dev->next, dev = dev->next)
		if (grub_strcmp(dev->devname, name) == 0)
			break;

	if (!dev)
		return grub_error(GRUB_ERR_BAD_DEVICE, "device not found");

	/* Remove the device from the list.  */
	*prev = dev->next;

	grub_free(dev->devname);
	grub_file_close(dev->file);
	grub_free(dev);

	return 0;
}

grub_err_t
loopback_add(const char* name)
{
	grub_file_t file = NULL;
	struct grub_loopback* newdev;
	grub_err_t ret;
	char lpdev[] = "ld4294967295";

	if (!name)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "filename expected");

	file = grub_file_open(name, GRUB_FILE_TYPE_LOOPBACK);
	if (!file)
		return grub_errno;

	/* Unable to replace it, make a new entry.  */
	newdev = grub_zalloc(sizeof(struct grub_loopback));
	if (!newdev)
		goto fail;

	grub_snprintf(lpdev, sizeof(lpdev), "ld%lu", last_id);
	newdev->devname = grub_strdup(lpdev);
	if (!newdev->devname)
	{
		grub_free(newdev);
		goto fail;
	}

	newdev->file = file;
	newdev->id = last_id++;

	/* Add the new entry to the list.  */
	newdev->next = loopback_list;
	loopback_list = newdev;

	return 0;

fail:
	ret = grub_errno;
	grub_file_close(file);
	return ret;
}

int
loopback_iterate(grub_disk_iterate_hook_t hook, void* hook_data)
{
	struct grub_loopback* d;
	for (d = loopback_list; d; d = d->next)
	{
		if (hook(d->devname, hook_data))
			return 1;
	}
	return 0;
}

grub_err_t
loopback_open(const char* name, grub_disk_t disk)
{
	struct grub_loopback* dev;

	for (dev = loopback_list; dev; dev = dev->next)
		if (grub_strcmp(dev->devname, name) == 0)
			break;

	if (!dev)
		return grub_error(GRUB_ERR_UNKNOWN_DEVICE, "can't open device");

	/* Use the filesize for the disk size, round up to a complete sector.  */
	if (dev->file->size != GRUB_FILE_SIZE_UNKNOWN)
		disk->total_sectors = ((dev->file->size + GRUB_DISK_SECTOR_SIZE - 1) / GRUB_DISK_SECTOR_SIZE);
	else
		disk->total_sectors = GRUB_DISK_SIZE_UNKNOWN;
	/* Avoid reading more than 512M.  */
	disk->max_agglomerate = 1 << (29 - GRUB_DISK_SECTOR_BITS - GRUB_DISK_CACHE_BITS);

	disk->type = GRUB_DISK_LOOPBACK_ID;

	disk->id = dev->id;

	disk->data = dev;

	return 0;
}

void
loopback_close(struct grub_disk* disk)
{
	(void)disk;
}

grub_err_t
loopback_read(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, char* buf)
{
	grub_file_t file = ((struct grub_loopback*)disk->data)->file;
	grub_off_t pos;

	grub_file_seek(file, sector << GRUB_DISK_SECTOR_BITS);
	grub_file_read(file, buf, size << GRUB_DISK_SECTOR_BITS);
	if (grub_errno)
		return grub_errno;

	/* In case there is more data read than there is available, in case
	   of files that are not a multiple of GRUB_DISK_SECTOR_SIZE, fill
	   the rest with zeros.  */
	pos = (sector + size) << GRUB_DISK_SECTOR_BITS;
	if (pos > file->size)
	{
		grub_size_t amount = pos - file->size;
		grub_memset(buf + (size << GRUB_DISK_SECTOR_BITS) - amount, 0, amount);
	}

	return 0;
}

grub_err_t
loopback_write(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, const char* buf)
{
	(void)disk;
	(void)sector;
	(void)size;
	(void)buf;
	return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "loopback write is not supported");
}
