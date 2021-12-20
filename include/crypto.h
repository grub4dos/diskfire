// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _CRYPTO_HEADER
#define _CRYPTO_HEADER 1

#include "compat.h"

/* Type for the md_init function.  */
typedef void (*gcry_md_init_t) (void* c);

/* Type for the md_write function.  */
typedef void (*gcry_md_write_t) (void* c, const void* buf, grub_size_t nbytes);

/* Type for the md_final function.  */
typedef void (*gcry_md_final_t) (void* c);

/* Type for the md_read function.  */
typedef unsigned char* (*gcry_md_read_t) (void* c);

typedef struct gcry_md_oid_spec
{
	const char* oidstring;
} gcry_md_oid_spec_t;

typedef struct gcry_md_spec
{
	const char* name;
	unsigned char* asnoid;
	int asnlen;
	gcry_md_oid_spec_t* oids;
	grub_size_t mdlen;
	gcry_md_init_t init;
	gcry_md_write_t write;
	gcry_md_final_t final;
	gcry_md_read_t read;
	grub_size_t contextsize; /* allocate this amount of context */
	/* Block size, needed for HMAC.  */
	grub_size_t blocksize;
#ifdef GRUB_UTIL
	const char* modname;
#endif
	struct gcry_md_spec* next;
} gcry_md_spec_t;

extern gcry_md_spec_t _gcry_digest_spec_crc32;

#define GRUB_MD_CRC32 ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)

#endif
