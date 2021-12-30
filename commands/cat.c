// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "file.h"
#include "command.h"

static grub_err_t cmd_cat(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	grub_file_t file = 0;
	char* line = NULL;
	if (argc < 1)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing file name");
	file = grub_file_open(argv[0], GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
		return grub_error(GRUB_ERR_BAD_FILENAME, "bad file");
	while (1)
	{
		line = grub_file_getline(file);
		if (!line)
			break;
		grub_printf("%s\n", line);
		grub_free(line);
	}

	return 0;
}

static void
help_cat(struct grub_command* cmd)
{
	grub_printf("%s FILE\n", cmd->name);
	grub_printf("Show the contents of a file.\n");
}

struct grub_command grub_cmd_cat =
{
	.name = "cat",
	.func = cmd_cat,
	.help = help_cat,
	.next = 0,
};
