// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "compat.h"
#include "disk.h"
#include "misc.h"

static BOOL
windisk_get_drive(const char* name, DWORD* drive)
{
	DWORD d;
	DWORD drive_max = GetDriveCount();
	if (!name || name[0] != 'h' || name[1] != 'd' || !grub_isdigit(name[2]))
		goto fail;
	d = grub_strtoul(name + 2, NULL, 10);
	if (d > drive_max)
		goto fail;
	*drive = d;
	return TRUE;
fail:
	grub_error(GRUB_ERR_UNKNOWN_DEVICE, "not a windisk");
	return FALSE;
}

static int
windisk_call_hook(grub_disk_iterate_hook_t hook, void* hook_data, DWORD drive)
{
	char name[] = "hd4294967295";
	grub_snprintf(name, sizeof(name), "hd%lu", drive);
	return hook(name, hook_data);
}

int
windisk_iterate(grub_disk_iterate_hook_t hook, void* hook_data)
{
	DWORD drive;
	DWORD drive_max = GetDriveCount();
	for (drive = 0; drive < drive_max; drive++)
	{
		if (windisk_call_hook(hook, hook_data, drive))
			return 1;
	}
	return 0;
}

grub_err_t
windisk_open(const char* name, struct grub_disk* disk)
{
	DWORD drive;
	HANDLE* data;

	if (!windisk_get_drive(name, &drive))
		return grub_errno;

	disk->type = GRUB_DISK_WINDISK_ID;

	data = GetHandleById(drive);
	if (data == INVALID_HANDLE_VALUE)
		return grub_error(GRUB_ERR_UNKNOWN_DEVICE, "invalid windisk");

	disk->id = drive;
	disk->log_sector_size = GRUB_DISK_SECTOR_BITS;
	disk->total_sectors = GetDriveSize(data) >> GRUB_DISK_SECTOR_BITS;
	disk->max_agglomerate = 1048576 >> (GRUB_DISK_SECTOR_BITS
		+ GRUB_DISK_CACHE_BITS);

	disk->data = data;

	return GRUB_ERR_NONE;
}

void
windisk_close(struct grub_disk* disk)
{
	HANDLE* data = disk->data;
	CHECK_CLOSE_HANDLE(data);
}

grub_err_t
windisk_read(struct grub_disk* disk, grub_disk_addr_t sector, grub_size_t size, char* buf)
{
	HANDLE dh = disk->data;
	__int64 distance = sector << GRUB_DISK_SECTOR_BITS;
	LARGE_INTEGER li = { 0 };
	DWORD dwsize;
	grub_dprintf("windisk", "windisk read %s sector 0x%llx size 0x%llx\n", disk->name, sector, size);
	if (size > (DWORD_MAX >> GRUB_DISK_SECTOR_BITS))
		return grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to read more than 4GB data");
	dwsize = (DWORD)(size << GRUB_DISK_SECTOR_BITS);
	li.QuadPart = distance;
	li.LowPart = SetFilePointer(dh, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to read outside of disk %s", disk->name);
	grub_dprintf("windisk", "windisk readfile offset 0x%llx size 0x%lx\n", distance, dwsize);
	if (ReadFile(dh, buf, dwsize, &dwsize, NULL))
		return GRUB_ERR_NONE;
	grub_dprintf("windisk", "windisk readfile failed %u\n", GetLastError());
	return grub_error(GRUB_ERR_READ_ERROR, "failure reading sector 0x%llx from %s", sector, disk->name);
}

grub_err_t
windisk_write(struct grub_disk* disk, grub_disk_addr_t sector, grub_size_t size, const char* buf)
{
	HANDLE dh = disk->data;
	__int64 distance = sector << GRUB_DISK_SECTOR_BITS;
	LARGE_INTEGER li = { 0 };
	DWORD dwsize;
	if (size > (DWORD_MAX >> GRUB_DISK_SECTOR_BITS))
		return grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to write more than 4GB data");
	dwsize = (DWORD)(size << GRUB_DISK_SECTOR_BITS);
	li.QuadPart = distance;
	li.LowPart = SetFilePointer(dh, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return grub_error(GRUB_ERR_OUT_OF_RANGE, "attempt to write outside of disk %s", disk->name);

	if (WriteFile(dh, buf, dwsize, &dwsize, NULL))
		return GRUB_ERR_NONE;
	return grub_error(GRUB_ERR_READ_ERROR, "failure writing sector 0x%llx from %s", sector, disk->name);
}
