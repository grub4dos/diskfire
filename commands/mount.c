// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"

#include <virtdisk.h>

static DWORD
NTOpenVirtualDisk(PVIRTUAL_STORAGE_TYPE VirtualStorageType,
	PCWSTR Path,
	VIRTUAL_DISK_ACCESS_MASK VirtualDiskAccessMask,
	OPEN_VIRTUAL_DISK_FLAG Flags,
	POPEN_VIRTUAL_DISK_PARAMETERS Parameters,
	PHANDLE Handle)
{
	DWORD (WINAPI * NT6OpenVirtualDisk)(PVIRTUAL_STORAGE_TYPE VirtualStorageType,
		PCWSTR Path,
		VIRTUAL_DISK_ACCESS_MASK VirtualDiskAccessMask,
		OPEN_VIRTUAL_DISK_FLAG Flags,
		POPEN_VIRTUAL_DISK_PARAMETERS Parameters,
		PHANDLE Handle) = NULL;
	HINSTANCE hL = LoadLibraryA("VirtDisk.dll");
	if (hL)
		*(FARPROC*)&NT6OpenVirtualDisk = GetProcAddress(hL, "OpenVirtualDisk");
	if (NT6OpenVirtualDisk)
		return NT6OpenVirtualDisk(VirtualStorageType, Path, VirtualDiskAccessMask, Flags, Parameters, Handle);
	return ERROR_VIRTDISK_PROVIDER_NOT_FOUND;
}

static DWORD
NTAttachVirtualDisk(HANDLE VirtualDiskHandle,
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	ATTACH_VIRTUAL_DISK_FLAG Flags,
	ULONG ProviderSpecificFlags,
	PATTACH_VIRTUAL_DISK_PARAMETERS Parameters,
	LPOVERLAPPED Overlapped)
{
	DWORD (WINAPI * NT6AttachVirtualDisk) (HANDLE VirtualDiskHandle,
		PSECURITY_DESCRIPTOR SecurityDescriptor,
		ATTACH_VIRTUAL_DISK_FLAG Flags,
		ULONG ProviderSpecificFlags,
		PATTACH_VIRTUAL_DISK_PARAMETERS Parameters,
		LPOVERLAPPED Overlapped) = NULL;
	HINSTANCE hL = LoadLibraryA("VirtDisk.dll");
	if (hL)
		*(FARPROC*)&NT6AttachVirtualDisk = GetProcAddress(hL, "AttachVirtualDisk");
	if (NT6AttachVirtualDisk)
		return NT6AttachVirtualDisk(VirtualDiskHandle, SecurityDescriptor, Flags, ProviderSpecificFlags, Parameters, Overlapped);
	return ERROR_VIRTDISK_PROVIDER_NOT_FOUND;
}

#if 0
static DWORD
NTGetVirtualDiskPhysicalPath(HANDLE VirtualDiskHandle, PULONG DiskPathSizeInBytes, PWSTR DiskPath)
{
	DWORD (WINAPI * NT6GetVirtualDiskPhysicalPath)(HANDLE VirtualDiskHandle,
		PULONG DiskPathSizeInBytes,
		PWSTR DiskPath) = NULL;
	HINSTANCE hL = LoadLibraryA("VirtDisk.dll");
	if (hL)
		*(FARPROC*)&NT6GetVirtualDiskPhysicalPath = GetProcAddress(hL, "GetVirtualDiskPhysicalPath");
	if (NT6GetVirtualDiskPhysicalPath)
		return NT6GetVirtualDiskPhysicalPath(VirtualDiskHandle, DiskPathSizeInBytes, DiskPath);
	return ERROR_VIRTDISK_PROVIDER_NOT_FOUND;
}
#endif

static DWORD
NTDetachVirtualDisk(HANDLE VirtualDiskHandle, DETACH_VIRTUAL_DISK_FLAG Flags, ULONG ProviderSpecificFlags)
{
	DWORD(WINAPI * NT6DetachVirtualDisk) (HANDLE VirtualDiskHandle,
		DETACH_VIRTUAL_DISK_FLAG Flags,
		ULONG ProviderSpecificFlags) = NULL;
	HINSTANCE hL = LoadLibraryA("VirtDisk.dll");
	if (hL)
		*(FARPROC*)&NT6DetachVirtualDisk = GetProcAddress(hL, "DetachVirtualDisk");
	if (NT6DetachVirtualDisk)
		return NT6DetachVirtualDisk(VirtualDiskHandle, Flags, ProviderSpecificFlags);
	return ERROR_INVALID_FUNCTION;
}

