// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _MSDOS_PARTITION_HEADER
#define _MSDOS_PARTITION_HEADER 1

#include "compat.h"
#include "disk.h"
#include "partition.h"

/* The signature.  */
#define GRUB_PC_PARTITION_SIGNATURE		0xaa55

/* This is not a flag actually, but used as if it were a flag.  */
#define GRUB_PC_PARTITION_TYPE_HIDDEN_FLAG	0x10

/* DOS partition types.  */
#define GRUB_PC_PARTITION_TYPE_NONE		0
#define GRUB_PC_PARTITION_TYPE_FAT12		1
#define GRUB_PC_PARTITION_TYPE_FAT16_LT32M	4
#define GRUB_PC_PARTITION_TYPE_EXTENDED		5
#define GRUB_PC_PARTITION_TYPE_FAT16_GT32M	6
#define GRUB_PC_PARTITION_TYPE_NTFS	        7
#define GRUB_PC_PARTITION_TYPE_FAT32		0xb
#define GRUB_PC_PARTITION_TYPE_FAT32_LBA	0xc
#define GRUB_PC_PARTITION_TYPE_FAT16_LBA	0xe
#define GRUB_PC_PARTITION_TYPE_WIN95_EXTENDED	0xf
#define GRUB_PC_PARTITION_TYPE_PLAN9            0x39
#define GRUB_PC_PARTITION_TYPE_LDM		0x42
#define GRUB_PC_PARTITION_TYPE_EZD		0x55
#define GRUB_PC_PARTITION_TYPE_MINIX		0x80
#define GRUB_PC_PARTITION_TYPE_LINUX_MINIX	0x81
#define GRUB_PC_PARTITION_TYPE_LINUX_SWAP	0x82
#define GRUB_PC_PARTITION_TYPE_EXT2FS		0x83
#define GRUB_PC_PARTITION_TYPE_LINUX_EXTENDED	0x85
#define GRUB_PC_PARTITION_TYPE_VSTAFS		0x9e
#define GRUB_PC_PARTITION_TYPE_FREEBSD		0xa5
#define GRUB_PC_PARTITION_TYPE_OPENBSD		0xa6
#define GRUB_PC_PARTITION_TYPE_NETBSD		0xa9
#define GRUB_PC_PARTITION_TYPE_HFS		0xaf
#define GRUB_PC_PARTITION_TYPE_GPT_DISK		0xee
#define GRUB_PC_PARTITION_TYPE_LINUX_RAID	0xfd

PRAGMA_BEGIN_PACKED
/* The partition entry.  */
struct grub_msdos_partition_entry
{
	/* If active, 0x80, otherwise, 0x00.  */
	grub_uint8_t flag;

	/* The head of the start.  */
	grub_uint8_t start_head;

	/* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
	   is the cylinder of the start. Note that S is counted from one.  */
	grub_uint8_t start_sector;

	/* (C & 0xFF) where C is the cylinder of the start.  */
	grub_uint8_t start_cylinder;

	/* The partition type.  */
	grub_uint8_t type;

	/* The end versions of start_head, start_sector and start_cylinder,
	   respectively.  */
	grub_uint8_t end_head;
	grub_uint8_t end_sector;
	grub_uint8_t end_cylinder;

	/* The start sector. Note that this is counted from zero.  */
	grub_uint32_t start;

	/* The length in sector units.  */
	grub_uint32_t length;
};

/* The structure of MBR.  */
struct grub_msdos_partition_mbr
{
	/* The code area (actually, including BPB).  */
	union
	{
		grub_uint8_t code[446];
		struct
		{
			grub_uint8_t code1[218];
			grub_uint8_t timestamp[6];
			grub_uint8_t code2[216];
			grub_uint8_t disk_signature[4];
			grub_uint16_t disk_flag; // 0x0000, 0x5a5a = copy-protected
		} modern_code;
	} bootstrap;

	/* Four partition entries.  */
	struct grub_msdos_partition_entry entries[4];

	/* The signature 0xaa55.  */
	grub_uint16_t signature;
};
PRAGMA_END_PACKED

static inline int
grub_msdos_partition_is_empty(int type)
{
	return (type == GRUB_PC_PARTITION_TYPE_NONE);
}

static inline int
grub_msdos_partition_is_extended(int type)
{
	return (type == GRUB_PC_PARTITION_TYPE_EXTENDED
		|| type == GRUB_PC_PARTITION_TYPE_WIN95_EXTENDED
		|| type == GRUB_PC_PARTITION_TYPE_LINUX_EXTENDED);
}

grub_err_t
grub_partition_msdos_iterate(grub_disk_t disk,
	grub_partition_iterate_hook_t hook,
	void* hook_data);

#endif
