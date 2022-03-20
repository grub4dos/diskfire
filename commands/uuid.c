// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"

static grub_err_t cmd_uuid(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	int braces = 0, guid = 0;
	char buf[39];
	UUID uuid;
	grub_uint32_t* p = (void *)&uuid;
	for (i = 0; i < argc; i++)
	{
		if (grub_strcmp(argv[i], "-g") == 0)
			guid = 1;
		else if (grub_strcmp(argv[i], "-b") == 0)
			braces = 1;
	}
	if (guid)
	{
		if (CoCreateGuid(&uuid) == S_OK)
			goto ok;
		return grub_error(GRUB_ERR_IO, "cannot create guid");
	}
	grub_srand(grub_get_time_ms());
	for (i = 0; i < sizeof (UUID)/sizeof (grub_uint32_t); i++)
		p[i] = grub_rand();
ok:
	grub_snprintf(buf, sizeof(buf), "%s%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x%s",
		braces ? "{" : "", grub_le_to_cpu32(uuid.Data1),
		grub_le_to_cpu16(uuid.Data2), grub_le_to_cpu16(uuid.Data3),
		uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3],
		uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7],
		braces ? "}" : "");
	grub_printf("%s", buf);
	return 0;
}

static void
help_uuid(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS]\n", cmd->name);
	grub_printf("Generate a UUID.\nOPTIONS:\n");
	grub_printf("  -g        Create a GUID.\n");
	grub_printf("  -b        Add braces.\n");
}

struct grub_command grub_cmd_uuid =
{
	.name = "uuid",
	.func = cmd_uuid,
	.help = help_uuid,
	.next = 0,
};
