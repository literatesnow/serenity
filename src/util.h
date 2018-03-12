/*
Copyright (C) 2004 Chris Cuthbertson

This file is part of Serenity.

Serenity is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Serenity is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Serenity; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ESCAPE_LENGTH 128

int validip(char *s);
int remainder(char *s);
int atoipz(char *s);
void escape(char *s, int max);
void datetime(char *s, int len);
int leapyear(int year);
int monthdays(int month, int year);
int memcatz(char *dst, int dstlen, const char *src, int srclen, int siz);
unsigned long atolz(char *s);
unsigned int epoch(void);

#ifdef WIN32
#define _strcasecmp(s1, s2) _stricmp((s1), (s2)) //idea from Quake source, id rocks
#define _strncasecmp(s1, s2, n) _strnicmp((s1), (s2), (n))
#define _strnprintf _snprintf
#define vsnprintf _vsnprintf
#define mkdir _mkdir
#define _initmutex(m) InitializeCriticalSection(m)
#define _lockmutex(m) EnterCriticalSection(m)
#define _unlockmutex(m) LeaveCriticalSection(m)
#define _destroymutex(m) DeleteCriticalSection(m)
#define _endthread return 0
#define _threadvoid LPVOID
#define _close(s) closesocket(s)
#define _sleep(t) Sleep(t)
#define _mutex CRITICAL_SECTION
#endif
#ifdef UNIX
#define _strnprintf snprintf
#define _strcasecmp(s1, s2) strcasecmp((s1), (s2))
#define _strncasecmp(s1, s2, n) strncasecmp((s1), (s2), (n))
#define _initmutex(m) pthread_mutex_init(m, NULL)
#define _lockmutex(m) pthread_mutex_lock(m)
#define _unlockmutex(m) pthread_mutex_unlock(m)
#define _destroymutex(m) pthread_mutex_destroy(m)
#define _endthread pthread_exit(NULL)
#define _threadvoid void
#define _close(s) close(s)
#define _sleep(t) usleep(t * 1000)
#define _mutex pthread_mutex_t
#endif

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

