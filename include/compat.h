// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _COMPAT_HEADER
#define _COMPAT_HEADER 1

#include <stdint.h>
#include <windows.h>
#include <intsafe.h>

#define PRAGMA_BEGIN_PACKED __pragma(pack(push, 1))
#define PRAGMA_END_PACKED   __pragma(pack(pop))

#define grub_add(a, b, res)	(UIntPtrAdd(a, b, (UINT_PTR *)res) != S_OK)
#define grub_sub(a, b, res)	(UIntPtrSub(a, b, (UINT_PTR *)res) != S_OK)
#define grub_mul(a, b, res)	(UIntPtrMult(a, b, (UINT_PTR *)res) != S_OK)

#define N_(x) x

#define ALIGN_UP(addr, align) \
	(((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_UP_OVERHEAD(addr, align) ((-(addr)) & ((align)-1))
#define ALIGN_DOWN(addr, align) \
	((addr) & ~((align) - 1))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#endif

typedef INT8 grub_int8_t;
typedef INT16 grub_int16_t;
typedef INT32 grub_int32_t;
typedef INT64 grub_int64_t;

typedef UINT8 grub_uint8_t;
typedef UINT16 grub_uint16_t;
typedef UINT32 grub_uint32_t;
#define PRIxGRUB_UINT32_T "x"
#define PRIuGRUB_UINT32_T "u"
typedef UINT64 grub_uint64_t;
#define PRIxGRUB_UINT64_T "llx"
#define PRIuGRUB_UINT64_T "llu"

typedef ULONG_PTR grub_addr_t;
typedef SIZE_T grub_size_t;
typedef SSIZE_T grub_ssize_t;

#ifdef _WIN64
#define PRIxGRUB_SIZE "llx"
#define PRIxGRUB_ADDR "llx"
#define PRIuGRUB_SIZE "llu"
#define PRIdGRUB_SSIZE "lld"
#else
#define PRIxGRUB_SIZE "lx"
#define PRIxGRUB_ADDR "lx"
#define PRIuGRUB_SIZE "lu"
#define PRIdGRUB_SSIZE "ld"
# endif

#define GRUB_SCHAR_MAX 127
#define GRUB_SCHAR_MIN (-GRUB_SCHAR_MAX - 1)
#define GRUB_UCHAR_MAX 0xFF
#define GRUB_USHRT_MAX 65535
#define GRUB_SHRT_MAX 0x7fff
#define GRUB_SHRT_MIN (-GRUB_SHRT_MAX - 1)
#define GRUB_UINT_MAX 4294967295U
#define GRUB_INT_MAX 0x7fffffff
#define GRUB_INT_MIN (-GRUB_INT_MAX - 1)
#define GRUB_INT32_MAX 2147483647
#define GRUB_INT32_MIN (-GRUB_INT32_MAX - 1)

typedef grub_uint64_t grub_properly_aligned_t;

#define GRUB_PROPERLY_ALIGNED_ARRAY(name, size) grub_properly_aligned_t name[((size) + sizeof (grub_properly_aligned_t) - 1) / sizeof (grub_properly_aligned_t)]

/* The type for representing a file offset.  */
typedef grub_uint64_t	grub_off_t;

/* The type for representing a disk block address.  */
typedef grub_uint64_t	grub_disk_addr_t;

PRAGMA_BEGIN_PACKED
struct grub_packed_guid
{
	grub_uint32_t data1;
	grub_uint16_t data2;
	grub_uint16_t data3;
	grub_uint8_t data4[8];
};
PRAGMA_END_PACKED
typedef struct grub_packed_guid grub_packed_guid_t;

int grub_printf(const char* fmt, ...);

int grub_snprintf(char* str, grub_size_t n, const char* fmt, ...);

void grub_dprintf(const char* cond, const char* fmt, ...);

static inline grub_uint16_t grub_swap_bytes16(grub_uint16_t _x)
{
	return (grub_uint16_t)((_x << 8) | (_x >> 8));
}

#define grub_swap_bytes16_compile_time(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define grub_swap_bytes32_compile_time(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000UL) >> 24))
#define grub_swap_bytes64_compile_time(x) \
((grub_uint64_t) ((((grub_uint64_t)x) << 56) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF00ULL) << 40) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF0000ULL) << 24) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF000000ULL) << 8) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF00000000ULL) >> 8) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF0000000000ULL) >> 24) \
					| ((((grub_uint64_t)x) & (grub_uint64_t) 0xFF000000000000ULL) >> 40) \
					| (((grub_uint64_t)x) >> 56)) \
)

