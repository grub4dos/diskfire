// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _CRYPTO_HEADER
#define _CRYPTO_HEADER 1

#include "compat.h"

/* Don't rely on this. Check!  */
#define GRUB_CRYPTO_MAX_MDLEN 64
#define GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE 16
#define GRUB_CRYPTO_MAX_MD_CONTEXT_SIZE 256

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

void
grub_crypto_hash(const gcry_md_spec_t* hash, void* out, const void* in, grub_size_t inlen);

const gcry_md_spec_t*
grub_crypto_lookup_md_by_name(const char* name);

extern gcry_md_spec_t _gcry_digest_spec_adler32;
extern gcry_md_spec_t _gcry_digest_spec_crc32;
extern gcry_md_spec_t _gcry_digest_spec_crc64;
extern gcry_md_spec_t _gcry_digest_spec_sha256;

#define GRUB_MD_ADLER32 ((const gcry_md_spec_t *) &_gcry_digest_spec_adler32)
#define GRUB_MD_CRC32 ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)
#define GRUB_MD_CRC64 ((const gcry_md_spec_t *) &_gcry_digest_spec_crc64)
#define GRUB_MD_SHA256 ((const gcry_md_spec_t *) &_gcry_digest_spec_sha256)

#ifndef STR
#define STR(v) #v
#endif
#define STR2(v) STR(v)
#define DIM(v) (sizeof(v)/sizeof((v)[0]))
#define DIMof(type,member) DIM(((type *)0)->member)

/* Stack burning.  */

void _gcry_burn_stack(int bytes);


/* To avoid that a compiler optimizes certain memset calls away, these
   macros may be used instead. */
#define wipememory2(_ptr,_set,_len) do { \
              volatile char *_vptr=(volatile char *)(_ptr); \
              size_t _vlen=(_len); \
              while(_vlen) { *_vptr=(_set); _vptr++; _vlen--; } \
                  } while(0)
#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)

#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#define ror(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )

#endif
