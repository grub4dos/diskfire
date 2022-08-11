// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include "command.h"
#include "misc.h"
#include "fs.h"
#include "partition.h"
#include "file.h"
#include "br.h"
#include "command.h"

PHY_DRIVE_INFO* gDriveList = NULL;
DWORD gDriveCount = 0;

static
grub_off_t build_time_read (struct grub_file* file, void* data, grub_size_t sz)
{
	const char date[] = __DATE__ " " __TIME__;
	if (data)
		grub_memcpy(data, date + file->offset, sz);
	return sizeof(date);
}

static
grub_off_t dd_zero_read(struct grub_file* file, void* data, grub_size_t sz)
{
	(void)file;
	if (data)
		grub_memset(data, 0, sz);
	return GRUB_FILE_SIZE_UNKNOWN;
}

static void procfs_init(void)
{
	proc_add("build_time", NULL, build_time_read);
	proc_add("zero", NULL, dd_zero_read);
}

static char** args_init(int argc, wchar_t* u16_argv[])
{
	int i;
	char** u8_argv = calloc (argc, sizeof(char*));
	if (!u8_argv)
		exit(-1);
	for (i = 0; i < argc; i++)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, u16_argv[i], -1, NULL, 0, NULL, NULL);
		if (len <= 0)
			exit(-1);
		u8_argv[i] = malloc(len);
		len = WideCharToMultiByte(CP_UTF8, 0, u16_argv[i], -1, u8_argv[i], len, NULL, NULL);
		if (len <= 0)
			exit(-1);
	}
	return u8_argv;
}

static void args_fini(int argc, char* u8_argv[])
{
	int i;
	for (i = 0; i < argc; i++)
	{
		free(u8_argv[i]);
	}
}

int wmain(int argc, wchar_t *argv[])
{
	int i;
	char** u8_argv = NULL;
	grub_command_t p = NULL;
	u8_argv = args_init(argc, argv);
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		grub_error(GRUB_ERR_ACCESS_DENIED, "permission denied\n");
		goto fini;
	}
	SetDebug("");
	if (!GetDriveInfoList(&gDriveList, &gDriveCount))
	{
		grub_error(GRUB_ERR_UNKNOWN_DEVICE, "could not get drive info list\n");
		goto fini;
	}
	procfs_init();
	grub_disk_dev_init();
	grub_command_init();
	grub_file_filter_init();
	grub_fs_init();
	grub_partmap_init();
	grub_br_init();
	if (argc < 2)
	{
		grub_command_execute("help", 0, NULL);
		goto fini;
	}
	for (i = 1; i < argc; i++)
	{
		if (_strnicmp(u8_argv[i], "-d=", 3) == 0 && u8_argv[i][3])
		{
			SetDebug(&u8_argv[i][3]);
		}
		else if (_strnicmp(u8_argv[i], "-m=", 3) == 0 && u8_argv[i][3])
		{
			if (loopback_add(&u8_argv[i][3]))
				goto fini;
		}
		else if ((p = grub_command_find(u8_argv[i])) != NULL)
		{
			int new_argc = argc - i - 1;
			char** new_argv = new_argc ? &u8_argv[i + 1] : NULL;
			p->func(p, new_argc, new_argv);
			goto fini;
		}
		else
			break;
	}
	grub_error(GRUB_ERR_BAD_ARGUMENT, "Unknown command %s\n", u8_argv[1]);
fini:
	if (grub_errno)
		grub_print_error();
	if (gDriveList)
		free(gDriveList);
	args_fini(argc, u8_argv);
	return grub_errno;
}
