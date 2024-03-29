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
#include "compat.h"
#include "disk.h"
#include "partition.h"

grub_disk_dev_t grub_disk_dev_list;

#define	GRUB_CACHE_TIMEOUT 2

/* The last time the disk was used.  */
static grub_uint64_t grub_last_time = 0;

struct grub_disk_cache grub_disk_cache_table[GRUB_DISK_CACHE_NUM];

struct part_ent
{
	struct part_ent* next;
	char* name;
};

/* Context for grub_device_iterate.  */
struct grub_disk_iterate_ctx
{
	grub_disk_iterate_hook_t hook;
	void* hook_data;
	struct part_ent* ents;
};

/* Helper for grub_device_iterate.  */
static int
iterate_partition(grub_disk_t disk, const grub_partition_t partition, void* data)
{
	struct grub_disk_iterate_ctx* ctx = data;
	struct part_ent* p;
	char* part_name;
	grub_size_t p_name_len;

	p = grub_malloc(sizeof(*p));
	if (!p)
		return 1;

	part_name = grub_partition_get_name(partition);
	if (!part_name)
	{
		grub_free(p);
		return 1;
	}
	p_name_len = grub_strlen(disk->name) + grub_strlen(part_name) + 1 + 1;
	p->name = grub_malloc(p_name_len);
	if (!p->name)
	{
		grub_free(part_name);
		grub_free(p);
		return 1;
	}
	grub_snprintf(p->name, p_name_len, "%s,%s", disk->name, part_name);
	grub_free(part_name);

	p->next = ctx->ents;
	ctx->ents = p;

	return 0;
}

/* Helper for grub_device_iterate.  */
static int
iterate_disk(const char* disk_name, void* data)
{
	struct grub_disk_iterate_ctx* ctx = data;
	grub_disk_t disk;
	struct part_ent* p;
	int ret = 0;

	if (ctx->hook(disk_name, ctx->hook_data))
		return 1;

	disk = grub_disk_open(disk_name);
	if (!disk)
	{
		grub_errno = GRUB_ERR_NONE;
		return 0;
	}

	ctx->ents = NULL;
	(void)grub_partition_iterate(disk, iterate_partition, ctx);
	grub_disk_close(disk);

	grub_errno = GRUB_ERR_NONE;

	p = ctx->ents;
	while (p != NULL)
	{
		struct part_ent* next = p->next;
		if (!ret)
			ret = ctx->hook(p->name, ctx->hook_data);
		grub_free(p->name);
		grub_free(p);
		p = next;
	}
	return ret;
}

int
grub_disk_iterate(grub_disk_iterate_hook_t hook, void* hook_data)
{
	grub_disk_dev_t dev;
	struct grub_disk_iterate_ctx ctx = { hook, hook_data, NULL };
	FOR_DISKDEVS(dev)
	{
		if (dev->disk_iterate(iterate_disk, &ctx))
			return 1;
	}
	return 0;
}

/* This function performs three tasks:
   - Make sectors disk relative from partition relative.
   - Normalize offset to be less than the sector size.
   - Verify that the range is inside the partition.  */
static grub_err_t
grub_disk_adjust_range(grub_disk_t disk, grub_disk_addr_t* sector,
	grub_off_t* offset, grub_size_t size)
{
	grub_partition_t part;
	grub_disk_addr_t total_sectors;

	*sector += *offset >> GRUB_DISK_SECTOR_BITS;
	*offset &= GRUB_DISK_SECTOR_SIZE - 1;

	for (part = disk->partition; part; part = part->parent)
	{
		grub_disk_addr_t start;
		grub_uint64_t len;

		start = part->start;
		len = part->len;

		if (*sector >= len
			|| len - *sector < ((*offset + size + GRUB_DISK_SECTOR_SIZE - 1)
				>> GRUB_DISK_SECTOR_BITS))
			return grub_error(GRUB_ERR_OUT_OF_RANGE,
				"attempt to read or write outside of partition");
		*sector += start;
	}

	/* Transform total_sectors to number of 512B blocks.  */
	total_sectors = disk->total_sectors << (disk->log_sector_size -
		GRUB_DISK_SECTOR_BITS);

	/*
	 * Some drivers have problems with disks above reasonable sizes.
	 * Clamp the size to GRUB_DISK_MAX_SECTORS. Just one condition is enough
	 * since GRUB_DISK_SIZE_UNKNOWN is always above GRUB_DISK_MAX_SECTORS,
	 * assuming a maximum 4 KiB sector size.
	 */
	if (total_sectors > GRUB_DISK_MAX_SECTORS)
		total_sectors = GRUB_DISK_MAX_SECTORS;

	if ((total_sectors <= *sector
		|| ((*offset + size + GRUB_DISK_SECTOR_SIZE - 1)
			>> GRUB_DISK_SECTOR_BITS) > total_sectors - *sector))
		return grub_error(GRUB_ERR_OUT_OF_RANGE,
			"attempt to read or write outside of disk %s", disk->name);

	return GRUB_ERR_NONE;
}

