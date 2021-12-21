// SPDX-License-Identifier: LGPL-2.1-or-later

#include "compat.h"
#include "crypto.h"
#include "gcry_wrap.h"

void
_gcry_burn_stack(int bytes)
{
	char buf[64];

	wipememory(buf, sizeof buf);
	bytes -= sizeof buf;
	if (bytes > 0)
		_gcry_burn_stack(bytes);
}

void
grub_crypto_hash(const gcry_md_spec_t* hash, void* out, const void* in, grub_size_t inlen)
{
	GRUB_PROPERLY_ALIGNED_ARRAY(ctx, GRUB_CRYPTO_MAX_MD_CONTEXT_SIZE);

	if (hash->contextsize > sizeof(ctx))
	{
		grub_printf("Too large md context\n");
		exit(-1);
	}
	hash->init(&ctx);
	hash->write(&ctx, in, inlen);
	hash->final(&ctx);
	grub_memcpy(out, hash->read(&ctx), hash->mdlen);
}

const gcry_md_spec_t*
grub_crypto_lookup_md_by_name(const char* name)
{
	const gcry_md_spec_t* md = NULL;
	if (grub_strcasecmp(name, "ADLER32") == 0)
		md = GRUB_MD_ADLER32;
	else if (grub_strcasecmp(name, "CRC32") == 0)
		md = GRUB_MD_CRC32;
	//else if (grub_strcasecmp(name, "CRC64") == 0)
		//md = GRUB_MD_CRC64;
	else if (grub_strcasecmp(name, "SHA1") == 0)
		md = GRUB_MD_SHA1;
	else if (grub_strcasecmp(name, "SHA256") == 0)
		md = GRUB_MD_SHA256;
	else if (grub_strcasecmp(name, "MD5") == 0)
		md = GRUB_MD_MD5;
	return md;
}
