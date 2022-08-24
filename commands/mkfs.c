// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "command.h"
#include "misc.h"

#include <stdio.h>

static ULONG
get_cluster_size(const char* str)
{
	const char* p = NULL;
	ULONG sz = 0;
	ULONG unit = 1;
	sz = grub_strtoul(str, &p, 10);
	if (*p == 'K' || *p == 'k')
		unit = 1024;
	else if (*p == 'M' || *p == 'm')
		unit = 1024 * 1024;
	return sz * unit;
}

static wchar_t*
get_volume_label(const wchar_t* volume)
{
	size_t len = wcslen(volume) + 2;
	wchar_t* path = calloc(len, sizeof(wchar_t));
	wchar_t label[MAX_PATH + 1] = L"\0";
	if (!path)
		return NULL;
	swprintf(path, len, L"%s\\", volume);
	GetVolumeInformationW(path, label, MAX_PATH + 1, NULL, NULL, NULL, NULL, 0);
	free(path);
	return _wcsdup(label);
}

/*
 * https://github.com/pbatard/rufus/blob/master/src/format.c
 * FormatEx callback. Return FALSE to halt operations
 */
static BOOLEAN __stdcall FormatExCallback(FILE_SYSTEM_CALLBACK_COMMAND Command, DWORD Action, PVOID pData)
{
	(void)Action;
	switch (Command)
	{
	case FCC_PROGRESS:
		break;
	case FCC_STRUCTURE_PROGRESS:	// No progress on quick format
		break;
	case FCC_DONE:
		if (*(BOOLEAN*)pData == FALSE)
			grub_error(GRUB_ERR_IO, "error while formatting");
		break;
	case FCC_DONE_WITH_STRUCTURE:
		break;
	case FCC_INCOMPATIBLE_FILE_SYSTEM:
		grub_error(GRUB_ERR_BAD_FS, "incompatible fs");
		break;
	case FCC_ACCESS_DENIED:
		grub_error(GRUB_ERR_ACCESS_DENIED, "access denied");
		break;
	case FCC_MEDIA_WRITE_PROTECTED:
		grub_error(GRUB_ERR_WRITE_ERROR, "write protected media");
		break;
	case FCC_VOLUME_IN_USE:
		grub_error(GRUB_ERR_WRITE_ERROR, "volume in use");
		break;
	case FCC_DEVICE_NOT_READY:
		grub_error(GRUB_ERR_BAD_DEVICE, "device not ready");
		break;
	case FCC_CANT_QUICK_FORMAT:
		grub_error(GRUB_ERR_IO, "cannot quick format");
		break;
	case FCC_BAD_LABEL:
		grub_error(GRUB_ERR_BAD_ARGUMENT, "bad label");
		break;
	case FCC_OUTPUT:
		//OutputUTF8Message(((PTEXTOUTPUT)pData)->Output);
		break;
	case FCC_CLUSTER_SIZE_TOO_BIG:
	case FCC_CLUSTER_SIZE_TOO_SMALL:
		grub_error(GRUB_ERR_BAD_ARGUMENT, "unsupported cluster size");
		break;
	case FCC_VOLUME_TOO_BIG:
	case FCC_VOLUME_TOO_SMALL:
		grub_error(GRUB_ERR_BAD_ARGUMENT, "volume too %s", (Command == FCC_VOLUME_TOO_BIG) ? "big" : "small");
		break;
	case FCC_NO_MEDIA_IN_DRIVE:
		grub_error(GRUB_ERR_BAD_DEVICE, "no media in drive");
		break;
	case FCC_ALIGNMENT_VIOLATION:
		grub_error(GRUB_ERR_BAD_DEVICE, "partition start offset is not aligned to the cluster size");
		break;
	default:
		grub_error(GRUB_ERR_BAD_ARGUMENT, "FormatExCallback: Received unhandled command 0x%02X - aborting", Command);
		break;
	}
	return (grub_errno ? FALSE : TRUE);
}

static grub_err_t
cmd_mkfs(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	BOOL ntfs_compression = FALSE;
	BOOL quick_format = FALSE;
	BOOL keep_label = FALSE;
	WCHAR* drive = NULL;
	WCHAR* fs = NULL;
	WCHAR* label = NULL;
	ULONG sz = 0;
	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "-f=", 3) == 0)
			fs = grub_get_utf16(&argv[i][3]);
		else if (grub_strncmp(argv[i], "-v=", 3) == 0)
			label = grub_get_utf16(&argv[i][3]);
		else if (grub_strcmp(argv[i], "-v") == 0)
			keep_label = TRUE;
		else if (grub_strcmp(argv[i], "-c") == 0)
			ntfs_compression = TRUE;
		else if (grub_strcmp(argv[i], "-q") == 0)
			quick_format = TRUE;
		else if (grub_strncmp(argv[i], "-a=", 3) == 0)
			sz = get_cluster_size(&argv[i][3]);
		else
			drive = grub_get_utf16(argv[i]);
	}
	if (!drive)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing volume path");
		goto fail;
	}
	if (!fs)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing fs type");
		goto fail;
	}
	if (keep_label && !label)
		label = get_volume_label(drive);
	FmifsFormatEx(drive, fs, label, quick_format, ntfs_compression, sz, FormatExCallback);
fail:
	if (drive)
		free(drive);
	if (fs)
		free(fs);
	if (label)
		free(label);
	return grub_errno;
}

static void
help_mkfs(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] DRIVE_LETTER|MOUNT_POINT\n", cmd->name);
	grub_printf("Format a disk.\nOPTIONS:\n");
	grub_printf("  -f=TYPE    Specify filesystem type (FAT|FAT32|exFAT|NTFS).\n");
	grub_printf("  -v[=TEXT]  Specify filesystem label.\n");
	grub_printf("  -c         Enable NTFS compression.\n");
	grub_printf("  -q         Perform quick format.\n");
	grub_printf("  -a=SIZE    Override the default allocation unit size.\n");
	grub_printf("             NTFS supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K, 128K, 256K, 512K, 1M, 2M.\n");
	grub_printf("             FAT supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K.\n");
	grub_printf("             FAT32 supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K.\n");
	grub_printf("             exFAT supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K, 128K, 256K, 512K, 1M, 2M, 4M, 8M, 16M, 32M.\n");
	grub_printf("             NTFS compression is not supported for allocation unit sizes above 4096.\n");
}

struct grub_command grub_cmd_mkfs =
{
	.name = "mkfs",
	.func = cmd_mkfs,
	.help = help_mkfs,
	.next = 0,
};
