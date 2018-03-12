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

#include "sqlquery.h"

extern serenity_t ne;

int OpenMySQL(MYSQL *mysql) {
#ifdef MYSQL_OPT_RECONNECT
  my_bool my_true = TRUE;
#endif

#ifdef SUPERDEBUG
  printf("OpenMySQL()\n");
#endif

  if (!mysql_init(mysql)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL: Failed to init\n");
#endif
    return 0;
  }

  if (!mysql_real_connect(
           mysql,
           ne.mysql_address,
           ne.mysql_user,
           ne.mysql_pass,
           ne.mysql_database,
           ne.mysql_port,
           NULL,
           CLIENT_MULTI_STATEMENTS)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(mysql));
#endif
    CloseMySQL(mysql);
    return 0;
  }

#ifdef MYSQL_OPT_RECONNECT
  if (mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true)) {
#ifdef SUPERDEBUG
    printf("MySQL Warning: Unknown option MYSQL_OPT_RECONNECT\n");
#endif
  }
#endif
  mysql->reconnect = 1;

  return 1;
}


int OpenMySQLQuery(mysqlquery_t *query) {
  char *sqlbuf = NULL;
  int sqlbuflen, sqlbufsz, bindparamsz;
  int i, r = 0;

#ifdef SUPERDEBUG
  //printf("OpenMySQLQuery() [%s] Need a query of %d (bind:%d, mark:%d)\n", query->what, query->body_count, (query->body_count * query->qmark), query->qmark);
#endif

  query->stmt = NULL;
  query->bindparam = NULL;
  query->bindres = NULL;

  sqlbufsz = query->sql_prefix_len
             + (query->sql_body_len * query->body_count)
             + ((query->body_count - 1) * query->sql_separator_len)
             + query->sql_suffix_len
             + sizeof(char);
  sqlbuf = (char *)_malloc_(sqlbufsz);
  if (!sqlbuf) {
    goto _err;
  }

  bindparamsz = sizeof(MYSQL_BIND) * ((query->body_count * query->qmark) + query->other_qmark);
  query->bindparam = (MYSQL_BIND *)_malloc_(bindparamsz);
  if (!query->bindparam) {
    goto _err;
  }
  memset(query->bindparam, '\0', bindparamsz);

  strlcpy(sqlbuf, query->sql_prefix, sqlbufsz);
  for (i = 0; i < query->body_count; i++) {
    if (i > 0) {
      strlcat(sqlbuf, query->sql_separator, sqlbufsz);
    }
    strlcat(sqlbuf, query->sql_body, sqlbufsz);
  }
  sqlbuflen = strlcat(sqlbuf, query->sql_suffix, sqlbufsz);

#ifdef PARANOID
  if (sqlbuflen >= sqlbufsz) {
#ifdef SUPERDEBUG
    printf("MySQLQuery() [%s] WARNING! SQL truncation occured! %d >= %d\n", query->what, sqlbuflen, sqlbufsz);
#endif
    goto _err;
  }
#endif

  query->stmt = mysql_stmt_init(query->mysql);
  if (!query->stmt) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query - Init statement failed: %s\n", mysql_error(query->mysql));
#endif
    goto _err;
  }

#ifdef SUPERDEBUG
  //printf("MySQLQuery() [%s] [%d:%d] %s\n", query->what, sqlbuflen, sqlbufsz, sqlbuf);
#endif

  if (mysql_stmt_prepare(query->stmt, sqlbuf, sqlbuflen)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query - Init statement prepare failed: %s\n", mysql_stmt_error(query->stmt));
#endif
    goto _err;
  }

  r = 1;
_err:
#ifdef SUPERDEBUG
  if (!r) {
    printf("OpenMySQLQuery() [%s] FAILED\n", query->what);
  }
#endif
  if (sqlbuf) {
    _free_(sqlbuf);
  }
  if (!r) {
    CloseMySQLQuery(query);
  }

  return r;
}

int RunMySQLQuery(mysqlquery_t *query) {
#ifdef SUPERDEBUG
  //printf("RunMySQLQuery() [%s]\n", query->what);
#endif
  assert(query);
  assert(query->stmt);
  assert(query->bindparam);

  if (mysql_stmt_bind_param(query->stmt, query->bindparam)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query - Bind param failed: %s\n", mysql_stmt_error(query->stmt));
#endif
    return 0;
  }

  if (mysql_stmt_execute(query->stmt)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query - Execute failed: %s\n", mysql_stmt_error(query->stmt));
#endif
    return 0;
  }

  return 1;
}

int ResultMySQLQuery(mysqlquery_t *query) {
#ifdef SUPERDEBUG
  //printf("ResultMySQLQuery() [%s]\n", query->what);
#endif

  assert(query);
  assert(query->stmt);
  assert(query->bindres);

  if (mysql_stmt_bind_result(query->stmt, query->bindres)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query- Bind result failed: %s\n", mysql_stmt_error(query->stmt));
#endif
    return 0;
  }

  if (mysql_stmt_store_result(query->stmt)) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Query - Store result failed: %s\n", mysql_stmt_error(query->stmt));
#endif
    return 0;
  }

  return 1;
}

void CloseMySQLQuery(mysqlquery_t *query) {
#ifdef SUPERDEBUG
  //printf("CloseMySQLQuery() [%s]\n", query->what);
#endif

  assert(query);

  if (query->stmt) {
    mysql_stmt_close(query->stmt);
  }
  if (query->bindparam) {
    _free_(query->bindparam);
  }
}

