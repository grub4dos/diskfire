// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"
#include "misc.h"

typedef struct _WIM_MOUNT_LIST
{
	WCHAR  WimPath[MAX_PATH];
	WCHAR  MountPath[MAX_PATH];
	DWORD  ImageIndex;
	BOOL   MountedForRW;
} WIM_MOUNT_LIST, *PWIM_MOUNT_LIST, *LPWIM_MOUNT_LIST, WIM_MOUNT_INFO_LEVEL0, *PWIM_MOUNT_INFO_LEVEL0, LPWIM_MOUNT_INFO_LEVEL0;

typedef struct _WIM_MOUNT_INFO_LEVEL1
{
	WCHAR  WimPath[MAX_PATH];
	WCHAR  MountPath[MAX_PATH];
	DWORD  ImageIndex;
	DWORD  MountFlags;
} WIM_MOUNT_INFO_LEVEL1, *PWIM_MOUNT_INFO_LEVEL1, *LPWIM_MOUNT_INFO_LEVEL1;

typedef enum _MOUNTED_IMAGE_INFO_LEVELS
{
	MountedImageInfoLevel0,
	MountedImageInfoLevel1,
	MountedImageInfoLevelInvalid
} MOUNTED_IMAGE_INFO_LEVELS;

static BOOL
WIMMountImage(PWSTR pszMountPath, PWSTR pszWimFileName, DWORD dwImageIndex, PWSTR pszTempPath)
{
	BOOL(WINAPI * MountImage)(PWSTR, PWSTR, DWORD, PWSTR) = NULL;
	HINSTANCE hL = LoadLibraryA("wimgapi.dll");
	if (hL)
		*(FARPROC*)&MountImage = GetProcAddress(hL, "WIMMountImage");
	if (MountImage)
		return MountImage(pszMountPath, pszWimFileName, dwImageIndex, pszTempPath);
	return FALSE;
}

static BOOL
WIMUnmountImage(PWSTR pszMountPath, PWSTR pszWimFileName, DWORD dwImageIndex, BOOL bCommitChanges)
{
	BOOL(WINAPI * UnmountImage)(PWSTR, PWSTR, DWORD, BOOL) = NULL;
	HINSTANCE hL = LoadLibraryA("wimgapi.dll");
	if (hL)
		*(FARPROC*)&UnmountImage = GetProcAddress(hL, "WIMUnmountImage");
	if (UnmountImage)
		return UnmountImage(pszMountPath, pszWimFileName, dwImageIndex, bCommitChanges);
	return FALSE;
}

#if 0
static BOOL
WIMGetMountedImageInfo(MOUNTED_IMAGE_INFO_LEVELS fInfoLevelId,
	PDWORD pdwImageCount, PVOID pMountInfo, DWORD cbMountInfoLength, PDWORD pcbReturnLength)
{
	BOOL(WINAPI * GetMountedImageInfo)(MOUNTED_IMAGE_INFO_LEVELS, PDWORD, PVOID, DWORD, PDWORD) = NULL;
	HINSTANCE hL = LoadLibraryA("wimgapi.dll");
	if (hL)
		*(FARPROC*)&GetMountedImageInfo = GetProcAddress(hL, "WIMGetMountedImageInfo");
	if (GetMountedImageInfo)
		return GetMountedImageInfo(fInfoLevelId, pdwImageCount, pMountInfo, cbMountInfoLength, pcbReturnLength);
	return FALSE;
}
#endif

static grub_err_t
wim_mount(const char* file, const char* dest, DWORD index, const char* temp)
{
	wchar_t* file16 = grub_get_utf16(file);
	wchar_t* dest16 = grub_get_utf16(dest);
	wchar_t* temp16 = grub_get_utf16(temp);
	if (!file16 || !dest16)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		goto fail;
	}
	if (WIMMountImage(dest16, file16, index, temp16))
		grub_errno = GRUB_ERR_NONE;
	else
		grub_error(GRUB_ERR_IO, "wim %s (id=%lu) mount (%s) failed", file, index, dest);
