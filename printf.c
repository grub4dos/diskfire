// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "compat.h"
#include "misc.h"
#include "charset.h"

#pragma warning(disable:4127)

union printf_arg
{
	/* Yes, type is also part of union as the moment we fill the value
	   we don't need to store its type anymore (when we'll need it, we'll
	   have format spec again. So save some space.  */
	enum
	{
		P_INT,
		P_LONG,
		P_LONGLONG,
		P_UNSIGNED_INT = 3,
		P_UNSIGNED_LONG,
		P_UNSIGNED_LONGLONG,
		P_STRING
	} type;
	long long ll;
};

struct printf_args
{
	union printf_arg prealloc[32];
	union printf_arg* ptr;
	grub_size_t count;
};

static char debug_cond[128];

static void
parse_printf_args(const char* fmt0, struct printf_args* args,
	va_list args_in);
static int
grub_vsnprintf_real(char* str, grub_size_t max_len, const char* fmt0,
	struct printf_args* args);

void SetDebug(const char* cond)
{
	grub_memset(debug_cond, 0, sizeof(debug_cond));
	if (!cond)
		return;
	grub_snprintf(debug_cond, sizeof(debug_cond), "%s", cond);
}

int
grub_debug_enabled(const char* condition)
{
	const char* found;
	grub_size_t clen;
	int ret = 0;

	if (grub_strword(debug_cond, "all"))
	{
		if (debug_cond[3] == '\0')
			return 1;
		ret = 1;
	}

	clen = grub_strlen(condition);
	found = debug_cond - 1;
	while (1)
	{
		found = grub_strstr(found + 1, condition);

		if (found == NULL)
			break;

		/* Found condition is not a whole word, so ignore it. */
		if (*(found + clen) != '\0' && *(found + clen) != ','
			&& !grub_isspace(*(found + clen)))
			continue;

		/*
		 * If found condition is at the start of debug or the start is on a word
		 * boundary, then enable debug. Else if found condition is prefixed with
		 * '-' and the start is on a word boundary, then disable debug. If none
		 * of these cases, ignore.
		 */
		if (found == debug_cond || *(found - 1) == ',' || grub_isspace(*(found - 1)))
			ret = 1;
		else if (*(found - 1) == '-' && ((found == debug_cond + 1) || (*(found - 2) == ','
			|| grub_isspace(*(found - 2)))))
			ret = 0;
	}

	return ret;
}

static void
free_printf_args(struct printf_args* args)
{
	if (args->ptr != args->prealloc)
		grub_free(args->ptr);
}

int
grub_printf(const char* fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = grub_vprintf(fmt, ap);
	va_end(ap);
	return ret;
}

void
grub_real_dprintf(const char* file, const int line, const char* condition,
	const char* fmt, ...)
{
	va_list args;

	if (grub_debug_enabled(condition))
	{
		grub_printf("%s:%d:%s: ", file, line, condition);
		va_start(args, fmt);
		grub_vprintf(fmt, args);
		va_end(args);
	}
}

#define PREALLOC_SIZE 255

static void grub_xputs(const char* str)
{
	wchar_t* ws = grub_get_utf16(str);
	if (!ws)
		return;
	wprintf(ws, L"%s");
	grub_free(ws);
}

int
grub_vprintf(const char* fmt, va_list ap)
{
	grub_size_t s;
	static char buf[PREALLOC_SIZE + 1];
	char* curbuf = buf;
	struct printf_args args;

	parse_printf_args(fmt, &args, ap);

	s = grub_vsnprintf_real(buf, PREALLOC_SIZE, fmt, &args);
	if (s > PREALLOC_SIZE)
	{
		curbuf = grub_malloc(s + 1);
		if (!curbuf)
		{
			grub_errno = GRUB_ERR_NONE;
			buf[PREALLOC_SIZE - 3] = '.';
			buf[PREALLOC_SIZE - 2] = '.';
			buf[PREALLOC_SIZE - 1] = '.';
			buf[PREALLOC_SIZE] = 0;
			curbuf = buf;
		}
		else
			s = grub_vsnprintf_real(curbuf, s, fmt, &args);
	}

	free_printf_args(&args);

	grub_xputs(curbuf);

	if (curbuf != buf)
		grub_free(curbuf);

	return s;
}

