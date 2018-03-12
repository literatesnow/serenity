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

#ifndef _H_GLOBAL
#define _H_GLOBAL

#include <my_global.h>
#include <mysql.h>

#include <stdio.h>

#include "util.h"

#ifdef WIN32
#include "MemLeakCheck.h"
#endif

#define LOG_PREFIX "serenity"
#define LOG_BASE "base"
//#define LOG_RESOLVE "resolve"

#define TIME_SAVE_TRANSFER (60 * 10)
#define TIME_UPDATE_STATUS (60 * 1)
#define TIME_RESOLVE_ADDRESS (15 * 60 * 1)
#define TIME_PBSTREAM_CHECK (60 * 7)
#define TIME_FILE_TIMEOUT 30

typedef struct serenity_s {
  MYSQL mysql;
  char *mysql_address;
  char *mysql_user;
  char *mysql_pass;
  char *mysql_database;
  int mysql_port;

  char *admin_name;
  char *internal_address;
  char *external_address;
#ifdef NVEDEXT
  char *ss_dir;
  char *pbscan_command;
#endif
  char *log_dir;
  int reconnect_delay;
  int start_port;
  int end_port;
#ifdef BF2CCEXT
  int bf2cc_start_port;
  int bf2cc_end_port;
#endif

  char *wdir;
  int state;
  int runservice;
  double laststatus;
  double lastsave;
  double lastresolve;
} serenity_t;

void WriteLog(char *fmt, ...);

#endif //_H_GLOBAL

