// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"

HRESULT CppCreateShortcut(WCHAR* pTarget, WCHAR* pLnkPath, WCHAR* pParam, WCHAR* pIcon, INT id, INT sw);

static grub_err_t cmd_shortcut(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	int i;
	wchar_t* lnk_target = NULL;
	wchar_t* lnk_path = NULL;
	wchar_t* lnk_param = NULL;
	wchar_t* lnk_icon = NULL;
	int lnk_id = 0;
	int lnk_sw = SW_SHOWNORMAL;
	HRESULT hres;
	for (i = 0; i < argc; i++)
	{
		if (grub_strncmp(argv[i], "-o=", 3) == 0)
		{
			char* path = NULL;
			grub_size_t len = grub_strlen(&argv[i][3]) + 5; // .lnk \0
			path = grub_malloc(len);
			if (!path)
				return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
			grub_snprintf(path, len, "%s.lnk", &argv[i][3]);
			lnk_path = grub_get_utf16(path);
			grub_free(path);
		}
		else if (grub_strncmp(argv[i], "-p=", 3) == 0)
			lnk_param = grub_get_utf16 (&argv[i][3]);
		else if (grub_strncmp(argv[i], "-i=", 3) == 0)
			lnk_icon = grub_get_utf16 (&argv[i][3]);
		else if (grub_strncmp(argv[i], "-d=", 3) == 0)
			lnk_id = grub_strtoul(&argv[i][3], NULL, 0);
		else if (grub_strncmp(argv[i], "-s=", 3) == 0)
		{
			if (grub_strcasecmp(&argv[i][3], "MIN") == 0)
				lnk_sw = SW_SHOWMINNOACTIVE;
			else if (grub_strcasecmp(&argv[i][3], "MAX") == 0)
				lnk_sw = SW_SHOWMAXIMIZED;
			else
				lnk_sw = SW_SHOWNORMAL;
		}
		else
			lnk_target = grub_get_utf16 (argv[i]);
	}
	if (!lnk_target || !lnk_path)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "missing path/target");
		goto fail;
	}
	hres = CppCreateShortcut(lnk_target, lnk_path, lnk_param, lnk_icon, lnk_id, lnk_sw);
	if (SUCCEEDED(hres))
		grub_errno = GRUB_ERR_NONE;
	else
		grub_error(GRUB_ERR_IO, "error create shortcut");
fail:
	if (lnk_target)
		grub_free(lnk_target);
	if (lnk_path)
		grub_free(lnk_path);
	if (lnk_icon)
		grub_free(lnk_icon);
	if (lnk_param)
		grub_free(lnk_param);
	return grub_errno;
}

static void
help_shortcut(struct grub_command* cmd)
{
	grub_printf("%s [OPTIONS] FILE\n", cmd->name);
	grub_printf("Create a shortcut.\nOPTIONS:\n");
	grub_printf("  -o=LINK            Specify the path of the shortcut (no '.lnk' extension).\n");
	grub_printf("  -p=PARAM           Specify the arguments.\n");
	grub_printf("  -i=ICON            Specify the path of icon file.\n");
	grub_printf("  -d=ID              Specify the id of icon in resource file. [default = 0]\n");
	grub_printf("  -s=NORM|MIN|MAX    Specify the expected window state. [default = NORM]\n");
}

struct grub_command grub_cmd_shortcut =
{
	.name = "shortcut",
	.func = cmd_shortcut,
	.help = help_shortcut,
	.next = 0,
};
