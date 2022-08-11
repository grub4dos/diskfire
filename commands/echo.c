// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "file.h"
#include "command.h"
#include <stdio.h>

static grub_err_t cmd_echo(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	SetConsoleOutputCP(CP_UTF8);
	for (i = 0; i < argc; i++)
	{
		grub_printf("%s%s", i ? " " : "", argv[i]);
	}
	grub_printf("\n");
	return 0;
}

static void
help_echo(struct grub_command* cmd)
{
	grub_printf("%s STRING\n", cmd->name);
	grub_printf("Display a line of text.\n");
}

struct grub_command grub_cmd_echo =
{
	.name = "echo",
	.func = cmd_echo,
	.help = help_echo,
	.next = 0,
};
