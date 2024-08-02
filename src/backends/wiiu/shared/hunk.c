/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file implements the low level part of the Hunk_* memory system
 *
 * =======================================================================
 */

#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <whb/log.h>

#include "../../../common/header/common.h"

// __WIIU_TODO__
// There is no virtual memory on WiiU (at least not to my knowledge)
// so we simply allocate the requested block and live with the overhead
// realloc would be an option, but it might move the allocated block even if just downsizing

// #define HUNK_DEBUG

byte *membase;
size_t maxhunksize;
size_t curhunksize;

#define OVERSIZE_HACK 1024
void *
Hunk_Begin(int maxsize)
{
	// __WIIU_TODO__ not sure why, if it is not oversized it fails loading a texture
	maxsize += OVERSIZE_HACK;

	maxhunksize = maxsize + sizeof(size_t) + 32;
	curhunksize = 0;
	membase = malloc(maxsize);

	// no memset --> later crash, it seems that loading does rely on the buffer being zeroed
	memset(membase, 0, maxsize);

	*((size_t *)membase) = curhunksize;

#ifdef HUNK_DEBUG
	WHBLogWritef("Hunk_Begin(%d) --> %08X\n", maxsize, membase + sizeof(size_t));
#endif
	return membase + sizeof(size_t);
	//return membase;
}

void *
Hunk_Alloc(int size)
{
	byte *buf;

	/* round to cacheline */
	size = (size + 31) & ~31;

	if (curhunksize + size > maxhunksize)
	{
		Sys_Error("Hunk_Alloc overflow");
	}

	buf = membase + sizeof(size_t) + curhunksize;
	curhunksize += size;
	return buf;
}

int
Hunk_End(void)
{
	return maxhunksize;
}

void
Hunk_Free(void *base)
{
#ifdef HUNK_DEBUG
	WHBLogWritef("Hunk_Free(%08X)\n", base);
#endif
	if (base)
	{
		byte *m;

		m = ((byte *)base) - sizeof(size_t);

		free(m);
	}
}

