// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _COMMANDS_HEADER
#define _COMMANDS_HEADER 1

#include "compat.h"

grub_err_t cmd_ls(int argc, char* argv[]);

grub_err_t cmd_extract(int argc, char* argv[]);

grub_err_t cmd_probe(int argc, char* argv[]);

grub_err_t
loopback_delete(const char* name);

grub_err_t
loopback_add(const char* name);

#endif
