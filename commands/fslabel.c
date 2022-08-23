// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "command.h"
#include "misc.h"

static grub_err_t
cmd_fslabel(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	WCHAR* label = NULL;
	WCHAR* volume = NULL;
	char* path;
	if (argc != 2)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing argument");
	if (grub_strcmp(argv[0], "-d") == 0)
		path = argv[1];
	else
	{
		path = argv[0];
		label = grub_get_utf16(argv[1]);
		if (!label)
			goto fail;
	}
	if (grub_strlen(path) == 1) /* drive letter */
		path = grub_xasprintf("%c:\\", path[0]);
	else
		path = grub_xasprintf("%s\\", path);
	if (!path)
		goto fail;
	volume = grub_get_utf16(path);
	grub_free(path);
	if (!volume)
		goto fail;
	if (!SetVolumeLabelW(volume, label))
		grub_error(GRUB_ERR_BAD_DEVICE, "SetVolumeLabel failed");
fail:
	if (label)
		grub_free(label);
	if (volume)
		grub_free(volume);
	return grub_errno;
}

static void
help_fslabel(struct grub_command* cmd)
{
	grub_printf("%s [-d] DRIVE_LETTER|MOUNT_POINT LABEL\n", cmd->name);
	grub_printf("Set the label of a file system volume.\n");
	grub_printf("  -d  Delete existing label.\n");
}

struct grub_command grub_cmd_fslabel =
{
	.name = "fslabel",
	.func = cmd_fslabel,
	.help = help_fslabel,
	.next = 0,
};
