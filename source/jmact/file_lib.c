/*
 * file_lib.c
 * File functions to emulate MACT
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>
#include "types.h"
#include "file_lib.h"

#ifndef O_BINARY
#define  O_BINARY 0
#endif
#ifndef O_TEXT
#define  O_TEXT   0
#endif



int32 SafeOpenRead(const char *filename, int32 filetype)
{
	int32 h;

	h = open(filename, O_RDONLY|(filetype==filetype_binary?O_BINARY:O_TEXT));
	if (h<0) Error("Failed opening %s for reading.\n", filename);

	return h;
}

void SafeClose ( int32 handle )
{
	if (handle < 0) return;
	close(handle);
}

boolean SafeFileExists(const char *filename)
{
	int h;

	h = open(filename, O_RDONLY);
	if (h<0) return 0;
	close(h);
	return 1;
}

int32 SafeFileLength ( int32 handle )
{
	if (handle < 0) return 0;
	return Bfilelength(handle);
}

void SafeRead(int32 handle, void *buffer, int32 count)
{
	int32 b;

	b = read(handle, buffer, count);
	if (b != count) {
		close(handle);
		Error("Failed reading %d bytes.\n", count);
	}
}