static inline grub_disk_addr_t
transform_sector(grub_disk_t disk, grub_disk_addr_t sector)
{
	return sector >> (disk->log_sector_size - GRUB_DISK_SECTOR_BITS);
}

static unsigned
grub_disk_cache_get_index(unsigned long dev_id, unsigned long disk_id,
	grub_disk_addr_t sector)
{
	return ((dev_id * 524287UL + disk_id * 2606459UL
		+ ((unsigned)(sector >> GRUB_DISK_CACHE_BITS)))
		% GRUB_DISK_CACHE_NUM);
}

static void
grub_disk_cache_invalidate(unsigned long dev_id, unsigned long disk_id,
	grub_disk_addr_t sector)
{
	unsigned cache_index;
	struct grub_disk_cache* cache;

	sector &= ~((grub_disk_addr_t)GRUB_DISK_CACHE_SIZE - 1);
	cache_index = grub_disk_cache_get_index(dev_id, disk_id, sector);
	cache = grub_disk_cache_table + cache_index;

	if (cache->dev_id == dev_id && cache->disk_id == disk_id
		&& cache->sector == sector && cache->data)
	{
		cache->lock = 1;
		grub_free(cache->data);
		cache->data = 0;
		cache->lock = 0;
	}
}

void
grub_disk_cache_invalidate_all(void)
{
	unsigned i;

	for (i = 0; i < GRUB_DISK_CACHE_NUM; i++)
	{
		struct grub_disk_cache* cache = grub_disk_cache_table + i;

		if (cache->data && !cache->lock)
		{
			grub_free(cache->data);
			cache->data = 0;
		}
	}
}

static char*
grub_disk_cache_fetch(unsigned long dev_id, unsigned long disk_id,
	grub_disk_addr_t sector)
{
	struct grub_disk_cache* cache;
	unsigned cache_index;

	cache_index = grub_disk_cache_get_index(dev_id, disk_id, sector);
	cache = grub_disk_cache_table + cache_index;

	if (cache->dev_id == dev_id && cache->disk_id == disk_id
		&& cache->sector == sector)
	{
		cache->lock = 1;
		return cache->data;
	}

	return 0;
}

static void
grub_disk_cache_unlock(unsigned long dev_id, unsigned long disk_id,
	grub_disk_addr_t sector)
{
	struct grub_disk_cache* cache;
	unsigned cache_index;

	cache_index = grub_disk_cache_get_index(dev_id, disk_id, sector);
	cache = grub_disk_cache_table + cache_index;

	if (cache->dev_id == dev_id && cache->disk_id == disk_id
		&& cache->sector == sector)
		cache->lock = 0;
}

