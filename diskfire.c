// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include "command.h"
#include "misc.h"
#include "fs.h"
#include "partition.h"
#include "file.h"

int main(int argc, char *argv[])
{
	int i;
	grub_command_t p = NULL;
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		printf("permission denied\n");
		return -1;
	}
	SetConsoleOutputCP(65001);
	SetDebug("");
	grub_command_init();
	grub_file_filter_init();
	grub_fs_init();
	grub_partmap_init();
	if (argc < 2)
	{
		grub_command_execute("help", 0, NULL);
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
		else if ((p = grub_command_find(argv[i])) != NULL)
		{
			int new_argc = argc - i - 1;
			char** new_argv = new_argc ? &argv[i + 1] : NULL;
			return p->func(p, new_argc, new_argv);
		}
		else
			break;
	}
	printf("Unknown command %s\n", argv[1]);
	return -1;
}
