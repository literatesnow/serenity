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

#ifndef _H_SERENITY
#define _H_SERENITY

#include <my_global.h>
#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libconfig.h>

#include "global.h"

#include "bf2.h"
#include "src.h"

#include "sqlquery.h"

#ifdef UNIX
#include <signal.h>
#include <stdint.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h> 
#include <ctype.h>
#endif

#ifdef WIN32
#include <direct.h>
#endif

#define _NVE_HEADER_TITLE "Serenity"
#define _NVE_HEADER_AUTHOR "bliP"
#define _NVE_HEADER_EMAIL "spawn@nisda.net"
#define _NVE_HEADER_COPYRIGHT "Copyright 2006-2007"
#define _NVE_HEADER_URL "http://www.nisda.net/"
#define _NVE_VERSION "1.99"
#ifdef WIN32
#define _NEV_VER_SUFFIX "Win32"
#endif
#ifdef UNIX
#define _NEV_VER_SUFFIX "Linux"
#endif
#define _NVE_STATUS_VERSION _NVE_HEADER_TITLE " " _NVE_VERSION " " _NEV_VER_SUFFIX

#define _NVE_STATUS_ACTIVE 1
#define _NVE_STATUS_QUIT 0
#define _NVE_STATUS_RESTART -1

#define MAX_SERVERTYPE 2
#define SERVERTYPE_BATTLEFIELD2 "0"
#define SERVERTYPE_SOURCE "1"

#define DEFAULT_CONFIG_FILE "serenity.conf"
#define DEFAULT_MYSQL_PORT 3306
#define DEFAULT_LISTEN_PORT 42550

#define UNIX_NEWDIR 0755

#define MAX_GROUP 16
#define MAX_URL 256

#if 0
typedef struct resolver_s {
  MUTEX mutex;
  int active;
#ifdef UNIX
  pthread_t thread;
#endif
#ifdef WIN32
  HANDLE thread;
#endif
} resolver_t;
#endif

static int ServiceStartup(int *ret, int argc, char *argv[]);
static int Init(char *path);
static void Frame(void);
static void Finish(void);

static void UpdateStatus(void);

static int InsanityCheck(void);

#if 0
static void ResolveAddresses(void);
static void *ProcessAddressResolve(void *param);
static void FreeResolver(int now);
#endif

//bf2
static int ReadBF2ServerList(int *segs);
//source
static int ReadSRCServerList(void);

#ifdef SUPERDEBUG
static void ShowDebugStuff(void);
#endif

#ifdef WIN32
static BOOL HandleSignal(DWORD signal);
#endif
#ifdef UNIX
static void HandleSignal(int signal);
#endif

static int best_of(int n);

#endif //_H_SERENITY

