// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "misc.h"

static grub_err_t cmd_service(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	BOOL stop = FALSE;
	int i;
	for (i = 0; i < argc; i++)
	{
		if (i == 0 && grub_strcmp(argv[i], "-x") == 0)
		{
			stop = TRUE;
			continue;
		}
		if (ManageService(argv[i], stop) == FALSE)
			grub_error(GRUB_ERR_IO, "service %s %s failed", argv[i], stop ? "stop" : "start");
		else
			grub_errno = GRUB_ERR_NONE;
	}
	return grub_errno;
}

static void
help_service(struct grub_command* cmd)
{
	grub_printf("%s [-x] NAME\n", cmd->name);
	grub_printf("Start|Stop a service.\n");
	grub_printf("  -x  Stop the service.\n");
}

struct grub_command grub_cmd_service =
{
	.name = "service",
	.func = cmd_service,
	.help = help_service,
	.next = 0,
};
