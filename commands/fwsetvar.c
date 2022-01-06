// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"
#include "efi.h"
#include "file.h"

static void*
get_var_data(const char* type, const char* input, grub_size_t* size)
{
	if (grub_strcasecmp(type, "HEX") == 0)
	{
		grub_size_t i;
		char* hexstr = NULL;
		grub_size_t len = grub_strlen(input);
		if (len & 0x01)
		{
			grub_error(GRUB_ERR_EOF, "invalid hex data");
			return NULL;
		}
		*size = len / 2;
		hexstr = grub_zalloc(*size);
		if (!hexstr)
		{
			grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
			return NULL;
		}
		for (i = 0; i < len; i++)
		{
			int c;
			if ((input[i] >= '0') && (input[i] <= '9'))
				c = input[i] - '0';
			else if ((input[i] >= 'A') && (input[i] <= 'F'))
				c = input[i] - 'A' + 10;
			else if ((input[i] >= 'a') && (input[i] <= 'f'))
				c = input[i] - 'a' + 10;
			else
			{
				grub_free(hexstr);
				grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid hex string");
				return NULL;
			}
			if ((i & 1) == 0)
				c <<= 4;
			hexstr[i >> 1] |= c;
		}
		return hexstr;
	}
	else if (grub_strcasecmp(type, "UTF8") == 0)
	{
		char* p = grub_strdup(input);
		*size = p ? grub_strlen(p) + 1 : 0;
		return p;
	}
	else if (grub_strcasecmp(type, "UCS2") == 0)
	{
		grub_uint16_t* var16;
		grub_size_t len, len16;

		len = grub_strlen(input);
		len16 = len * GRUB_MAX_UTF16_PER_UTF8;
		var16 = grub_calloc(len16 + 1, sizeof(grub_uint16_t));
		if (!var16)
		{
			grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
			return NULL;
		}
		len16 = grub_utf8_to_utf16(var16, len16, (const grub_uint8_t*)input, len, NULL);
		var16[len16] = 0;
		*size = (len16 + 1) * sizeof(grub_uint16_t);
		return var16;
	}
	else
		grub_error(GRUB_ERR_BAD_ARGUMENT, "unsupported type");
	return NULL;
}

static grub_err_t
cmd_setvar(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	int unset = 0;
	const char* guid = GRUB_EFI_GV_GUID;
	const char* type = "HEX";
	const char* input = NULL;
	const char* var = NULL;
	grub_size_t var_size = 0;
	void* var_data = NULL;
	grub_uint32_t attribute = GRUB_EFI_VARIABLE_NON_VOLATILE | GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS | GRUB_EFI_VARIABLE_RUNTIME_ACCESS;
	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "-g=", 3) == 0)
			guid = &argv[i][3];
		else if (grub_strcmp(argv[i], "-u") == 0)
			unset = 1;
		else if (grub_strncmp(argv[i], "-t=", 3) == 0)
			type = &argv[i][3];
		else if (grub_strncmp(argv[i], "-i=", 3) == 0)
			input = &argv[i][3];
		else if (grub_strncmp(argv[i], "-a=", 3) == 0)
			attribute = grub_strtoul(&argv[i][3], NULL, 0);
		else
			var = argv[i];
	}
	if (!var)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing variable name");
	if (unset)
		return grub_efi_set_variable(var, guid, 0, NULL, NULL);
	if (!input)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "missing input data");
	var_data = get_var_data(type, input, &var_size);
	if (!var_data)
		goto fail;
	grub_efi_set_variable(var, guid, var_size, var_data, &attribute);

fail:
	if (var_data)
		grub_free(var_data);

	return grub_errno;
}

static void
help_setvar(struct grub_command* cmd)
{
	grub_printf("%s OPTION VARIABLE\n", cmd->name);
	grub_printf("Set a UEFI variable.\nOPTIONS:\n");
	grub_printf("  -g={GUID}    Specify the GUID of the UEFI variable.\n");
	grub_printf("  -u           Unset UEFI variable.\n");
	grub_printf("  -t=TYPE      Specify the type of the UEFI variable:\n");
	grub_printf("     HEX       Hexadecimal bytes [default]\n");
	grub_printf("     UTF8      UTF-8 encoded string\n");
	grub_printf("     UCS2      UCS-2 encoded string\n");
	//grub_printf("     FILE      Raw data from file\n");
	grub_printf("  -i=DATA      Specify input data.\n");
	grub_printf("  -a=ATTR      Specify variable attributes. [default=7]\n");
}

struct grub_command grub_cmd_fwsetvar =
{
	.name = "fwsetvar",
	.func = cmd_setvar,
	.help = help_setvar,
	.next = 0,
};
