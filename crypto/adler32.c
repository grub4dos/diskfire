// SPDX-License-Identifier: GPL-3.0-or-later

#include "compat.h"
#include "crypto.h"
#include "gcry_wrap.h"

struct adler32_context
{
	grub_uint16_t a, b;
	grub_uint32_t c;
};

static void
adler32_init(void* context)
{
	struct adler32_context* ctx = context;

	ctx->a = 1;
	ctx->b = 0;
}

#define MOD 65521

static grub_uint16_t
mod_add(grub_uint16_t a, grub_uint16_t b)
{
	if ((grub_uint32_t)a + (grub_uint32_t)b >= MOD)
		return a + b - MOD;
	return a + b;
}

static void
adler32_write(void* context, const void* inbuf, grub_size_t inlen)
{
	struct adler32_context* ctx = context;
	const grub_uint8_t* ptr = inbuf;

	while (inlen)
	{
		ctx->a = mod_add(ctx->a, *ptr);
		ctx->b = mod_add(ctx->a, ctx->b);
		inlen--;
		ptr++;
	}
}

static void
adler32_final(void* context)
{
	(void)context;
}

static grub_uint8_t*
adler32_read(void* context)
{
	struct adler32_context* ctx = context;
	if (ctx->a > MOD)
		ctx->a -= MOD;
	if (ctx->b > MOD)
		ctx->b -= MOD;
	ctx->c = grub_cpu_to_be32(ctx->a | (ctx->b << 16));
	return (grub_uint8_t*)&ctx->c;
}

gcry_md_spec_t _gcry_digest_spec_adler32 =
{
  "ADLER32", 0, 0, 0, 4,
  adler32_init, adler32_write, adler32_final, adler32_read,
  sizeof(struct adler32_context),
  .blocksize = 64
};
