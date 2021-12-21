// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DEFLATE_HEADER
#define _DEFLATE_HEADER 1

#include "compat.h"

grub_ssize_t
grub_zlib_decompress (char *inbuf, grub_size_t insize, grub_off_t off,
	char *outbuf, grub_size_t outsize);

grub_ssize_t
grub_deflate_decompress (char *inbuf, grub_size_t insize, grub_off_t off,
	char *outbuf, grub_size_t outsize);

#endif
