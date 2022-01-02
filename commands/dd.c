// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "disk.h"
#include "file.h"
#include "fs.h"
#include "partition.h"
#include "command.h"
#include "misc.h"

static grub_err_t
cmd_dd(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	grub_file_t in = 0, out = 0;
	grub_uint32_t bs = 512;
	grub_uint64_t count = 0, skip = 0, seek = 0;
	grub_uint8_t* data = NULL;
	HANDLE* hVolList = NULL;
	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "if=", 3) == 0)
		{
			in = grub_file_open(&argv[i][3], GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
			if (!in)
			{
				grub_error(GRUB_ERR_BAD_FILENAME, "can't open %s", &argv[i][3]);
				goto fail;
			}
		}
		else if (grub_strncmp(argv[i], "of=", 3) == 0)
		{
			out = grub_file_open(&argv[i][3], GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
			if (!out)
			{
				grub_error(GRUB_ERR_BAD_FILENAME, "can't open %s", &argv[i][4]);
				goto fail;
			}
			if (!grub_blocklist_convert(out))
			{
				grub_error(GRUB_ERR_FILE_READ_ERROR, "can't get blocklist");
				goto fail;
			}
		}
		else if (grub_strncmp(argv[i], "bs=", 3) == 0)
		{
			bs = grub_strtoul(&argv[i][3], NULL, 0);
			if (bs < 1 || bs > 8192)
			{
				grub_error(GRUB_ERR_BAD_NUMBER, "invalid block size");
				goto fail;
			}
		}
		else if (grub_strncmp(argv[i], "count=", 6) == 0)
			count = grub_strtoull(&argv[i][6], NULL, 0);
		else if (grub_strncmp(argv[i], "skip=", 5) == 0)
			skip = grub_strtoull(&argv[i][5], NULL, 0);
		else if (grub_strncmp(argv[i], "seek=", 5) == 0)
			seek = grub_strtoull(&argv[i][5], NULL, 0);
		else
		{
			grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid argument %s", argv[i]);
			goto fail;
		}
	}
	if (!in || !out)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing %s file", in ? "input" : "output");
		goto fail;
	}
	data = grub_malloc(bs);
	if (!data)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "can't allocate buffer");
	}

	count *= bs;

	if ((skip >= in->size) || (seek >= out->size))
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid skip/seek");
		goto fail;
	}

	if (!count)
		count = in->size - skip;

	if (skip + count > in->size)
	{
		grub_printf("WARNING: skip + count > input_size\n");
		count = in->size - skip;
	}

	if (seek + count > out->size)
	{
		grub_printf("WARNING: seek + count > output_size\n");
		count = out->size - seek;
	}

	if (out->disk->dev->id == GRUB_DISK_WINDISK_ID)
		hVolList = LockDriveById(out->disk->id);

	while (count)
	{
		grub_uint32_t copy_bs;
		copy_bs = (bs > count) ? count : bs;
		/* read */
		grub_file_seek(in, skip);
		grub_file_read(in, data, copy_bs);
		if (grub_errno)
			break;
		/* write */
		grub_file_seek(out, seek);
		grub_blocklist_write(out, (char*)data, copy_bs);
		if (grub_errno)
			break;

		skip += copy_bs;
		seek += copy_bs;
		count -= copy_bs;
	}

fail:
	UnlockDrive(hVolList);
	if (data)
		grub_free(data);
	if (in)
		grub_file_close(in);
	if (out)
		grub_file_close(out);
	return grub_errno;
}

static void
help_dd(struct grub_command* cmd)
{
	grub_printf("%s OPERAND ..\n", cmd->name);
	grub_printf("Copy a file, converting and formatting according to the operands.\nOPERANDS:\n");
	grub_printf("  if=FILE|DISK    read from FILE or DISK\n");
	grub_printf("  of=FILE|DISK    write to FILE(convert to blocklists) or DISK\n");
	grub_printf("  bs=SIZE         Specify block size (1~8192). [default=512]\n");
	grub_printf("  count=N         Specify number of blocks to copy.\n");
	grub_printf("  skip=N          Skip N bytes at input.\n");
	grub_printf("  seek=N          Skip N bytes at output.\n");
}

struct grub_command grub_cmd_dd =
{
	.name = "dd",
	.func = cmd_dd,
	.help = help_dd,
	.next = 0,
};