static inline void
grub_reverse(char* str)
{
	char* p = str + grub_strlen(str) - 1;

	while (str < p)
	{
		char tmp;

		tmp = *str;
		*str = *p;
		*p = tmp;
		str++;
		p--;
	}
}

/* Convert a long long value to a string. This function avoids 64-bit
   modular arithmetic or divisions.  */
static inline char*
grub_lltoa(char* str, int c, unsigned long long n)
{
	unsigned base = ((c == 'x') || (c == 'X')) ? 16 : 10;
	char* p;

	if ((long long)n < 0 && c == 'd')
	{
		n = (unsigned long long) (-((long long)n));
		*str++ = '-';
	}

	p = str;

	if (base == 16)
		do
		{
			unsigned d = (unsigned)(n & 0xf);
			*p++ = (d > 9) ? d + ((c == 'x') ? 'a' : 'A') - 10 : d + '0';
		} while (n >>= 4);
	else
		/* BASE == 10 */
		do
		{
			grub_uint64_t m;

			n = grub_divmod64(n, 10, &m);
			*p++ = m + '0';
		} while (n);

		*p = 0;

		grub_reverse(str);
		return p;
}

/*
 * Parse printf() fmt0 string into args arguments.
 *
 * The parsed arguments are either used by a printf() function to format the fmt0
 * string or they are used to compare a format string from an untrusted source
 * against a format string with expected arguments.
 *
 * When the fmt_check is set to !0, e.g. 1, then this function is executed in
 * printf() format check mode. This enforces stricter rules for parsing the
 * fmt0 to limit exposure to possible errors in printf() handling. It also
 * disables positional parameters, "$", because some formats, e.g "%s%1$d",
 * cannot be validated with the current implementation.
 *
 * The max_args allows to set a maximum number of accepted arguments. If the fmt0
 * string defines more arguments than the max_args then the parse_printf_arg_fmt()
 * function returns an error. This is currently used for format check only.
 */
static grub_err_t
parse_printf_arg_fmt(const char* fmt0, struct printf_args* args,
	int fmt_check, grub_size_t max_args)
{
	const char* fmt;
	char c;
	grub_size_t n = 0;

	args->count = 0;

	fmt = fmt0;
	while ((c = *fmt++) != 0)
	{
		if (c != '%')
			continue;

		if (*fmt == '-')
			fmt++;

		while (grub_isdigit(*fmt))
			fmt++;

		if (*fmt == '$')
		{
			if (fmt_check)
				return grub_error(GRUB_ERR_BAD_ARGUMENT,
					"positional arguments are not supported");
			fmt++;
		}

		if (*fmt == '-')
			fmt++;

		while (grub_isdigit(*fmt))
			fmt++;

		if (*fmt == '.')
			fmt++;

		while (grub_isdigit(*fmt))
			fmt++;

		c = *fmt++;
		if (c == 'l')
			c = *fmt++;
		if (c == 'l')
			c = *fmt++;

		switch (c)
		{
		case 'p':
		case 'x':
		case 'X':
		case 'u':
		case 'd':
		case 'c':
		case 'C':
		case 's':
			args->count++;
			break;
		case '%':
			/* "%%" is the escape sequence to output "%". */
			break;
		default:
			if (fmt_check)
				return grub_error(GRUB_ERR_BAD_ARGUMENT, "unexpected format");
			break;
		}
	}

	if (fmt_check && args->count > max_args)
		return grub_error(GRUB_ERR_BAD_ARGUMENT, "too many arguments");

	if (args->count <= ARRAY_SIZE(args->prealloc))
		args->ptr = args->prealloc;
	else
	{
		args->ptr = grub_calloc(args->count, sizeof(args->ptr[0]));
		if (!args->ptr)
		{
			if (fmt_check)
				return grub_errno;

			grub_errno = GRUB_ERR_NONE;
			args->ptr = args->prealloc;
			args->count = ARRAY_SIZE(args->prealloc);
		}
	}

	grub_memset(args->ptr, 0, args->count * sizeof(args->ptr[0]));

	fmt = fmt0;
	n = 0;
	while ((c = *fmt++) != 0)
	{
		int longfmt = 0;
		grub_size_t curn;
		const char* p;

		if (c != '%')
			continue;

		curn = n++;

		if (*fmt == '-')
			fmt++;

		p = fmt;

		while (grub_isdigit(*fmt))
			fmt++;

		if (*fmt == '$')
		{
			curn = grub_strtoull(p, 0, 10) - 1;
			fmt++;
		}

		if (*fmt == '-')
			fmt++;

		while (grub_isdigit(*fmt))
			fmt++;

		if (*fmt == '.')
			fmt++;

		while (grub_isdigit(*fmt))
			fmt++;

		c = *fmt++;
		if (c == '%')
		{
			n--;
			continue;
		}

		if (c == 'l')
		{
			c = *fmt++;
			longfmt = 1;
		}
		if (c == 'l')
		{
			c = *fmt++;
			longfmt = 2;
		}
		if (curn >= args->count)
			continue;
		switch (c)
		{
		case 'x':
		case 'X':
		case 'u':
			args->ptr[curn].type = P_UNSIGNED_INT + longfmt;
			break;
		case 'd':
			args->ptr[curn].type = P_INT + longfmt;
			break;
		case 'p':
			if (sizeof(void*) == sizeof(long long))
				args->ptr[curn].type = P_UNSIGNED_LONGLONG;
			else
				args->ptr[curn].type = P_UNSIGNED_INT;
			break;
		case 's':
			args->ptr[curn].type = P_STRING;
			break;
		case 'C':
		case 'c':
			args->ptr[curn].type = P_INT;
			break;
		}
	}

	return GRUB_ERR_NONE;
}

