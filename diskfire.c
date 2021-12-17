// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include "commands.h"
#include "misc.h"
#include "fs.h"
#include "partition.h"

int main(int argc, char *argv[])
{
	int i;
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		printf("permission denied\n");
		return -1;
	}
	SetConsoleOutputCP(65001);
	SetDebug("");
	grub_fs_init();
	grub_partmap_init();
	if (argc < 2)
	{
		printf("Commands: ls, extract\n");
		return 0;
	}
	for (i = 1; i < argc; i++)
	{
		if (_strnicmp(argv[i], "-d=", 3) == 0 && argv[i][3])
		{
			SetDebug(&argv[i][3]);
		}
		else if (_strnicmp(argv[i], "-m=", 3) == 0 && argv[i][3])
		{
			if (loopback_add(&argv[i][3]))
			{
				grub_print_error();
				return grub_errno;
			}
		}
		else if (_stricmp(argv[i], "ls") == 0)
		{
			int new_argc = argc - i - 1;
			char** new_argv = new_argc ? &argv[i + 1] : NULL;
			return cmd_ls(new_argc, new_argv);
		}
		else if (_stricmp(argv[i], "extract") == 0)
		{
			int new_argc = argc - i - 1;
			char** new_argv = new_argc ? &argv[i + 1] : NULL;
			return cmd_extract(new_argc, new_argv);
		}
		else
			break;
	}
	printf("Unknown command %s\n", argv[1]);
	return -1;
}
