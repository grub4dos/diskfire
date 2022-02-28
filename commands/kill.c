// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "misc.h"

static grub_err_t cmd_kill(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	if (argc < 1)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing process name");

	for (i = 0; i < argc; i++)
	{
		if (grub_strncasecmp(argv[i], "PID=", 4) == 0)
		{
			DWORD pid = grub_strtoul(&argv[i][4], NULL, 0);
			KillProcessById(pid, 1);
		}
		else
		{
			WCHAR* name = grub_get_utf16(argv[i]);
			KillProcessByName(name, 1);
			grub_free(name);
		}
	}

	return 0;
}

static void
help_kill(struct grub_command* cmd)
{
	grub_printf("%s PROCESS_NAME|PID=XXX\n", cmd->name);
	grub_printf("Kill process by name or pid.\n");
}

struct grub_command grub_cmd_kill =
{
	.name = "kill",
	.func = cmd_kill,
	.help = help_kill,
	.next = 0,
};