static inline grub_uint32_t grub_swap_bytes32(grub_uint32_t _x)
{
	return ((_x << 24)
		| ((_x & (grub_uint32_t)0xFF00UL) << 8)
		| ((_x & (grub_uint32_t)0xFF0000UL) >> 8)
		| (_x >> 24));
}

static inline grub_uint64_t grub_swap_bytes64(grub_uint64_t _x)
{
	return ((_x << 56)
		| ((_x & (grub_uint64_t)0xFF00ULL) << 40)
		| ((_x & (grub_uint64_t)0xFF0000ULL) << 24)
		| ((_x & (grub_uint64_t)0xFF000000ULL) << 8)
		| ((_x & (grub_uint64_t)0xFF00000000ULL) >> 8)
		| ((_x & (grub_uint64_t)0xFF0000000000ULL) >> 24)
		| ((_x & (grub_uint64_t)0xFF000000000000ULL) >> 40)
		| (_x >> 56));
}

# define grub_cpu_to_le16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64(x)	((grub_uint64_t) (x))
# define grub_le_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_le_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_le_to_cpu64(x)	((grub_uint64_t) (x))
# define grub_cpu_to_be16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_be32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_be64(x)	grub_swap_bytes64(x)
# define grub_be_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_be_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_be_to_cpu64(x)	grub_swap_bytes64(x)
# define grub_cpu_to_be16_compile_time(x)	grub_swap_bytes16_compile_time(x)
# define grub_cpu_to_be32_compile_time(x)	grub_swap_bytes32_compile_time(x)
# define grub_cpu_to_be64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_be_to_cpu64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_cpu_to_le16_compile_time(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32_compile_time(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64_compile_time(x)	((grub_uint64_t) (x))

PRAGMA_BEGIN_PACKED
struct grub_unaligned_uint16
{
	grub_uint16_t val;
};
struct grub_unaligned_uint32
{
	grub_uint32_t val;
};
struct grub_unaligned_uint64
{
	grub_uint64_t val;
};
PRAGMA_END_PACKED

typedef struct grub_unaligned_uint16 grub_unaligned_uint16_t;
typedef struct grub_unaligned_uint32 grub_unaligned_uint32_t;
typedef struct grub_unaligned_uint64 grub_unaligned_uint64_t;

static inline grub_uint16_t grub_get_unaligned16(const void* ptr)
{
	const struct grub_unaligned_uint16* dd
		= (const struct grub_unaligned_uint16*)ptr;
	return dd->val;
}

static inline void grub_set_unaligned16(void* ptr, grub_uint16_t val)
{
	struct grub_unaligned_uint16* dd = (struct grub_unaligned_uint16*)ptr;
	dd->val = val;
}

static inline grub_uint32_t grub_get_unaligned32(const void* ptr)
{
	const struct grub_unaligned_uint32* dd
		= (const struct grub_unaligned_uint32*)ptr;
	return dd->val;
}

static inline void grub_set_unaligned32(void* ptr, grub_uint32_t val)
{
	struct grub_unaligned_uint32* dd = (struct grub_unaligned_uint32*)ptr;
	dd->val = val;
}

static inline grub_uint64_t grub_get_unaligned64(const void* ptr)
{
	const struct grub_unaligned_uint64* dd
		= (const struct grub_unaligned_uint64*)ptr;
	return dd->val;
}

static inline void grub_set_unaligned64(void* ptr, grub_uint64_t val)
{
	struct grub_unaligned_uint64* dd = (struct grub_unaligned_uint64*)ptr;
	dd->val = val;
}

struct grub_list
{
	struct grub_list* next;
	struct grub_list** prev;
};
typedef struct grub_list* grub_list_t;

void grub_list_push (grub_list_t* head, grub_list_t item);
void grub_list_remove (grub_list_t item);

