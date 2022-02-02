// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"
#include "misc.h"

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

static wchar_t* get_utf16(const char* path)
{
	wchar_t* path16 = NULL;
	grub_size_t len, len16;
	if (!path)
		return NULL;
	len = grub_strlen(path);
	len16 = len * GRUB_MAX_UTF16_PER_UTF8;
	path16 = grub_calloc(len16 + 1, sizeof(WCHAR));
	if (!path16)
		return NULL;
	len16 = grub_utf8_to_utf16(path16, len16, (grub_uint8_t*)path, len, NULL);
	path16[len16] = 0;
	return path16;
}

static grub_err_t
wim_mount(const char* file, const char* dest, DWORD index, const char* temp)
{
	wchar_t* file16 = get_utf16(file);
	wchar_t* dest16 = get_utf16(dest);
	wchar_t* temp16 = get_utf16(temp);
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
	wchar_t* file16 = get_utf16(file);
	wchar_t* dest16 = get_utf16(dest);
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


static grub_err_t cmd_wim(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i = 1;
	DWORD index = 1;
	BOOL commit = FALSE;
	if (argc < 2)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "bad argument");
	ManageService("WimFltr", FALSE);
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
