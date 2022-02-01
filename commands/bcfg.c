// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "charset.h"
#include "efi.h"

/*
  typedef struct _EFI_LOAD_OPTION
  {
	UINT32 Attributes;
	UINT16 FilePathListLength;
	CHAR16 Description[];
	EFI_DEVICE_PATH_PROTOCOL FilePathList[];
	UINT8 OptionalData[];
  } EFI_LOAD_OPTION;
*/

/*
 * EFI_LOAD_OPTION Attributes
 * All values 0x00000200-0x00001F00 are reserved
 * If a load option is marked as LOAD_OPTION_ACTIVE,
 * the boot manager will attempt to boot automatically
 * using the device path information in the load option.
 * This provides an easy way to disable or enable load options
 * without needing to delete and re-add them.
 *
 * If any Driver#### load option is marked as LOAD_OPTION_FORCE_RECONNECT,
 * then all of the UEFI drivers in the system
 * will be disconnected and reconnected after the last
 * Driver#### load option is processed.
 * This allows a UEFI driver loaded with a Driver#### load option
 * to override a UEFI driver that was loaded prior to the execution
 * of the UEFI Boot Manager.
 *
 * The LOAD_OPTION_CATEGORY is a sub-field of Attributes that provides
 * details to the boot manager to describe how it should group
 * the Boot#### load options. This field is ignored for variables
 * of the form Driver####, SysPrep####,or OsRecovery####.
 * Boot#### load options with LOAD_OPTION_CATEGORY
 * set to LOAD_OPTION_CATEGORY_BOOT are meant to be part of
 * the normal boot processing.
 *
 * Boot#### load options with LOAD_OPTION_CATEGORY
 * set to LOAD_OPTION_CATEGORY_APP are executables which are not
 * part of the normal boot processing but can be optionally chosen
 * for execution if boot menu is provided, or via Hot Keys.
 *
 * Boot options with reserved category values, will be ignored by
 * the boot manager. If any Boot#### load option is marked
 * as LOAD_OPTION_HIDDEN, then the load option will not appear
 * in the menu (if any) provided by the boot manager for load option selection.
 */
#define LOAD_OPTION_ACTIVE          0x00000001  // AC
#define LOAD_OPTION_FORCE_RECONNECT 0x00000002  // FR
#define LOAD_OPTION_HIDDEN          0x00000008  // HI
#define LOAD_OPTION_CATEGORY        0x00001F00  // CT
#define LOAD_OPTION_CATEGORY_BOOT   0x00000000  // CB
#define LOAD_OPTION_CATEGORY_APP    0x00000100  // CA

 /*
  * Boot Manager Capabilities
  * The boot manager can report its capabilities through the
  * global variable BootOptionSupport.
  * If the global variable is not present, then an installer
  * or application must act as if a value of 0 was returned.
  *
  * If EFI_BOOT_OPTION_SUPPORT_KEY is set then the boot manager
  * supports launching of Boot#### load options using key presses.
  *
  * If EFI_BOOT_OPTION_SUPPORT_APP is set then the boot manager
  * supports boot options with LOAD_OPTION_CATEGORY_APP.
  *
  * If EFI_BOOT_OPTION_SUPPORT_SYSPREP is set then the boot manager
  * supports boot options of form SysPrep####.
  *
  * The value specified in EFI_BOOT_OPTION_SUPPORT_COUNT
  * describes the maximum number of key presses which the boot manager
  * supports in the EFI_KEY_OPTION.KeyData.InputKeyCount.
  * This value is only valid if EFI_BOOT_OPTION_SUPPORT_KEY is set.
  * Key sequences with more keys specified are ignored.
  */
#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002
#define EFI_BOOT_OPTION_SUPPORT_SYSPREP 0x00000010
#define EFI_BOOT_OPTION_SUPPORT_COUNT   0x00000300

PRAGMA_BEGIN_PACKED
struct _efi_loadopt
{
	grub_uint32_t attr;
	grub_uint16_t dp_len;
	grub_uint8_t data[0];
};
PRAGMA_END_PACKED
typedef struct _efi_loadopt* efi_loadopt;