#define FOR_LIST_ELEMENTS(var, list) for ((var) = (list); (var); (var) = (var)->next)
#define FOR_LIST_ELEMENTS_NEXT(var, list) for ((var) = (var)->next; (var); (var) = (var)->next)
#define FOR_LIST_ELEMENTS_SAFE(var, nxt, list) for ((var) = (list), (nxt) = ((var) ? (var)->next : 0); (var); (var) = (nxt), ((nxt) = (var) ? (var)->next : 0))

static inline void*
grub_bad_type_cast_real(int line, const char* file)
{
	grub_printf("error:%s:%u: bad type cast between incompatible grub types", file, line);
	exit(1);
}

#define grub_bad_type_cast() grub_bad_type_cast_real(__LINE__, __FILE__)

#define GRUB_FIELD_MATCH(ptr, type, field) \
  ((char *) &(ptr)->field == (char *) &((type) (ptr))->field)

#define GRUB_AS_LIST(ptr) \
  (GRUB_FIELD_MATCH (ptr, grub_list_t, next) && GRUB_FIELD_MATCH (ptr, grub_list_t, prev) ? \
   (grub_list_t) ptr : (grub_list_t) grub_bad_type_cast ())

#define GRUB_AS_LIST_P(pptr) \
  (GRUB_FIELD_MATCH (*pptr, grub_list_t, next) && GRUB_FIELD_MATCH (*pptr, grub_list_t, prev) ? \
   (grub_list_t *) (void *) pptr : (grub_list_t *) grub_bad_type_cast ())

struct grub_named_list
{
	struct grub_named_list* next;
	struct grub_named_list** prev;
	char* name;
};
typedef struct grub_named_list* grub_named_list_t;

void* grub_named_list_find (grub_named_list_t head, const char* name);

#define GRUB_AS_NAMED_LIST(ptr) \
  ((GRUB_FIELD_MATCH (ptr, grub_named_list_t, next) \
	&& GRUB_FIELD_MATCH (ptr, grub_named_list_t, prev)	\
	&& GRUB_FIELD_MATCH (ptr, grub_named_list_t, name))? \
   (grub_named_list_t) ptr : (grub_named_list_t) grub_bad_type_cast ())

#define GRUB_AS_NAMED_LIST_P(pptr) \
  ((GRUB_FIELD_MATCH (*pptr, grub_named_list_t, next) \
	&& GRUB_FIELD_MATCH (*pptr, grub_named_list_t, prev)   \
	&& GRUB_FIELD_MATCH (*pptr, grub_named_list_t, name))? \
   (grub_named_list_t *) (void *) pptr : (grub_named_list_t *) grub_bad_type_cast ())

typedef enum
{
	GRUB_ERR_NONE = 0,
	GRUB_ERR_TEST_FAILURE,
	GRUB_ERR_OUT_OF_MEMORY,
	GRUB_ERR_BAD_FILE_TYPE,
	GRUB_ERR_FILE_NOT_FOUND,
	GRUB_ERR_FILE_READ_ERROR,
	GRUB_ERR_BAD_FILENAME,
	GRUB_ERR_UNKNOWN_FS,
	GRUB_ERR_BAD_FS,
	GRUB_ERR_BAD_NUMBER,
	GRUB_ERR_OUT_OF_RANGE,
	GRUB_ERR_UNKNOWN_DEVICE,
	GRUB_ERR_BAD_DEVICE,
	GRUB_ERR_READ_ERROR,
	GRUB_ERR_WRITE_ERROR,
	GRUB_ERR_UNKNOWN_COMMAND,
	GRUB_ERR_INVALID_COMMAND,
	GRUB_ERR_BAD_ARGUMENT,
	GRUB_ERR_BAD_PART_TABLE,
	GRUB_ERR_NOT_IMPLEMENTED_YET,
	GRUB_ERR_SYMLINK_LOOP,
	GRUB_ERR_BAD_COMPRESSED_DATA,
	GRUB_ERR_IO,
	GRUB_ERR_ACCESS_DENIED,
	GRUB_ERR_BUG,
	GRUB_ERR_EOF,
	GRUB_ERR_BAD_SIGNATURE,
} grub_err_t;

#define GRUB_MAX_ERRMSG 256

struct grub_error_saved
{
	grub_err_t grub_errno;
	char errmsg[GRUB_MAX_ERRMSG];
};

