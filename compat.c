// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "compat.h"
#include "misc.h"

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

grub_uint64_t
grub_get_time_ms(void)
{
	return GetTickCount64();
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
