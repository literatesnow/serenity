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

server_t *servers;
info_t *info;
_mutex mutex;
extern server_type_t server_type[SUPPORTED_GAMES];

/*
 * main - entry point
 */
int main (int argc, char *argv[])
{
  if (!init(argc, argv))
  {
    finish(0);
    return 1;
  }

  if (!readServersPHP())
  {
    finish(0);
    return 2;
  }

  queryServers();

  parseServers();
  
#if 0
  printServers();
#endif

#ifdef RRDTOOL
  graphServers();
#endif
  graphServersPHP();

  if (!updateDbPHP())
  {
    finish(0);
    return 3;
  }

  finish(0);

  return 0;
}

/*
 * setSockAddr - init a socket for an ip and port
 */
void setSockAddr(struct sockaddr_in *udp, char *ip, int port)
{
  memset(&(udp->sin_zero), '\0', sizeof(udp->sin_zero));
  udp->sin_family = AF_INET;
  udp->sin_addr.s_addr = (ip) ? inet_addr(ip) : htonl(INADDR_ANY);
  udp->sin_port = htons((short)port);
}

/*
 * setSock - binds a socket
 */
int setSock(int *sock, int port)
{
  int i;
  struct sockaddr_in udp;

#ifdef WIN32
  unsigned long nonblocking = 1;
#endif

#ifdef WIN32
  if ((*sock = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
  {
#endif
#ifdef UNIX
  if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
#endif
    echo(DEBUG_FATAL, "setSock: socket() failed\n");
    return 0;
  }

#ifdef WIN32
  if (ioctlsocket(*sock, FIONBIO, &nonblocking) != 0) //win32 does not nonblock by default, force
  {
    echo(DEBUG_FLOOD, "setSock: ioctlsocket() failed\n");
    return 0;
  }
#endif

  //find a free socket
  i = 0;
  while (1)
  {
    setSockAddr(&udp, NULL, port);
#ifdef WIN32
    if (bind(*sock, (LPSOCKADDR)&udp, sizeof(struct sockaddr)) == 0)
#endif
#ifdef UNIX
    if (bind(*sock, (struct sockaddr *)&udp, sizeof(struct sockaddr)) != -1)
#endif
      break;

    echo(DEBUG_FATAL, "setSock: bind() to port %d failed\n", port);
    //port++;// += info->total_servers + 1;
    port += info->query_retry;
    if (port > MAX_PORT_CIELING)
      port = DEFAULT_LOCAL_PORT;
    i++;

    if (i > MAX_PORT_RETRY)
    {
      echo(DEBUG_FATAL, "setSock: couldn\'t find a port to bind\n");
      return 0;
    }
  }
  echo(DEBUG_FLOOD, "setSock: using port: %d\n", port);

  return 1;
}

/*
 * getSock - send and recieve udp data
 */
int getSock(int *sock, struct sockaddr_in *send, getsockinfo_t *sockinfo)
{
  int i, start, hsz, sz;
  char buff[MAX_RECIEVE+1];
  struct sockaddr_in recv;

  sockinfo->bytes_rx = 0;
  sockinfo->bytes_tx = 0;
  sockinfo->rx = 0;
  sockinfo->rxlen = 0;

#ifdef WIN32
  hsz = sizeof(SOCKADDR);
#endif
#ifdef UNIX
  hsz = sizeof(struct sockaddr_in);
#endif

  //retry if needed
  for (i = 0; i < info->query_retry && !sockinfo->rx; i++)
  {
    //send data
#ifdef WIN32
    sz = sendto(*sock, sockinfo->txdata, sockinfo->txdatalen, 0, (LPSOCKADDR)send, sizeof(SOCKADDR));
#endif
#ifdef UNIX
    sz = sendto(*sock, sockinfo->txdata, sockinfo->txdatalen, 0, (struct sockaddr *)send, sizeof(struct sockaddr));
#endif
    if (sz == -1)
    {
      echo(DEBUG_FLOOD, "getSock: [%d] sendto() error\n", sockinfo->id);
      continue;
    }
    sockinfo->bytes_tx += sz;

    //wait for reply
    start = info->realtime;
    while (info->realtime - start < info->timeout)
    { 
      echo(DEBUG_FLOOD, "getSock: [%d] thinking(%d)...\n", sockinfo->id, info->realtime - start);
      _sleep(500);

      //recieve data
#ifdef WIN32        
      sz = recvfrom(*sock, buff, sizeof(buff), 0, (LPSOCKADDR)&recv, &hsz);
#endif
#ifdef UNIX
      sz = recvfrom(*sock, buff, sizeof(buff), MSG_DONTWAIT, (struct sockaddr *)&recv, &hsz);
#endif
      if (sz == -1)
        continue;

      //got something
      buff[sz] = '\0';
      sockinfo->bytes_rx += sz;
      sockinfo->rx++;
      echo(DEBUG_FLOOD, "getSock: [%d] recv got %d bytes from %s:%d (%d total)\n", sockinfo->id, sz, inet_ntoa(recv.sin_addr), ntohs(recv.sin_port), sockinfo->rx);
      memcatz(sockinfo->rxdata, sockinfo->rxlen, buff, sz, sockinfo->rxdatalen);
      sockinfo->rxlen += sz; //keep this below memcatz BECAUSE OTHERWISE IT DOESN'T WORK OK
    }
  }

  return sockinfo->rx;
}

/*
 * singleQuery - thread for querying a server
 */
void *singleQuery(_threadvoid *data) 
{
  char query[MAX_QUERY+1];
  server_t *server = (server_t *)data;
  struct sockaddr_in send;
  getsockinfo_t sockinfo;
  int sock;

  //make socket
  if (!setSock(&sock, server->id))
  {
    echo(DEBUG_FATAL, "singleQuery: [%d] couldn\'t bind socket for %s:%d\n", server->id, server->ip, server->query_port);
    _endthread;
  }

  query[0] = '\0';
  setSockAddr(&send, server->ip, server->query_port);
  server->type->packet_query(query, 1000);
  sockinfo.txdata = query;
  sockinfo.txdatalen = server->type->query_length;
  sockinfo.rxdata = server->packet;
  sockinfo.rxdatalen = sizeof(server->packet);
  sockinfo.id = server->id;

  echo(DEBUG_FLOOD, "singleQuery: [%d] querying %s:%d\n", server->id, server->ip, server->query_port);

  //get data
  if (getSock(&sock, &send, &sockinfo))
  {
    if (server->type->info_query(server->packet))
    {
      server->bytes_recieved += sockinfo.bytes_rx;
      server->bytes_sent += sockinfo.bytes_tx;
      server->packet_length = sockinfo.rxlen;
      server->packet_recieved = sockinfo.rx;
    }
    else
    {
      server->trashed++;
      echo(DEBUG_FLOOD, "singleQuery: [%d] bad reply\n", server->id);
    }
  }  

  _close(sock);
  _endthread;
}

/*
 * queryServers - query all server, spawns and waits for threads
 */
void queryServers(void)
{ 
  server_t *s;
  int start, i;
  int port;

  start = info->realtime;
  port = info->local_port;

  for (i = 0, s = servers; s; s = s->next, i++)
  {
    s->id = port;
    s->threadid = s;
    echo(DEBUG_FLOOD, "queryServers: query starting %s:%d\n", s->ip, s->game_port);
    //thread creation
#ifdef WIN32
    s->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)singleQuery, (void *)&*s->threadid, 0, NULL);
    if (!s->thread)
    {
#endif
#ifdef UNIX   
    if (pthread_create(&s->thread, NULL, singleQuery, (void *)&*s->threadid/*<-- that's kinda trippy..*/))
    {
#endif
		  echo(DEBUG_FLOOD, "queryServers: couldn\'t create a thread for %s:%d\n", s->ip, s->game_port);
      continue;
    }
    port += info->total_servers + 1;
    if (port > MAX_PORT_CIELING)
      port = DEFAULT_LOCAL_PORT;

    if (info->query_max && i > info->query_max)
    {
      echo(DEBUG_FLOOD, "queryServers: sleeping\n");
      _sleep(1000 * info->timeout);
      i = 0;
    }
  }

  //wait for all the threads to finish
  for (s = servers; s; s = s->next)
  {
    echo(DEBUG_FLOOD, "queryServers: waiting for threads\n");
#ifdef WIN32
    WaitForSingleObject(s->thread, INFINITE);
    CloseHandle(s->thread);
#endif
#ifdef UNIX
    pthread_join(s->thread, NULL);
#endif
    s->threadid = NULL;
  }

  info->query_time = info->realtime - start;
  echo(DEBUG_FLOOD, "queryServers: query completed: %d servers in %d seconds\n", info->total_servers, info->query_time);
}

/*
 * parseServers - parse the queries which have been recieved
 */
void parseServers(void)
{
  server_t *s;

  for (s = servers; s; s = s->next)
  {
    if (!s->packet_recieved)
    {
      info->no_reply++;
      echo(DEBUG_INFO, "parseServers: no packets for %s:%d (timeout/error)\n", s->ip, s->game_port);
      continue;
    }

    echoc(DEBUG_FLOOD, s->packet, s->packet_length);

    if (!s->type->parse_query(s))
    {
      echo(DEBUG_INFO, "parseServers: bad reply for %s:%d\n", s->ip, s->game_port);
      info->bad_reply++;
      continue;
    }
    s->got_reply = 1;
  }
}

#ifdef RRDTOOL
void *singleGraph(_threadvoid *data)
{
  server_t *server = (server_t *)data;
  FILE *fp;
  char s[512];
  unsigned int seconds;

  if (!info->use_rrdtool || !server->got_reply)
    _endthread;

  seconds = epoch();

  _strnprintf(s, sizeof(s), "./data/%s_%d.rrd", server->ip, server->game_port);
  fp = fopen(s, "rb"); //insert better file exists here
  if (!fp)
    _strnprintf(s, sizeof(s), "%s create ./data/%s_%d.rrd --start %ld DS:speed:GAUGE:300:U:U RRA:AVERAGE:0.5:1:288 RRA:AVERAGE:0.5:6:336 RRA:AVERAGE:0.5:24:372 RRA:AVERAGE:0.5:288:351", info->rrdtool_filepath, server->ip, server->game_port, seconds);
  else
  {
    _strnprintf(s, sizeof(s), "%s update ./data/%s_%d.rrd %ld:%d", info->rrdtool_filepath, server->ip, server->game_port, seconds, server->num_players);
    fclose(fp);
  }
  system(s);

  _strnprintf(s, sizeof(s), "%s graph %s/%s_%d_day.png --lazy --units-exponent 0 -a PNG --no-legend -w 225 -h 65 --vertical-label Players DEF:players=./data/%s_%d.rrd:speed:AVERAGE AREA:players#FF9900 > /dev/null", info->rrdtool_filepath, info->rrdtool_images, server->ip, server->game_port, server->ip, server->game_port);
  system(s);
  _strnprintf(s, sizeof(s), "%s graph %s/%s_%d_week.png --lazy --units-exponent 0 --start %ld -a PNG --no-legend -w 225 -h 65 --vertical-label Players DEF:players=./data/%s_%d.rrd:speed:AVERAGE AREA:players#FF9900 > /dev/null", info->rrdtool_filepath, info->rrdtool_images, server->ip, server->game_port, (seconds - 604800), server->ip, server->game_port);
  system(s);
  _strnprintf(s, sizeof(s), "%s graph %s/%s_%d_month.png --lazy --units-exponent 0 --start %ld -a PNG --no-legend -w 225 -h 65 --vertical-label Players DEF:players=./data/%s_%d.rrd:speed:AVERAGE AREA:players#FF9900 > /dev/null", info->rrdtool_filepath, info->rrdtool_images, server->ip, server->game_port, (seconds - 2592000), server->ip, server->game_port);
  system(s);
  _strnprintf(s, sizeof(s), "%s graph %s/%s_%d_year.png --lazy --units-exponent 0 --start %ld -a PNG --no-legend -w 225 -h 65 --vertical-label Players DEF:players=./data/%s_%d.rrd:speed:AVERAGE AREA:players#FF9900 > /dev/null", info->rrdtool_filepath, info->rrdtool_images, server->ip, server->game_port, (seconds - 31536000), server->ip, server->game_port);
  system(s);
  echo(DEBUG_FLOOD, "graphServer: %s:%d @ %ld\n", server->ip, server->game_port, seconds);

  _endthread;
}
#endif

#ifdef RRDTOOL
void graphServers(void)
{
  server_t *s;
  int i;

  for (i = 0, s = servers; s; s = s->next, i++)
  {
    s->threadid = s;
    echo(DEBUG_FLOOD, "graphServers: graph %s:%d\n", s->ip, s->game_port);
#ifdef WIN32
    s->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)singleGraph, (void *)&*s->threadid, 0, NULL);
    if (!s->thread)
    {
#endif
#ifdef UNIX   
    if (pthread_create(&s->thread, NULL, singleGraph, (void *)&*s->threadid))
    {
#endif
		  echo(DEBUG_FLOOD, "graphServers: couldn\'t create a thread for %s:%d\n", s->ip, s->game_port);
      continue;
    }
  }

  for (s = servers; s; s = s->next)
  {
    echo(DEBUG_FLOOD, "graphServers: waiting for threads\n");
#ifdef WIN32
    WaitForSingleObject(s->thread, INFINITE);
    CloseHandle(s->thread);
#endif
#ifdef UNIX
    pthread_join(s->thread, NULL);
#endif
    s->threadid = NULL;
  }
}
#endif

