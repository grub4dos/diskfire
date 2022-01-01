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

#ifndef _EXFAT_HEADER
#define _EXFAT_HEADER 1

#include "compat.h"

PRAGMA_BEGIN_PACKED
struct grub_exfat_bpb
{
	grub_uint8_t jmp_boot[3];
	grub_uint8_t oem_name[8];
	grub_uint8_t mbz[53];
	grub_uint64_t num_hidden_sectors; /* 0x40 */
	grub_uint64_t num_total_sectors; /* 0x48 */
	grub_uint32_t num_reserved_sectors; /* 0x50 */
	grub_uint32_t sectors_per_fat; /* 0x54 */
	grub_uint32_t cluster_offset; /* 0x58 */
	grub_uint32_t cluster_count; /* 0x5C */
	grub_uint32_t root_cluster; /* 0x60 */
	grub_uint32_t num_serial; /* 0x64 */
	grub_uint16_t fs_revision; /* 0x68 */
	grub_uint16_t volume_flags; /* 0x6A */
	grub_uint8_t bytes_per_sector_shift; /* 0x6C */
	grub_uint8_t sectors_per_cluster_shift; /* 0x6D */
	grub_uint8_t num_fats; /* 0x6E */
	grub_uint8_t num_ph_drive; /* 0x6F */
	grub_uint8_t reserved[8]; /* 0x70 */
};
PRAGMA_END_PACKED

#endif
