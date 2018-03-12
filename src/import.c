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

#include "game.h"

extern info_t *info;
extern server_t *servers;
extern server_type_t server_type[SUPPORTED_GAMES];

/*
 * importServers - import a list of servers from a file
 */
void importServers(void)
{
  info->kill = 0; //don't nuke me
  if (!importASEList())
  {
    echo(DEBUG_FATAL, "importServers: no servers\n");
    return;
  }
  queryServers();
  importParse();
#ifdef USE_MYSQL
  importAddMySql();
#endif
  importAddPHP();
}

/*
 * importASEList - reads IP:Port into servers struct
 */
int importASEList(void)
{
  FILE *fp;
  char ip[MAX_IP+1];
  char port[MAX_PORT+1];
  char c;
  char *p;
  int i, j, game, total, gport, qport, dup, len;
  server_t *s;

  total = 0;
  for (game = 0; game < info->num_types; game++)
  {
    fp = fopen(server_type[game].import_file, "rt");
    if (!fp)
    {
      echo(DEBUG_FATAL, "importASEList: couldn\'t open %s\n", server_type[game].import_file);
      continue;
    }

    c = ' ';    
    while (c != EOF)
    {
      for (i = 0; i < 2; i++)
      {
        if (!i)
        {
          p = ip;
          len = sizeof(ip);
        }
        else
        {
          p = port;
          len = sizeof(port);
        }

        j = 0;
        while (1)
        {
          c = fgetc(fp);
          if (c == ':' || c == '\n' || c == EOF)
            break;
          if (j < len)
          {
            *p++ = c;
            j++;
          }
        }
        *p = 0;

        if (c == EOF || c == '\n')
          break;
      }
      gport = atoi(port);

      if (!validip(ip) || !gport)
      {
        echo(DEBUG_FATAL, "importASEList: bad server: %s:%d type:%d\n", ip, gport, game);
        continue;
      }

      for (dup = 0, s = servers; s; s = s->next)
      {
        if (!strcmp(s->ip, ip) && (s->game_port == gport) && (s->type->game_type == game))
        {
          dup = 1;
          break;
        }
      }
      if (dup)
      {
        echo(DEBUG_INFO, "importASEList: dup server %s:%d(%d) type(%d), skipped\n", ip, gport, qport, game);
        continue;
      }

      qport = server_type[game].default_query_port;
      for (i = 0; i < MAX_PORT_SEARCH; i++)
      {
        if (!newServer(servers))
        {
          echo(DEBUG_FATAL, "importASEList: malloc failure for %s:%d(%d)\n", ip, gport, qport);
          continue;
        }
        strlcpy(servers->ip, ip, sizeof(servers->ip));
        servers->type = &server_type[game];
        servers->game_port = gport;
        servers->query_port = qport;

        echo(DEBUG_FLOOD, "importASEList: added %s:%d(%d) type:%d\n", ip, gport, qport, game);

        total++;
        qport++;
      }
    }
    fclose(fp);
  }
  info->total_servers = total;

  return total;
}

/*
 * importParse - checks if any data was recieved
 */
void importParse(void)
{
  server_t *s;

  for (s = servers; s; s = s->next)
  {
    if (!s->packet_recieved)
    {
      echo(DEBUG_INFO, "parseServers: no packets for %s:%d(%d) (timeout/error)\n", s->ip, s->game_port, s->query_port);
      continue;
    }

    if (!s->type->match_query(s->game_port, s->packet))
    {
      echo(DEBUG_INFO, "parseServers: no match for %s:%d(%d)\n", s->ip, s->game_port, s->query_port);
      continue;
    }
    echo(DEBUG_INFO, "parseServers: match: %s:%d\'s query port is %d\n", s->ip, s->game_port, s->query_port);
    s->got_reply = 1;
  }
}

#ifdef USE_MYSQL
void importAddMySql(void)
{
  char buff[MAX_MYSQL_QUERY+1];
  server_t *s;

  if (mysql_ping(info->db))
  {
    echo(DEBUG_FATAL, "importAdd: couldn\'t reconnect to database: %s\n", mysql_error(info->db));
    return;
  }

  for (s = servers; s; s = s->next)
  {
    if (!s->got_reply)
      continue;

    strlcpy(buff, MYSQL_QUERY_SERVERLIST_INSERT, sizeof(buff));
    if (!percent(info, s, NULL, NULL, buff))
    {
      echo(DEBUG_FATAL, "importAdd: error: bad percent (serverlist insert)\n");
      continue;
    }
    echo(DEBUG_INFO, "importAdd: serenity_serverlist: %s\n", buff);
    if (mysql_query(info->db, buff))
    {
      echo(DEBUG_FATAL, "importAdd: error: %s\n", mysql_error(info->db));
      continue;
    }
  }
}
#endif

/*
 * importAddPHP - generates a script for adding servers to database
 */
void importAddPHP(void)
{
  char sys[MAX_FILE_PATH+MAX_PHP_PARAMETERS+1];
  FILE *fp;
  server_t *s;

  fp = fopen(SERENITY_FILE_IMPORT, "w");
  if (!fp)
  {
    echo(DEBUG_INFO, "importAddPHP: couldn\'t write to file\n");
    return;
  }
  fprintf(fp, "<?php \n" \
              "\n" \
              "/* %s v%s - %s %s (%s)\n" \
              " * " SERENITY_FILE_IMPORT " - automagically generated on %s.\n" \
              " */\n" \
              "\n",
              HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, info->datetime);
  fprintf(fp, "$db = mysql_connect(\'%s:%d\', \'%s\', \'%s\');\n" \
              "if (!$db) {\n" \
              "  echo \'[php] could not connect: \' . mysql_error() . \"\\n\";\n" \
              "  exit(2);\n" \
              "}\n" \
              "if (!mysql_select_db(\'%s\', $db)) {\n" \
              "  echo \'[php] could not select: \' . mysql_error() . \"\\n\";\n" \
              "  exit(3);\n" \
              "}\n" \
              "\n",
              info->mysql_address,
              info->mysql_port,
              info->mysql_user,
              info->mysql_pass,
              info->mysql_database);

  for (s = servers; s; s = s->next)
  {
    if (!s->got_reply)
      continue;

    fprintf(fp, "$query = \"INSERT INTO serenity_serverlist VALUES \" .\n" \
                "         \"(0, \" .\n" \
                "         \"\'%s\', \" .\n" \
                "         \"%d, \" .\n" \
                "         \"%d, \" .\n" \
                "         \"\'\', \" .\n" \
                "         \"%d)\";\n",
                s->ip,
                s->game_port,
                s->query_port,
                s->type->game_type);
    fprintf(fp, "$result = mysql_query($query, $db);\n" \
                "if (!$result) {\n" \
                "  echo \"[php] $query\\n\";\n" \
                "  echo \'[php] invalid query (server insert): \' . mysql_error() . \"\\n\";\n" \
                "  exit(2);\n" \
                "}\n" \
                "\n");
  }
  fprintf(fp, "?>");
  fclose(fp);

  _strnprintf(sys, sizeof(sys), "%s %s " SERENITY_FILE_IMPORT, info->php_filepath, info->php_parameters);
  /*echo(DEBUG_FLOOD, "updateDbPHP: system: %s\n", sys); 
  i = system(sys);
  echo(DEBUG_FLOOD, "updateDbPHP: system returned: %d\n", i);*/
  run(sys, "updateDbPHP");
}

