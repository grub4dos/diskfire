// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _EXFAT_HEADER
#define _EXFAT_HEADER 1

#include "compat.h"

PRAGMA_BEGIN_PACKED
struct grub_exfat_bpb
{
	grub_uint8_t jmp_boot[3];
	grub_uint8_t oem_name[8];
	grub_uint8_t mbz[53];
	grub_uint64_t num_hidden_sectors;
	grub_uint64_t num_total_sectors;
	grub_uint32_t num_reserved_sectors;
	grub_uint32_t sectors_per_fat;
	grub_uint32_t cluster_offset;
	grub_uint32_t cluster_count;
	grub_uint32_t root_cluster;
	grub_uint32_t num_serial;
	grub_uint16_t fs_revision;
	grub_uint16_t volume_flags;
	grub_uint8_t bytes_per_sector_shift;
	grub_uint8_t sectors_per_cluster_shift;
	grub_uint8_t num_fats;
	grub_uint8_t num_ph_drive;
	grub_uint8_t reserved[8];
};
PRAGMA_END_PACKED

#endif