static grub_err_t
grub_disk_cache_store(unsigned long dev_id, unsigned long disk_id,
	grub_disk_addr_t sector, const char* data)
{
	unsigned cache_index;
	struct grub_disk_cache* cache;

	cache_index = grub_disk_cache_get_index(dev_id, disk_id, sector);
	cache = grub_disk_cache_table + cache_index;

	cache->lock = 1;
	grub_free(cache->data);
	cache->data = 0;
	cache->lock = 0;

	cache->data = grub_malloc(GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
	if (!cache->data)
		return grub_errno;

	grub_memcpy(cache->data, data,
		GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
	cache->dev_id = dev_id;
	cache->disk_id = disk_id;
	cache->sector = sector;

	return GRUB_ERR_NONE;
}

static const char*
find_part_sep(const char* name)
{
	const char* p = name;
	char c;

	while ((c = *p++) != '\0')
	{
		if (c == '\\' && *p == ',')
			p++;
		else if (c == ',')
			return p - 1;
	}
	return NULL;
}

grub_disk_t
grub_disk_open(const char* name)
{
	const char* p;
	grub_disk_t disk;
	char* raw = (char*)name;
	grub_uint64_t current_time;
	grub_disk_dev_t dev;

	if (!name)
		return 0;

	grub_dprintf("disk", "Opening %s...\n", name);

	disk = (grub_disk_t)grub_zalloc(sizeof(*disk));
	if (!disk)
		return 0;
	disk->log_sector_size = GRUB_DISK_SECTOR_BITS;
	/* Default 1MiB of maximum agglomerate.  */
	disk->max_agglomerate = 1048576 >> (GRUB_DISK_SECTOR_BITS
		+ GRUB_DISK_CACHE_BITS);

	p = find_part_sep(name);
	if (p)
	{
		grub_size_t len = p - name;

		raw = grub_malloc(len + 1);
		if (!raw)
			goto fail;

		grub_memcpy(raw, name, len);
		raw[len] = '\0';
		disk->name = grub_strdup(raw);
	}
	else
		disk->name = grub_strdup(name);
	if (!disk->name)
		goto fail;

	FOR_DISKDEVS(dev)
	{
		if ((dev->disk_open) (raw, disk) == GRUB_ERR_NONE)
			break;
		else if (grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
			grub_errno = GRUB_ERR_NONE;
		else
			goto fail;
	}
	if (!dev)
	{
		grub_error(GRUB_ERR_UNKNOWN_DEVICE, "disk %s not found", name);
		goto fail;
	}

	if (disk->log_sector_size > GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS
		|| disk->log_sector_size < GRUB_DISK_SECTOR_BITS)
	{
		grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET,
			"sector sizes of %d bytes aren't supported yet",
			(1 << disk->log_sector_size));
		goto fail;
	}

	disk->dev = dev;

	if (p)
	{
		disk->partition = grub_partition_probe(disk, p + 1);
		if (!disk->partition)
		{
			/* TRANSLATORS: It means that the specified partition e.g.
			   hd0,msdos1=/dev/sda1 doesn't exist.  */
			grub_error(GRUB_ERR_UNKNOWN_DEVICE, "no such partition");
			goto fail;
		}
	}

	/* The cache will be invalidated about 2 seconds after a device was
	   closed.  */
	current_time = grub_get_time_ms();

	if (current_time > (grub_last_time
		+ GRUB_CACHE_TIMEOUT * 1000))
		grub_disk_cache_invalidate_all();

	grub_last_time = current_time;

fail:

	if (raw && raw != name)
		grub_free(raw);

	if (grub_errno != GRUB_ERR_NONE)
	{
		grub_error_push();
		grub_dprintf("disk", "Opening %s failed.\n", name);
		grub_error_pop();

		grub_disk_close(disk);
		return 0;
	}

	return disk;
}

void
grub_disk_close(grub_disk_t disk)
{
	grub_partition_t part;
	grub_dprintf("disk", "Closing %s.\n", disk->name);

	if (disk->dev && disk->dev->disk_close)
		(disk->dev->disk_close) (disk);

	/* Reset the timer.  */
	grub_last_time = grub_get_time_ms();

	while (disk->partition)
	{
		part = disk->partition->parent;
		grub_free(disk->partition);
		disk->partition = part;
	}
	grub_free((void*)disk->name);
	grub_free(disk);
}

/* Small read (less than cache size and not pass across cache unit boundaries).
   sector is already adjusted and is divisible by cache unit size.
 */
static grub_err_t
grub_disk_read_small_real(grub_disk_t disk, grub_disk_addr_t sector,
	grub_off_t offset, grub_size_t size, void* buf)
{
	char* data;
	char* tmp_buf;

	/* Fetch the cache.  */
	data = grub_disk_cache_fetch(disk->dev->id, disk->id, sector);
	if (data)
	{
		/* Just copy it!  */
		grub_memcpy(buf, data + offset, size);
		grub_disk_cache_unlock(disk->dev->id, disk->id, sector);
		return GRUB_ERR_NONE;
	}

	/* Allocate a temporary buffer.  */
	tmp_buf = grub_malloc(GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
	if (!tmp_buf)
		return grub_errno;

	/* Otherwise read data from the disk actually.  */
	if (disk->total_sectors == GRUB_DISK_SIZE_UNKNOWN
		|| sector + GRUB_DISK_CACHE_SIZE
		< (disk->total_sectors << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS)))
	{
		grub_err_t err;
		err = disk->dev->disk_read(disk, transform_sector(disk, sector),
			1ULL << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS - disk->log_sector_size), tmp_buf);
		if (!err)
		{
			/* Copy it and store it in the disk cache.  */
			grub_memcpy(buf, tmp_buf + offset, size);
			grub_disk_cache_store(disk->dev->id, disk->id,
				sector, tmp_buf);
			grub_free(tmp_buf);
			return GRUB_ERR_NONE;
		}
	}

	grub_free(tmp_buf);
	grub_errno = GRUB_ERR_NONE;

	{
		/* Uggh... Failed. Instead, just read necessary data.  */
		unsigned num;
		grub_disk_addr_t aligned_sector;

		sector += (offset >> GRUB_DISK_SECTOR_BITS);
		offset &= ((1 << GRUB_DISK_SECTOR_BITS) - 1);
		aligned_sector = (sector & ~((1ULL << (disk->log_sector_size
			- GRUB_DISK_SECTOR_BITS))
			- 1));
		offset += ((sector - aligned_sector) << GRUB_DISK_SECTOR_BITS);
		num = (unsigned int)((size + offset + (1ULL << (disk->log_sector_size))
			- 1) >> (disk->log_sector_size));

		tmp_buf = grub_malloc(((unsigned long long)num) << disk->log_sector_size);
		if (!tmp_buf)
			return grub_errno;

		if (disk->dev->disk_read(disk, transform_sector(disk, aligned_sector),
			num, tmp_buf))
		{
			grub_error_push();
			grub_dprintf("disk", "%s read failed\n", disk->name);
			grub_error_pop();
			grub_free(tmp_buf);
			return grub_errno;
		}
		grub_memcpy(buf, tmp_buf + offset, size);
		grub_free(tmp_buf);
		return GRUB_ERR_NONE;
	}
}

static grub_err_t
grub_disk_read_small(grub_disk_t disk, grub_disk_addr_t sector,
	grub_off_t offset, grub_size_t size, void* buf)
{
	grub_err_t err;

	err = grub_disk_read_small_real(disk, sector, offset, size, buf);
	if (err)
		return err;
	if (disk->read_hook)
		err = (disk->read_hook) (sector + (offset >> GRUB_DISK_SECTOR_BITS),
			offset & (GRUB_DISK_SECTOR_SIZE - 1),
			(unsigned)size, buf, disk->read_hook_data);
	return err;
}

/* Read data from the disk.  */
grub_err_t
grub_disk_read(grub_disk_t disk, grub_disk_addr_t sector,
	grub_off_t offset, grub_size_t size, void* buf)
{
	/* First of all, check if the region is within the disk.  */
	if (grub_disk_adjust_range(disk, &sector, &offset, size) != GRUB_ERR_NONE)
	{
		grub_error_push();
		grub_dprintf("disk", "Read out of range: sector 0x%llx (%s).\n",
			(unsigned long long) sector, grub_errmsg);
		grub_error_pop();
		return grub_errno;
	}
	grub_dprintf("disk", "disk read %s sector 0x%llx+0x%llx size 0x%llx\n", disk->name, sector, offset, size);
	/* First read until first cache boundary.   */
	if (offset || (sector & (GRUB_DISK_CACHE_SIZE - 1)))
	{
		grub_disk_addr_t start_sector;
		grub_size_t pos;
		grub_err_t err;
		grub_size_t len;

		start_sector = sector & ~((grub_disk_addr_t)GRUB_DISK_CACHE_SIZE - 1);
		pos = (sector - start_sector) << GRUB_DISK_SECTOR_BITS;
		len = ((GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS)
			- pos - offset);
		if (len > size)
			len = size;
		err = grub_disk_read_small(disk, start_sector,
			offset + pos, len, buf);
		if (err)
			return err;
		buf = (char*)buf + len;
		size -= len;
		offset += len;
		sector += (offset >> GRUB_DISK_SECTOR_BITS);
		offset &= ((1 << GRUB_DISK_SECTOR_BITS) - 1);
	}

	/* Until SIZE is zero...  */
	while (size >= (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS))
	{
		char* data = NULL;
		grub_disk_addr_t agglomerate;
		grub_err_t err;

		/* agglomerate read until we find a first cached entry.  */
		for (agglomerate = 0; agglomerate
			< (size >> (GRUB_DISK_SECTOR_BITS + GRUB_DISK_CACHE_BITS))
			&& agglomerate < disk->max_agglomerate;
			agglomerate++)
		{
			data = grub_disk_cache_fetch(disk->dev->id, disk->id,
				sector + (agglomerate
					<< GRUB_DISK_CACHE_BITS));
			if (data)
				break;
		}

		if (data)
		{
			grub_memcpy((char*)buf
				+ (agglomerate << (GRUB_DISK_CACHE_BITS
					+ GRUB_DISK_SECTOR_BITS)),
				data, GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
			grub_disk_cache_unlock(disk->dev->id, disk->id,
				sector + (agglomerate
					<< GRUB_DISK_CACHE_BITS));
		}

		if (agglomerate)
		{
			grub_disk_addr_t i;

			err = disk->dev->disk_read(disk, transform_sector(disk, sector),
				agglomerate << (GRUB_DISK_CACHE_BITS
					+ GRUB_DISK_SECTOR_BITS
					- disk->log_sector_size),
				buf);
			if (err)
				return err;

			for (i = 0; i < agglomerate; i++)
				grub_disk_cache_store(disk->dev->id, disk->id,
					sector + (i << GRUB_DISK_CACHE_BITS),
					(char*)buf
					+ (i << (GRUB_DISK_CACHE_BITS
						+ GRUB_DISK_SECTOR_BITS)));


			if (disk->read_hook)
				(disk->read_hook) (sector, 0, (unsigned)
					(agglomerate << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS)),
					buf, disk->read_hook_data);

			sector += agglomerate << GRUB_DISK_CACHE_BITS;
			size -= agglomerate << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS);
			buf = (char*)buf
				+ (agglomerate << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS));
		}

		if (data)
		{
			if (disk->read_hook)
				(disk->read_hook) (sector, 0, (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS),
					buf, disk->read_hook_data);
			sector += GRUB_DISK_CACHE_SIZE;
			buf = (char*)buf + (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
			size -= (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
		}
	}

	/* And now read the last part.  */
	if (size)
	{
		grub_err_t err;
		err = grub_disk_read_small(disk, sector, 0, size, buf);
		if (err)
			return err;
	}

	return grub_errno;
}

grub_err_t
grub_disk_write(grub_disk_t disk, grub_disk_addr_t sector,
	grub_off_t offset, grub_size_t size, const void* buf)
{
	unsigned real_offset;
	grub_disk_addr_t aligned_sector;

	grub_dprintf("disk", "Writing `%s'...\n", disk->name);

	if (grub_disk_adjust_range(disk, &sector, &offset, size) != GRUB_ERR_NONE)
		return -1;

	aligned_sector = (sector & ~((1ULL << (disk->log_sector_size
		- GRUB_DISK_SECTOR_BITS)) - 1));
	real_offset = (unsigned) (offset + ((sector - aligned_sector) << GRUB_DISK_SECTOR_BITS));
	sector = aligned_sector;

	while (size)
	{
		if (real_offset != 0 || (size < (1ULL << disk->log_sector_size)
			&& size != 0))
		{
			char* tmp_buf;
			grub_size_t len;
			grub_partition_t part;

			tmp_buf = grub_malloc(1ULL << disk->log_sector_size);
			if (!tmp_buf)
				return grub_errno;

			part = disk->partition;
			disk->partition = 0;
			if (grub_disk_read(disk, sector,
				0, (1ULL << disk->log_sector_size), tmp_buf)
				!= GRUB_ERR_NONE)
			{
				disk->partition = part;
				grub_free(tmp_buf);
				goto finish;
			}
			disk->partition = part;

			len = (1ULL << disk->log_sector_size) - real_offset;
			if (len > size)
				len = size;

			grub_memcpy(tmp_buf + real_offset, buf, len);

			grub_disk_cache_invalidate(disk->dev->id, disk->id, sector);

			if (disk->dev->disk_write(disk, transform_sector(disk, sector),
				1, tmp_buf) != GRUB_ERR_NONE)
			{
				grub_free(tmp_buf);
				goto finish;
			}

			grub_free(tmp_buf);

			sector += (1ULL << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS));
			buf = (const char*)buf + len;
			size -= len;
			real_offset = 0;
		}
		else
		{
			grub_size_t len;
			grub_size_t n;

			len = size & ~((1ULL << disk->log_sector_size) - 1);
			n = size >> disk->log_sector_size;

			if (n > ((grub_uint64_t)(disk->max_agglomerate)
				<< (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS
					- disk->log_sector_size)))
				n = ((grub_uint64_t)(disk->max_agglomerate)
					<< (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS
						- disk->log_sector_size));

			if (disk->dev->disk_write(disk, transform_sector(disk, sector),
				n, buf) != GRUB_ERR_NONE)
				goto finish;

			while (n--)
			{
				grub_disk_cache_invalidate(disk->dev->id, disk->id, sector);
				sector += (1ULL << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS));
			}

			buf = (const char*)buf + len;
			size -= len;
		}
	}

finish:

	return grub_errno;
}

grub_uint64_t
grub_disk_native_sectors(grub_disk_t disk)
{
	if (disk->partition)
		return grub_partition_get_len(disk->partition);
	else if (disk->total_sectors != GRUB_DISK_SIZE_UNKNOWN)
		return disk->total_sectors << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS);
	else
		return GRUB_DISK_SIZE_UNKNOWN;
}

void
grub_disk_dev_init(void)
{
	grub_disk_dev_register(&grub_procdisk_dev);
	grub_disk_dev_register(&grub_loopback_dev);
	grub_disk_dev_register(&grub_windisk_dev);
}
