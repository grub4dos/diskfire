// SPDX-License-Identifier: GPL-3.0-or-later

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
            grub_uint8_t num_ph_drive;
            grub_uint8_t reserved;
            grub_uint8_t boot_sig;
            grub_uint32_t num_serial;
            grub_uint8_t label[11];
            grub_uint8_t fstype[8];
        } fat12_or_fat16;
        struct
        {
            grub_uint32_t sectors_per_fat_32;
            grub_uint16_t extended_flags;
            grub_uint16_t fs_version;
            grub_uint32_t root_cluster;
            grub_uint16_t fs_info;
            grub_uint16_t backup_boot_sector;
            grub_uint8_t reserved[12];
            grub_uint8_t num_ph_drive;
            grub_uint8_t reserved1;
            grub_uint8_t boot_sig;
            grub_uint32_t num_serial;
            grub_uint8_t label[11];
            grub_uint8_t fstype[8];
        } fat32;
    } version_specific;
};
PRAGMA_END_PACKED

#endif
