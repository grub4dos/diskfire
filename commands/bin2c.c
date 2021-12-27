/*
 * This is bin2c program, which allows you to convert binary file to
 * C language array, for use as embedded resource, for instance you can
 * embed graphics or audio file directly into your program.
 * This is public domain software, use it on your own risk.
 * Contact Serge Fukanchik at fuxx@mail.ru  if you have any questions.
 *
 * Some modifications were made by Gwilym Kuiper (kuiper.gwilym@gmail.com)
 * I have decided not to change the licence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "file.h"
#include "command.h"

static grub_err_t
bin2c_main(const char* in, const char* out, const char* array_name, unsigned in_length,
	int print_size, int print_static, unsigned column)
{
	char* buf;
	unsigned int i, file_size, need_comma;
	grub_file_t f_input = 0;
	FILE* f_output;
	if (!column)
		column = 12;

	f_input = grub_file_open(in, GRUB_FILE_TYPE_CAT | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!f_input)
		return grub_error(GRUB_ERR_FILE_NOT_FOUND, "file not found");

	file_size = grub_file_size(f_input);

	if (in_length && in_length <= file_size)
		file_size = in_length;

	buf = (char*)malloc(file_size);
	if (!buf)
	{
		grub_file_close(f_input);
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "insufficient memory");
	}

	if (grub_file_read(f_input, buf, file_size) != (grub_ssize_t)file_size)
	{
		grub_file_close(f_input);
		return grub_error(GRUB_ERR_FILE_READ_ERROR, "file read error");
	}
	grub_file_close(f_input);

	if (fopen_s(&f_output, out, "w") != 0)
		return grub_error(GRUB_ERR_FILE_NOT_FOUND, "can't open %s for writing", out);

	need_comma = 0;

	fprintf(f_output, "%sunsigned char %s[] = {", print_static ? "static " : "", array_name);
	for (i = 0; i < file_size; ++i)
	{
		if (need_comma)
			fprintf(f_output, ", ");
		else
			need_comma = 1;
		if ((i % column) == 0)
			fprintf(f_output, "\n  ");
		fprintf(f_output, "0x%.2x", buf[i] & 0xff);
	}
	fprintf(f_output, "\n};\n");

	if (print_size)
		fprintf(f_output, "\n%sunsigned int %s_len = %i;\n", print_static ? "static " : "", array_name, file_size);

	fclose(f_output);

	return 0;
}

static grub_err_t cmd_bin2c(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	const char* in = NULL, * out = NULL, * array_name = NULL;
	int print_size = 1, print_static = 0;
	unsigned in_length = 0, column = 12;

	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "-i=", 3) == 0)
			in = &argv[i][3];
		else if (grub_strncmp(argv[i], "-a=", 3) == 0)
			array_name = &argv[i][3];
		else if (grub_strncmp(argv[i], "-l=", 3) == 0)
			in_length = grub_strtoul(&argv[i][3], NULL, 0);
		else if (grub_strncmp(argv[i], "-c=", 3) == 0)
			column = grub_strtoul(&argv[i][3], NULL, 0);
		else if (grub_strcmp(argv[i], "-n") == 0)
			print_size = 0;
		else if (grub_strcmp(argv[i], "-s") == 0)
			print_static = 1;
		else
			out = argv[i];
	}
	if (!in || !out || !array_name)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing argument");

	return bin2c_main(in, out, array_name, in_length, print_size, print_static, column);
}

static void
help_bin2c(struct grub_command* cmd)
{
	grub_printf("%s OPTIONS FILE\n", cmd->name);
	grub_printf("Convert binary file to C language array.\nOPTIONS:\n");
	grub_printf("  -i=FILE  Specify input binary file.\n");
	grub_printf("  -a=NAME  Specify array name.\n");
	grub_printf("  -l=N     Specify input length.\n");
	grub_printf("  -n       Do not write data length to output.\n");
	grub_printf("  -s       Use static variables.\n");
	grub_printf("  -c=N     Specify column number [default=12].\n");
}

struct grub_command grub_cmd_bin2c =
{
	.name = "bin2c",
	.func = cmd_bin2c,
	.help = help_bin2c,
	.next = 0,
};
