// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "disk.h"
#include "file.h"

static void
hexdump(unsigned long bse, char* buf, int len)
{
	int pos;
	char line[80];

	while (len > 0)
	{
		int cnt, i;

		pos = grub_snprintf(line, sizeof(line), "%08lx  ", bse);
		cnt = 16;
		if (cnt > len)
			cnt = len;

		for (i = 0; i < cnt; i++)
		{
			pos += grub_snprintf(&line[pos], sizeof(line) - pos,
				"%02x ", (unsigned char)buf[i]);
			if ((i & 7) == 7)
				line[pos++] = ' ';
		}

		for (; i < 16; i++)
		{
			pos += grub_snprintf(&line[pos], sizeof(line) - pos, "   ");
			if ((i & 7) == 7)
				line[pos++] = ' ';
		}

		line[pos++] = '|';

		for (i = 0; i < cnt; i++)
			line[pos++] = ((buf[i] >= 32) && (buf[i] < 127)) ? buf[i] : '.';

		line[pos++] = '|';

		line[pos] = 0;

		grub_printf("%s\n", line);

		/* Print only first and last line if more than 3 lines are identical.  */
		if (len >= 4 * 16
			&& !grub_memcmp(buf, buf + 1 * 16, 16)
			&& !grub_memcmp(buf, buf + 2 * 16, 16)
			&& !grub_memcmp(buf, buf + 3 * 16, 16))
		{
			grub_printf("*\n");
			do
			{
				bse += 16;
				buf += 16;
				len -= 16;
			} while (len >= 3 * 16 && !grub_memcmp(buf, buf + 2 * 16, 16));
		}

		bse += 16;
		buf += 16;
		len -= cnt;
	}
}

static void
hxd_file(const char* filename, int decompress, grub_disk_addr_t skip, grub_ssize_t length)
{
	grub_file_t file = 0;
	grub_ssize_t size;
	char buf[GRUB_DISK_SECTOR_SIZE];
	enum grub_file_type type = GRUB_FILE_TYPE_CAT;
	if (!decompress)
		type |= GRUB_FILE_TYPE_NO_DECOMPRESS;

	file = grub_file_open(filename, type);
	if (!file)
		return;

	file->offset = skip;

	while ((size = grub_file_read(file, buf, sizeof(buf))) > 0)
	{
		unsigned long len;

		len = ((length) && (size > length)) ? length : size;
		hexdump(skip, buf, len);
		skip += len;
		if (length)
		{
			length -= len;
			if (!length)
				break;
		}
	}

	grub_file_close(file);
}

static grub_err_t cmd_hxd(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	const char* filename = NULL;
	grub_disk_addr_t skip = 0;
	grub_ssize_t length = GRUB_DISK_SECTOR_SIZE;
	int decompress = 0;
	int i;
	for (i = 0; i < argc; i++)
	{
		if (grub_strcmp(argv[i], "-d") == 0)
			decompress = 1;
		else if (grub_strncmp(argv[i], "-s=", 3) == 0)
			skip = grub_strtoull(&argv[i][3], NULL, 0);
		else if (grub_strncmp(argv[i], "-n=", 3) == 0)
			length = grub_strtoul(&argv[i][3], NULL, 0);
		else
			filename = argv[i];
	}
	if (!filename)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing filename");
	hxd_file(filename, decompress, skip, length);
	return 0;
}

static void
help_hxd(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] FILE\n", cmd->name);
	grub_printf("Show raw contents of a file.\nOPTIONS:\n");
	grub_printf("  -d    Decompress file.\n");
	grub_printf("  -s=N  Skip N bytes from the beginning of file.\n");
	grub_printf("  -n=N  Read only N bytes. [default=%lu]\n", GRUB_DISK_SECTOR_SIZE);
}

struct grub_command grub_cmd_hxd =
{
	.name = "hxd",
	.func = cmd_hxd,
	.help = help_hxd,
	.next = 0,
};
