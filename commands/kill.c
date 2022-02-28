// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "misc.h"

enum kill_cmd_type
{
	KILL_BY_NAME,
	KILL_BY_PID,
	KILL_BY_TITLE,
};

static grub_err_t cmd_kill(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	DWORD exit_code = 1;
	enum kill_cmd_type t = KILL_BY_NAME;
	char* proc = NULL;

	for (i = 0; i < argc; i++)
	{
		if (grub_strcmp(argv[i], "-p") == 0)
			t = KILL_BY_PID;
		else if (grub_strcmp(argv[i], "-t") == 0)
			t = KILL_BY_TITLE;
		else if (grub_strncmp(argv[i], "-x=", 3) == 0)
			exit_code = grub_strtoul(&argv[i][3], NULL, 0);
		else
			proc = argv[i];
	}
	if (!proc)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "no process");

	switch (t)
	{
		case KILL_BY_PID:
		{
			DWORD pid = grub_strtoul(proc, NULL, 0);
			KillProcessById(pid, exit_code);
			break;
		}
		case KILL_BY_NAME:
		{
			WCHAR* name = grub_get_utf16(proc);
			if (!name)
				break;
			KillProcessByName(name, 1);
			grub_free(name);
			break;
		}
		case KILL_BY_TITLE:
		{
			DWORD pid;
			HWND handle = FindWindowA(NULL, proc);
			if (!handle)
				break;
			GetWindowThreadProcessId(handle, &pid);
			KillProcessById(pid, exit_code);
			break;
		}
	}

	return 0;
}

static void
help_kill(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] PROCESS\n", cmd->name);
	grub_printf("Kill a process.\nOPTIONS:\n");
	grub_printf("  -p        Kill process by pid.\n");
	grub_printf("  -t        Kill process by window title.\n");
	grub_printf("  -x=NUM    Set the exit code. [default = 1]\n");

}

struct grub_command grub_cmd_kill =
{
	.name = "kill",
	.func = cmd_kill,
	.help = help_kill,
	.next = 0,
};
