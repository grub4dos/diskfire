// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _COMMANDS_HEADER
#define _COMMANDS_HEADER 1

#include "compat.h"

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

extern struct grub_command grub_cmd_ls;
extern struct grub_command grub_cmd_extract;
extern struct grub_command grub_cmd_probe;
extern struct grub_command grub_cmd_hashsum;
extern struct grub_command grub_cmd_hxd;
extern struct grub_command grub_cmd_stat;

extern struct grub_command grub_cmd_help;

grub_err_t
loopback_delete(const char* name);

grub_err_t
loopback_add(const char* name);

#endif
