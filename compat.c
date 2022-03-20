// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "compat.h"
#include "misc.h"
#include "datetime.h"
#include "charset.h"

void*
grub_named_list_find(grub_named_list_t head, const char* name)
{
	grub_named_list_t item;

	FOR_LIST_ELEMENTS(item, head)
		if (grub_strcmp(item->name, name) == 0)
			return item;

	return NULL;
}

void
grub_list_push(grub_list_t* head, grub_list_t item)
{
	item->prev = head;
	if (*head)
		(*head)->prev = &item->next;
	item->next = *head;
	*head = item;
}

void
grub_list_remove(grub_list_t item)
{
	if (item->prev)
		*item->prev = item->next;
	if (item->next)
		item->next->prev = item->prev;
	item->next = 0;
	item->prev = 0;
}

void*
grub_zalloc(grub_size_t size)
{
	void* ret;
	ret = malloc(size);
	if (ret)
		ZeroMemory(ret, size);
	return ret;
}

void*
grub_calloc(grub_size_t nmemb, grub_size_t size)
{
	grub_size_t sz = 0;

	if (grub_mul(nmemb, size, &sz))
	{
		grub_error(GRUB_ERR_OUT_OF_RANGE, "overflow is detected");
		return NULL;
	}

	return grub_zalloc(sz);
}

int
grub_snprintf(char* str, grub_size_t n, const char* fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = vsnprintf(str, n, fmt, ap);
	va_end(ap);
	return ret;
}

int
grub_printf(const char* fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);
	return ret;
}

static char debug_cond[128];

void SetDebug(const char* cond)
{
	ZeroMemory(debug_cond, sizeof(debug_cond));
	if (!cond)
		return;
	snprintf(debug_cond, sizeof(debug_cond), "%s", cond);
}

static int
grub_debug_enabled(const char* condition)
{
	if (grub_strword(debug_cond, "all") || grub_strword(debug_cond, condition))
		return 1;
	return 0;
}

