// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DATETIME_HEADER
#define _DATETIME_HEADER 1

#include "compat.h"

struct grub_datetime
{
	grub_uint16_t year;
	grub_uint8_t month;
	grub_uint8_t day;
	grub_uint8_t hour;
	grub_uint8_t minute;
	grub_uint8_t second;
};

static inline int
grub_datetime2unixtime(const struct grub_datetime* datetime, grub_int64_t* nix)
{
	grub_int32_t ret;
	int y4, ay;
	const grub_uint16_t monthssum[12]
		= { 0,
			31,
			31 + 28,
			31 + 28 + 31,
			31 + 28 + 31 + 30,
			31 + 28 + 31 + 30 + 31,
			31 + 28 + 31 + 30 + 31 + 30,
			31 + 28 + 31 + 30 + 31 + 30 + 31,
			31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
			31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
			31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
			31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
	};
	const grub_uint8_t months[12] = { 31, 28, 31, 30, 31, 30,
									 31, 31, 30, 31, 30, 31
	};
	const int SECPERMIN = 60;
	const int SECPERHOUR = 60 * SECPERMIN;
	const int SECPERDAY = 24 * SECPERHOUR;
	const int SECPERYEAR = 365 * SECPERDAY;
	const int SECPER4YEARS = 4 * SECPERYEAR + SECPERDAY;

	if (datetime->year > 2038 || datetime->year < 1901)
		return 0;
	if (datetime->month > 12 || datetime->month < 1)
		return 0;

	/* In the period of validity of unixtime all years divisible by 4
	   are bissextile*/
	   /* Convenience: let's have 3 consecutive non-bissextile years
		  at the beginning of the epoch. So count from 1973 instead of 1970 */
	ret = 3 * SECPERYEAR + SECPERDAY;

	/* Transform C divisions and modulos to mathematical ones */
	y4 = ((datetime->year - 1) >> 2) - (1973 / 4);
	ay = datetime->year - 1973 - 4 * y4;
	ret += y4 * SECPER4YEARS;
	ret += ay * SECPERYEAR;

	ret += monthssum[datetime->month - 1] * SECPERDAY;
	if (ay == 3 && datetime->month >= 3)
		ret += SECPERDAY;

	ret += (datetime->day - 1) * SECPERDAY;
	if ((datetime->day > months[datetime->month - 1]
		&& (!ay || datetime->month != 2 || datetime->day != 29))
		|| datetime->day < 1)
		return 0;

	ret += datetime->hour * SECPERHOUR;
	if (datetime->hour > 23)
		return 0;
	ret += datetime->minute * 60;
	if (datetime->minute > 59)
		return 0;

	ret += datetime->second;
	/* Accept leap seconds.  */
	if (datetime->second > 60)
		return 0;

	if ((datetime->year > 1980 && ret < 0)
		|| (datetime->year < 1960 && ret > 0))
		return 0;
	*nix = ret;
	return 1;
}

#endif
