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
 * This file implements all system dependent generic functions.
 *
 * =======================================================================
 */

#include <dirent.h>
//#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/select.h> /* for fd_set */
#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif

#include "../../common/header/common.h"
#include "../../common/header/glob.h"

#include <whb/log.h>

// Pointer to game library
static void *game_library;

// Evil hack to determine if stdin is available
qboolean stdin_active = true;

// Config dir
char cfgdir[MAX_OSPATH] = CFGDIR;

// Console logfile
extern FILE	*logfile;

/* ================================================================ */

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char string[1024];

#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif
	Qcommon_Shutdown();

	va_start(argptr, error);
	vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	
	WHBLogWritef("Sys_Error\n");
	WHBLogWritef("Error: %s\n", string);

// __WIIU_TODO__ proper exiting
	exit(1);
}

void
Sys_Quit(void)
{
#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif

	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}

	Qcommon_Shutdown();

	WHBLogWrite("Sys_Quit\n");
	WHBLogWrite("------------------------------------\n");

// __WIIU_TODO__ proper exiting
	exit(0);
}

void
Sys_Init(void)
{
	// no console on WiiU
}

/* ================================================================ */

char *
Sys_ConsoleInput(void)
{
	// no console on WiiU
	return NULL;
}

void
Sys_ConsoleOutput(char *string)
{
	WHBLogWritef("%s", string);
}

/* ================================================================ */

long long
Sys_Microseconds(void)
{
// __WIIU_TODO__ replace with WiiU native functions?
	struct timespec now;
	static struct timespec first;
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &now);
#else
	clock_gettime(CLOCK_REALTIME, &now);
#endif

	if(first.tv_sec == 0)
	{
		long long nsec = now.tv_nsec;
		long long sec = now.tv_sec;
		// set back first by 1ms so neither this function nor Sys_Milliseconds()
		// (which calls this) will ever return 0
		nsec -= 1000000;
		if(nsec < 0)
		{
			nsec += 1000000000ll; // 1s in ns => definitely positive now
			--sec;
		}

		first.tv_sec = sec;
		first.tv_nsec = nsec;
	}

	long long sec = now.tv_sec - first.tv_sec;
	long long nsec = now.tv_nsec - first.tv_nsec;

	if(nsec < 0)
	{
		nsec += 1000000000ll; // 1s in ns
		--sec;
	}

	return sec*1000000ll + nsec/1000ll;
}

int
Sys_Milliseconds(void)
{
	return (int)(Sys_Microseconds()/1000ll);
}

void
Sys_Nanosleep(int nanosec)
{
	struct timespec t = {0, nanosec};
	nanosleep(&t, NULL);
}

/* ================================================================ */

/* The musthave and canhave arguments are unused in YQ2. We
   can't remove them since Sys_FindFirst() and Sys_FindNext()
   are defined in shared.h and may be used in custom game DLLs. */

static char findbase[MAX_OSPATH];
static char findpath[MAX_OSPATH];
static char findpattern[MAX_OSPATH];
static DIR *fdir;

