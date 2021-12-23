// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "file.h"
#include "command.h"
#include "misc.h"

static const char* d_human_sizes[6] =
{ " B", " KB", " MB", " GB", " TB", " PB", };

static grub_err_t
cmd_extract(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	const char* in = NULL, * out = NULL;
	grub_file_t file = 0;
	enum grub_file_type type = GRUB_FILE_TYPE_EXTRACT;
	HANDLE fd = INVALID_HANDLE_VALUE;
	char* buf = NULL;
	grub_size_t copy_size = 8ULL * 1024 * 1024; // 8MB
	DWORD dwout;
	if (argc < 2 || (argc < 3 && grub_strcmp(argv[0], "-d") == 0))
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing argument");
		goto fail;
	}
	if (grub_strcmp(argv[0], "-d") == 0)
	{
		in = argv[1];
		out = argv[2];
	}
	else
	{
		in = argv[0];
		out = argv[1];
		type |= GRUB_FILE_TYPE_NO_DECOMPRESS;
	}
	file = grub_file_open(in, type);
	if (!file || file->size == GRUB_FILE_SIZE_UNKNOWN)
	{
		grub_error(GRUB_ERR_BAD_FILENAME, "invalid input file");
		goto fail;
	}
	fd = CreateFileA(out, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (fd == INVALID_HANDLE_VALUE)
	{
		grub_error(GRUB_ERR_BAD_FILENAME, "cannot create output file");
		goto fail;
	}
	buf = grub_malloc(copy_size);
	if (!buf)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		goto fail;
	}
	grub_printf("size: %s\n", grub_get_human_size(file->size, d_human_sizes, 1024));
	while (file->offset < file->size)
	{
		if (file->offset + copy_size > file->size)
			copy_size = file->size - file->offset;
		if (grub_file_read(file, buf, copy_size) != (grub_ssize_t)copy_size)
		{
			grub_error(GRUB_ERR_READ_ERROR, "file read error");
			goto fail;
		}
		if (!WriteFile(fd, buf, (DWORD)copy_size, &dwout, NULL) || dwout != copy_size)
		{
			grub_error(GRUB_ERR_WRITE_ERROR, "file write error %u", GetLastError());
			goto fail;
		}
		grub_printf("%llu%%\n", file->offset * 100 / file->size);
	}
fail:
	if (file)
		grub_file_close(file);
	CHECK_CLOSE_HANDLE(fd);
	if (buf)
		grub_free(buf);
	return grub_errno;
}

static void
help_extract(struct grub_command* cmd)
{
	grub_printf("%s [-d] SRC_FILE DST_FILE\n", cmd->name);
	grub_printf("Extract file from source location.\n");
	grub_printf("  -d  Decompress source file.\n");
}

struct grub_command grub_cmd_extract =
{
	.name = "extract",
	.func = cmd_extract,
	.help = help_extract,
	.next = 0,
};
