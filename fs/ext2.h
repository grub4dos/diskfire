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
#ifndef _EXT2_HEADER
#define _EXT2_HEADER 1

#include "compat.h"

/* The ext2 superblock.  */
struct grub_ext2_sblock
{
	grub_uint32_t total_inodes;
	grub_uint32_t total_blocks;
	grub_uint32_t reserved_blocks;
	grub_uint32_t free_blocks;
	grub_uint32_t free_inodes;
	grub_uint32_t first_data_block;
	grub_uint32_t log2_block_size;
	grub_uint32_t log2_fragment_size;
	grub_uint32_t blocks_per_group;
	grub_uint32_t fragments_per_group;
	grub_uint32_t inodes_per_group;
	grub_uint32_t mtime;
	grub_uint32_t utime;
	grub_uint16_t mnt_count;
	grub_uint16_t max_mnt_count;
	grub_uint16_t magic;
	grub_uint16_t fs_state;
	grub_uint16_t error_handling;
	grub_uint16_t minor_revision_level;
	grub_uint32_t lastcheck;
	grub_uint32_t checkinterval;
	grub_uint32_t creator_os;
	grub_uint32_t revision_level;
	grub_uint16_t uid_reserved;
	grub_uint16_t gid_reserved;
	grub_uint32_t first_inode;
	grub_uint16_t inode_size;
	grub_uint16_t block_group_number;
	grub_uint32_t feature_compatibility;
	grub_uint32_t feature_incompat;
	grub_uint32_t feature_ro_compat;
	grub_uint16_t uuid[8];
	char volume_name[16];
	char last_mounted_on[64];
	grub_uint32_t compression_info;
	grub_uint8_t prealloc_blocks;
	grub_uint8_t prealloc_dir_blocks;
	grub_uint16_t reserved_gdt_blocks;
	grub_uint8_t journal_uuid[16];
	grub_uint32_t journal_inum;
	grub_uint32_t journal_dev;
	grub_uint32_t last_orphan;
	grub_uint32_t hash_seed[4];
	grub_uint8_t def_hash_version;
	grub_uint8_t jnl_backup_type;
	grub_uint16_t group_desc_size;
	grub_uint32_t default_mount_opts;
	grub_uint32_t first_meta_bg;
	grub_uint32_t mkfs_time;
	grub_uint32_t jnl_blocks[17];
};

#endif
