// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _COMMANDS_HEADER
#define _COMMANDS_HEADER 1

#include "compat.h"

struct grub_file;
struct grub_command;

typedef grub_err_t(*grub_command_func_t) (struct grub_command* cmd, int argc, char* argv[]);
typedef void(*grub_command_help_t) (struct grub_command* cmd);

struct grub_command
{
	struct grub_command* next;
	struct grub_command** prev;
	const char* name;
	grub_command_func_t func;
	grub_command_help_t help;
};
typedef struct grub_command* grub_command_t;

extern grub_command_t grub_command_list;

void grub_command_register(struct grub_command* cmd);

grub_command_t grub_command_find(const char* name);

grub_err_t grub_command_execute(const char* name, int argc, char** argv);

#define FOR_COMMANDS(var) FOR_LIST_ELEMENTS((var), grub_command_list)

void grub_command_init(void);

extern struct grub_command grub_cmd_bin2c;
extern struct grub_command grub_cmd_ls;
extern struct grub_command grub_cmd_extract;
extern struct grub_command grub_cmd_probe;
extern struct grub_command grub_cmd_hashsum;
extern struct grub_command grub_cmd_hxd;
extern struct grub_command grub_cmd_stat;
extern struct grub_command grub_cmd_blocklist;
extern struct grub_command grub_cmd_mbr;
extern struct grub_command grub_cmd_pbr;
extern struct grub_command grub_cmd_dd;
extern struct grub_command grub_cmd_cat;
extern struct grub_command grub_cmd_mount;
extern struct grub_command grub_cmd_umount;

extern struct grub_command grub_cmd_help;

grub_err_t
loopback_delete(const char* name);

grub_err_t
loopback_add(const char* name);

struct grub_procfs_entry
{
	struct grub_procfs_entry* next;
	struct grub_procfs_entry** prev;
	char* name;
	void* data;
	grub_off_t(*get_contents) (struct grub_file* file, void* data, grub_size_t sz);
};

grub_err_t
proc_add(const char* name, void* data,
	grub_off_t(*get_contents) (struct grub_file *file, void* data, grub_size_t sz));

void proc_delete(const char* name);

#endif
