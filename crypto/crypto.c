// SPDX-License-Identifier: GPL-3.0-or-later

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
