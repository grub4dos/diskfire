/*
** $Id: linit.c,v 1.39.1.1 2017/04/19 17:20:42 roberto Exp $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"

#include <include/compat.h>
#include <include/command.h>
#include <include/disk.h>
#include <include/file.h>

/* Updates the globals grub_errno and grub_msg, leaving their values on the
   top of the stack, and clears grub_errno. When grub_errno is zero, grub_msg
   is not left on the stack. The value returned is the number of values left on
   the stack. */
int
push_result(lua_State* L)
{
	int saved_errno;
	int num_results;

	saved_errno = grub_errno;
	grub_errno = 0;

	/* Push once for setfield, and again to leave on the stack */
	lua_pushinteger(L, saved_errno);
	lua_pushinteger(L, saved_errno);
	lua_setglobal(L, "grub_errno");

	if (saved_errno)
	{
		/* Push once for setfield, and again to leave on the stack */
		lua_pushstring(L, grub_errmsg);
		lua_pushstring(L, grub_errmsg);
		num_results = 2;
	}
	else
	{
		lua_pushnil(L);
		num_results = 1;
	}

	lua_setglobal(L, "grub_errmsg");

	return num_results;
}

static int
df_enum_device_iter(const char* name, void* data)
{
	lua_State* L = data;
	lua_Integer result;
	grub_disk_t disk;
	const char* d_human_sizes[6] = { "B", "KB", "MB", "GB", "TB", "PB", };

	result = 0;
	disk = grub_disk_open(name);
	if (disk)
	{
		grub_fs_t fs;
		const char* human_size = NULL;
		lua_pushvalue(L, 1);
		lua_pushstring(L, name);
		fs = grub_fs_probe(disk);
		if (fs)
		{
			lua_pushstring(L, fs->name);
			if (!fs->fs_uuid)
				lua_pushnil(L);
			else
			{
				int err;
				char* uuid = NULL;
				err = fs->fs_uuid(disk, &uuid);
				if (err || !uuid)
				{
					grub_errno = 0;
					lua_pushnil(L);
				}
				else
				{
					lua_pushstring(L, uuid);
					grub_free(uuid);
				}
			}
			if (!fs->fs_label)
				lua_pushnil(L);
			else
			{
				int err;
				char* label = NULL;
				err = fs->fs_label(disk, &label);
				if (err || !label)
				{
					grub_errno = 0;
					lua_pushnil(L);
				}
				else
				{
					lua_pushstring(L, label);
					grub_free(label);
				}
			}
		}
		else
		{
			grub_errno = 0;
			lua_pushnil(L); // fs name
			lua_pushnil(L); // fs uuid
			lua_pushnil(L); // fs label
		}
		human_size = grub_get_human_size(grub_disk_native_sectors(disk)
			<< GRUB_DISK_SECTOR_BITS, d_human_sizes, 1024);
		if (human_size)
			lua_pushstring(L, human_size);
		else
			lua_pushnil(L);
		lua_call(L, 5, 1);
		result = lua_tointeger(L, -1);
		lua_pop(L, 1);
		grub_disk_close(disk);
	}
	else
		grub_errno = 0;

	return (int)result;
}

static int
df_enum_device(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TFUNCTION);
	grub_disk_iterate(df_enum_device_iter, L);
	return push_result(L);
}

static int
df_enum_file_iter(const char* name, const struct grub_dirhook_info* info, void* data)
{
	lua_Integer result;
	lua_State* L = data;
	if (grub_strcmp(name, ".") == 0 || grub_strcmp(name, "..") == 0 ||
		grub_strcmp(name, "System Volume Information") == 0)
		return 0;

	lua_pushvalue(L, 1);
	lua_pushstring(L, name);
	lua_pushinteger(L, info->dir != 0);
	lua_call(L, 2, 1);
	result = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (int)result;
}

static int
df_enum_file(lua_State* L)
{
	char* device_name;
	const char* arg;
	grub_disk_t disk;

	luaL_checktype(L, 1, LUA_TFUNCTION);
	arg = luaL_checkstring(L, 2);
	device_name = grub_file_get_device_name(arg);
	disk = grub_disk_open(device_name);
	if (disk)
	{
		grub_fs_t fs;
		const char* path;
		fs = grub_fs_probe(disk);
		path = grub_strchr(arg, ')');
		if (!path)
			path = arg;
		else
			path++;
		if ((!path && !device_name) || !*path)
			grub_printf("invalid path\n");
		if (fs)
			(fs->fs_dir) (disk, path, df_enum_file_iter, L);
		grub_disk_close(disk);
	}
	grub_free(device_name);
	return push_result(L);
}

//int luaopen_winapi(lua_State* L);

static int
df_cmd(lua_State* L)
{
	int argc = 0, i;
	char** argv = NULL;
	const char* cmd;
	cmd = luaL_checkstring(L, 1);
	argc = lua_gettop(L) - 1;
	if (argc <= 0)
		goto exec;
	argv = calloc(argc, sizeof(char*));
	if (!argv)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		goto fail;
	}
	for (i = 0; i < argc; i++)
	{
		const char* p = luaL_checkstring(L, 2 + i);
		argv[i] = grub_strdup(p);
		if (!argv[i])
		{
			grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
			goto fail;
		}
	}
exec:
	grub_command_execute(cmd, argc, argv);
fail:
	if (argv)
	{
		for (i = 0; (argv[i] != NULL) && (i < argc); i++)
			free(argv[i]);
		free(argv);
	}
	return push_result(L);
}

static const luaL_Reg dflib[] =
{
	{"enum_device", df_enum_device},
	{"enum_file", df_enum_file},
	{"cmd", df_cmd},
	{NULL, NULL}
};

LUAMOD_API int luaopen_df(lua_State* L)
{
	luaL_newlib(L, dflib);
	return 1;
}

/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const luaL_Reg loadedlibs[] =
{
	{"_G", luaopen_base},
	{LUA_LOADLIBNAME, luaopen_package},
	{LUA_COLIBNAME, luaopen_coroutine},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_IOLIBNAME, luaopen_io},
	{LUA_OSLIBNAME, luaopen_os},
	{LUA_STRLIBNAME, luaopen_string},
#if !defined(LUA_NUMBER_INTEGRAL)
	{LUA_MATHLIBNAME, luaopen_math},
#endif
	{LUA_UTF8LIBNAME, luaopen_utf8},
	{LUA_DBLIBNAME, luaopen_debug},
#if defined(LUA_COMPAT_BITLIB)
	{LUA_BITLIBNAME, luaopen_bit32},
#endif
	{"df", luaopen_df},
	//{"winapi", luaopen_winapi},
	{NULL, NULL}
};

LUALIB_API void luaL_openlibs (lua_State *L)
{
	const luaL_Reg *lib;
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++)
	{
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}