extern grub_err_t grub_errno;
extern char grub_errmsg[GRUB_MAX_ERRMSG];

grub_err_t grub_error (grub_err_t n, const char* fmt, ...);
void grub_error_push (void);
int grub_error_pop (void);
void grub_print_error (void);
extern int grub_err_printed_errors;
int grub_err_printf(const char* fmt, ...);

static inline void*
grub_malloc(grub_size_t size)
{
	return malloc(size);
}

void* grub_zalloc(grub_size_t size);

void* grub_calloc(grub_size_t nmemb, grub_size_t size);

static inline void
grub_free(void* ptr)
{
	free(ptr);
}

static inline void*
grub_realloc(void* ptr, grub_size_t size)
{
	return realloc(ptr, size);
}

static inline int
grub_memcmp(const void* s1, const void* s2, grub_size_t n)
{
	return memcmp(s1, s2, n);
}

static inline void*
grub_memmove(void* dst, const void* src, grub_size_t n)
{
	return memmove(dst, src, n);
}

static inline void*
grub_memcpy(void* dst, const void* src, grub_size_t n)
{
	return memcpy(dst, src, n);
}

static inline void*
grub_memset(void* s, int c, grub_size_t len)
{
	return memset(s, c, len);
}

static inline int
grub_isspace(int c)
{
	return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

static inline int
grub_isprint(int c)
{
	return (c >= ' ' && c <= '~');
}

static inline int
grub_iscntrl(int c)
{
	return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
grub_isalpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
grub_islower(int c)
{
	return (c >= 'a' && c <= 'z');
}

static inline int
grub_isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}

static inline int
grub_isgraph(int c)
{
	return (c >= '!' && c <= '~');
}

static inline int
grub_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

static inline int
grub_isxdigit(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
grub_isalnum(int c)
{
	return grub_isalpha(c) || grub_isdigit(c);
}

static inline int
grub_tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

static inline int
grub_toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	return c;
}

static inline int
grub_strcasecmp(const char* s1, const char* s2)
{
	while (*s1 && *s2)
	{
		if (grub_tolower((grub_uint8_t)*s1) != grub_tolower((grub_uint8_t)*s2))
			break;
		s1++;
		s2++;
	}
	return (int)grub_tolower((grub_uint8_t)*s1) - (int)grub_tolower((grub_uint8_t)*s2);
}

static inline int
grub_strncasecmp(const char* s1, const char* s2, grub_size_t n)
{
	if (n == 0)
		return 0;
	while (*s1 && *s2 && --n)
	{
		if (grub_tolower(*s1) != grub_tolower(*s2))
			break;
		s1++;
		s2++;
	}
	return (int)grub_tolower((grub_uint8_t)*s1) - (int)grub_tolower((grub_uint8_t)*s2);
}

static inline unsigned long
grub_strtoul(const char* str, const char** const end, int base)
{
	return strtoul(str, (char**) end, base);
}

static inline unsigned long long
grub_strtoull(const char* str, const char** const end, int base)
{
	return strtoull(str, (char**) end, base);
}

char* grub_strcpy(char* dst, const char* src);

char* grub_strncpy(char* dst, const char* src, int c);

int grub_strcmp(const char* s1, const char* s2);

int grub_strncmp(const char* s1, const char* s2, grub_size_t n);

grub_size_t grub_strlen(const char* s);

char* grub_strchr(const char* s, int c);

char* grub_strrchr(const char* s, int c);

int grub_strword(const char* haystack, const char* needle);

char* grub_strdup(const char* s);

char* grub_strndup(const char* s, grub_size_t n);

static inline char*
grub_stpcpy(char* dest, const char* src)
{
	char* d = dest;
	const char* s = src;

	do
		*d++ = *s;
	while (*s++ != '\0');

	return d - 1;
}

grub_uint64_t grub_get_time_ms(void);

const char*
grub_get_human_size(grub_uint64_t size, const char* human_sizes[6], grub_uint64_t base);

#define NSEC_PER_SEC ((grub_int64_t) 1000000000)

grub_uint32_t grub_getcrc32c(grub_uint32_t crc, const void* buf, int size);

grub_uint64_t
grub_divmod64(grub_uint64_t n, grub_uint64_t d, grub_uint64_t* r);

#endif
