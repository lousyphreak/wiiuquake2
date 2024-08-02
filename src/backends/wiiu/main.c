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
 * This file is the starting point of the program. It does some platform
 * specific initialization stuff and calls the common initialization code.
 *
 * =======================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/proc.h>
#include <whb/sdcard.h>

#include "../../common/header/common.h"

int
main(int argc, char **argv)
{
	WHBLogCafeInit();
	WHBLogUdpInit();
    WHBProcInit();
	WHBLogWritef("Starting YQuake2...\n");
	WHBMountSdCard();
	WHBGfxInit();

	// Setup FPU if necessary
	Sys_SetupFPU();

	// Implement command line options that the rather
	// crappy argument parser can't parse.
	for (int i = 0; i < argc; i++)
	{
		// Are we portable?
		if (strcmp(argv[i], "-portable") == 0)
		{
			is_portable = true;
		}

		// Inject a custom data dir.
		if (strcmp(argv[i], "-datadir") == 0)
		{
			// Mkay, did the user give us an argument?
			if (i != (argc - 1))
			{
				// Check if it exists.
				struct stat sb;

				if (stat(argv[i + 1], &sb) == 0)
				{
					if (!S_ISDIR(sb.st_mode))
					{
						WHBLogWritef("-datadir %s is not a directory\n", argv[i + 1]);
						return 1;
					}

					if(realpath(argv[i + 1], datadir) == NULL)
					{
						WHBLogWritef("realpath(datadir) failed: %s\n", strerror(errno));
						datadir[0] = '\0';
					}
				}
				else
				{
					WHBLogWritef("-datadir %s could not be found\n", argv[i + 1]);
					return 1;
				}
			}
			else
			{
				WHBLogWritef("-datadir needs an argument\n");
				return 1;
			}
		}

		// Inject a custom config dir.
		if (strcmp(argv[i], "-cfgdir") == 0)
		{
			// We need an argument.
			if (i != (argc - 1))
			{
				Q_strlcpy(cfgdir, argv[i + 1], sizeof(cfgdir));
			}
			else
			{
				WHBLogWritef("-cfgdir needs an argument\n");
				return 1;
			}

		}
	}

	// Initialize the game.
	// Never returns.
	Qcommon_Init(argc, argv);

    WHBLogWritef("Exiting...\n");

    WHBUnmountSdCard();
    WHBGfxShutdown();
    WHBProcShutdown();
    WHBLogCafeDeinit();
    WHBLogUdpDeinit();

	return 0;
}

