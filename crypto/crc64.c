// SPDX-License-Identifier: GPL-3.0-or-later

#include "compat.h"
#include "crypto.h"
#include "gcry_wrap.h"

static grub_uint64_t crc64_table[256];

/* Helper for init_crc64_table.  */
static grub_uint64_t
reflect(grub_uint64_t ref, int len)
{
    grub_uint64_t result = 0;
    int i;

    for (i = 1; i <= len; i++)
    {
        if (ref & 1)
            result |= 1ULL << (len - i);
        ref >>= 1;
    }

    return result;
}

static void
init_crc64_table(void)
{
    grub_uint64_t polynomial = 0x42f0e1eba9ea3693ULL;
    int i, j;

    for (i = 0; i < 256; i++)
    {
        crc64_table[i] = reflect(i, 8) << 56;
        for (j = 0; j < 8; j++)
        {
            crc64_table[i] = (crc64_table[i] << 1) ^
                (crc64_table[i] & (1ULL << 63) ? polynomial : 0);
        }
        crc64_table[i] = reflect(crc64_table[i], 64);
    }
}

static void
crc64_init(void* context)
{
    if (!crc64_table[1])
        init_crc64_table();
    *(grub_uint64_t*)context = 0;
}

static void
crc64_write(void* context, const void* buf, grub_size_t size)
{
    unsigned i;
    const grub_uint8_t* data = buf;
    grub_uint64_t crc = ~grub_le_to_cpu64(*(grub_uint64_t*)context);

    for (i = 0; i < size; i++)
    {
        crc = (crc >> 8) ^ crc64_table[(crc & 0xFF) ^ *data];
        data++;
    }

    *(grub_uint64_t*)context = grub_cpu_to_le64(~crc);
}

static grub_uint8_t*
crc64_read(void* context)
{
    return context;
}

static void
crc64_final(void* context)
{
    (void)context;
}

gcry_md_spec_t _gcry_digest_spec_crc64 =
{
  "CRC64", 0, 0, 0, 8,
  crc64_init, crc64_write, crc64_final, crc64_read,
  sizeof(grub_uint64_t),
  .blocksize = 64
};
