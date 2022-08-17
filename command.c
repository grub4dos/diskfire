// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"

grub_command_t grub_command_list;

static void
dummy_help(struct grub_command* cmd)
{
	grub_printf("%s: no help text\n", cmd->name);
}

void
grub_command_register(struct grub_command* cmd)
{
	if (!cmd->help)
		cmd->help = dummy_help;
	grub_list_push(GRUB_AS_LIST_P(&grub_command_list), GRUB_AS_LIST(cmd));
}

grub_command_t
grub_command_find(const char* name)
{
	return grub_named_list_find(GRUB_AS_NAMED_LIST(grub_command_list), name);
}

grub_err_t
grub_command_execute(const char* name, int argc, char** argv)
{
	grub_command_t cmd;
	cmd = grub_command_find(name);
	return (cmd) ? cmd->func(cmd, argc, argv) : GRUB_ERR_FILE_NOT_FOUND;
}

void
grub_command_init(void)
{
	grub_command_register(&grub_cmd_bin2c);
	grub_command_register(&grub_cmd_ls);
	grub_command_register(&grub_cmd_extract);
	grub_command_register(&grub_cmd_probe);
	grub_command_register(&grub_cmd_hashsum);
	grub_command_register(&grub_cmd_hxd);
	grub_command_register(&grub_cmd_stat);
	grub_command_register(&grub_cmd_blocklist);
	grub_command_register(&grub_cmd_mbr);
	grub_command_register(&grub_cmd_pbr);
	grub_command_register(&grub_cmd_dd);
	grub_command_register(&grub_cmd_cat);
	grub_command_register(&grub_cmd_umount);
	grub_command_register(&grub_cmd_mount);
	grub_command_register(&grub_cmd_fwsetvar);
	grub_command_register(&grub_cmd_fwgetvar);
	grub_command_register(&grub_cmd_bcfg);
	grub_command_register(&grub_cmd_service);
	grub_command_register(&grub_cmd_wim);
	grub_command_register(&grub_cmd_imdisk);
	grub_command_register(&grub_cmd_lua);
	grub_command_register(&grub_cmd_luac);
	grub_command_register(&grub_cmd_shortcut);
	grub_command_register(&grub_cmd_kill);
	grub_command_register(&grub_cmd_uuid);
	grub_command_register(&grub_cmd_echo);
	grub_command_register(&grub_cmd_loopback);

	grub_command_register(&grub_cmd_help);
}
