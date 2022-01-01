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
#ifndef _FAT_HEADER
#define _FAT_HEADER 1

#include "compat.h"

PRAGMA_BEGIN_PACKED
struct grub_fat_bpb
{
	grub_uint8_t jmp_boot[3];
	grub_uint8_t oem_name[8];
	grub_uint16_t bytes_per_sector;
	grub_uint8_t sectors_per_cluster;
	grub_uint16_t num_reserved_sectors;
	grub_uint8_t num_fats;		/* 0x10 */
	grub_uint16_t num_root_entries;
	grub_uint16_t num_total_sectors_16;
	grub_uint8_t media;			/* 0x15 */
	grub_uint16_t sectors_per_fat_16;
	grub_uint16_t sectors_per_track;	/* 0x18 */
	grub_uint16_t num_heads;		/* 0x1A */
	grub_uint32_t num_hidden_sectors;	/* 0x1C */
	grub_uint32_t num_total_sectors_32;	/* 0x20 */
	union
	{
		struct
		{
			grub_uint8_t num_ph_drive; /* 0x24 */
			grub_uint8_t reserved; /* 0x25 */
			grub_uint8_t boot_sig; /* 0x26 */
			grub_uint32_t num_serial; /* 0x27 */
			grub_uint8_t label[11]; /* 0x2B */
			grub_uint8_t fstype[8]; /* 0x36 */
		} fat12_or_fat16; /* 0x3E */
		struct
		{
			grub_uint32_t sectors_per_fat_32; /* 0x24 */
			grub_uint16_t extended_flags; /* 0x28 */
			grub_uint16_t fs_version; /* 0x2A */
			grub_uint32_t root_cluster; /* 0x2C */
			grub_uint16_t fs_info; /* 0x30 */
			grub_uint16_t backup_boot_sector; /* 0x32 */
			grub_uint8_t reserved[12]; /* 0x34 */
			grub_uint8_t num_ph_drive; /* 0x40 */
			grub_uint8_t reserved1; /* 0x41 */
			grub_uint8_t boot_sig; /* 0x42 */
			grub_uint32_t num_serial; /* 0x43 */
			grub_uint8_t label[11]; /* 0x47 */
			grub_uint8_t fstype[8]; /* 0x52 */
		} fat32; /* 0x5A */
	} version_specific;
};
PRAGMA_END_PACKED

struct grub_fat_data;
struct grub_fat_data* grub_fat_mount(grub_disk_t disk);

#endif
