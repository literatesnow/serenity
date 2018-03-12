/*
  Copyright (C) 2004-2008 Chris Cuthbertson

  This file is part of Serenity.

  Serenity is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Serenity is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Serenity.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _H_UTIL
#define _H_UTIL

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#ifdef UNIX
#include <sys/time.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#define CHARSIZE(x) (sizeof(x) - sizeof(char))

#ifdef WIN32
#define _strcasecmp(s1, s2) _stricmp((s1), (s2))
#define _strncasecmp(s1, s2, n) _strnicmp((s1), (s2), (n))
#define _strnprintf _snprintf
#define _vstrnprintf _vsnprintf
#define stat fstat
#define MUTEX CRITICAL_SECTION
#define _initmutex(m) InitializeCriticalSection(m)
#define _lockmutex(m) EnterCriticalSection(m)
#define _unlockmutex(m) LeaveCriticalSection(m)
#define _destroymutex(m) DeleteCriticalSection(m)
#define _sleep(t) Sleep(t)
#define uint64_t unsigned __int64
#define in_addr_t unsigned long
#endif
#ifdef UNIX
#define _malloc_ malloc
#define _realloc_ realloc
#define _calloc_ calloc
#define _strdup_ strdup
#define _free_ free
#define _strnprintf snprintf
#define _vstrnprintf vsnprintf
#define _strcasecmp(s1, s2) strcasecmp((s1), (s2))
#define _strncasecmp(s1, s2, n) strncasecmp((s1), (s2), (n))
#define MUTEX pthread_mutex_t
#define _initmutex(m) pthread_mutex_init(m, NULL)
#define _lockmutex(m) pthread_mutex_lock(m)
#define _unlockmutex(m) pthread_mutex_unlock(m)
#define _destroymutex(m) pthread_mutex_destroy(m)
#define _sleep(t) usleep(t * 1000)
#define MAX_PATH 256
#endif

double DoubleTime(void);

size_t pcrestrcpy(char *dst, const char *src, int pos, int ovec[], int max);
int pcreatoi(const char *src, int pos, int ovec[]);

char *workingdir(char *path);

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

#endif //_H_UTIL

