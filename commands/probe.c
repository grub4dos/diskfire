// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "partition.h"
#include "gpt_partition.h"
#include "msdos_partition.h"
#include "fs.h"
#include "file.h"
#include "command.h"
#include "misc.h"

static const char*
guid_to_str(grub_packed_guid_t *guid)
{
	static char str[37] = { 0 };
	grub_snprintf(str, sizeof(str), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		grub_le_to_cpu32(guid->data1),
		grub_le_to_cpu16(guid->data2),
		grub_le_to_cpu16(guid->data3),
		guid->data4[0], guid->data4[1], guid->data4[2],
		guid->data4[3], guid->data4[4], guid->data4[5],
		guid->data4[6], guid->data4[7]);
	return str;
}

static int probe_partmap_hook(struct grub_disk* disk, const grub_partition_t partition, void* data)
{
	(void)disk;
	(void)partition;
	(void)data;
	return 1;
}

static grub_err_t probe_partmap(grub_disk_t disk)
{
	grub_disk_t parent = 0;
	grub_partition_map_t partmap;
	if (disk->partition && disk->partition->partmap->name)
	{
		grub_printf("%s", disk->partition->partmap->name);
		return GRUB_ERR_NONE;
	}
	parent = grub_disk_open(disk->name);
	if (!parent)
		return grub_errno;
	FOR_PARTITION_MAPS(partmap)
	{
		if (partmap->iterate(parent, probe_partmap_hook, NULL) == GRUB_ERR_NONE)
		{
			grub_printf("%s", partmap->name);
			goto out;
		}
		grub_errno = GRUB_ERR_NONE;
	}
	grub_printf("none");
out:
	grub_errno = GRUB_ERR_NONE;
	grub_disk_close(parent);
	return grub_errno;
}

static grub_err_t probe_fs(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->name)
		grub_printf("%s", fs->name);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_fsuuid(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->fs_uuid)
	{
		char* uuid;
		(fs->fs_uuid) (disk, &uuid);
		if (grub_errno == GRUB_ERR_NONE && uuid)
		{
			if (grub_strlen(uuid))
				grub_printf("%s", uuid);
			grub_free(uuid);
			grub_errno = GRUB_ERR_NONE;
		}
	}
	return grub_errno;
}

static grub_err_t probe_label(grub_disk_t disk)
{
	grub_fs_t fs;
	fs = grub_fs_probe(disk);
	if (fs && fs->fs_label)
	{
		char* label;
		(fs->fs_label) (disk, &label);
		if (grub_errno == GRUB_ERR_NONE && label)
		{
			if (grub_strlen(label))
				grub_printf("%s", label);
			grub_free(label);
			grub_errno = GRUB_ERR_NONE;
		}
	}
	return grub_errno;
}

