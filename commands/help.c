// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"

static grub_err_t cmd_help(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	grub_command_t p = NULL;
	if (argc > 0)
	{
		p = grub_command_find(argv[0]);
		if (p)
			p->help(p);
		else
			grub_printf("%s not found\n", argv[0]);
		return 0;
	}
	grub_printf("diskfire [-d=DEBUG] [-m=FILE ...] COMMAND\n");
	grub_printf("OPTIONS:\n");
	grub_printf("  -d=DEBUG  Set debug conditions.\n");
	grub_printf("  -m=FILE   Make a virtual drive (ldX) from a file.\n");
	grub_printf("COMMANDS:\n\n");
	FOR_COMMANDS(p)
	{
		p->help(p);
		grub_printf("\n");
	}
	return 0;
}

static void
help_help(struct grub_command* cmd)
{
	grub_printf("%s [COMMAND]\n", cmd->name);
	grub_printf("Print help text.\n");
}

struct grub_command grub_cmd_help =
{
	.name = "help",
	.func = cmd_help,
	.help = help_help,
	.next = 0,
};
