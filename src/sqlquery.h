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

#ifndef _H_SQLQUERY
#define _H_SQLQUERY

#include <my_global.h>
#include <mysql.h>

#include "global.h"
#include "util.h"

typedef struct mysqlquery_s {
  MYSQL *mysql;
  const char *sql_prefix;
  const char *sql_body;
  const char *sql_separator;
  const char *sql_suffix;
  int sql_prefix_len;
  int sql_body_len;
  int sql_separator_len;
  int sql_suffix_len;
  int body_count; //number of times sql_body_occurs
  int qmark; //number of question marks in body_count
  int other_qmark; //number of other questions marks in query (not body)
#ifdef SUPERDEBUG
  char what[64];
#endif

  //internalish stuff
  MYSQL_STMT *stmt;
  MYSQL_BIND *bindparam;
  MYSQL_BIND *bindres; //not malloc'd
} mysqlquery_t;

int OpenMySQL(MYSQL *mysql);
#define CloseMySQL(x) mysql_close(x) //for appearances

int OpenMySQLQuery(mysqlquery_t *query);
int RunMySQLQuery(mysqlquery_t *query);
int ResultMySQLQuery(mysqlquery_t *query);
void CloseMySQLQuery(mysqlquery_t *query);

#endif //_H_SQLQUERY

