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
	const grub_uint8_t* code;
	grub_size_t code_size;
	grub_disk_addr_t reserved_sectors;
	int (*identify) (grub_uint8_t *sector);
	grub_err_t(*install) (grub_disk_t disk, void* options);
};
typedef struct grub_br* grub_br_t;

extern grub_br_t grub_br_list;

void grub_br_register(grub_br_t br);

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

const char*
grub_br_get_fs_type(grub_disk_t disk);

grub_disk_addr_t
grub_br_get_fs_reserved_sectors(grub_disk_t disk);

void grub_br_init(void);

extern struct grub_br grub_mbr_nt6;
extern struct grub_br grub_mbr_nt5;
extern struct grub_br grub_mbr_grldr;
extern struct grub_br grub_mbr_grub2;
extern struct grub_br grub_mbr_xorboot;
extern struct grub_br grub_mbr_plop;
extern struct grub_br grub_mbr_fbinst;
extern struct grub_br grub_mbr_ventoy;
extern struct grub_br grub_mbr_ultraiso;
extern struct grub_br grub_mbr_syslinux;
extern struct grub_br grub_mbr_wee;
extern struct grub_br grub_mbr_empty;

extern struct grub_br grub_pbr_grldr;
extern struct grub_br grub_pbr_ntldr;
extern struct grub_br grub_pbr_bootmgr;

#endif