static grub_err_t
mount_vdisk(const char* path)
{
	WCHAR* path16 = NULL;
	VIRTUAL_STORAGE_TYPE StorageType;
	OPEN_VIRTUAL_DISK_PARAMETERS OpenParameters;
	ATTACH_VIRTUAL_DISK_PARAMETERS AttachParameters;
	HANDLE Handle;
	DWORD Status;
	ATTACH_VIRTUAL_DISK_FLAG AttachFlag = ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY |
		ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;

	path16 = grub_get_utf16(path);
	if (!path16)
		return grub_errno;

	grub_memset(&StorageType, 0, sizeof(StorageType));
	grub_memset(&OpenParameters, 0, sizeof(OpenParameters));
	grub_memset(&AttachParameters, 0, sizeof(AttachParameters));
	OpenParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
	AttachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
	Status = NTOpenVirtualDisk(&StorageType, path16, VIRTUAL_DISK_ACCESS_READ, 0, &OpenParameters, &Handle);
	grub_free(path16);
	if (Status != ERROR_SUCCESS)
	{
		if (ERROR_VIRTDISK_PROVIDER_NOT_FOUND == Status)
			return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "api not supported");
		return grub_error(GRUB_ERR_FILE_READ_ERROR, "virtdisk open failed");
	}

	Status = NTAttachVirtualDisk(Handle, NULL, AttachFlag, 0, &AttachParameters, NULL);
	if (Status != ERROR_SUCCESS)
		grub_error(GRUB_ERR_BAD_DEVICE, "virtdisk attach failed");
	else
		grub_errno = GRUB_ERR_NONE;

	CloseHandle(Handle);
	return grub_errno;
}

static grub_err_t
umount_vdisk(const char* path)
{
	WCHAR* path16 = NULL;
	VIRTUAL_STORAGE_TYPE StorageType;
	OPEN_VIRTUAL_DISK_PARAMETERS OpenParameters;
	HANDLE Handle;
	DWORD Status;

	path16 = grub_get_utf16(path);
	if (!path16)
		return grub_errno;

	grub_memset(&StorageType, 0, sizeof(StorageType));
	grub_memset(&OpenParameters, 0, sizeof(OpenParameters));
	OpenParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
	Status = NTOpenVirtualDisk(&StorageType, path16, VIRTUAL_DISK_ACCESS_READ, 0, &OpenParameters, &Handle);
	grub_free(path16);
	if (Status != ERROR_SUCCESS)
	{
		if (ERROR_VIRTDISK_PROVIDER_NOT_FOUND == Status)
			return grub_error(GRUB_ERR_NOT_IMPLEMENTED_YET, "api not supported");
		return grub_error(GRUB_ERR_FILE_READ_ERROR, "virtdisk open failed");
	}

	Status = NTDetachVirtualDisk(Handle, DETACH_VIRTUAL_DISK_FLAG_NONE, 0);
	if (Status != ERROR_SUCCESS)
	{
		CloseHandle(Handle);
		return grub_error(GRUB_ERR_BAD_DEVICE, "virtdisk detach failed");
	}

	CloseHandle(Handle);
	return GRUB_ERR_NONE;
}

static grub_err_t cmd_mount(struct grub_command* cmd, int argc, char* argv[])
{
	if (argc < 1)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing file name");
	if (cmd->name[0] == 'u')
		return umount_vdisk(argv[0]);

	return mount_vdisk(argv[0]);
}

static void
help_mount(struct grub_command* cmd)
{
	grub_printf("%s FILE\n", cmd->name);
	if (cmd->name[0] == 'u')
		grub_printf("Unmount VHD|ISO.\n");
	else
		grub_printf("Mount VHD|ISO using AttachVirtualDisk WINAPI.\n");
}

struct grub_command grub_cmd_mount =
{
	.name = "mount",
	.func = cmd_mount,
	.help = help_mount,
	.next = 0,
};

struct grub_command grub_cmd_umount =
{
	.name = "umount",
	.func = cmd_mount,
	.help = help_mount,
	.next = 0,
};