void grub_dprintf(const char* cond, const char* fmt, ...)
{
	va_list ap;
	if (!grub_debug_enabled(cond))
		return;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

char*
grub_strcpy(char* dst, const char* src)
{
	char* p = dst;
	while ((*p++ = *src++) != '\0');
	return dst;
}

char*
grub_strncpy(char* dst, const char* src, int c)
{
	char* p = dst;
	while ((*p++ = *src++) != '\0' && --c);
	return dst;
}

int
grub_strcmp(const char* s1, const char* s2)
{
	while (*s1 && *s2)
	{
		if (*s1 != *s2)
			break;
		s1++;
		s2++;
	}
	return (int)(grub_uint8_t)*s1 - (int)(grub_uint8_t)*s2;
}

int
grub_strncmp(const char* s1, const char* s2, grub_size_t n)
{
	if (n == 0)
		return 0;
	while (*s1 && *s2 && --n)
	{
		if (*s1 != *s2)
			break;

		s1++;
		s2++;
	}
	return (int)(grub_uint8_t)*s1 - (int)(grub_uint8_t)*s2;
}

grub_size_t
grub_strlen(const char* s)
{
	const char* p = s;
	while (*p)
		p++;
	return p - s;
}

char*
grub_strchr(const char* s, int c)
{
	do
	{
		if (*s == c)
			return (char*)s;
	} while (*s++);
	return 0;
}

char*
grub_strrchr(const char* s, int c)
{
	char* p = NULL;
	do
	{
		if (*s == c)
			p = (char*)s;
	} while (*s++);
	return p;
}

static int
grub_iswordseparator(int c)
{
	return (grub_isspace(c) || c == ',' || c == ';' || c == '|' || c == '&');
}

int
grub_strword(const char* haystack, const char* needle)
{
	const char* n_pos = needle;

	while (grub_iswordseparator(*haystack))
		haystack++;

	while (*haystack)
	{
		/* Crawl both the needle and the haystack word we're on.  */
		while (*haystack && !grub_iswordseparator(*haystack)
			&& *haystack == *n_pos)
		{
			haystack++;
			n_pos++;
		}

		/* If we reached the end of both words at the same time, the word
		is found. If not, eat everything in the haystack that isn't the
		next word (or the end of string) and "reset" the needle.  */
		if ((!*haystack || grub_iswordseparator(*haystack))
			&& (!*n_pos || grub_iswordseparator(*n_pos)))
			return 1;
		else
		{
			n_pos = needle;
			while (*haystack && !grub_iswordseparator(*haystack))
				haystack++;
			while (grub_iswordseparator(*haystack))
				haystack++;
		}
	}

	return 0;
}

char*
grub_strdup(const char* s)
{
	grub_size_t len;
	char* p;

	len = grub_strlen(s) + 1;
	p = (char*)grub_malloc(len);
	if (!p)
		return 0;

	return grub_memcpy(p, s, len);
}

char*
grub_strndup(const char* s, grub_size_t n)
{
	grub_size_t len;
	char* p;

	len = grub_strlen(s);
	if (len > n)
		len = n;
	p = (char*)grub_malloc(len + 1);
	if (!p)
		return 0;

	grub_memcpy(p, s, len);
	p[len] = '\0';
	return p;
}

#define GRUB_ERROR_STACK_SIZE	10

grub_err_t grub_errno;
char grub_errmsg[GRUB_MAX_ERRMSG];
int grub_err_printed_errors;

static struct grub_error_saved grub_error_stack_items[GRUB_ERROR_STACK_SIZE];

static int grub_error_stack_pos;
static int grub_error_stack_assert;

grub_err_t
grub_error(grub_err_t n, const char* fmt, ...)
{
	va_list ap;

	grub_errno = n;

	va_start(ap, fmt);
	vsnprintf(grub_errmsg, sizeof(grub_errmsg), fmt, ap);
	va_end(ap);

	return n;
}

void
grub_error_push(void)
{
	/* Only add items to stack, if there is enough room.  */
	if (grub_error_stack_pos < GRUB_ERROR_STACK_SIZE)
	{
		/* Copy active error message to stack.  */
		grub_error_stack_items[grub_error_stack_pos].grub_errno = grub_errno;
		grub_memcpy(grub_error_stack_items[grub_error_stack_pos].errmsg, grub_errmsg, sizeof(grub_errmsg));

		/* Advance to next error stack position.  */
		grub_error_stack_pos++;
	}
	else
	{
		/* There is no room for new error message. Discard new error message
		   and mark error stack assertion flag.  */
		grub_error_stack_assert = 1;
	}

	/* Allow further operation of other components by resetting
	   active errno to GRUB_ERR_NONE.  */
	grub_errno = GRUB_ERR_NONE;
}

int
grub_error_pop(void)
{
	if (grub_error_stack_pos > 0)
	{
		/* Pop error message from error stack to current active error.  */
		grub_error_stack_pos--;

		grub_errno = grub_error_stack_items[grub_error_stack_pos].grub_errno;
		grub_memcpy(grub_errmsg, grub_error_stack_items[grub_error_stack_pos].errmsg, sizeof(grub_errmsg));

		return 1;
	}
	else
	{
		/* There is no more items on error stack, reset to no error state.  */
		grub_errno = GRUB_ERR_NONE;

		return 0;
	}
}

void
grub_print_error(void)
{
	/* Print error messages in reverse order. First print active error message
	   and then empty error stack.  */
	do
	{
		if (grub_errno != GRUB_ERR_NONE)
		{
			grub_err_printf("error: %s.\n", grub_errmsg);
			grub_err_printed_errors++;
		}
	} while (grub_error_pop());

	/* If there was an assert while using error stack, report about it.  */
	if (grub_error_stack_assert)
	{
		grub_err_printf("assert: error stack overflow detected!\n");
		grub_error_stack_assert = 0;
	}
}

int
grub_err_printf(const char* fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return ret;
}

//fuck
#pragma warning(disable:4702)

grub_uint64_t
grub_get_time_ms(void)
{
	return GetTickCount64();
}

static grub_uint32_t next = 42;

grub_uint32_t
grub_rand(void)
{
	next = next * 1103515245 + 12345;
	return (next << 16) | ((next >> 16) & 0xFFFF);
}

void
grub_srand(grub_uint32_t seed)
{
	next = seed;
}

const char*
grub_get_human_size(grub_uint64_t size, const char* human_sizes[6], grub_uint64_t base)
{
	grub_uint64_t fsize = size, frac = 0;
	unsigned units = 0;
	static char buf[48];
	const char* umsg;

	while (fsize >= base && units < 5)
	{
		frac = fsize % base;
		fsize = fsize / base;
		units++;
	}

	umsg = human_sizes[units];

	if (units)
	{
		if (frac)
			frac = frac * 100 / base;
		snprintf(buf, sizeof(buf), "%llu.%02llu%s", fsize, frac, umsg);
	}
	else
		snprintf(buf, sizeof(buf), "%llu%s", size, umsg);
	return buf;
}

static grub_uint32_t crc32c_table[256];

/* Helper for init_crc32c_table.  */
static grub_uint32_t
reflect(grub_uint32_t ref, int len)
{
	grub_uint32_t result = 0;
	int i;

	for (i = 1; i <= len; i++)
	{
		if (ref & 1)
			result |= 1 << (len - i);
		ref >>= 1;
	}

	return result;
}

static void
init_crc32c_table(void)
{
	grub_uint32_t polynomial = 0x1edc6f41;
	int i, j;

	for (i = 0; i < 256; i++)
	{
		crc32c_table[i] = reflect(i, 8) << 24;
		for (j = 0; j < 8; j++)
			crc32c_table[i] = (crc32c_table[i] << 1) ^
			(crc32c_table[i] & (1 << 31) ? polynomial : 0);
		crc32c_table[i] = reflect(crc32c_table[i], 32);
	}
}

grub_uint32_t
grub_getcrc32c(grub_uint32_t crc, const void* buf, int size)
{
	int i;
	const grub_uint8_t* data = buf;

	if (!crc32c_table[1])
		init_crc32c_table();

	crc ^= 0xffffffff;

	for (i = 0; i < size; i++)
	{
		crc = (crc >> 8) ^ crc32c_table[(crc & 0xFF) ^ *data];
		data++;
	}

	return crc ^ 0xffffffff;
}

/* Divide N by D, return the quotient, and store the remainder in *R.  */
grub_uint64_t
grub_divmod64(grub_uint64_t n, grub_uint64_t d, grub_uint64_t* r)
{
	/* This algorithm is typically implemented by hardware. The idea
	   is to get the highest bit in N, 64 times, by keeping
	   upper(N * 2^i) = (Q * D + M), where upper
	   represents the high 64 bits in 128-bits space.  */
	unsigned bits = 64;
	grub_uint64_t q = 0;
	grub_uint64_t m = 0;

	/* Skip the slow computation if 32-bit arithmetic is possible.  */
	if (n < 0xffffffff && d < 0xffffffff)
	{
		if (r)
			*r = ((grub_uint32_t)n) % (grub_uint32_t)d;

		return ((grub_uint32_t)n) / (grub_uint32_t)d;
	}

	while (bits--)
	{
		m <<= 1;

		if (n & (1ULL << 63))
			m |= 1;

		q <<= 1;
		n <<= 1;

		if (m >= d)
		{
			q |= 1;
			m -= d;
		}
	}

	if (r)
		*r = m;

	return q;
}

#define SECPERMIN 60
#define SECPERHOUR (60*SECPERMIN)
#define SECPERDAY (24*SECPERHOUR)
#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)

void
grub_unixtime2datetime(grub_int64_t nix, struct grub_datetime* datetime)
{
	int i;
	grub_uint8_t months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	/* In the period of validity of unixtime all years divisible by 4
	   are bissextile*/
	   /* Convenience: let's have 3 consecutive non-bissextile years
		  at the beginning of the counting date. So count from 1901. */
	int days_epoch;
	/* Number of days since 1st Januar, 1901.  */
	unsigned days;
	/* Seconds into current day.  */
	unsigned secs_in_day;

	/* Transform C divisions and modulos to mathematical ones */
	if (nix < 0)
		/*
		 * The result of division here shouldn't be larger than GRUB_INT_MAX.
		 * So, it's safe to store the result back in an int.
		 */
		days_epoch = -(int)(grub_divmod64(((grub_int64_t)(SECPERDAY)-nix - 1), SECPERDAY, NULL));
	else
		days_epoch = grub_divmod64(nix, SECPERDAY, NULL);

	secs_in_day = nix - ((grub_int64_t)days_epoch) * SECPERDAY;
	days = days_epoch + 69 * DAYSPERYEAR + 17;

	datetime->year = 1901 + 4 * (days / DAYSPER4YEARS);
	days %= DAYSPER4YEARS;
	/* On 31st December of bissextile years 365 days from the beginning
	   of the year elapsed but year isn't finished yet */
	if (days / DAYSPERYEAR == 4)
	{
		datetime->year += 3;
		days -= 3 * DAYSPERYEAR;
	}
	else
	{
		datetime->year += days / DAYSPERYEAR;
		days %= DAYSPERYEAR;
	}
	for (i = 0; i < 12
		&& days >= (unsigned)(i == 1 && datetime->year % 4 == 0
			? 29 : months[i]); i++)
		days -= (i == 1 && datetime->year % 4 == 0
			? 29 : months[i]);
	datetime->month = i + 1;
	datetime->day = 1 + days;
	datetime->hour = (secs_in_day / SECPERHOUR);
	secs_in_day %= SECPERHOUR;
	datetime->minute = secs_in_day / SECPERMIN;
	datetime->second = secs_in_day % SECPERMIN;
}

wchar_t* grub_get_utf16(const char* path)
{
	wchar_t* path16 = NULL;
	grub_size_t len, len16;
	if (!path)
		return NULL;
	len = grub_strlen(path);
	len16 = len * GRUB_MAX_UTF16_PER_UTF8;
	path16 = grub_calloc(len16 + 1, sizeof(WCHAR));
	if (!path16)
		return NULL;
	len16 = grub_utf8_to_utf16(path16, len16, (grub_uint8_t*)path, len, NULL);
	path16[len16] = 0;
	return path16;
}