struct _bcfg_loadopt
{
	grub_uint32_t attr;
	char* desc;
	grub_efi_device_path_t* dp;
	void* data;
	grub_size_t data_len;
};
typedef struct _bcfg_loadopt* bcfg_loadopt;

static grub_size_t
u16strsize(const grub_uint16_t* str)
{
	grub_size_t len = 0;
	while (*(str++))
		len++;
	len = sizeof(grub_uint16_t) * (len + 1);
	return len;
}

static int
u8u16strncmp(const grub_uint16_t* s1, const char* s2, grub_size_t n)
{
	if (!n)
		return 0;
	while (*s1 && *s2 && --n)
	{
		if (*s1 != (grub_uint16_t)*s2)
			break;
		s1++;
		s2++;
	}
	return (int)*s1 - (int)*s2;
}

static void* efi_get_env(const char* var, grub_size_t* datasize_out)
{
	void* ret = NULL;
	const char* guid = GRUB_EFI_GV_GUID;
	grub_efi_get_variable(var, guid, datasize_out, &ret, NULL);
	return ret;
}

static void efi_set_env(const char* var, grub_size_t datasize, void* data)
{
	const char* guid = GRUB_EFI_GV_GUID;
	grub_efi_set_variable(var, guid, datasize, data, NULL);
}

static void parse_flag(grub_uint32_t* attr, grub_uint32_t flag, char op)
{
	if (op == '+')
		*attr |= flag;
	if (op == '-')
		*attr &= ~flag;
	if (op == '^')
		*attr ^= flag;
}

static void loadopt_str_to_attr(const char* str, grub_uint32_t* attr)
{
	grub_size_t len, i;
	if (!str)
		return;
	len = grub_strlen(str);
	if (len < 3 && len > 18)
		return;
	if (str[0] == '0' && str[1] == 'x')
		*attr = grub_strtoul(str, NULL, 16);
	for (i = 0; i < len; i += 3)
	{
		if (str[i + 1] == '\0' || str[i + 2] == '\0')
			break;
		if (grub_strncmp(&str[i], "AC", 2) == 0)
			parse_flag(attr, LOAD_OPTION_ACTIVE, str[i + 2]);
		if (grub_strncmp(&str[i], "FR", 2) == 0)
			parse_flag(attr, LOAD_OPTION_FORCE_RECONNECT, str[i + 2]);
		if (grub_strncmp(&str[i], "HI", 2) == 0)
			parse_flag(attr, LOAD_OPTION_HIDDEN, str[i + 2]);
		if (grub_strncmp(&str[i], "CT", 2) == 0)
			parse_flag(attr, LOAD_OPTION_CATEGORY, str[i + 2]);
		if (grub_strncmp(&str[i], "CB", 2) == 0)
			parse_flag(attr, LOAD_OPTION_CATEGORY_BOOT, str[i + 2]);
		if (grub_strncmp(&str[i], "CA", 2) == 0)
			parse_flag(attr, LOAD_OPTION_CATEGORY_APP, str[i + 2]);
	}
}

static void loadopt_free(bcfg_loadopt loadopt)
{
	if (loadopt->desc)
		grub_free(loadopt->desc);
	loadopt->desc = NULL;
	if (loadopt->dp)
		grub_free(loadopt->dp);
	loadopt->dp = NULL;
	if (loadopt->data)
		grub_free(loadopt->data);
	loadopt->data = NULL;
	loadopt->data_len = 0;
	loadopt->attr = 0;
}

static int bcfg_env_check_num(const char* str, grub_uint16_t* num)
{
	if (!str || grub_strlen(str) != 4 ||
		!grub_isxdigit(str[0]) || !grub_isxdigit(str[1]) ||
		!grub_isxdigit(str[2]) || !grub_isxdigit(str[3]))
		return 0;
	if (num)
		*num = (grub_uint16_t)grub_strtoul(str, NULL, 16);
	return 1;
}

