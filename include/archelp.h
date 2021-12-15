// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _ARCHELP_HEADER
#define _ARCHELP_HEADER 1

#include "compat.h"
#include "fs.h"
#include "file.h"

typedef enum
{
	GRUB_ARCHELP_ATTR_TYPE = 0160000,
	GRUB_ARCHELP_ATTR_FILE = 0100000,
	GRUB_ARCHELP_ATTR_DIR = 0040000,
	GRUB_ARCHELP_ATTR_LNK = 0120000,
	GRUB_ARCHELP_ATTR_NOTIME = 0x80000000,
	GRUB_ARCHELP_ATTR_END = 0xffffffff
} grub_archelp_mode_t;

struct grub_archelp_data;

struct grub_archelp_ops
{
	grub_err_t
	(*find_file) (struct grub_archelp_data* data, char** name,
		grub_int32_t* mtime,
		grub_archelp_mode_t* mode);

	char*
		(*get_link_target) (struct grub_archelp_data* data);

	void
	(*rewind) (struct grub_archelp_data* data);
};

grub_err_t
grub_archelp_dir(struct grub_archelp_data* data,
	struct grub_archelp_ops* ops,
	const char* path_in,
	grub_fs_dir_hook_t hook, void* hook_data);

grub_err_t
grub_archelp_open(struct grub_archelp_data* data,
	struct grub_archelp_ops* ops,
	const char* name_in);

#endif