static void
parse_printf_args(const char* fmt0, struct printf_args* args, va_list args_in)
{
	grub_size_t n;

	parse_printf_arg_fmt(fmt0, args, 0, 0);

	for (n = 0; n < args->count; n++)
		switch (args->ptr[n].type)
		{
		case P_INT:
			args->ptr[n].ll = va_arg(args_in, int);
			break;
		case P_LONG:
			args->ptr[n].ll = va_arg(args_in, long);
			break;
		case P_UNSIGNED_INT:
			args->ptr[n].ll = va_arg(args_in, unsigned int);
			break;
		case P_UNSIGNED_LONG:
			args->ptr[n].ll = va_arg(args_in, unsigned long);
			break;
		case P_LONGLONG:
		case P_UNSIGNED_LONGLONG:
			args->ptr[n].ll = va_arg(args_in, long long);
			break;
		case P_STRING:
			if (sizeof(void*) == sizeof(long long))
				args->ptr[n].ll = va_arg(args_in, long long);
			else
				args->ptr[n].ll = va_arg(args_in, unsigned int);
			break;
		}
}

static inline void
write_char(char* str, grub_size_t* count, grub_size_t max_len,
	unsigned char ch)
{
	if (*count < max_len)
		str[*count] = ch;

	(*count)++;
}

static int
grub_vsnprintf_real(char* str, grub_size_t max_len, const char* fmt0,
	struct printf_args* args)
{
	char c;
	grub_size_t n = 0;
	grub_size_t count = 0;
	const char* fmt;

	fmt = fmt0;

