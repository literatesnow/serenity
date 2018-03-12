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

#include "global.h"

extern serenity_t ne;

void WriteLog(char *fmt, ...) {
  FILE *fp;
  char buf[MAX_PATH];
  va_list argptr;
  struct tm *t;
	time_t long_time;
#ifdef WIN32
  int pid = GetCurrentProcessId();
  int tid = GetCurrentThreadId();
#endif
#ifdef UNIX
  pid_t pid = getpid();
  pthread_t tid = pthread_self();
#endif

#ifdef _DEBUG
  /* *************************************** */
	va_start(argptr, fmt);
	vprintf(fmt, argptr);
	va_end(argptr);
  return;
  /* *************************************** */
#endif

#ifdef SUPERDEBUG
  printf("WriteLog()\n");
#endif

	time(&long_time);
	t = localtime(&long_time);
  
  if (!t) {
#ifdef _DEBUG
    printf("WriteLog() time failed\n");
#endif
    return; // :(
  }

  _strnprintf(buf, sizeof(buf), "%s%s%s_%04d%02d%02d_%d-%d.log",
      ne.wdir,
      ne.log_dir ? ne.log_dir : "",
      LOG_PREFIX,
      t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
      (int) pid, (int) tid
      );

  fp = fopen(buf, "at");
  if (!fp) {
#ifdef _DEBUG
    printf("WriteLog() failed to open %s\n", buf);
#endif
    return;
  }

  _strnprintf(buf, sizeof(buf), "[%02d:%02d] ", t->tm_hour, t->tm_min);

	va_start(argptr, fmt);
  fputs(buf, fp);
	vfprintf(fp, fmt, argptr);
	va_end(argptr);

  fclose(fp);
}