#ifdef USE_MYSQL
int updateDbMySql(void)
{
  MYSQL_RES *res;
  MYSQL_ROW	row;
  char buff[MAX_MYSQL_QUERY+1];
  unsigned long i;
  server_t *s;
  player_t *p;
  rule_t *r;
  struct tm *timeinfo;
  time_t rawtime;

  if (mysql_ping(info->db))
  {
    echo(DEBUG_FATAL, "updateDatabase: couldn\'t reconnect to database: %s\n", mysql_error(info->db));
    return 0;
  }

  res = NULL;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(info->datetime, sizeof(info->datetime), "%Y-%m-%d %H:%M:%S", timeinfo);
  echo(DEBUG_FLOOD, "updateDatabase: datetime(%s)\n", info->datetime);

  //escape strings to be mysql compatible
  //fix me .. tidy up ..
  for (s = servers; s; s = s->next)
  {
    if (!s->got_reply)
      continue;
    mysql_real_escape_string(info->db, buff, s->host_name, strlen(s->host_name));
    strlcpy(s->host_name, buff, sizeof(s->host_name));
    mysql_real_escape_string(info->db, buff, s->map_name, strlen(s->map_name));
    strlcpy(s->map_name, buff, sizeof(s->map_name));
    mysql_real_escape_string(info->db, buff, s->game_type, strlen(s->game_type));
    strlcpy(s->game_type, buff, sizeof(s->game_type));
    mysql_real_escape_string(info->db, buff, s->game_id, strlen(s->game_id));
    strlcpy(s->game_id, buff, sizeof(s->game_id));
    mysql_real_escape_string(info->db, buff, s->version, strlen(s->version));
    strlcpy(s->version, buff, sizeof(s->version));
    for (p = s->players; p; p = p->next)
    {
      mysql_real_escape_string(info->db, buff, p->keyhash, strlen(p->keyhash));
      strlcpy(p->keyhash, buff, sizeof(p->keyhash));
      mysql_real_escape_string(info->db, buff, p->name, strlen(p->name));
      strlcpy(p->name, buff, sizeof(p->name));    
    }
    for (r = s->rules; r; r = r->next)
    {
      mysql_real_escape_string(info->db, buff, r->key, strlen(r->key));
      strlcpy(r->key, buff, sizeof(r->key));
      mysql_real_escape_string(info->db, buff, r->value, strlen(r->value));
      strlcpy(r->value, buff, sizeof(r->value));    
    }
  }

  //delete all rules
  strlcpy(buff, MYSQL_QUERY_RULE_DELETE, sizeof(buff));
  if (!percent(info, s, NULL, NULL, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: bad percent (rule delete)\n");
    return 0;
  }
  if (mysql_query(info->db, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
    return 0;
  }

  for (s = servers; s; s = s->next)
  {
    /* server */
    info->server_id = 0;
    strlcpy(buff, MYSQL_QUERY_SERVER_SELECT, sizeof(buff));
    if (!percent(info, s, NULL, NULL, buff))
    {
      echo(DEBUG_FATAL, "updateDatabase: error: bad percent (server select)\n");
      continue;
    }
    echo(DEBUG_INFO, "updateDatabase: serenity_server: %s\n", buff);
    if (mysql_query(info->db, buff))
    {
      echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
      continue;
    }
    res = mysql_store_result(info->db);
    i = (unsigned long)mysql_num_rows(res);
    if (i > 0)
    {
      row = mysql_fetch_row(res);
      if (row)
      {
        info->server_id = atolz(row[MYSQL_TABLE_SERVER_ID]);
        s->update_success = (unsigned int)atolz(row[MYSQL_TABLE_SERVER_UPDATE_SUCCESS]);
        s->update_fail = (unsigned int)atolz(row[MYSQL_TABLE_SERVER_UPDATE_FAIL]);
        s->update_consecutive = (unsigned int)atolz(row[MYSQL_TABLE_SERVER_CONSECUTIVE_FAILS]);
        s->trashed += (unsigned int)atolz(row[MYSQL_TABLE_SERVER_TRASHED]);
        s->bytes_recieved += atolz(row[MYSQL_TABLE_SERVER_BYTESRECIEVED]);
        s->bytes_sent += atolz(row[MYSQL_TABLE_SERVER_BYTESSENT]);
      }
      if (s->got_reply)
      {
        s->update_success++;
        strlcpy(buff, MYSQL_QUERY_SERVER_SUCCESS, sizeof(buff));
        if (!percent(info, s, NULL, NULL, buff)) {
          echo(DEBUG_FATAL, "updateDatabase: error: bad percent (server success)\n");
          continue;
        }
      }
      else
      {
        s->update_success++;
        s->update_consecutive++;
        strlcpy(buff, MYSQL_QUERY_SERVER_FAILED, sizeof(buff));
        if (!percent(info, s, NULL, NULL, buff))
        {
          echo(DEBUG_FATAL, "updateDatabase: error: bad percent (server failed)\n");
          continue;
        }
      }
    }
    else
    {
      if (s->got_reply)
        s->update_success = 1;
      else
      {
        s->update_fail = 1;
        s->update_consecutive = 1;
      }
      strlcpy(buff, MYSQL_QUERY_SERVER_INSERT, sizeof(buff));
      if (!percent(info, s, NULL, NULL, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: bad percent (server insert)\n");
        continue;
      }
    }
    echo(DEBUG_INFO, "updateDatabase: serenity_server: %s\n", buff);
    if (mysql_query(info->db, buff))
    {
      echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
      continue;
    }

    if (!s->got_reply)
      continue;

    if (!info->server_id)
      info->server_id = (unsigned long)mysql_insert_id(info->db);

    /* rules */
    for (r = s->rules; r; r = r->next)
    {
      strlcpy(buff, MYSQL_QUERY_RULE_INSERT, sizeof(buff));
      if (!percent(info, s, NULL, r, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: bad percent (rule insert)\n");
        continue;
      }
      echo(DEBUG_INFO, "updateDatabase: serenity_rule: %s\n", buff);
      if (mysql_query(info->db, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
        continue;
      }
    }

    for (p = s->players; p; p = p->next)
    {
      if (!s->type->check_player(p))
        continue;

      /* keyhash */
      info->keyhash_id = 0; //0 means we don't know their keyhash
      if (p->keyhash[0]) //only add real keyhashes
      {
        strlcpy(buff, MYSQL_QUERY_KEYHASH_SELECT, sizeof(buff));
        if (!percent(NULL, s, p, NULL, buff))
        {
          echo(DEBUG_FATAL, "updateDatabase: error: bad percent (keyhash select)\n");
          continue;
        }
        echo(DEBUG_INFO, "updateDatabase: serenity_keyhash: %s\n", buff);
        if (mysql_query(info->db, buff))
        {
          echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
          continue;
        }
        res = mysql_store_result(info->db);
        i = (unsigned long)mysql_num_rows(res);
        if (i > 0)
        {
          row = mysql_fetch_row(res);
          if (row)
            info->keyhash_id = atolz(row[MYSQL_TABLE_KEYHASH_ID]);
        }
        else
        {
          strlcpy(buff, MYSQL_QUERY_KEYHASH_INSERT, sizeof(buff));
          if (!percent(NULL, s, p, NULL, buff))
          {
            echo(DEBUG_FATAL, "updateDatabase: error: bad percent (keyhash insert)\n");
            continue;
          }
          echo(DEBUG_INFO, "updateDatabase: serenity_keyhash: %s\n", buff);
          if (mysql_query(info->db, buff))
          {
            echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
            continue;
          }
          info->keyhash_id = (unsigned long)mysql_insert_id(info->db);
        }
      }

      /* player */
      strlcpy(buff, MYSQL_QUERY_PLAYER_SELECT, sizeof(buff));
      if (!percent(info, s, p, NULL, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: bad percent (player select)\n");
        continue;
      } 
      echo(DEBUG_INFO, "updateDatabase: serenity_player: %s\n", buff);
      if (mysql_query(info->db, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
        continue;
      }
      res = mysql_store_result(info->db);
      i = (unsigned long)mysql_num_rows(res);
      if (i > 0)
        strlcpy(buff, MYSQL_QUERY_PLAYER_UPDATE, sizeof(buff));
      else
        strlcpy(buff, MYSQL_QUERY_PLAYER_INSERT, sizeof(buff));
      if (!percent(info, s, p, NULL, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: bad percent (player update)\n");
        continue;
      } 
      echo(DEBUG_INFO, "updateDatabase: serenity_player: %s\n", buff);
      if (mysql_query(info->db, buff))
      {
        echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
        continue;
      }
    }
  }
  if (res)
    mysql_free_result(res);

  /* info */
  info->runtime[RUNTIME_LOW] = info->realtime;
  info->runtime[RUNTIME_HIGH] = info->realtime;
  info->runtime[RUNTIME_LAST] = info->realtime;
  strlcpy(buff, MYSQL_QUERY_INFO_SELECT, sizeof(buff));
  if (!percent(info, NULL, NULL, NULL, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: bad percent (info select)\n");
    return 0;
  }
  echo(DEBUG_INFO, "updateDatabase: serenity_info: %s\n", buff);
  if (mysql_query(info->db, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
    return 0;
  }
  res = mysql_store_result(info->db);
  i = (unsigned long)mysql_num_rows(res);
  if (i > 0)
  {
    row = mysql_fetch_row(res);
    if (row)
    {
      info->bad_reply += atolz(row[MYSQL_TABLE_INFO_BAD_REPLY]);
      info->no_reply += atolz(row[MYSQL_TABLE_INFO_NO_REPLY]);
      info->runtime[RUNTIME_LOW] = (unsigned short)atolz(row[MYSQL_TABLE_INFO_LOW_RUN_TIME]);
      info->runtime[RUNTIME_HIGH] = (unsigned short)atolz(row[MYSQL_TABLE_INFO_HIGH_RUN_TIME]);
      if (info->realtime < info->runtime[RUNTIME_LOW])
        info->runtime[RUNTIME_LOW] = info->realtime;
      else if (info->realtime > info->runtime[RUNTIME_HIGH])
        info->runtime[RUNTIME_HIGH] = info->realtime;
    }
    strlcpy(buff, MYSQL_QUERY_INFO_UPDATE, sizeof(buff));
  }
  else
  {
    strlcpy(buff, MYSQL_QUERY_INFO_INSERT, sizeof(buff));
  }
  if (!percent(info, NULL, NULL, NULL, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: bad percent (info insert)\n");
    return 0;
  } 
  echo(DEBUG_INFO, "updateDatabase: serenity_info: %s\n", buff);
  if (mysql_query(info->db, buff))
  {
    echo(DEBUG_FATAL, "updateDatabase: error: %s\n", mysql_error(info->db));
  }

  if (res)
    mysql_free_result(res);

  return 1;
}
#endif

/*
 * updateDbPHP - generates scripts to update the database
 * The Nightmare procedure, miss one \\' and it borks.
 * The fun part was writing the PHP then trying to write the C code to generate it.
 */
int updateDbPHP(void)
{
  char sys[MAX_FILE_PATH+MAX_PHP_PARAMETERS+1];
  FILE *fp;
  server_t *s;
  player_t *p;
  rule_t *r;

  datetime(info->datetime, sizeof(info->datetime));

  //these string are escaped for php
  for (s = servers; s; s = s->next)
  {
    if (!s->got_reply)
      continue;
    escape(s->host_name, sizeof(s->host_name));
    escape(s->map_name, sizeof(s->map_name));
    escape(s->game_type, sizeof(s->game_type));
    escape(s->game_id, sizeof(s->game_id));
    escape(s->version, sizeof(s->version));
    for (p = s->players; p; p = p->next)
    {
      escape(p->keyhash, sizeof(p->keyhash));
      escape(p->name, sizeof(p->name));    
    }
    for (r = s->rules; r; r = r->next)
    {
      escape(r->key, sizeof(r->key));
      escape(r->value, sizeof(r->value));    
    }
  }

  fp = fopen(SERENITY_FILE_UPDATE, "w");
  if (!fp)
  {
    echo(DEBUG_INFO, "updateDbPHP: couldn\'t write to file\n");
    return 0;
  }

  fprintf(fp, "<?php \n" \
              "\n" \
              "/* %s v%s - %s %s (%s)\n" \
              " * " SERENITY_FILE_UPDATE " - automagically generated on %s.\n" \
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
  fprintf(fp, "/* all rules */\n" \
              "\n" \
              "$result = mysql_query(\'TRUNCATE TABLE serenity_rule\');\n" \
              "if (!$result) {\n" \
              "  echo \'[php] invalid query (all rules): \' . mysql_error() . \"\\n\";\n" \
              "  exit(4);\n" \
              "}\n" \
              "\n");
  fprintf(fp, "/* init */\n" \
              "$timestamp = \"%s\";\n" \
              "\n",
              info->datetime);

  for (s = servers; s; s = s->next)
  {
    /* server */
    fprintf(fp, "$server_id = 0;\n" \
                "$server_gotreply = %d;\n" \
                "$server_type = %d;\n" \
                "$server_ip = \"%s\";\n" \
                "$server_gameport = %d;\n" \
                "$server_queryport = %d;\n" \
                "$server_ping = %d;\n" \
                "$server_hostname = mysql_real_escape_string(\'%s\', $db);\n" \
                "$server_gametype = mysql_real_escape_string(\'%s\', $db);\n" \
                "$server_gameid = mysql_real_escape_string(\'%s\', $db);\n" \
                "$server_mapname = mysql_real_escape_string(\'%s\', $db);\n" \
                "$server_numplayers = %d;\n" \
                "$server_maxplayers = %d;\n" \
                "$server_version = mysql_real_escape_string(\'%s\', $db);\n" \
                "$server_bytesrecieved = %d;\n" \
                "$server_bytessent = %d;\n" \
                "$server_trashed = %d;\n" \
                "$server_updatesuccess = 0;\n" \
                "$server_updatefail = 0;\n" \
                "$server_updateconsecutive = 0;\n",
                s->got_reply,
                s->type->game_type,
                s->ip,
                s->game_port,
                s->query_port,
                s->ping,
                s->host_name,
                s->game_type,
                s->game_id,
                s->map_name,
                s->num_players,
                s->max_players,
                s->version,
                s->bytes_recieved,
                s->bytes_sent,
                s->trashed,
                s->update_success,
                s->update_fail,
                s->update_consecutive);
    fprintf(fp, "$query = \"SELECT * FROM serenity_server WHERE \" .\n" \
                "         \"server_type = $server_type AND \" .\n" \
                "         \"server_ip = \'$server_ip\' AND \" .\n" \
                "         \"server_gameport = $server_gameport\";\n" \
                "\n");
    fprintf(fp, "$result = mysql_query($query, $db);\n" \
                "if (!$result) {\n" \
                "  echo \"[php] $query\\n\";\n" \
                "  echo \'[php] invalid query (server select): \' . mysql_error() . \"\\n\";\n" \
                "  exit(5);\n" \
                "}\n" \
                "if (mysql_num_rows($result) > 0) {\n" \
                "  $server_id = mysql_result($result, 0, %d);\n" \
                "  $server_updatesuccess = mysql_result($result, 0, %d);\n" \
                "  $server_updatefail = mysql_result($result, 0, %d);\n" \
                "  $server_updateconsecutive = mysql_result($result, 0, %d);\n" \
                "  $server_trashed += mysql_result($result, 0, %d);\n" \
                "  $server_bytesrecieved += mysql_result($result, 0, %d);\n" \
                "  $server_bytessent += mysql_result($result, 0, %d);\n" \
                "\n",
                MYSQL_TABLE_SERVER_ID,
                MYSQL_TABLE_SERVER_UPDATE_SUCCESS,
                MYSQL_TABLE_SERVER_UPDATE_FAIL,
                MYSQL_TABLE_SERVER_CONSECUTIVE_FAILS,
                MYSQL_TABLE_SERVER_TRASHED,
                MYSQL_TABLE_SERVER_BYTESRECIEVED,
                MYSQL_TABLE_SERVER_BYTESSENT);
    fprintf(fp, "  if ($server_gotreply) {\n" \
                "    $server_updatesuccess++;\n" \
                "    $query = \"UPDATE serenity_server SET \" .\n" \
                "             \"server_ping = $server_ping, \" .\n" \
                "             \"server_hostname = \'$server_hostname\', \" .\n" \
                "             \"server_gametype = \'$server_gametype\', \" .\n" \
                "             \"server_gameid = \'$server_gameid\', \" .\n" \
                "             \"server_mapname = \'$server_mapname\', \" .\n" \
                "             \"server_numplayers = $server_numplayers, \" .\n" \
                "             \"server_maxplayers = $server_maxplayers, \" .\n" \
                "             \"server_version = \'$server_version\', \" .\n" \
                "             \"server_bytesrecieved = $server_bytesrecieved, \" .\n" \
                "             \"server_bytessent = $server_bytessent, \" .\n" \
                "             \"server_trashed = $server_trashed, \" .\n" \
                "             \"server_updatesuccess = $server_updatesuccess, \" .\n" \
                "             \"server_consecutivefails = 0, \" .\n" \
                "             \"server_lastupdate = \'$timestamp\' \" .\n" \
                "             \"WHERE server_type = $server_type AND \" .\n" \
                "             \"server_ip = \'$server_ip\' AND \" .\n" \
                "             \"server_gameport = $server_gameport\";\n" \
                "  }\n");
    fprintf(fp, "  else {\n" \
                "    $server_updatefail++;\n" \
                "    $server_updateconsecutive++;\n" \
                "    $query = \"UPDATE serenity_server SET \" .\n" \
                "             \"server_updatefail = $server_updatefail, \" .\n" \
                "             \"server_consecutivefails = $server_updateconsecutive \" .\n" \
                "             \"WHERE server_type = $server_type AND \" .\n" \
                "             \"server_ip = \'$server_ip\' AND \" .\n" \
                "             \"server_gameport = $server_gameport\";\n" \
                "  }\n",
                s->type->game_type,
                s->ip,
                s->game_port);
    fprintf(fp, "}\n" \
                "else {\n" \
                "  if ($server_gotreply) {\n" \
                "    $server_updatesuccess = 1;\n" \
                "  }\n" \
                "  else {\n" \
                "    $server_updatefail = 1;\n" \
                "    $server_updateconsecutive = 1;\n" \
                "  }\n" \
                "  $query = \"INSERT INTO serenity_server VALUES \" .\n" \
                "           \"(0, \" .\n" \
                "           \"$server_type, \" .\n" \
                "           \"\'$server_ip\', \" .\n" \
                "           \"$server_gameport, \" .\n" \
                "           \"$server_queryport, \" .\n" \
                "           \"$server_ping, \" .\n" \
                "           \"\'$server_hostname\', \" .\n" \
                "           \"\'$server_gametype\', \" .\n" \
                "           \"\'$server_gameid\', \" .\n" \
                "           \"\'$server_mapname\', \" .\n" \
                "           \"$server_numplayers, \" .\n" \
                "           \"$server_maxplayers, \" .\n" \
                "           \"\'$server_version\', \" .\n" \
                "           \"$server_bytesrecieved, \" .\n" \
                "           \"$server_bytessent, \" .\n" \
                "           \"$server_trashed, \" .\n" \
                "           \"$server_updatesuccess, \" .\n" \
                "           \"$server_updatefail, \" .\n" \
                "           \"$server_updateconsecutive, \" .\n" \
                "           \"\'$timestamp\')\";\n" \
                "}\n" \
                "\n");
    fprintf(fp, "$result = mysql_query($query, $db);\n"
                "if (!$result) {\n" \
                "  echo \"[php] $query\\n\";\n" \
                "  echo \'[php] invalid query (server update/insert): \' . mysql_error() . \"\\n\";\n" \
                "  exit(6);\n" \
                "}\n" \
                "\n");

    if (!s->got_reply)
      continue;

    fprintf(fp, "if (!$server_id) {\n" \
                "  $server_id = mysql_insert_id($db);\n" \
                "}\n" \
                "\n");

    /* rules */
    fprintf(fp, "/* rules (%s:%d) */\n" \
                "\n",
                s->ip,
                s->game_port);
    for (r = s->rules; r; r = r->next)
    {
      fprintf(fp, "$rule_key = mysql_real_escape_string(\'%s\', $db);\n" \
                  "$rule_value = mysql_real_escape_string(\'%s\', $db);\n",
                  r->key,
                  r->value);
      fprintf(fp, "$query = \"INSERT INTO serenity_rule VALUES \" .\n" \
                  "         \"(0, \" .\n" \
                  "         \"$server_id, \" .\n" \
                  "         \"\'$rule_key\', \" .\n" \
                  "         \"\'$rule_value\')\";\n" \
                  "$result = mysql_query($query, $db);\n" \
                  "if (!$result) {\n" \
                  "  echo \"[php] $query\\n\";\n" \
                  "  echo \'[php] invalid query (rule insert): \' . mysql_error() . \"\\n\";\n" \
                  "  exit(7);\n" \
                  "}\n" \
                  "\n");
    }

    /* players */
    fprintf(fp, "/*%s players (%s:%d) */\n" \
                "\n",
                (s->num_players) ? "" : " no",
                s->ip,
                s->game_port);
    for (p = s->players; p; p = p->next)
    {
      if (!s->type->check_player(p))
        continue;

      /* keyhash */
      fprintf(fp, "$keyhash_id = 0;\n");
      if (p->keyhash[0]) //only add real keyhashes
      {
        fprintf(fp, "$keyhash_hash = mysql_real_escape_string(\'%s\', $db);\n" \
                    "\n" \
                    "$query = \"SELECT * FROM serenity_keyhash WHERE \" .\n" \
                    "         \"keyhash_type = $server_type AND keyhash_hash = \'$keyhash_hash\'\";\n" \
                    "$result = mysql_query($query, $db);\n" \
                    "if (!$result) {\n" \
                    "  echo \"[php] $query\\n\";\n" \
                    "  echo \'[php] invalid query (keyhash select): \' . mysql_error() . \"\\n\";\n" \
                    "  exit(8);\n" \
                    "}\n" \
                    "if (mysql_num_rows($result) > 0) {\n" \
                    "  $keyhash_id = mysql_result($result, 0, %d);\n" \
                    "}\n" \
                    "else {\n" \
                    "  $query = \"INSERT INTO serenity_keyhash VALUES (0, $server_type, \'$keyhash_hash\')\";\n" \
                    "  $result = mysql_query($query, $db);\n" \
                    "  if (!$result) {\n" \
                    "    echo \"[php] $query\";\n" \
                    "    echo \'[php] invalid query (keyhash insert): \' . mysql_error() . \"\\n\";\n" \
                    "    exit(9);\n" \
                    "  }\n" \
                    "  $keyhash_id = mysql_insert_id($db);\n" \
                    "}\n" \
                    "\n",
                    p->keyhash,
                    MYSQL_TABLE_KEYHASH_ID);
      }
      
      /* player */
      fprintf(fp, "$player_name = mysql_real_escape_string(\'%s\', $db);\n" \
                  "$player_ip = mysql_real_escape_string(\'%s\', $db);\n" \
                  "$player_userid = %d;\n" \
                  "$player_score = %d;\n" \
                  "$player_frags = %d;\n" \
                  "$player_deaths = %d;\n" \
                  "$player_team = mysql_real_escape_string(\'%s\', $db);\n" \
                  "$player_ping = %d;\n" \
                  "$player_connect = %d;\n",
                  p->name,
                  p->ip,
                  p->userid,
                  p->score,
                  p->frags,
                  p->deaths,
                  p->team,
                  p->ping,
                  p->connect);
      fprintf(fp, "$query = \"SELECT * FROM serenity_player WHERE \" .\n" \
                  "         \"player_type = $server_type AND \" .\n" \
                  "         \"player_name = \'$player_name\'\";\n" \
                  "$result = mysql_query($query, $db);\n" \
                  "if (!$result) {\n" \
                  "  echo \"[php] $query\\n\";\n" \
                  "  echo \'[php] invalid query (player select): \' . mysql_error() . \"\\n\";\n" \
                  "  exit(10);\n" \
                  "}\n");
      fprintf(fp, "if (mysql_num_rows($result) > 0) {\n" \
                  "  $query = \"UPDATE serenity_player SET \" .\n" \
                  "           \"player_server = $server_id, \" .\n" \
                  "           \"player_keyhash = $keyhash_id, \" .\n" \
                  "           \"player_ip = \'$player_ip\', \" .\n" \
                  "           \"player_userid = $player_userid, \" .\n" \
                  "           \"player_score = $player_score, \" .\n" \
                  "           \"player_frags = $player_frags, \" .\n" \
                  "           \"player_deaths = $player_deaths, \" .\n" \
                  "           \"player_team = \'$player_team\', \" .\n" \
                  "           \"player_ping = $player_ping, \" .\n" \
                  "           \"player_connect = $player_connect, \" .\n" \
                  "           \"player_lastseen = \'$timestamp\'\" .\n" \
                  "           \"WHERE player_type = $server_type AND player_name = \'$player_name\'\";\n" \
                  "}\n");
      fprintf(fp, "else {\n" \
                  "  $query = \"INSERT INTO serenity_player VALUES \" .\n" \
                  "           \"(0, \" .\n" \
                  "           \"$server_id, \" .\n" \
                  "           \"$keyhash_id, \" .\n" \
                  "           \"$server_type, \" .\n" \
                  "           \"\'$player_ip\', \" .\n" \
                  "           \"\'$player_name\', \" .\n" \
                  "           \"$player_userid, \" .\n" \
                  "           \"$player_score, \" .\n" \
                  "           \"$player_frags, \" .\n" \
                  "           \"$player_deaths, \" .\n" \
                  "           \"\'player_team\', \" .\n" \
                  "           \"$player_ping, \" .\n" \
                  "           \"$player_connect, \" .\n" \
                  "           \"\'$timestamp\')\";\n" \
                  "}\n");
      fprintf(fp, "$result = mysql_query($query, $db);\n" \
                  "if (!$result) {\n" \
                  "  echo \"[php] $query\\n\";\n" \
                  "  echo \'[php] invalid query (player update/insert): \' . mysql_error() . \"\\n\";\n" \
                  "  exit(11);\n" \
                  "}\n" \
                  "\n");
    }
  }

  /* info */
  fprintf(fp, "/* info */\n" \
              "\n");
  fprintf(fp, "$version = \"%s\";\n" \
              "$bad_reply = %d;\n" \
              "$no_reply = %d;\n" \
              "$runtime_low = %d;\n" \
              "$runtime_high = %d;\n" \
              "$runtime_last = %d;\n" \
              "$realtime = %d;\n" \
              "$datetime = \"%s\";\n" \
              "$query = \"SELECT * FROM serenity_info\";\n" \
              "$result = mysql_query($query, $db);\n" \
              "if (!$result) {\n" \
              "  echo \"[php] $query\\n\";\n" \
              "  echo \'[php] invalid query (info select): \' . mysql_error() . \"\\n\";\n" \
              "  exit(12);\n" \
              "}\n",
              VERSION,
              info->bad_reply,
              info->no_reply,
              info->realtime,
              info->realtime,
              info->realtime,
              info->realtime,
              info->datetime);
  fprintf(fp, "if (mysql_num_rows($result) > 0) {\n" \
              "  $bad_reply += mysql_result($result, 0, %d);\n" \
              "  $no_reply += mysql_result($result, 0, %d);\n" \
              "  $runtime_low = mysql_result($result, 0, %d);\n" \
              "  $runtime_high = mysql_result($result, 0, %d);\n",
              //"  $runtime_last = mysql_result($result, 0, %d);\n",
              MYSQL_TABLE_INFO_BAD_REPLY,
              MYSQL_TABLE_INFO_NO_REPLY,
              MYSQL_TABLE_INFO_LOW_RUN_TIME,
              MYSQL_TABLE_INFO_HIGH_RUN_TIME);
              //MYSQL_TABLE_INFO_LAST_RUN_TIME);
  fprintf(fp, "  if ($realtime < $runtime_low) {\n" \
              "    $runtime_low = $realtime;\n" \
              "  }\n" \
              "  if ($realtime > $runtime_high) {\n" \
              "    $runtime_high = $realtime;\n" \
              "  }\n" \
              "  $query = \"UPDATE serenity_info SET \" .\n" \
              "           \"info_version = \'$version\', \" .\n" \
              "           \"info_noreply = $no_reply, \" .\n" \
              "           \"info_badreply = $bad_reply, \" .\n" \
              "           \"info_lowruntime = $runtime_low, \" .\n" \
              "           \"info_highruntime = $runtime_high, \" .\n" \
              "           \"info_lastruntime = $runtime_last, \" .\n" \
              "           \"info_lastrun = \'$datetime\'\";\n" \
              "}\n");
  fprintf(fp, "else {\n" \
              "  $query = \"INSERT INTO serenity_info VALUES \" .\n" \
              "           \"(0, \" .\n" \
              "           \"\'$version\', \" .\n" \
              "           \"$no_reply, \" .\n" \
              "           \"$bad_reply, \" .\n" \
              "           \"$runtime_low, \" .\n" \
              "           \"$runtime_high, \" .\n" \
              "           \"$runtime_last, \" .\n" \
              "           \"\'$datetime\', \" .\n" \
              "           \"\'$datetime\', \" .\n" \
              "           \"\'" MYSQL_INFO_DEFAULT_PASS ")\";\n" \
              "}\n" \
              "$result = mysql_query($query, $db);\n" \
              "if (!$result) {\n" \
              "  echo \"[php] $query\\n\";\n" \
              "  echo \'[php] invalid query (info update/insert): \' . mysql_error() . \"\\n\";\n" \
              "  exit(13);\n" \
              "}\n" \
              "\n" \
              "?>");
  fclose(fp);
  _strnprintf(sys, sizeof(sys), "%s %s " SERENITY_FILE_UPDATE, info->php_filepath, info->php_parameters);
  /*echo(DEBUG_FLOOD, "updateDbPHP: system: %s\n", sys); 
  i = system(sys);
  echo(DEBUG_FLOOD, "updateDbPHP: system returned: %d\n", i);*/

  return run(sys, "updateDbPHP"); //!i
}

/*
 * newServer - allocate memory and init a server struct
 */
int newServer(server_t *server)
{
  server = (server_t *)malloc(sizeof(server_t));
  if (!server)
    return 0;

  server->ip[0] = '\0';
  server->game_port = 0;
  server->query_port = 0;
  strlcpy(server->host_name, "unnamed", sizeof(server->host_name));
  server->map_name[0] = '\0';
  server->game_type[0] = '\0';
  server->game_id[0] = '\0';
  strlcpy(server->version, "none", sizeof(server->version));
  server->num_players = 0;
  server->max_players = 0;
  server->rcon[0] = '\0';
  server->packet[0] = '\0';
  server->packet_length = 0;
  server->packet_recieved = 0;
  server->bytes_recieved = 0;
  server->bytes_sent = 0;
  server->trashed = 0;
  server->update_success = 0;
  server->update_fail = 0;
  server->update_consecutive = 0;
  server->ping = 0;
  server->got_reply = 0;
  server->id = 0;
  server->threadid = NULL;
  server->type = NULL;
  server->rules = NULL;
  server->players = NULL;

  server->next = servers;
  servers = server;

  return 1;
}

/*
 * newRule - allocate memory and init a rule struct
 */
int newRule(server_t *server)
{
  rule_t *rule;

  rule = (rule_t *)malloc(sizeof(rule_t));
  if (!rule)
    return 0;

  rule->key[0] = '\0';
  rule->value[0] = '\0';

  rule->next = server->rules;
  server->rules = rule;

  return 1;
}

/*
 * newPlayer - allocate memory and init a player struct
 */
int newPlayer(server_t *server)
{
  player_t *player;

  player = (player_t *)malloc(sizeof(player_t));
  if (!player)
    return 0;

  strlcpy(player->name, "unnamed", sizeof(player->name));
  player->keyhash[0] = '\0';
  player->score = 0;
  player->frags = 0;
  player->deaths = 0;
  player->connect = 0;
  player->ping = -1;
  player->team[0] = '\0';
  player->ip[0] = '\0';
  player->end = 0;

  player->next = server->players;
  server->players = player;

  return 1;
}

//yuck, fix this mess -ok --fortuntely this isn't used any more
#if 0
int findServerType(int by, int thing)
{
  int i;

  i = 0;
  while (i < info->num_types)
  {
    switch (by)
    {
      case 1: if (thing == server_type[i].game_type)
                return i;
              break;
      case 2: if (thing >= server_type[i].default_game_port && thing < server_type[i].default_game_port + 1000)
                return i;
              break;
      default: return -1;
    }
    i++;
  }
  return -1;
}
#endif

#ifdef USE_MYSQL
int readServersMySql(void)
{
  MYSQL_RES *res;
  MYSQL_ROW	row;
  char ip[MAX_IP+1];
  int i, gport, qport, type, dup;
  server_t *s;

  if (mysql_ping(info->db))
  {
    echo(DEBUG_FATAL, "readServers: couldn\'t reconnect to database: %s\n", mysql_error(info->db));
    return 0;
  }

  if (mysql_query(info->db, "SELECT * FROM serenity_serverlist"))
  {
    echo(DEBUG_FATAL, "readServers: select error: %s\n", mysql_error(info->db));
    return 0;
  }
  res = mysql_store_result(info->db);
  i = (int)mysql_num_rows(res);
  if (!i)
  {
    echo(DEBUG_FATAL, "readServers: no servers\n");
    return 0;
  }

  i = 0;
  while ((row = mysql_fetch_row(res)))
  {
    strlcpy(ip, row[MYSQL_TABLE_SERVERLIST_IP], sizeof(ip));
    gport = atoi(row[MYSQL_TABLE_SERVERLIST_GAME_PORT]);
    qport = atoi(row[MYSQL_TABLE_SERVERLIST_QUERY_PORT]);
    type = atoi(row[MYSQL_TABLE_SERVERLIST_TYPE]);

    if (!validip(ip) || !qport || !gport || !type)
    {
      echo(DEBUG_FATAL, "readServers: bad server: %s:%d(%d) type:%d\n", ip, gport, qport, type);
      continue;
    }

    dup = 0;
    s = servers;
    while (s)
    {
      if (!strcmp(s->ip, ip) && s->game_port == gport && s->query_port == qport && s->type->game_type == type)
      {
        dup = 1;
        break;
      }
      s = s->next;
    }
    if (dup)
    {
      echo(DEBUG_INFO, "readservers: dup server %s:%d(%d) type(%d), skipped\n", ip, gport, qport, type);
      continue;
    }
    type = findServerType(1, type);
    if (type == -1)
    {
      echo(DEBUG_INFO, "readservers: bad type for server %s:%d(%d), skipped\n", ip, gport, qport);
      continue;
    }     
    if (!newServer(servers))
    {
      echo(DEBUG_FATAL, "readservers: malloc failure for %s:%d(%d)\n", ip, gport, qport);
      continue;
    }
    strlcpy(servers->ip, ip, sizeof(servers->ip));
    servers->game_port = gport;
    servers->query_port = qport;
    servers->type = &server_type[type];
    i++;

    echo(DEBUG_FLOOD, "readservers: added %s:%d(%d) type:%d\n", ip, gport, qport, servers->type->game_type);
  }

  info->total_servers = i;

  return i;
}
#endif

/*
 * readServersPHP - reads server list from database so we can use it
 */
int readServersPHP(void)
{
  FILE *fp;
  char sys[MAX_FILE_PATH+MAX_PHP_PARAMETERS+1];
  char ip[MAX_IP+1];
  char rcon[MAX_RCON+1];
  char buff[MAX_MYSQL_QUERY+1];
  char c = ' ';
  char *p;
  int t, i, j, l, gport, qport, type, dup;
  server_t *s;

  fp = fopen(SERENITY_FILE_SERVERS, "w");
  if (!fp)
  {
    echo(DEBUG_INFO, "readServersPHP: couldn\'t write to file\n");
    return 0;
  }
  fprintf(fp, "<?php \n" \
              "\n" \
              "/* %s v%s - %s %s (%s)\n" \
              " * " SERENITY_FILE_SERVERS " - automagically generated on %s.\n" \
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
  fprintf(fp, "$result = mysql_query(\'SELECT * FROM serenity_serverlist\', $db);\n" \
              "if (!$result) {\n" \
              "  echo \'[php] invalid query: \' . mysql_error() . \"\\n\";\n" \
              "  exit(4);\n" \
              "}\n" \
              "$fp = fopen(\'" SERENITY_FILE_SERVERLIST "\', \'w\');\n" \
              "for ($i = 0; $i < mysql_num_rows($result); $i++) {\n" \
              "  for ($j = 1; $j < mysql_num_fields($result); $j++) {\n" \
              "    fwrite($fp, mysql_result($result, $i, $j) . \"\\\\\");\n" \
              "  }\n" \
              "  fwrite($fp, \"\\n\");\n" \
              "}\n" \
              "fclose($fp);\n" \
              "mysql_close($db);\n" \
              "?>");
  fclose(fp);

  _strnprintf(sys, sizeof(sys), "%s %s " SERENITY_FILE_SERVERS, info->php_filepath, info->php_parameters);
  if (!run(sys, "readServersPHP"))
    return 0;
  /*echo(DEBUG_FLOOD, "readServersPHP: system: %s\n", sys); 
  i = system(sys);
  echo(DEBUG_FLOOD, "readServersPHP: system returned: %d\n", i);
  if (i > 0)
  {
    if (i == 1)
      echo(DEBUG_FATAL, "readServersPHP: couldn\'t run script\n");
    else
      echo(DEBUG_FATAL, "readServersPHP: script failed\n");
    return 0;
  }*/

  fp = fopen(SERENITY_FILE_SERVERLIST, "r");
  if (!fp)
  {
    echo(DEBUG_INFO, "readServersPHP: couldn\'t read file\n");
    return 0;
  }

  /* 203.96.92.67\15567\23000\rcon\type\ */
  t = 0;
  while (c != EOF)
  {
    for (i = 0; i < 5; i++)
    {
      j = 0;
      l = 0;
      switch (i)
      {
        case 0: p = ip;
                l = MAX_IP; break;
        case 1: p = buff;
                l = MAX_PORT; break;
        case 2: p = buff;
                l = MAX_PORT; break;
        case 3: p = rcon;
                l = MAX_RCON; break;
        case 4: p = buff;
                l = 10;
        default: break;
      }

      while (1)
      {
        c = fgetc(fp);
        if (c == '\\' || c == '\n' || c == EOF)
          break;
        if (j < l)
        {
          *p++ = c;
          j++;
        }
      }
      *p = 0;
      switch (i)
      {
        case 1: gport = atoi(buff); break;
        case 2: qport = atoi(buff); break;
        case 4: type = atoi(buff); break;
        default: break;
      }
    }
    c = fgetc(fp);
    if (c == EOF)
      break;

    if (!validip(ip) || !qport || !gport || !type)
    {
      echo(DEBUG_FATAL, "readServersPHP: bad server: %s:%d(%d) type:%d\n", ip, gport, qport, type);
      continue;
    }

    dup = 0;
    s = servers;
    while (s)
    {
      if (!strcmp(s->ip, ip) && s->game_port == gport && s->query_port == qport && s->type->game_type == type)
      {
        dup = 1;
        break;
      }
      s = s->next;
    }
    if (dup)
    {
      echo(DEBUG_INFO, "readServersPHP: dup server %s:%d(%d) type(%d), skipped\n", ip, gport, qport, type);
      continue;
    }

    for (j = 0, i = 0; i < info->num_types; i++)
    {
      if (type == server_type[i].game_type)
      {
        type = i;
        j = 1;
        break;
      }
    }

    if (!j)
    {
      echo(DEBUG_INFO, "readServersPHP: bad type for server %s:%d(%d), skipped\n", ip, gport, qport);
      continue;
    }     
    if (!newServer(servers))
    {
      echo(DEBUG_FATAL, "readServersPHP: malloc failure for %s:%d(%d)\n", ip, gport, qport);
      continue;
    }
    strlcpy(servers->ip, ip, sizeof(servers->ip));
    servers->game_port = gport;
    servers->query_port = qport;
    servers->type = &server_type[type];
    t++;

    echo(DEBUG_FLOOD, "readServersPHP: added %s:%d(%d) type:%d\n", ip, gport, qport, servers->type->game_type);
  }
  fclose(fp);
  info->total_servers = t;

  return t;
}

#ifdef USE_MYSQL
void createMySqlDb(void)
{
  char buff[MAX_MYSQL_QUERY+1];
  int i;

  if (mysql_ping(info->db))
  {
    echo(DEBUG_FATAL, "initMysql: couldn\'t reconnect to database: %s\n", mysql_error(info->db));
    return;
  }

  echo(DEBUG_INFO, "initMysql: connected to mysql database (%s:%d)\n", info->mysql_address, !info->mysql_port ? MYSQL_SERVER_PORT : info->mysql_port);

  //create database
  _strnprintf(buff, sizeof(buff), "CREATE DATABASE IF NOT EXISTS %s", info->mysql_database);
  if (mysql_query(info->db, buff))
  {
    echo(DEBUG_FATAL, "initMysql: error creating database: %s\n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created database\n");

  //switch to database
  if (mysql_select_db(info->db, info->mysql_database))
  {
    echo(DEBUG_FATAL, "initMysql: failed to switch database \"%s\": %s\n", info->mysql_database, mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: switched database\n");
 
  //create table server type
  if (mysql_query(info->db, MYSQL_SERVERTYPE_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating servertype table: %s \n", mysql_error(info->db));
    return;
  }

  for (i = 0; i < info->num_types; i++)
  {
    _strnprintf(buff, sizeof(buff), "INSERT INTO serenity_servertype VALUES (%d, \'%s\', \'%s\')", server_type[i].game_type, server_type[i].game_desc, server_type[i].game_short_desc);
    if (mysql_query(info->db, buff))
      echo(DEBUG_FATAL, "initMysql: type %d insert error: %s\n", server_type[i].game_type, mysql_error(info->db));
  }
  echo(DEBUG_INFO, "initMysql: created servertype table\n");

  //create table server list
  if (mysql_query(info->db, MYSQL_SERVERLIST_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating serverlist table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created serverlist table\n");

  //create table info
  if (mysql_query(info->db, MYSQL_INFO_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating info table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created info table\n");

  //create table server
  if (mysql_query(info->db, MYSQL_SERVER_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating server table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created server table\n");

  //create table server
  if (mysql_query(info->db, MYSQL_RULE_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating rule table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created rule table\n");

  //create table keyhash
  if (mysql_query(info->db, MYSQL_KEYHASH_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating keyhash table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created keyhash table\n");

  //create table player
  if (mysql_query(info->db, MYSQL_PLAYER_TABLE))
  {
    echo(DEBUG_FATAL, "initMysql: error creating player table: %s \n", mysql_error(info->db));
    return;
  }
  echo(DEBUG_INFO, "initMysql: created player table\n");
}
#endif

/*
 * createDbPHP - generates script for creating database tables (-mc)
 */
void createDbPHP(void)
{
  char sys[MAX_FILE_PATH+MAX_PHP_PARAMETERS+1];
  FILE *fp;
  int i;

  echo(DEBUG_INFO, "createDbPHP: mysql database (%s:%d)\n", info->mysql_address, !info->mysql_port ? MYSQL_SERVER_PORT : info->mysql_port);

  fp = fopen(SERENITY_FILE_CREATEDB, "w");
  if (!fp)
  {
    echo(DEBUG_INFO, "createDbPHP: couldn\'t write to file\n");
    return;
  }

  fprintf(fp, "<?php \n" \
              "\n" \
              "/* %s v%s - %s %s (%s)\n" \
              " * " SERENITY_FILE_CREATEDB " - automagically generated on %s.\n" \
              " */\n" \
              "\n",
              HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, info->datetime);
  fprintf(fp, "$db = mysql_connect(\'%s:%d\', \'%s\', \'%s\');\n" \
              "if (!$db) {\n" \
              "  echo \'[php] could not connect: \' . mysql_error() . \"\\n\";\n" \
              "  exit(2);\n" \
              "}\n" \
              "\n" \
              "mysql_query(\"CREATE DATABASE IF NOT EXISTS %s\");\n" \
              "\n" \
              "if (!mysql_select_db(\'%s\', $db)) {\n" \
              "  echo \'[php] could not select: \' . mysql_error() . \"\\n\";\n" \
              "  exit(3);\n" \
              "}\n" \
              "\n",
              info->mysql_address,
              info->mysql_port,
              info->mysql_user,
              info->mysql_pass,
              info->mysql_database,
              info->mysql_database);
  
  //create table server type
  fprintf(fp, "echo \"[php] createDbPHP: create table server type\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_SERVERTYPE_TABLE);
  for (i = 0; i < info->num_types; i++)
  {
    fprintf(fp, "mysql_query(\"INSERT INTO serenity_servertype VALUES (%d, \'%s\', \'%s\')\");\n",
                server_type[i].game_type,
                server_type[i].game_desc,
                server_type[i].game_short_desc);
  }

  //create table server list
  fprintf(fp, "echo \"[php] createDbPHP: create table server list\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_SERVERLIST_TABLE);

  //create table info
  fprintf(fp, "echo \"[php] createDbPHP: create table info\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_INFO_TABLE);

  fprintf(fp, "mysql_query(\"INSERT INTO serenity_info VALUES (1, \'%s\', 0, 0, 0, 0, 0, \'%s\', \'%s\', \'%s\', \'%s\', 2047, 255, 1023, 63)\");\n",
              VERSION,
              info->datetime,
              info->datetime,
              MYSQL_INFO_DEFAULT_USER,
              MYSQL_INFO_DEFAULT_PASS);

  //create table server
  fprintf(fp, "echo \"[php] createDbPHP: create table server\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_SERVER_TABLE);

  //create table rule
  fprintf(fp, "echo \"[php] createDbPHP: create table rule\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_RULE_TABLE);

  //create table keyhash
  fprintf(fp, "echo \"[php] createDbPHP: create table keyhash\\n\";\n" \
              "mysql_query(\'%s\');\n",
              MYSQL_KEYHASH_TABLE);

  //create table player
  fprintf(fp, "echo \"[php] createDbPHP: create table player\\n\";\n" \
              "mysql_query(\'%s\');\n" \
              "\n",
              MYSQL_PLAYER_TABLE);

  fprintf(fp, "mysql_close($db);\n" \
              "?>");

  fclose(fp);

  _strnprintf(sys, sizeof(sys), "%s %s " SERENITY_FILE_CREATEDB, info->php_filepath, info->php_parameters);
  /*echo(DEBUG_FLOOD, "createDbPHP: system: %s\n", sys); 
  system(sys);
  echo(DEBUG_FLOOD, "createDbPHP: system returned: %d\n", i);*/
  run(sys, "createDbPHP");

#ifdef RRDTOOL
  mkdir(info->rrdtool_images);
#endif
}

/*
 * active - timer thread
 * Self destructing programs are cool.
 */
void *active(_threadvoid *arg) 
{
  while (info->program_active)
  {
    _sleep(1000);
    info->realtime++;

    if (info->kill && info->realtime > (unsigned int)info->max_runtime)
    {
      echo(DEBUG_FATAL, "active: program runtime too long, terminating (over %d seconds)\n", info->max_runtime);
      finish(FINISH_CLEANUP);
      exit(4);
    }
  }
  echo(DEBUG_FLOOD, "active: program finished normally\n");
  _endthread;
}

/*
 * init - start of everything
 */
int init(int argc, char *argv[])
{
#ifdef WIN32
	WSADATA wsaData;
#endif
  int tables = 0;
  int import = 0;
  int help = 0;
  int i;

  printf("%s v%s - %s %s (%s)\n\n", HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL);

  _initmutex(&mutex);
  servers = NULL;
  info = NULL;

  info = (info_t *)malloc(sizeof(info_t));
  if (!info)
  {
    printf("init: info malloc failed\n");
    return 0;
  }
  info->realtime = 0;
  info->program_active = 1;
  info->timeout = DEFAULT_TIMEOUT;
  info->kill = 1;
  info->local_port = DEFAULT_LOCAL_PORT;
  info->debug_level = DEBUG_FATAL;
  info->print_list = 0;
  info->query_retry = DEFAULT_QUERY_RETRY;
  info->query_max = DEFAULT_QUERY_MAX;
  info->total_servers = 0;
  info->num_types = (sizeof(server_type) / sizeof(server_type_t));
  info->query_time = 0;
  info->server_id = 0;
  info->keyhash_id = 0;
  info->no_reply = 0;
  info->bad_reply = 0;
  info->unlink = 1;
  info->max_runtime = MAX_PROGRAM_RUNTIME;
  info->runtime[RUNTIME_LOW] = 0;
  info->runtime[RUNTIME_HIGH] = 0;
  info->runtime[RUNTIME_LAST] = 0;
  info->datetime[0] = '\0';
  strlcpy(info->mysql_database, "serenity", sizeof(info->mysql_database));
  strlcpy(info->mysql_address, "127.0.0.1", sizeof(info->mysql_address));
  strlcpy(info->mysql_user, "bob", sizeof(info->mysql_user));
  strlcpy(info->mysql_pass, "secret", sizeof(info->mysql_pass));
  strlcpy(info->php_filepath, "php", sizeof(info->php_filepath));
  //strlcpy(info->php_parameters, "-q", sizeof(info->php_parameters));
  info->php_parameters[0] = '\0'; //changed this - you can add to parameters but not remove
  info->mysql_port = MYSQL_SERVER_PORT;
#ifdef RRDTOOL
  strlcpy(info->rrdtool_filepath, "rrdtool", sizeof(info->rrdtool_filepath));
  strlcpy(info->rrdtool_images, "./images", sizeof(info->rrdtool_images));
  info->use_rrdtool = 0;
#endif
  strlcpy(info->graph_dir, "./images", sizeof(info->graph_dir));
  info->use_graphs = 0;
  strlcpy(info->debug_file, SERENITY_FILE_DEBUG, sizeof(info->debug_file));
  //strlcpy(info->read_file, "serenity.import", sizeof(info->read_file));
  info->log_file = NULL;
  datetime(info->datetime, sizeof(info->datetime));
#ifdef USE_MYSQL
  info->db = NULL;
#endif

  //command line
  for (i = 1; argc > 1 && i < argc; i++)
  {
    if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "/?"))
    {
      help = 1;
      continue;
    }
    if (argv[i][0] != '-')
      continue;
    if (!strcmp(argv[i], "-mc"))
    {
      tables = 1;
      continue;
    }
    if (!strcmp(argv[i], "-i"))
    {
      import = 1;
      continue;
    }
    if (!argv[i + 1])
    {
      help = 1;
      break;
    }
    switch (argv[i][1])
    {
      case 't': info->timeout = atoi(argv[i + 1]); break;
      case 'f': strlcpy(info->debug_file, argv[i + 1], sizeof(info->debug_file)); break;
      case 'd': info->debug_level = atoi(argv[i + 1]); break;
      case 'p': info->local_port = atoi(argv[i + 1]); break;
      case 'n': info->query_retry = atoi(argv[i + 1]); break;
      case 'q': info->query_max = atoi(argv[i + 1]); break;
      case 'l': info->print_list = atoi(argv[i + 1]); break;
      case 'u': info->unlink = atoi(argv[i + 1]); break;
      case 'x': info->max_runtime = atoi(argv[i + 1]); break;
      //case 'i': strlcpy(info->read_file, argv[i + 1], sizeof(info->read_file)); import = 1; break;
      case 'm':
      case 'h':
        if (!argv[i][2])
          continue;
        switch (argv[i][2])
        {
          case 'i': strlcpy(info->mysql_address, argv[i + 1], sizeof(info->mysql_address)); break;
          case 'p': info->mysql_port = atoi(argv[i + 1]); break;
          case 'u': strlcpy(info->mysql_user, argv[i + 1], sizeof(info->mysql_user)); break;
          case 'w': strlcpy(info->mysql_pass, argv[i + 1], sizeof(info->mysql_pass)); break;
          case 'd': strlcpy(info->mysql_database, argv[i + 1], sizeof(info->mysql_database)); break;
          case 'f': strlcpy(info->php_filepath, argv[i + 1], sizeof(info->php_filepath)); break;
          case 'c': strlcpy(info->php_parameters, argv[i + 1], sizeof(info->php_parameters)); i++; /*<-hackity hack*/ break;
#ifdef RRDTOOL
          case 't': strlcpy(info->rrdtool_filepath, argv[i + 1], sizeof(info->rrdtool_filepath)); break;
          case 'm': strlcpy(info->rrdtool_images, argv[i + 1], sizeof(info->rrdtool_images)); break;
          case 'r': info->use_rrdtool = atoi(argv[i + 1]); break;
#endif
          case 's': strlcpy(info->graph_dir, argv[i + 1], sizeof(info->graph_dir)); break;
          case 'g': info->use_graphs = atoi(argv[i + 1]); break;
          default: break;
        }
        break;
      default: break;
    }
  }

  if (help)
  {
    showHelp();
    return 0;
  }

  if (info->debug_level > DEBUG_NONE)
  {
    info->log_file = fopen(info->debug_file, "at");
    if (!info->log_file)
      printf("init: couldn't write to %s\n", info->debug_file);
  }

  mkdir(DATA_DIRECTORY);
  if (info->use_graphs)
    mkdir(info->graph_dir);
  
#ifdef WIN32
  if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0)
  {
    echo(DEBUG_FATAL, "init: WSAStartup() failed\n");
    return 0;
  }
#endif

#ifdef USE_MYSQL
  info->db = mysql_init(NULL);
  if (!info->db)
  {
    echo(DEBUG_FATAL, "init: mysql_init() failure\n");
    return 0;
  }
  echo(DEBUG_INFO, "init: connecting to mysql database...\n");
  if(!mysql_real_connect(info->db, info->mysql_address, info->mysql_user, info->mysql_pass, NULL, info->mysql_port, NULL, 0))
  {
    echo(DEBUG_FATAL, "init: failed to connect to database: %s\n", mysql_error(info->db));
    //mysql_close(info->db); -- if there's an error and db isn't open a segmentation fault occurs with this
    return 0;
  }
  echo(DEBUG_INFO, "init: connected\n");
#endif

  if (tables)
  {
#ifdef USE_MYSQL
    createDbMySql();
#endif
    createDbPHP();
    return 0;
  }

#ifdef USE_MYSQL
  if (mysql_select_db(info->db, info->mysql_database))
  {
    echo(DEBUG_FATAL, "initMysql: failed to switch database \"%s\": %s\n", info->mysql_database, mysql_error(info->db));
    mysql_close(info->db);
    return 0;
  }
#endif

#ifdef WIN32
  info->active = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)active, NULL, 0, NULL);
  if (!info->active)
  {
#endif
#ifdef UNIX   
  if (pthread_create(&info->active, NULL, active, NULL))
  {
#endif
    echo(DEBUG_FATAL, "init: couldn\'t create active thread\n");
    return 0;
  }

  if (import)
  {
    importServers();
    return 0;
  }

  return 1;
}

/*
 * finish - clean up procedure, terminates threads if needed
 */
void finish(int style)
{
  server_t *server;
  player_t *player;
  rule_t *rule;

  //kill threads
  if (style)
  {
    echo(DEBUG_FLOOD, "finish: terminating all threads\n");
    for (server = servers; server; server = server->next) {
      if (server->threadid)
      {
#ifdef WIN32
        TerminateThread(server->thread, 1); //msdn reckoned it's dangerous to use this, OH WELL
        CloseHandle(server->thread);
#endif
#ifdef UNIX
        pthread_cancel(server->thread);
#endif
      }
    }
  }
  else
  {
    echo(DEBUG_FLOOD, "finish: waiting for active thread\n");
    info->program_active = 0;
#ifdef WIN32
    WaitForSingleObject(info->active, INFINITE);
    CloseHandle(info->active);
#endif
#ifdef UNIX
    pthread_join(info->active, NULL);
#endif

    if (info->total_servers)
      echo(DEBUG_INFO, "finish: query of %d servers took %d seconds (total program run time %d seconds)\n", info->total_servers, info->query_time, info->realtime);
  }

#ifdef UNIX
  pthread_exit(NULL);
#endif

#ifdef USE_MYSQL  
  if (info->db)
  {
    mysql_close(info->db);
    echo(DEBUG_FLOOD, "finish: mysql disconnected\n");
  }
#endif

  if (info->log_file)
  {
    fclose(info->log_file);
  }

  //delete generated files
  if (info->unlink)
  {
    unlink(SERENITY_FILE_IMPORT);
    unlink(SERENITY_FILE_UPDATE);
    unlink(SERENITY_FILE_SERVERS);
    unlink(SERENITY_FILE_CREATEDB);
  }

  //free memory that we've allocated
  while (servers)
  {
    server = servers;
    while (server->players)
    {
      player = server->players;
      server->players = server->players->next;
      free(player);
    }
    while (server->rules)
    {
      rule = server->rules;
      server->rules = server->rules->next;
      free(rule);
    }
    servers = servers->next;
    free(server);
  }

  free(info);
  _destroymutex(&mutex);
#ifdef WIN32
  WSACleanup();
#endif
}

/*
 * echo - printing messages and writing to log
 */
void echo(int level, char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_PACKET+1];
  char type[6];

	//idea from Quake source, cheers!
	va_start(argptr, fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

  if (info->debug_level >= level)
  {
    switch (level)
    {
      case DEBUG_FLOOD: strlcpy(type, "flood", sizeof(type)); break;
      case DEBUG_INFO:  strlcpy(type, "info", sizeof(type)); break;
      case DEBUG_FATAL: strlcpy(type, "fatal", sizeof(type)); break;
      case DEBUG_NONE:   strlcpy(type, "all", sizeof(type)); break;
      default: strlcpy(type, "flood", sizeof(type)); break;
    }
	  printf("[%-5s] %s", type, msg);
    if (info->log_file)
    {
      _lockmutex(&mutex);
      fprintf(info->log_file, "[%-5s] %s", type, msg);
      fflush(info->log_file);
      _unlockmutex(&mutex);
    }
  }
}

/*
 * echo - prints a string out character by character, non-printing replaced with fullstop
 */
void echoc(int level, char *s, int len)
{
  int i;

  if (!info->log_file)
    return;

  if (info->debug_level >= level)
  {
    _lockmutex(&mutex);
    i = 0;
    while (i < len)
    {
      if (*s)
        fputc(*s, info->log_file);
      else
        fputc('.', info->log_file);
      i++;
      s++;
    }
    fputc('\n', info->log_file);
    fflush(info->log_file);
    _unlockmutex(&mutex);
  }
}

/*
 * run - executing external programs
 */
int run(char *sys, char *func)
{
  int r;

  echo(DEBUG_FLOOD, "%s: system: %s\n", func, sys); 
  r = system(sys);
  echo(DEBUG_FLOOD, "%s: system returned: %d\n", func, r);
  if (r > 0)
  {
    if (r == 1)
      echo(DEBUG_FATAL, "%s: couldn\'t run script\n", func);
    else
      echo(DEBUG_FATAL, "%s: script failed\n", func);

    return 0;
  }

  return 1;
}

#ifdef USE_MYSQL
//fix me: add overflow checking - no.
//this used to be a good idea, have custom sql statements, ended up in a big mess
int percent(info_t *in, server_t *se, player_t *pl, rule_t *re, char *s)
{
  char line[MAX_MYSQL_QUERY+1];
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  char *p, *q, *r;

  r = s;
  q = line;
  while (*s)
  {
    while (*s && *s != '%')
      *q++ = *s++; 
    if (!*s)
      break;
    s++;
    p = key;
    while (*s && *s != '%')
      *p++ = *s++;
    *p = 0;

    if (in && !strcmp(key, "info_realtime"))
      sprintf(value, "%u", in->realtime);
    else if (in && !strcmp(key, "info_mysql_database"))
      sprintf(value, "%s", in->mysql_database);
    else if (in && !strcmp(key, "info_mysql_address"))
      sprintf(value, "%s", in->mysql_address);
    else if (in && !strcmp(key, "info_mysql_port"))
      sprintf(value, "%d", info->mysql_port);
    else if (in && !strcmp(key, "info_mysql_user"))
      sprintf(value, "%s", in->mysql_user);
    else if (in && !strcmp(key, "info_mysql_pass"))
      sprintf(value, "%s", in->mysql_pass); 
    else if (in && !strcmp(key, "info_no_reply"))
      sprintf(value, "%u", in->no_reply);
    else if (in && !strcmp(key, "info_bad_reply"))
      sprintf(value, "%u", in->bad_reply);
    else if (in && !strcmp(key, "info_query_time"))
      sprintf(value, "%d", in->query_time);
    else if (in && !strcmp(key, "info_server_id"))
      sprintf(value, "%lu", in->server_id);
    else if (in && !strcmp(key, "info_keyhash_id"))
      sprintf(value, "%lu", in->keyhash_id);
    else if (in && !strcmp(key, "info_datetime"))
      sprintf(value, "%s", in->datetime);
    else if (in && !strcmp(key, "info_version"))
      strcpy(value, VERSION);
    else if (in && !strcmp(key, "info_low_runtime"))
      sprintf(value, "%hu", in->runtime[RUNTIME_LOW]);
    else if (in && !strcmp(key, "info_high_runtime"))
      sprintf(value, "%hu", in->runtime[RUNTIME_HIGH]);
    else if (in && !strcmp(key, "info_last_runtime"))
      sprintf(value, "%hu", in->runtime[RUNTIME_LAST]);

    else if (se && !strcmp(key, "server_ip"))
      sprintf(value, "%s", se->ip);
    else if (se && !strcmp(key, "server_host_name"))
      sprintf(value, "%s", se->host_name);
    else if (se && !strcmp(key, "server_map_name"))
      sprintf(value, "%s", se->map_name);
    else if (se && !strcmp(key, "server_game_type"))
      sprintf(value, "%s", se->game_type);
    else if (se && !strcmp(key, "server_game_id"))
      sprintf(value, "%s", se->game_id);
    else if (se && !strcmp(key, "server_version"))
      sprintf(value, "%s", se->version);
    else if (se && !strcmp(key, "server_type"))
      sprintf(value, "%d", se->type->game_type);
    else if (se && !strcmp(key, "server_game_port"))
      sprintf(value, "%hu", se->game_port);
    else if (se && !strcmp(key, "server_query_port"))
      sprintf(value, "%hu", se->query_port);
    else if (se && !strcmp(key, "server_num_players"))
      sprintf(value, "%u", se->num_players);
    else if (se && !strcmp(key, "server_max_players"))
      sprintf(value, "%u", se->max_players);
    else if (se && !strcmp(key, "server_bytes_recieved"))
      sprintf(value, "%lu", se->bytes_recieved);
    else if (se && !strcmp(key, "server_bytes_sent"))
      sprintf(value, "%lu", se->bytes_sent);
    else if (se && !strcmp(key, "server_trashed"))
      sprintf(value, "%u", se->trashed);
    else if (se && !strcmp(key, "server_update_success"))
      sprintf(value, "%u", se->update_success);
    else if (se && !strcmp(key, "server_update_fail"))
      sprintf(value, "%u", se->update_fail);
    else if (se && !strcmp(key, "server_update_consecutive"))
      sprintf(value, "%u", se->update_consecutive);
    else if (se && !strcmp(key, "server_ping"))
      sprintf(value, "%hu", se->ping);
   
    else if (pl && !strcmp(key, "player_name"))
      sprintf(value, "%s", pl->name);
    else if (pl && !strcmp(key, "player_keyhash"))
      sprintf(value, "%s", pl->keyhash);
    else if (pl && !strcmp(key, "player_ip"))
      sprintf(value, "%s", pl->ip);
    else if (pl && !strcmp(key, "player_userid"))
      sprintf(value, "%d", pl->userid);    
    else if (pl && !strcmp(key, "player_score"))
      sprintf(value, "%d", pl->score);
    else if (pl && !strcmp(key, "player_frags"))
      sprintf(value, "%d", pl->frags);
    else if (pl && !strcmp(key, "player_deaths"))
      sprintf(value, "%d", pl->deaths);
    else if (pl && !strcmp(key, "player_connect"))
      sprintf(value, "%u", pl->connect);
    else if (pl && !strcmp(key, "player_ping"))
      sprintf(value, "%hu", pl->ping);
    else if (pl && !strcmp(key, "player_team"))
      sprintf(value, "%u", pl->team);

    else if (re && !strcmp(key, "rule_key"))
      sprintf(value, "%s", re->key);
    else if (re && !strcmp(key, "rule_value"))
      sprintf(value, "%s", re->value);

    else
    {
      echo(DEBUG_FATAL, "percent: unknown key: %s\n", key);
      return 0;
    }    

    p = value;
    while (*p)
      *q++ = *p++;

    if (*s)
      s++;
  }
  *q = 0;

  p = line;
  while (*p)
    *r++ = *p++;
  *r = 0;

  return 1;
}
#endif

/*
 * showHelp - displays switch syntax
 * Who needs documentation?! The code is obvious!
 */
void showHelp(void)
{
  printf(" -t  [timeout]    timeout time in msecs (%d)\n" 
         " -p  [port]       local port to use (%d)\n"
         " -n  [querynum]   maximum number of ports to scan (%d)\n"
         " -q  [querymax]   maximum number of servers to query at a time 0-all (%d)\n"
         " -f  [debugfile]  output debug messages to this file (%s)\n"
         " -d  [debuglevel] messages to display: 0-none, 1-fatal, 2-info, 3-flood (%d)\n"
         //" -l  [printlevel] display server and player listing 0-off, 1-on (%d)\n"
         " -u  [unlink]     keep or delete temp scripts/files (%d)\n"
         " -x  [runtime]    maximum program run time (%d)\n"
         " -hf [php exe]    full path to php (%s)\n"
         " -hc [php param]  any extra parameters for php\n"
#ifdef RRDTOOL
         " -ht [rrd path]   full path to rrdtool (%s)\n"
         " -hm [rrd images] path where graphs are stored (%s)\n"
         " -hr [use rrd]    run rrdtool or not (%d)\n"
#endif
         " -ms [graph path] full path to write graph images (%s)\n"
         " -mg [use graph]  generate graph images (%d)\n"
         " -mi [mysql ip]   mysql ip address (%s)\n"
         " -mp [mysql port] mysql port (%d)\n"
         " -md [mysql db]   mysql database name (%s)\n"
         " -mu [mysql user] mysql user name (%s)\n"
         " -mw [mysql pass] mysql password (%s)\n"
         " -mc              test mysql and create database and tables\n"
         " -i               import server list (serenity.bf42/serenity.bfv)\n",
         info->timeout,
         info->local_port,
         info->query_retry,
         info->query_max,
         info->debug_file,
         info->debug_level,
         //info->print_list,
         //info->read_file,
         info->unlink,
         info->max_runtime,
         info->php_filepath,
         //info->php_parameters,
#ifdef RRDTOOL
         info->rrdtool_filepath,
         info->rrdtool_images,
         info->use_rrdtool,
#endif
         info->graph_dir,
         info->use_graphs,
         info->mysql_address,
         info->mysql_port,
         info->mysql_database,
         info->mysql_user,
         info->mysql_pass
        );

  info->debug_level = DEBUG_NONE; //hide finish messages if any
}