static const char* bcfg_env_check_name(const char* str)
{
	if (!str)
		return NULL;
	if (grub_strcmp(str, "boot") == 0)
		return "Boot";
	else if (grub_strcmp(str, "driver") == 0)
		return "Driver";
	else if (grub_strcmp(str, "sysprep") == 0)
		return "SysPrep";
	return NULL;
}

static void
loadopt_dump(bcfg_loadopt loadopt)
{
	grub_printf("Description: %s\n", loadopt->desc ? loadopt->desc : "(null)");
	grub_printf("Path: ");
	if (loadopt->dp)
		grub_efi_print_device_path(loadopt->dp);
	else
		grub_printf("(null)\n");
	grub_printf("Attributes: AC%sFR%sHI%sCT%sCB%sCA%s\n",
		(loadopt->attr & LOAD_OPTION_ACTIVE) ? "+" : "-",
		(loadopt->attr & LOAD_OPTION_FORCE_RECONNECT) ? "+" : "-",
		(loadopt->attr & LOAD_OPTION_HIDDEN) ? "+" : "-",
		(loadopt->attr & LOAD_OPTION_CATEGORY) ? "+" : "-",
		(loadopt->attr & LOAD_OPTION_CATEGORY_BOOT) ? "+" : "-",
		(loadopt->attr & LOAD_OPTION_CATEGORY_APP) ? "+" : "-");
}

static grub_err_t
bcfg_env_get(const char* env, bcfg_loadopt loadopt)
{
	efi_loadopt data = NULL;
	grub_size_t size = 0, data_ofs;

	data = efi_get_env(env, &size);
	if (!data)
		return grub_error(GRUB_ERR_FILE_NOT_FOUND, N_("No such variable"));

	if (size < sizeof(grub_uint16_t) + sizeof(grub_uint32_t) ||
		size < data->dp_len + 2 * sizeof(grub_uint16_t) + sizeof(grub_uint32_t))
	{
		grub_free(data);
		return grub_error(GRUB_ERR_IO, "invalid bootopt");
	}
	size = size - (sizeof(grub_uint16_t) + sizeof(grub_uint32_t));
	loadopt_free(loadopt);

	loadopt->attr = data->attr;

	data_ofs = u16strsize((void*)data->data);
	loadopt->desc = grub_calloc(data_ofs / sizeof(grub_uint16_t), GRUB_MAX_UTF8_PER_UTF16);
	if (!loadopt->desc)
	{
		grub_free(data);
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	}
	*grub_utf16_to_utf8((grub_uint8_t*)loadopt->desc, (void*)data->data, data_ofs) = '\0';

	loadopt->dp = grub_efi_duplicate_device_path((void*)(data->data + data_ofs));
	if (!loadopt->dp)
	{
		grub_free(data);
		loadopt_free(loadopt);
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	}

	data_ofs += data->dp_len;
	if (data_ofs < size)
	{
		loadopt->data_len = size - data_ofs;
		loadopt->data = grub_malloc(loadopt->data_len);
		if (!loadopt->data)
		{
			grub_free(data);
			loadopt_free(loadopt);
			return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		}
		grub_memcpy(loadopt->data, data->data + data_ofs, loadopt->data_len);
	}
	grub_free(data);
	return GRUB_ERR_NONE;
}

static grub_err_t bcfg_env_set(const char* env,
	bcfg_loadopt loadopt)
{
	efi_loadopt data = NULL;
	grub_uint16_t* desc = NULL;
	grub_uint16_t dp_len;
	grub_size_t size, desc_len;

	desc_len = (grub_strlen(loadopt->desc) + 1) * sizeof(grub_uint16_t);
	desc = grub_zalloc(desc_len);
	if (!desc)
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	grub_utf8_to_utf16(desc, desc_len, (void*)loadopt->desc, (grub_size_t)-1, NULL);
	dp_len = grub_efi_get_dp_size(loadopt->dp);
	desc_len = u16strsize(desc);

	size = sizeof(grub_uint16_t) + sizeof(grub_uint32_t)
		+ desc_len + dp_len + loadopt->data_len;
	data = grub_zalloc(size);
	if (!data)
	{
		grub_free(desc);
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
	}
	data->attr = loadopt->attr;
	data->dp_len = dp_len;
	grub_memcpy(data->data, desc, desc_len);
	grub_memcpy(data->data + desc_len, loadopt->dp, dp_len);
	grub_memcpy(data->data + desc_len + dp_len,
		loadopt->data, loadopt->data_len);

	efi_set_env(env, size, data);

	grub_free(desc);
	grub_free(data);
	return grub_errno;
}

