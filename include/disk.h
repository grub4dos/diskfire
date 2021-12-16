// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DISK_HEADER
#define _DISK_HEADER 1

#include "compat.h"

struct grub_partition;

typedef int (*grub_disk_iterate_hook_t) (const char* name, void* data);

typedef void (*grub_disk_read_hook_t) (grub_disk_addr_t sector, unsigned offset, unsigned length, void* data);

#define GRUB_DISK_WINDISK_ID     1
#define GRUB_DISK_LOOPBACK_ID    2

/* Disk.  */
struct grub_disk
{
	/* The disk name.  */
	const char* name;

	/* Type of disk device.  */
	unsigned long type;

	/* The total number of sectors.  */
	grub_uint64_t total_sectors;

	/* Logarithm of sector size.  */
	unsigned int log_sector_size;

	/* Maximum number of sectors read divided by GRUB_DISK_CACHE_SIZE.  */
	unsigned int max_agglomerate;

	/* The id used by the disk cache manager.  */
	unsigned long id;

	/* The partition information. This is machine-specific.  */
	struct grub_partition* partition;

	/* Called when a sector was read. OFFSET is between 0 and
	   the sector size minus 1, and LENGTH is between 0 and the sector size.  */
	grub_disk_read_hook_t read_hook;

	/* Caller-specific data passed to the read hook.  */
	void* read_hook_data;

	/* Device-specific data.  */
	void* data;
};
typedef struct grub_disk* grub_disk_t;

/* The sector size.  */
#define GRUB_DISK_SECTOR_SIZE 0x200
#define GRUB_DISK_SECTOR_BITS 9

/*
 * Set max disk size at 1 EiB.
 */
#define GRUB_DISK_MAX_SECTORS (1ULL << (60 - GRUB_DISK_SECTOR_BITS))

 /* The maximum number of disk caches.  */
#define GRUB_DISK_CACHE_NUM 1021

/* The size of a disk cache in 512B units. Must be at least as big as the
   largest supported sector size, currently 16K.  */
#define GRUB_DISK_CACHE_BITS 6
#define GRUB_DISK_CACHE_SIZE (1 << GRUB_DISK_CACHE_BITS)

#define GRUB_DISK_MAX_MAX_AGGLOMERATE ((1 << (30 - GRUB_DISK_CACHE_BITS - GRUB_DISK_SECTOR_BITS)) - 1)

/* Return value of grub_disk_native_sectors() in case disk size is unknown. */
#define GRUB_DISK_SIZE_UNKNOWN 0xffffffffffffffffULL

/* Convert sector number from one sector size to another. */
static inline grub_disk_addr_t
grub_convert_sector(grub_disk_addr_t sector,
	grub_size_t log_sector_size_from,
	grub_size_t log_sector_size_to)
{
	if (log_sector_size_from == log_sector_size_to)
		return sector;
	else if (log_sector_size_from < log_sector_size_to)
	{
		sector = ALIGN_UP(sector, (1ULL << (log_sector_size_to - log_sector_size_from)));
		return sector >> (log_sector_size_to - log_sector_size_from);
	}
	else
		return sector << (log_sector_size_from - log_sector_size_to);
}

/* Convert to GRUB native disk sized sector from disk sized sector. */
static inline grub_disk_addr_t
grub_disk_from_native_sector(grub_disk_t disk, grub_disk_addr_t sector)
{
	return sector << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS);
}

grub_uint64_t grub_disk_native_sectors(grub_disk_t disk);

void grub_disk_cache_invalidate_all(void);

/* Disk cache.  */
struct grub_disk_cache
{
	unsigned long dev_id;
	unsigned long disk_id;
	grub_disk_addr_t sector;
	char* data;
	int lock;
};

extern struct grub_disk_cache grub_disk_cache_table[GRUB_DISK_CACHE_NUM];

int
grub_disk_iterate(grub_disk_iterate_hook_t hook, void* hook_data);

grub_disk_t grub_disk_open (const char* name);

void grub_disk_close (grub_disk_t disk);

grub_err_t
grub_disk_read (grub_disk_t disk, grub_disk_addr_t sector, grub_off_t offset, grub_size_t size, void* buf);

grub_err_t
grub_disk_write (grub_disk_t disk, grub_disk_addr_t sector, grub_off_t offset, grub_size_t size, const void* buf);

int
windisk_iterate(grub_disk_iterate_hook_t hook, void* hook_data);

grub_err_t
windisk_open(const char* name, struct grub_disk* disk);

void
windisk_close(struct grub_disk* disk);

grub_err_t
windisk_read(struct grub_disk* disk, grub_disk_addr_t sector, grub_size_t size, char* buf);

grub_err_t
windisk_write(struct grub_disk* disk, grub_disk_addr_t sector, grub_size_t size, const char* buf);

int
loopback_iterate(grub_disk_iterate_hook_t hook, void* hook_data);

grub_err_t
loopback_open(const char* name, grub_disk_t disk);

void
loopback_close(struct grub_disk* disk);

grub_err_t
loopback_read(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, char* buf);

grub_err_t
loopback_write(grub_disk_t disk, grub_disk_addr_t sector, grub_size_t size, const char* buf);

#endif