static grub_err_t probe_size(grub_disk_t disk)
{
	grub_printf("%llu", grub_disk_native_sectors(disk) << GRUB_DISK_SECTOR_BITS);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_startlba(grub_disk_t disk)
{
	grub_printf("%llu", disk->partition ? grub_partition_get_start(disk->partition) : 0);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_bus(grub_disk_t disk)
{
	if (disk->type != GRUB_DISK_WINDISK_ID)
		return GRUB_ERR_NONE;
	grub_printf("%s", GetBusTypeString(gDriveList[disk->id].BusType));
	return GRUB_ERR_NONE;
}

static grub_err_t probe_removable(grub_disk_t disk)
{
	if (disk->type != GRUB_DISK_WINDISK_ID)
		return GRUB_ERR_NONE;
	grub_printf("%s", gDriveList[disk->id].RemovableMedia ? "REMOVABLE" : "FIXED");
	return GRUB_ERR_NONE;
}

static grub_err_t probe_product(grub_disk_t disk)
{
	if (disk->type != GRUB_DISK_WINDISK_ID)
		return GRUB_ERR_NONE;
	grub_printf("%s", gDriveList[disk->id].ProductId);
	return GRUB_ERR_NONE;
}

static grub_err_t probe_vendor(grub_disk_t disk)
{
	if (disk->type != GRUB_DISK_WINDISK_ID)
		return GRUB_ERR_NONE;
	grub_printf("%s", gDriveList[disk->id].VendorId);
	return GRUB_ERR_NONE;
}

#define U64_SECTOR_SIZE (GRUB_DISK_SECTOR_SIZE / sizeof(grub_uint64_t))

static int sector_cmp(const grub_uint64_t* src, const grub_uint64_t* dst)
{
	unsigned i;
	for (i = 0; i < U64_SECTOR_SIZE; i++)
	{
		if (src[i] != dst[i])
			return 1;
	}
	return 0;
}

static grub_err_t probe_letter(grub_disk_t disk)
{
	int i;
	char path[] = "\\\\.\\C:";
	grub_uint64_t src[U64_SECTOR_SIZE] = { 0 };
	grub_uint64_t dst[U64_SECTOR_SIZE] = { 0 };
	if (disk->type != GRUB_DISK_WINDISK_ID || !gDriveList[disk->id].DriveLetters[0])
		goto fail;
	if (!disk->partition)
	{
		for (i = 0; gDriveList[disk->id].DriveLetters[i] && i < 26; i++)
			grub_printf(" %C:", gDriveList[disk->id].DriveLetters[i]);
		goto fail;
	}
	if (grub_disk_read(disk, 0, 0, sizeof(src), src) != GRUB_ERR_NONE)
		goto fail;
	for (i = 0; gDriveList[disk->id].DriveLetters[i] && i < 26; i++)
	{
		
		HANDLE fd = INVALID_HANDLE_VALUE;
		DWORD dwsize = sizeof(dst);
		grub_snprintf(path, sizeof(path), "\\\\.\\%C:", gDriveList[disk->id].DriveLetters[i]);
		fd = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		if (!fd || fd == INVALID_HANDLE_VALUE)
			continue;
		if (!ReadFile(fd, dst, dwsize, &dwsize, NULL))
		{
			CHECK_CLOSE_HANDLE(fd);
			continue;
		}
		if (sector_cmp(src, dst) == 0)
		{
			grub_printf("%C:", gDriveList[disk->id].DriveLetters[i]);
			break;
		}
	}
fail:
	grub_errno = GRUB_ERR_NONE;
	return GRUB_ERR_NONE;
}

#define GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC 0x1b8

static grub_err_t probe_partuuid(grub_disk_t disk)
{
	struct grub_partition* p = disk->partition;
	grub_disk_t parent = NULL;
	if (!p)
		goto fail;
	parent = grub_disk_open(disk->name);
	if (!parent)
		goto fail;
	if (grub_strcmp(disk->partition->partmap->name, "gpt") == 0)
	{
		struct grub_gpt_partentry entry;
		grub_gpt_part_guid_t* guid;

		if (grub_disk_read(parent, p->offset, p->index, sizeof(entry), &entry))
			goto fail;

		guid = &entry.guid;
		grub_printf("%s", guid_to_str(guid));
	}
	else if (grub_strcmp(disk->partition->partmap->name, "msdos") == 0)
	{
		grub_uint32_t nt_disk_sig;
		if (grub_disk_read(parent, 0, GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC, sizeof(nt_disk_sig), &nt_disk_sig))
			goto fail;
		grub_printf("%08x-%02x", grub_le_to_cpu32(nt_disk_sig), 1 + p->number);
	}
fail:
	if (parent)
		grub_disk_close(parent);
	grub_errno = GRUB_ERR_NONE;
	return grub_errno;
}

struct gpt_type_info
{
	grub_packed_guid_t guid;
	const char* name;
};

static struct gpt_type_info gpt_list[] =
{
	{ GRUB_GPT_PARTITION_TYPE_EMPTY, "EMPTY" },
	{ GRUB_GPT_PARTITION_TYPE_EFI_SYSTEM, "ESP" },
	{ GRUB_GPT_PARTITION_TYPE_BIOS_BOOT, "BIOS_BOOT" },
	{ GRUB_GPT_PARTITION_TYPE_LEGACY_MBR, "MBR" },
	{ GRUB_GPT_PARTITION_TYPE_LDM, "LDM" },
	{ GRUB_GPT_PARTITION_TYPE_LDM_DATA, "LDM_DATA" },
	{ GRUB_GPT_PARTITION_TYPE_MSR, "MSR" },
	{ GRUB_GPT_PARTITION_TYPE_WINRE, "WINRE" },
};

static const char* gpt_type_to_str(grub_packed_guid_t* guid)
{
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(gpt_list); i++)
	{
		if (grub_memcmp(guid, &gpt_list[i].guid, sizeof(grub_packed_guid_t)) == 0)
			return gpt_list[i].name;
	}
	return "DATA";
}

static grub_err_t probe_partflag(grub_disk_t disk)
{
	if (!disk->partition)
		goto fail;
	if (grub_strcmp(disk->partition->partmap->name, "gpt") == 0)
	{
		grub_printf("%s %s", guid_to_str(&disk->partition->gpttype), gpt_type_to_str(&disk->partition->gpttype));
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_REQUIRED))
			grub_printf(" OEM");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_NO_BLOCK_IO_PROTOCOL))
			grub_printf(" EFI_IGNORE");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_LEGACY_BIOS_BOOTABLE))
			grub_printf(" BIOS_ACTIVE");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_READ_ONLY))
			grub_printf(" READ_ONLY");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_SHADOW_COPY))
			grub_printf(" SHADOW_COPY");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_HIDDEN))
			grub_printf(" HIDDEN");
		if (grub_gpt_entry_attribute(disk->partition->flag, GRUB_GPT_PART_ATTR_OFFSET_NO_AUTO))
			grub_printf(" NO_LETTER");
	}
	else if (grub_strcmp(disk->partition->partmap->name, "msdos") == 0)
	{
		grub_printf("%s %02X%s%s",
			disk->partition->offset ? "EXTENDED" : "PRIMARY",
			disk->partition->msdostype,
			disk->partition->msdostype & GRUB_PC_PARTITION_TYPE_HIDDEN_FLAG ? " HIDDEN" : "",
			disk->partition->flag & 0x80 ? " ACTIVE" : "");
	}
