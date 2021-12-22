// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _BTRFS_HEADER
#define _BTRFS_HEADER 1

#include "compat.h"

enum
{
	GRUB_BTRFS_ITEM_TYPE_INODE_ITEM = 0x01,
	GRUB_BTRFS_ITEM_TYPE_INODE_REF = 0x0c,
	GRUB_BTRFS_ITEM_TYPE_DIR_ITEM = 0x54,
	GRUB_BTRFS_ITEM_TYPE_EXTENT_ITEM = 0x6c,
	GRUB_BTRFS_ITEM_TYPE_ROOT_ITEM = 0x84,
	GRUB_BTRFS_ITEM_TYPE_ROOT_BACKREF = 0x90,
	GRUB_BTRFS_ITEM_TYPE_DEVICE = 0xd8,
	GRUB_BTRFS_ITEM_TYPE_CHUNK = 0xe4
};

enum
{
	GRUB_BTRFS_ROOT_VOL_OBJECTID = 5,
	GRUB_BTRFS_TREE_ROOT_OBJECTID = 0x100,
};

struct grub_btrfs_root_item
{
	grub_uint8_t dummy[0xb0];
	grub_uint64_t tree;
	grub_uint64_t inode;
};

PRAGMA_BEGIN_PACKED
struct grub_btrfs_key
{
	grub_uint64_t object_id;
	grub_uint8_t type;
	grub_uint64_t offset;
};
PRAGMA_END_PACKED

struct grub_btrfs_root_backref
{
	grub_uint64_t inode_id;
	grub_uint64_t seqnr;
	grub_uint16_t n;
	char name[0];
};

struct grub_btrfs_inode_ref
{
	grub_uint64_t idxid;
	grub_uint16_t n;
	char name[0];
};

#endif
