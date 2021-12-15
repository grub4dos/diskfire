// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _FSHELP_HEADER
#define _FSHELP_HEADER 1

#include "compat.h"
#include "fs.h"
#include "disk.h"

typedef struct grub_fshelp_node* grub_fshelp_node_t;

#define GRUB_FSHELP_CASE_INSENSITIVE	0x100
#define GRUB_FSHELP_TYPE_MASK	0xff
#define GRUB_FSHELP_FLAGS_MASK	0x100

enum grub_fshelp_filetype
{
	GRUB_FSHELP_UNKNOWN,
	GRUB_FSHELP_REG,
	GRUB_FSHELP_DIR,
	GRUB_FSHELP_SYMLINK
};

typedef int (*grub_fshelp_iterate_dir_hook_t) (const char* filename,
	enum grub_fshelp_filetype filetype,
	grub_fshelp_node_t node,
	void* data);

/* Lookup the node PATH.  The node ROOTNODE describes the root of the
   directory tree.  The node found is returned in FOUNDNODE, which is
   either a ROOTNODE or a new malloc'ed node.  ITERATE_DIR is used to
   iterate over all directory entries in the current node.
   READ_SYMLINK is used to read the symlink if a node is a symlink.
   EXPECTTYPE is the type node that is expected by the called, an
   error is generated if the node is not of the expected type.  */
grub_err_t
grub_fshelp_find_file(const char* path,
	grub_fshelp_node_t rootnode,
	grub_fshelp_node_t* foundnode,
	int (*iterate_dir) (grub_fshelp_node_t dir,
		grub_fshelp_iterate_dir_hook_t hook,
		void* hook_data),
	char* (*read_symlink) (grub_fshelp_node_t node),
	enum grub_fshelp_filetype expect);


grub_err_t
grub_fshelp_find_file_lookup(const char* path,
	grub_fshelp_node_t rootnode,
	grub_fshelp_node_t* foundnode,
	grub_err_t(*lookup_file) (grub_fshelp_node_t dir,
		const char* name,
		grub_fshelp_node_t* foundnode,
		enum grub_fshelp_filetype* foundtype),
	char* (*read_symlink) (grub_fshelp_node_t node),
	enum grub_fshelp_filetype expect);

/* Read LEN bytes from the file NODE on disk DISK into the buffer BUF,
   beginning with the block POS.  READ_HOOK should be set before
   reading a block from the file.  GET_BLOCK is used to translate file
   blocks to disk blocks.  The file is FILESIZE bytes big and the
   blocks have a size of LOG2BLOCKSIZE (in log2).  */
grub_ssize_t
grub_fshelp_read_file(grub_disk_t disk, grub_fshelp_node_t node,
	grub_disk_read_hook_t read_hook,
	void* read_hook_data,
	grub_off_t pos, grub_size_t len, char* buf,
	grub_disk_addr_t(*get_block) (grub_fshelp_node_t node,
		grub_disk_addr_t block),
	grub_off_t filesize, int log2blocksize,
	grub_disk_addr_t blocks_start);

#endif
