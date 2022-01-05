// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"
#include "efi.h"

static grub_err_t
print_var(void* data, grub_size_t len, const char* type)
{
	if (len < 1)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid data");
	if (grub_strcasecmp(type, "HEX") == 0)
	{
		grub_size_t i;
		const grub_uint8_t* p = data;
		for (i = 0; i < len; i++)
			grub_printf("%s%02X", i ? " " : "", p[i]);
		grub_printf("\n");
	}
	else if (grub_strcasecmp(type, "UINT") == 0)
	{
		if (len == sizeof(grub_uint64_t))
		{
			const grub_uint64_t* p = data;
			grub_printf("%llu\n", *p);
		}
		else if (len == sizeof(grub_uint32_t))
		{
			const grub_uint32_t* p = data;
			grub_printf("%u\n", *p);
		}
		else if (len == sizeof(grub_uint16_t))
		{
			const grub_uint16_t* p = data;
			grub_printf("%u\n", *p);
		}
		else if (len == sizeof(grub_uint8_t))
		{
			const grub_uint8_t* p = data;
			grub_printf("%u\n", *p);
		}
		else
			grub_error(GRUB_ERR_BAD_NUMBER, "invalid number");
	}
	else if (grub_strcasecmp(type, "UTF8") == 0)
	{
		const char* p = data;
		char* p8 = grub_zalloc(len + 1);
		if (!p8)
			return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		grub_memcpy(p8, p, len);
		grub_printf("%s\n", p8);
		grub_free(p8);
	}
	else if (grub_strcasecmp(type, "UCS2") == 0)
	{
		const grub_uint16_t* p = data;
		char* p8 = grub_calloc(len / sizeof(grub_uint16_t) + 1, GRUB_MAX_UTF8_PER_UTF16);
		if (!p8)
			return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		*grub_utf16_to_utf8((grub_uint8_t*)p8, p, len / sizeof(grub_uint16_t)) = '\0';
		grub_printf("%s\n", p8);
		grub_free(p8);
	}
	else if (grub_strcasecmp(type, "DEVP") == 0)
	{
		grub_efi_device_path_t* dp = data;
		grub_efi_print_device_path(dp);
		grub_printf("\n");
	}
	else
		grub_error(GRUB_ERR_BAD_ARGUMENT, "unsupported type");
	return grub_errno;
}

static grub_err_t
cmd_getvar(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	const char* guid = GRUB_EFI_GV_GUID;
	const char* type = "HEX";
	const char* output = NULL;
	const char* var = NULL;
	grub_size_t var_size = 0;
	void* var_data = NULL;
	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "-g=", 3) == 0)
			guid = &argv[i][3];
		else if (grub_strncmp(argv[i], "-t=", 3) == 0)
			type = &argv[i][3];
		else if (grub_strncmp(argv[i], "-o=", 3) == 0)
			output = &argv[i][3];
		else
			var = argv[i];
	}
	if (!var)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing variable name");
	if (grub_efi_get_variable(var, guid, &var_size, &var_data, NULL) != GRUB_ERR_NONE)
		goto fail;
	if (var_size == 0)
		goto fail;
	if (output)
	{
		HANDLE fd = INVALID_HANDLE_VALUE;
		fd = CreateFileA(output, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if (!fd || fd == INVALID_HANDLE_VALUE)
		{
			grub_error(GRUB_ERR_BAD_FILENAME, "can't create file %s", output);
			goto fail;
		}
		if (!WriteFile(fd, var_data, var_size, NULL, NULL))
		{
			grub_error(GRUB_ERR_WRITE_ERROR, "write failed");
			goto fail;
		}
		grub_errno = GRUB_ERR_NONE;
		goto fail;
	}
	print_var(var_data, var_size, type);
fail:
	if (var_data)
		grub_free(var_data);

	return grub_errno;
}

static void
help_getvar(struct grub_command* cmd)
{
	grub_printf("%s [OPTION] VARIABLE\n", cmd->name);
	grub_printf("Display a UEFI variable.\nOPTIONS:\n");
	grub_printf("  -g={GUID}    Specify the GUID of the UEFI variable.\n");
	grub_printf("  -t=TYPE      Specify the type of the UEFI variable:\n");
	grub_printf("     HEX       Hexadecimal bytes [default]\n");
	grub_printf("     UINT      Unsigned integer\n");
	grub_printf("     UTF8      UTF-8 encoded string\n");
	grub_printf("     UCS2      UCS-2 encoded string\n");
	grub_printf("     DEVP      UEFI device path\n");
	grub_printf("  -o=FILE      Dump variable data to a file.\n");
}

struct grub_command grub_cmd_fwgetvar =
{
	.name = "fwgetvar",
	.func = cmd_getvar,
	.help = help_getvar,
	.next = 0,
};
