// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _FBFS_HEADER
#define _FBFS_HEADER 1

#include "compat.h"

#define FB_MAGIC			"FBBF"
#define FB_MAGIC_LONG		0x46424246

#define FB_AR_MAGIC			"FBAR"
#define FB_AR_MAGIC_LONG	0x52414246

PRAGMA_BEGIN_PACKED
struct fb_mbr
{
	grub_uint8_t jmp_code;
	grub_uint8_t jmp_ofs;
	grub_uint8_t boot_code[0x1ab];
	grub_uint8_t max_sec;			/* 0x1ad  */
	grub_uint16_t lba;				/* 0x1ae  */
	grub_uint8_t spt;				/* 0x1b0  */
	grub_uint8_t heads;				/* 0x1b1  */
	grub_uint16_t boot_base;		/* 0x1b2  */
	grub_uint32_t fb_magic;			/* 0x1b4  */
	grub_uint8_t mbr_table[0x46];	/* 0x1b8  */
	grub_uint16_t end_magic;		/* 0x1fe  */
};

struct fb_data
{
	grub_uint16_t boot_size;		/* 0x200  */
	grub_uint16_t flags;			/* 0x202  */
	grub_uint8_t ver_major;			/* 0x204  */
	grub_uint8_t ver_minor;			/* 0x205  */
	grub_uint16_t list_used;		/* 0x206  */
	grub_uint16_t list_size;		/* 0x208  */
	grub_uint16_t pri_size;			/* 0x20a  */
	grub_uint32_t ext_size;			/* 0x20c  */
};

struct fb_ar_data
{
	grub_uint32_t ar_magic;		/* 0x200  */
	grub_uint8_t ver_major;		/* 0x204  */
	grub_uint8_t ver_minor;		/* 0x205  */
	grub_uint16_t list_used;	/* 0x206  */
	grub_uint16_t list_size;	/* 0x208  */
	grub_uint16_t pri_size;		/* 0x20a  */
	grub_uint32_t ext_size;		/* 0x20c  */
};

struct fbm_file
{
	grub_uint8_t size;
	grub_uint8_t flag;
	grub_uint32_t data_start;
	grub_uint32_t data_size;
	grub_uint32_t data_time;
	char name[1];
};
PRAGMA_END_PACKED

#endif