fail:
	grub_free(file16);
	grub_free(dest16);
	grub_free(temp16);
	return grub_errno;
}

static grub_err_t
wim_umount(const char* file, const char* dest, DWORD index, BOOL commit)
{
	wchar_t* file16 = grub_get_utf16(file);
	wchar_t* dest16 = grub_get_utf16(dest);
	if (!dest16)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		goto fail;
	}
	if (WIMUnmountImage(dest16, file16, index, commit))
		grub_errno = GRUB_ERR_NONE;
	else
		grub_error(GRUB_ERR_IO, "wim (id=%lu%s) umount (%s) failed", index, commit ? ",commit" : "", dest);
fail:
	grub_free(file16);
	grub_free(dest16);
	return grub_errno;
}

#if 0
static int
wim_enum(int (*callback)(PWIM_MOUNT_LIST info, void* data), void* data)
{
	PWIM_MOUNT_LIST list = NULL, p;
	DWORD len = 0, count = 0, i;
	int ret = 0;
	WIMGetMountedImageInfo(MountedImageInfoLevel0, &count, NULL, 0, &len);
	if (!count || !len)
	{
		grub_printf("no mounted images");
		return 0;
	}
	grub_printf("count: %lu\n", count);
	list = grub_malloc(len);
	if (!list)
		return 0;
	if (WIMGetMountedImageInfo(MountedImageInfoLevel0, &count, list, len, &len) == FALSE)
	{
		grub_printf("WIMGetMountedImageInfo failed");
		goto fail;
	}
	for (i = 0, p = list; i < count; i++, p++)
	{
		ret = callback(p, data);
		if (ret)
			break;
	}
fail:
	grub_free(list);
	return ret;
}

static int wim_list(PWIM_MOUNT_LIST info, void* data)
{
	(void)data;
	grub_printf("%S -> %S, @%lu, %s\n",
		info->WimPath, info->MountPath, info->ImageIndex, info->MountedForRW ? "RW" : "R");
	return 0;
}
#endif

static grub_err_t cmd_wim(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i = 1;
	DWORD index = 1;
	BOOL commit = FALSE;
	ManageService("WimFltr", FALSE);
#if 0
	if (argc > 0 && grub_strcmp(argv[0], "list") == 0)
	{
		wim_enum(wim_list, NULL);
		return GRUB_ERR_NONE;
	}
#endif
	if (argc < 2)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "bad argument");
	if (grub_strncmp(argv[i], "--index=", 8) == 0)
	{
		index = grub_strtoul(&argv[1][8], NULL, 0);
		i++;
	}
	if (argc >= i + 1 && grub_strcmp(argv[i], "--commit") == 0)
	{
		commit = TRUE;
		i++;
	}
	if (grub_strcmp(argv[0], "mount") == 0 && argc >= i + 2)
		return wim_mount(argv[i], argv[i + 1], index, argc >= i + 3 ? argv[i + 2] : NULL);
	else if (grub_strcmp(argv[0], "umount") == 0 && argc >= i + 1)
		return wim_umount(NULL, argv[i], index, commit);

	return grub_error(GRUB_ERR_BAD_ARGUMENT, "bad argument");
}

static void
help_wim(struct grub_command* cmd)
{
	grub_printf("%s OPERAND ..\n", cmd->name);
	grub_printf("WIM file operations.\nUsage:\n");
#if 0
	grub_printf("  wim list\n");
#endif
	grub_printf("  wim mount [--index=n] FILE DEST [TEMP]\n");
	grub_printf("  wim umount [--index=n] [--commit] DEST\n");
}

struct grub_command grub_cmd_wim =
{
	.name = "wim",
	.func = cmd_wim,
	.help = help_wim,
	.next = 0,
};