fail:
	return GRUB_ERR_NONE;
}

static grub_err_t
parse_probe_opt(const char* arg, grub_disk_t disk)
{
	if (grub_strcmp(arg, "--partmap") == 0)
		return probe_partmap(disk);
	if (grub_strcmp(arg, "--fs") == 0)
		return probe_fs(disk);
	if (grub_strcmp(arg, "--fsuuid") == 0)
		return probe_fsuuid(disk);
	if (grub_strcmp(arg, "--label") == 0)
		return probe_label(disk);
	if (grub_strcmp(arg, "--size") == 0)
		return probe_size(disk);
	if (grub_strcmp(arg, "--start") == 0)
		return probe_startlba(disk);
	if (grub_strcmp(arg, "--bus") == 0)
		return probe_bus(disk);
	if (grub_strcmp(arg, "--rm") == 0)
		return probe_removable(disk);
	if (grub_strcmp(arg, "--pid") == 0)
		return probe_product(disk);
	if (grub_strcmp(arg, "--vid") == 0)
		return probe_vendor(disk);
	if (grub_strcmp(arg, "--letter") == 0)
		return probe_letter(disk);
	if (grub_strcmp(arg, "--partuuid") == 0)
		return probe_partuuid(disk);
	if (grub_strcmp(arg, "--flag") == 0)
		return probe_partflag(disk);

	return grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid option %s\n", arg);
}

static int
list_disk_iter(const char* name, void* data)
{
	const char* arg = data;
	grub_disk_t disk = 0;
	disk = grub_disk_open(name);
	if (arg && disk)
	{
		grub_printf("%s ", name);
		parse_probe_opt(arg, disk);
		grub_printf("\n");
	}
	if (disk)
		grub_disk_close(disk);
	grub_errno = GRUB_ERR_NONE;
	return 0;
}

static grub_err_t
list_disk(char* arg)
{
	grub_disk_iterate(list_disk_iter, arg);
	return GRUB_ERR_NONE;
}

static grub_err_t
cmd_probe(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	char* ptr;
	grub_disk_t disk = 0;
	if (argc < 1)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing arguments");
		goto fail;
	}
	if (argc == 1)
		return list_disk(argv[0]);
	ptr = argv[1] + grub_strlen(argv[1]) - 1;
	if (argv[1][0] == '(' && *ptr == ')')
	{
		*ptr = 0;
		disk = grub_disk_open(argv[1] + 1);
		*ptr = ')';
	}
	else
		disk = grub_disk_open(argv[1]);
	if (!disk)
		goto fail;
	parse_probe_opt(argv[0], disk);
	grub_printf("\n");
fail:
	if (disk)
		grub_disk_close(disk);
	return grub_errno;
}

static void
help_probe(struct grub_command* cmd)
{
	grub_printf("%s OPTIONS [DISK]\n", cmd->name);
	grub_printf("Retrieve disk info.\nOPTIONS:\n");
	grub_printf("  --partmap   Determine partition map type.\n");
	grub_printf("  --fs        Determine filesystem type.\n");
	grub_printf("  --fsuuid    Determine filesystem UUID.\n");
	grub_printf("  --label     Determine filesystem label.\n");
	grub_printf("  --size      Determine disk/partition size (bytes).\n");
	grub_printf("  --start     Determine partition starting LBA (sectors).\n");

	grub_printf("  --bus       Determine bus type.\n");
	grub_printf("  --rm        Determine if the disk is removable.\n");
	grub_printf("  --pid       Determine product id.\n");
	grub_printf("  --vid       Determine vendor id.\n");
	grub_printf("  --letter    Determine drive letters.\n");

	grub_printf("  --partuuid  Determine partition UUID.\n");
	grub_printf("  --flag      Determine partition flags.\n");
}

struct grub_command grub_cmd_probe =
{
	.name = "probe",
	.func = cmd_probe,
	.help = help_probe,
	.next = 0,
};