static grub_err_t cmd_bcfg(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	char env[30];
	const char* prefix;
	struct _bcfg_loadopt loadopt = { 0, NULL, NULL, NULL, 0 };
	if (!argc)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "bad argument");
	prefix = bcfg_env_check_name(argv[0]);
	if (prefix && argc >= 2)
	{
		if (argc >= 3 && bcfg_env_check_num(argv[2], NULL))
		{
			grub_snprintf(env, 30, "%s%s", prefix, argv[2]);
			if (grub_strcmp(argv[1], "dump") == 0)
			{
				if (bcfg_env_get(env, &loadopt))
					goto fail;
				loadopt_dump(&loadopt);
				goto fail;
			}
			else if (argc >= 4 && grub_strcmp(argv[1], "add") == 0)
			{
				loadopt.dp = grub_efi_convert_file_path(argv[3]);
				if (argc >= 5)
					loadopt.desc = grub_strdup(argv[4]);
				else
					loadopt.desc = grub_strdup("EFI BOOT");
				if (argc >= 6)
					loadopt_str_to_attr(argv[5], &loadopt.attr);
				bcfg_env_set(env, &loadopt);
				goto fail;
			}
			else if (argc >= 3 && grub_strcmp(argv[1], "del") == 0)
			{
				efi_set_env(env, 0, NULL);
				goto fail;
			}
			else if (argc >= 5 && grub_strcmp(argv[1], "edit") == 0)
			{
				if (bcfg_env_get(env, &loadopt))
					goto fail;
				if (grub_strcmp(argv[3], "desc") == 0)
					loadopt.desc = grub_strdup(argv[4]);
				else if (grub_strcmp(argv[3], "file") == 0)
					loadopt.dp = grub_efi_convert_file_path(argv[4]);
				else if (grub_strcmp(argv[3], "attr") == 0)
					loadopt_str_to_attr(argv[4], &loadopt.attr);
				else
				{
					grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid option %s", argv[3]);
					goto fail;
				}
				bcfg_env_set(env, &loadopt);
				goto fail;
			}
		}
	}
fail:
	loadopt_free(&loadopt);
	return grub_errno;
}

static void
help_bcfg(struct grub_command* cmd)
{
	grub_printf("%s OPERAND ..\n", cmd->name);
	grub_printf("Manage the boot options that are stored in NVRAM.\nUsage:\n");
	grub_printf("  bcfg boot|driver dump ####\n");
	grub_printf("  bcfg boot|driver add #### FILE DESC [ATTR]\n");
	grub_printf("  bcfg boot|driver del ####\n");
	grub_printf("  bcfg boot|driver edit #### desc|file|attr DATA\n");
#if 0
	grub_printf("  bcfg timeout|bootnext dump\n");
	grub_printf("  bcfg timeout|bootnext set ####\n");
	grub_printf("  bcfg timeout|bootnext unset\n");

	grub_printf("  bcfg bootorder|driverorder dump\n");
	grub_printf("  bcfg bootorder|driverorder swap #### ####\n");
	grub_printf("  bcfg bootorder|driverorder add ####\n");
	grub_printf("  bcfg bootorder|driverorder del ####\n");
#endif
}

struct grub_command grub_cmd_bcfg =
{
	.name = "bcfg",
	.func = cmd_bcfg,
	.help = help_bcfg,
	.next = 0,
};
