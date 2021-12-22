// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _HFS_HEADER
#define _HFS_HEADER 1

#include "compat.h"
#include "disk.h"

#define GRUB_HFS_MAGIC		0x4244

/* A single extent.	 A file consists of one or more extents.  */
PRAGMA_BEGIN_PACKED
struct grub_hfs_extent
{
	/* The first physical block.	*/
	grub_uint16_t first_block;
	grub_uint16_t count;
};
PRAGMA_END_PACKED

/* HFS stores extents in groups of 3.  */
typedef struct grub_hfs_extent grub_hfs_datarecord_t[3];

/* The HFS superblock (The official name is `Master Directory
   Block').	 */
PRAGMA_BEGIN_PACKED
struct grub_hfs_sblock
{
	grub_uint16_t magic;
	grub_uint32_t ctime;
	grub_uint32_t mtime;
	grub_uint8_t unused[10];
	grub_uint32_t blksz;
	grub_uint8_t unused2[4];
	grub_uint16_t first_block;
	grub_uint8_t unused4[6];

	/* A pascal style string that holds the volumename.  */
	grub_uint8_t volname[28];

	grub_uint8_t unused5[28];

	grub_uint32_t ppc_bootdir;
	grub_uint32_t intel_bootfile;
	/* Folder opened when disk is mounted. Unused by GRUB. */
	grub_uint32_t showfolder;
	grub_uint32_t os9folder;
	grub_uint8_t unused6[4];
	grub_uint32_t osxfolder;

	grub_uint64_t num_serial;
	grub_uint16_t embed_sig;
	struct grub_hfs_extent embed_extent;
	grub_uint8_t unused7[4];
	grub_hfs_datarecord_t extent_recs;
	grub_uint32_t catalog_size;
	grub_hfs_datarecord_t catalog_recs;
};
PRAGMA_END_PACKED

#endif
