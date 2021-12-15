// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include "commands.h"
#include "misc.h"
#include "fs.h"
#include "partition.h"

int main(int argc, char *argv[])
{
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		printf("permission denied\n");
		return 1;
	}
	SetDebug("");
	grub_fs_init();
	grub_partmap_init();
	if (argc < 2)
	{
		printf("Commands: ls\n");
		return 0;
	}
	if (_stricmp(argv[1], "ls") == 0)
		return cmd_ls(argc - 2, argc < 3 ? NULL : &argv[2]);
	printf("Unknown command %s\n", argv[1]);
	return 1;
}
