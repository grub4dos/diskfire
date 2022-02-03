// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"

#include "imdisk.h"
#include "misc.h"

static WCHAR find_free_drive(DWORD bits)
{
	int i;
	DWORD mask;
	DWORD list = GetLogicalDrives();

	/* Find from Z-->A */
	for (i = 25; i >= 0; i--)
	{
		mask = (DWORD)(1UL << i);
		if ((bits & mask) > 0 && (list & mask) == 0)
		{
			return (WCHAR)(L'A' + i);
		}
	}

	return L'[';
}

static BOOL
imdisk_mount(PCWSTR FileName, LARGE_INTEGER Offest, LARGE_INTEGER Length, MEDIA_TYPE MediaType, WCHAR DriveLetter)
{
	PIMDISK_CREATE_DATA ImdiskData;
	size_t ImdiskDataSize;
	HANDLE hImdisk;
	BOOL Status;

	hImdisk = CreateFileA("\\\\.\\ImDiskCtl",
		GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (!hImdisk || hImdisk == INVALID_HANDLE_VALUE)
	{
		grub_printf("Imdisk driver open failed.\n");
		return FALSE;
	}

	ImdiskDataSize = sizeof(IMDISK_CREATE_DATA) + (wcslen(FileName) + 1) * sizeof(WCHAR);
	ImdiskData = (PIMDISK_CREATE_DATA)grub_malloc(ImdiskDataSize);
	ImdiskData->DeviceNumber = IMDISK_AUTO_DEVICE_NUMBER;
	ImdiskData->DiskGeometry.MediaType = MediaType;

	if (MediaType == RemovableMedia)
	{
		ImdiskData->DiskGeometry.BytesPerSector = 2048;
		ImdiskData->Flags = IMDISK_OPTION_RO | IMDISK_TYPE_FILE | IMDISK_DEVICE_TYPE_CD;
	}
	else
	{
		ImdiskData->Flags = IMDISK_TYPE_FILE | IMDISK_DEVICE_TYPE_HD;
		ImdiskData->DiskGeometry.MediaType = FixedMedia;
		ImdiskData->DiskGeometry.BytesPerSector = 512;
		ImdiskData->DiskGeometry.SectorsPerTrack = 63;
		ImdiskData->DiskGeometry.TracksPerCylinder = 255;
	}
	ImdiskData->DiskGeometry.Cylinders = Length;
	ImdiskData->ImageOffset = Offest;
	ImdiskData->DriveLetter = ((DriveLetter == 0) ? (find_free_drive(0x7FFFF8)) : DriveLetter);
	ImdiskData->FileNameLength = (USHORT)(wcslen(FileName) * sizeof(WCHAR));
	memcpy(ImdiskData->FileName, FileName, ImdiskData->FileNameLength);

	Status = DeviceIoControl(hImdisk, IOCTL_IMDISK_CREATE_DEVICE, ImdiskData, (DWORD)ImdiskDataSize, NULL, 0, NULL, NULL);
	CloseHandle(hImdisk);
	grub_free(ImdiskData);
	return Status;
}

static grub_err_t cmd_imdisk(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	char full_path[MAX_PATH];
	wchar_t letter = 0;
	wchar_t* path = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	LARGE_INTEGER offset, length;
	size_t path_len;
	MEDIA_TYPE type = FixedMedia;
	if (argc < 1)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing file name");
	if (argc > 1)
		letter = (wchar_t)argv[1][0];
	grub_snprintf(full_path, sizeof(full_path), "\\??\\%s", argv[0]);
	path = grub_get_utf16(full_path);
	if (!path)
		return grub_error(GRUB_ERR_BAD_FILENAME, "out of memory");
	path_len = wcslen(path);
	file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		grub_error(GRUB_ERR_BAD_FILENAME, "file open failed");
		goto fail;
	}
	offset.QuadPart = 0;
	GetFileSizeEx(file, &length);
	if (path_len > 4 && _wcsnicmp(L".iso", &path[path_len - 4], 4) == 0)
		type = RemovableMedia;
	ManageService("ImDisk", FALSE);
	if (!imdisk_mount(path, offset, length, type, letter))
		grub_error(GRUB_ERR_IO, "imdisk mount failed");
fail:
	if (path)
		grub_free(path);
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	return grub_errno;
}

static void
help_imdisk(struct grub_command* cmd)
{
	grub_printf("%s FILE_FULL_PATH [DRIVE_LETTER]\n", cmd->name);
	grub_printf("Mount ISO|IMG (ImDisk).\n");
}

struct grub_command grub_cmd_imdisk =
{
	.name = "imdisk",
	.func = cmd_imdisk,
	.help = help_imdisk,
	.next = 0,
};