char *
Sys_FindFirst(char *path, unsigned musthave, unsigned canhave)
{
	struct dirent *d;
	char *p;

	if (fdir)
	{
		Sys_Error("Sys_BeginFind without close");
	}

	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL)
	{
		*p = 0;
		strcpy(findpattern, p + 1);
	}
	else
	{
		strcpy(findpattern, "*");
	}

	if (strcmp(findpattern, "*.*") == 0)
	{
		strcpy(findpattern, "*");
	}

	if ((fdir = opendir(findbase)) == NULL)
	{
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		if (!*findpattern || glob_match(findpattern, d->d_name))
		{
			if ((strcmp(d->d_name, ".") != 0) || (strcmp(d->d_name, "..") != 0))
			{
				snprintf(findpath, sizeof(findpath), "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}

	return NULL;
}

char *
Sys_FindNext(unsigned musthave, unsigned canhave)
{
	struct dirent *d;

	if (fdir == NULL)
	{
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		if (!*findpattern || glob_match(findpattern, d->d_name))
		{
			if ((strcmp(d->d_name, ".") != 0) || (strcmp(d->d_name, "..") != 0))
			{
				snprintf(findpath, sizeof(findpath), "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}

	return NULL;
}

void
Sys_FindClose(void)
{
	if (fdir != NULL)
	{
		closedir(fdir);
	}

	fdir = NULL;
}

/* ================================================================ */

void
Sys_UnloadGame(void)
{
// __WIIU_TODO__ dynamic game dll loading?
#ifndef __WIIU__
	if (game_library)
	{
		dlclose(game_library);
	}
#endif
	game_library = NULL;
}

void *
Sys_GetGameAPI(void *parms)
{
// __WIIU_TODO__ dynamic game dll loading?
	typedef void *(*fnAPI)(void *);
	fnAPI GetGameAPI; 

	char name[MAX_OSPATH];
	char *path;
	char *str_p;
#ifdef __APPLE__
	const char *gamename = "game.dylib";
#else
	const char *gamename = "game.so";
#endif

	if (game_library)
	{
		Com_Error(ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");
	}

	Com_Printf("Loading library: %s\n", gamename);

	/* now run through the search paths */
	path = NULL;

	while (1)
	{
		FILE *fp;

		path = FS_NextPath(path);

		if (!path)
		{
			return NULL;     /* couldn't find one anywhere */
		}

		snprintf(name, MAX_OSPATH, "%s/%s", path, gamename);

		/* skip it if it just doesn't exist */
		fp = fopen(name, "rb");

		if (fp == NULL)
		{
			continue;
		}

		fclose(fp);

#ifdef USE_SANITIZER
		game_library = dlopen(name, RTLD_NOW | RTLD_NODELETE);
#else
#ifndef __WIIU__
		game_library = dlopen(name, RTLD_NOW);
#else
		game_library = NULL;
#endif
#endif

		if (game_library)
		{
			Com_MDPrintf("Loading library: %s\n", name);
			break;
		}
		else
		{
			Com_Printf("Loading library: %s\n: ", name);

#ifndef __WIIU__
			path = (char *)dlerror();
#else
			path = "foobar";
#endif
			str_p = strchr(path, ':');   /* skip the path (already shown) */

			if (str_p == NULL)
			{
				str_p = path;
			}
			else
			{
				str_p++;
			}

			Com_Printf("%s\n", str_p);

			return NULL;
		}
	}

#ifndef __WIIU__
	GetGameAPI = (fnAPI)dlsym(game_library, "GetGameAPI");
#else
	GetGameAPI = NULL;
#endif

	if (!GetGameAPI)
	{
		Sys_UnloadGame();
		return NULL;
	}

	return GetGameAPI(parms);
}

/* ================================================================ */

void
Sys_Mkdir(const char *path)
{
	if (!Sys_IsDir(path))
	{
		if (mkdir(path, 0755) != 0)
		{
			// we dont fail on WiiU primarily because this sometimes wants to create
			// directories in write protected locations
		}
	}
}

qboolean
Sys_IsDir(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) != -1)
	{
		if (S_ISDIR(sb.st_mode))
		{
			return true;
		}
	}

	return false;
}

qboolean
Sys_IsFile(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) != -1)
	{
		if (S_ISREG(sb.st_mode))
		{
			return true;
		}
	}

	return false;
}

char *
Sys_GetHomeDir(void)
{
	static char gdir[MAX_OSPATH];

// __WIIU_TODO__ hardcoded for now
	snprintf(gdir, sizeof(gdir), "/vol/external01/quake2/");
	return gdir;
}

void
Sys_Remove(const char *path)
{
	remove(path);
}

int
Sys_Rename(const char *from, const char *to)
{
	return rename(from, to);
}

void
Sys_RemoveDir(const char *path)
{
	char filepath[MAX_OSPATH];
	struct dirent *file;
	DIR *directory;

	if (Sys_IsDir(path))
	{
		directory = opendir(path);
		if (directory)
		{
			while ((file = readdir(directory)) != NULL)
			{
				snprintf(filepath, MAX_OSPATH, "%s/%s", path, file->d_name);
				Sys_Remove(filepath);
			}

			closedir(directory);
			Sys_Remove(path);
		}
	}
}

qboolean
Sys_Realpath(const char *in, char *out, size_t size)
{
	char *converted = realpath(in, NULL);
	WHBLogWritef("Sys_Realpath %s --> %s\n", in, converted);

	if (converted == NULL)
	{
		Q_strlcpy(out, in, size);
		return true;
	}

	// __WIIU_TODO__ realpath seems to prefix the path with "fs:/" ??
	char *fixed = converted;
	if(fixed[0] == 'f' &&
		fixed[1] == 's' &&
		fixed[2] == ':' &&
		fixed[3] == '/')
		fixed = fixed + 3;
	Q_strlcpy(out, fixed, size);

	free(converted);

	return true;
}

/* ================================================================ */

void *
Sys_GetProcAddress(void *handle, const char *sym)
{
// __WIIU_TODO__ dynamic game dll loading?
#ifdef __WIIU__
		return NULL;
#else
    if (handle == NULL)
    {
#ifdef RTLD_DEFAULT
        return dlsym(RTLD_DEFAULT, sym);
#else
        /* POSIX suggests that this is a portable equivalent */
        static void *global_namespace = NULL;

        if (global_namespace == NULL)
            global_namespace = dlopen(NULL, RTLD_GLOBAL|RTLD_LAZY);

        return dlsym(global_namespace, sym);
#endif
    }
    return dlsym(handle, sym);
#endif
}

void
Sys_FreeLibrary(void *handle)
{
// __WIIU_TODO__ dynamic game dll loading?
#ifdef __WIIU__
#else
	if (handle && dlclose(handle))
	{
		Com_Error(ERR_FATAL, "dlclose failed on %p: %s", handle, dlerror());
	}
#endif
}

#include <coreinit/dynload.h>

void *
Sys_LoadLibrary(const char *path, const char *sym, void **handle)
{
// __WIIU_TODO__ dynamic game dll loading?
#ifdef __WIIU__
	//OSDynLoad_Module module;
	WHBLogWritef("loadlib: %s::%s\n", path, sym);
	//int err = OSDynLoad_Acquire("ref_wiiu", &module);
	//WHBLogWritef("loadlib: %s::%s --> %d --> %08X", path, sym, err, module);
	return NULL;
#else
	void *module, *entry;

	*handle = NULL;

#ifdef USE_SANITIZER
	module = dlopen(path, RTLD_LAZY | RTLD_NODELETE);
#else
	module = dlopen(path, RTLD_LAZY);
#endif

	if (!module)
	{
		Com_Printf("%s failed: %s\n", __func__, dlerror());
		return NULL;
	}

	if (sym)
	{
		entry = dlsym(module, sym);

		if (!entry)
		{
			Com_Printf("%s failed: %s\n", __func__, dlerror());
			dlclose(module);
			return NULL;
		}
	}
	else
	{
		entry = NULL;
	}

	Com_DPrintf("%s succeeded: %s\n", __func__, path);

	*handle = module;

	return entry;
#endif
}

/* ================================================================ */

void
Sys_GetWorkDir(char *buffer, size_t len)
{
	if (getcwd(buffer, len) != 0)
	{
		return;
	}

	buffer[0] = '\0';
}

qboolean
Sys_SetWorkDir(char *path)
{
	if (chdir(path) == 0)
	{
		return true;
	}

	return false;
}
