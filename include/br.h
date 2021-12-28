// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _BR_HEADER
#define _NR_HEADER 1

#include "compat.h"
#include "disk.h"

struct grub_br
{
	struct grub_br* next;
	struct grub_br** prev;
	const char* name;
	const char* desc;
	const grub_uint8_t* bootstrap_code;
	grub_disk_addr_t reserved_sectors;
	int (*identify) (grub_uint8_t *sector);
	grub_err_t(*install) (grub_disk_t disk, void* options);
};
typedef struct grub_br* grub_br_t;

extern grub_br_t grub_br_list;

static inline void
grub_br_register(grub_br_t br)
{
	grub_list_push(GRUB_AS_LIST_P(&grub_br_list), GRUB_AS_LIST(br));
}

static inline void
grub_br_unregister(grub_br_t br)
{
	grub_list_remove(GRUB_AS_LIST(br));
}

#define FOR_BOOTRECORDS(var) FOR_LIST_ELEMENTS((var), (grub_br_list))

static inline grub_br_t
grub_br_find(const char* name)
{
	return (grub_br_t)grub_named_list_find(GRUB_AS_NAMED_LIST(grub_br_list), name);
}

grub_disk_addr_t
grub_br_get_reserved_sectors(grub_disk_t disk, grub_disk_addr_t* start);

int
grub_br_check_data(const grub_uint8_t* data, grub_size_t data_len,
	grub_uint32_t offset, const grub_uint8_t* buf, grub_uint32_t buf_len);

grub_br_t
grub_br_probe(grub_disk_t disk);

void grub_br_init(void);

extern struct grub_br grub_mbr_nt6;
extern struct grub_br grub_mbr_nt5;
extern struct grub_br grub_mbr_grldr;
extern struct grub_br grub_mbr_grub2;
extern struct grub_br grub_mbr_xorboot;
extern struct grub_br grub_mbr_empty;

#endif