	while ((c = *fmt++) != 0)
	{
		unsigned int format1 = 0;
		unsigned int format2 = ~0U;
		char zerofill = ' ';
		char rightfill = 0;
		grub_size_t curn;

		if (c != '%')
		{
			write_char(str, &count, max_len, c);
			continue;
		}

		curn = n++;

	rescan:
		;

		if (*fmt == '-')
		{
			rightfill = 1;
			fmt++;
		}

		/* Read formatting parameters.  */
		if (grub_isdigit(*fmt))
		{
			if (fmt[0] == '0')
				zerofill = '0';
			format1 = grub_strtoul(fmt, &fmt, 10);
		}

		if (*fmt == '.')
			fmt++;

		if (grub_isdigit(*fmt))
			format2 = grub_strtoul(fmt, &fmt, 10);

		if (*fmt == '$')
		{
			curn = format1 - 1;
			fmt++;
			format1 = 0;
			format2 = ~0U;
			zerofill = ' ';
			rightfill = 0;

			goto rescan;
		}

		c = *fmt++;
		if (c == 'l')
			c = *fmt++;
		if (c == 'l')
			c = *fmt++;

		if (c == '%')
		{
			write_char(str, &count, max_len, c);
			n--;
			continue;
		}

		if (curn >= args->count)
			continue;

		long long curarg = args->ptr[curn].ll;

		switch (c)
		{
		case 'p':
			write_char(str, &count, max_len, '0');
			write_char(str, &count, max_len, 'x');
			c = 'x';
			/* Fall through. */
		case 'x':
		case 'X':
		case 'u':
		case 'd':
		{
			char tmp[32];
			const char* p = tmp;
			grub_size_t len;
			grub_size_t fill;

			len = grub_lltoa(tmp, c, curarg) - tmp;
			fill = len < format1 ? format1 - len : 0;
			if (!rightfill)
				while (fill--)
					write_char(str, &count, max_len, zerofill);
			while (*p)
				write_char(str, &count, max_len, *p++);
			if (rightfill)
				while (fill--)
					write_char(str, &count, max_len, zerofill);
		}
		break;

		case 'c':
			write_char(str, &count, max_len, curarg & 0xff);
			break;

		case 'C':
		{
			grub_uint32_t code = curarg;
			int shift;
			unsigned mask;

			if (code <= 0x7f)
			{
				shift = 0;
				mask = 0;
			}
			else if (code <= 0x7ff)
			{
				shift = 6;
				mask = 0xc0;
			}
			else if (code <= 0xffff)
			{
				shift = 12;
				mask = 0xe0;
			}
			else if (code <= 0x10ffff)
			{
				shift = 18;
				mask = 0xf0;
			}
			else
			{
				code = '?';
				shift = 0;
				mask = 0;
			}

			write_char(str, &count, max_len, mask | (code >> shift));

			for (shift -= 6; shift >= 0; shift -= 6)
				write_char(str, &count, max_len, 0x80 | (0x3f & (code >> shift)));
		}
		break;

		case 's':
		{
			grub_size_t len = 0;
			grub_size_t fill;
			const char* p = ((char*)(grub_addr_t)curarg) ? (char*)(grub_addr_t)curarg : "(null)";
			grub_size_t i;

			while (len < format2 && p[len])
				len++;

			fill = len < format1 ? format1 - len : 0;

			if (!rightfill)
				while (fill--)
					write_char(str, &count, max_len, zerofill);

			for (i = 0; i < len; i++)
				write_char(str, &count, max_len, *p++);

			if (rightfill)
				while (fill--)
					write_char(str, &count, max_len, zerofill);
		}

		break;

		default:
			write_char(str, &count, max_len, c);
			break;
		}
	}

	if (count < max_len)
		str[count] = '\0';
	else
		str[max_len] = '\0';
	return count;
}

int
grub_vsnprintf(char* str, grub_size_t n, const char* fmt, va_list ap)
{
	grub_size_t ret;
	struct printf_args args;

	if (!n)
		return 0;

	n--;

	parse_printf_args(fmt, &args, ap);

	ret = grub_vsnprintf_real(str, n, fmt, &args);

	free_printf_args(&args);

	return ret < n ? ret : n;
}

int
grub_snprintf(char* str, grub_size_t n, const char* fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = grub_vsnprintf(str, n, fmt, ap);
	va_end(ap);

	return ret;
}

char*
grub_xvasprintf(const char* fmt, va_list ap)
{
	grub_size_t s, as = PREALLOC_SIZE;
	char* ret;
	struct printf_args args;

	parse_printf_args(fmt, &args, ap);

	while (1)
	{
		ret = grub_malloc(as + 1);
		if (!ret)
		{
			free_printf_args(&args);
			return NULL;
		}

		s = grub_vsnprintf_real(ret, as, fmt, &args);

		if (s <= as)
		{
			free_printf_args(&args);
			return ret;
		}

		grub_free(ret);
		as = s;
	}
}

char*
grub_xasprintf(const char* fmt, ...)
{
	va_list ap;
	char* ret;

	va_start(ap, fmt);
	ret = grub_xvasprintf(fmt, ap);
	va_end(ap);

	return ret;
}
