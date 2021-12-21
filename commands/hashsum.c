// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat.h"
#include "command.h"
#include "file.h"
#include "crypto.h"

static grub_err_t
hash_file(grub_file_t file, const gcry_md_spec_t* hash, void* result)
{
	void* context;
	grub_uint8_t* readbuf;
#define BUF_SIZE 4096
	readbuf = grub_malloc(BUF_SIZE);
	if (!readbuf)
		return grub_errno;
	context = grub_zalloc(hash->contextsize);
	if (!readbuf || !context)
		goto fail;

	hash->init(context);
	while (1)
	{
		grub_ssize_t r;
		r = grub_file_read(file, readbuf, BUF_SIZE);
		if (r < 0)
			goto fail;
		if (r == 0)
			break;
		hash->write(context, readbuf, r);
	}
	hash->final(context);
	grub_memcpy(result, hash->read(context), hash->mdlen);

	grub_free(readbuf);
	grub_free(context);

	return GRUB_ERR_NONE;

fail:
	grub_free(readbuf);
	grub_free(context);
	return grub_errno;
}

static grub_err_t
hash_opt(const char* opt, grub_file_t file)
{
	unsigned i;
	const gcry_md_spec_t* hash = NULL;
	if (opt)
		hash = grub_crypto_lookup_md_by_name(opt);
	if (!hash)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "unknown hash");
	if (hash->mdlen > GRUB_CRYPTO_MAX_MDLEN)
		return grub_error(GRUB_ERR_BUG, "mdlen is too long");
	GRUB_PROPERLY_ALIGNED_ARRAY(result, GRUB_CRYPTO_MAX_MDLEN);
	grub_errno = GRUB_ERR_NONE;
	if (hash_file(file, hash, result) != GRUB_ERR_NONE)
		return grub_errno;
	for (i = 0; i < hash->mdlen; i++)
		grub_printf("%02x", ((grub_uint8_t*)result)[i]);
	grub_printf("\n");
	return GRUB_ERR_NONE;
}

static grub_err_t cmd_hashsum(struct grub_command* cmd, int argc, char* argv[])
{
	(void)cmd;
	grub_file_t file = 0;
	if (argc < 2 || grub_strlen(argv[0]) < 3 || grub_strncmp(argv[0], "--", 2) != 0)
	{
		grub_error(GRUB_ERR_BAD_ARGUMENT, "invalid argument");
		goto out;
	}
	file = grub_file_open(argv[1], GRUB_FILE_TYPE_HASH | GRUB_FILE_TYPE_NO_DECOMPRESS);
	if (!file)
		goto out;
	hash_opt(&argv[0][2], file);
out:
	if (grub_errno)
		grub_print_error();
	if (file)
		grub_file_close(file);
	return 0;
}

static void
help_hashsum(struct grub_command* cmd)
{
	grub_printf("%s OPTIONS FILE\n", cmd->name);
	grub_printf("Compute hash checksum.\nOPTIONS:\n");
	grub_printf("  --md5      Compute MD5 checksum.\n");
	grub_printf("  --crc32    Compute CRC32 checksum.\n");
	//grub_printf("  --crc64    Compute CRC64 checksum.\n");
	grub_printf("  --sha1     Compute SHA1 checksum.\n");
	grub_printf("  --sha256   Compute SHA256 checksum.\n");
}

struct grub_command grub_cmd_hashsum =
{
	.name = "hashsum",
	.func = cmd_hashsum,
	.help = help_hashsum,
	.next = 0,
};