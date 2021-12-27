// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "br.h"

grub_br_t grub_br_list;

int
grub_br_check_data(grub_uint8_t *data, grub_size_t data_len,
	grub_uint32_t offset, grub_uint8_t *buf, grub_uint32_t buf_len)
{
	if (data_len < (grub_uint64_t)offset + buf_len)
		return 0;
	if (grub_memcmp(data + offset, buf, buf_len) == 0)
		return 1;
	return 0;
}

grub_br_t
grub_br_probe(grub_disk_t disk)
{
	grub_br_t br = NULL;
	grub_uint8_t sector[GRUB_DISK_SECTOR_SIZE];
	if (grub_disk_read(disk, 0, 0, sizeof(sector), sector))
		return NULL;
	FOR_BOOTRECORDS(br)
	{
		if (br->identify(sector))
			return br;
	}
	return NULL;
}

void grub_br_init(void)
{
	grub_br_register(&grub_mbr_nt6);
	grub_br_register(&grub_mbr_nt5);
	grub_br_register(&grub_mbr_empty);
}
