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

#include "src.h"

extern serenity_t ne;
extern srcsegment_t *srcsegments;

/* ----------------------------------------------------------------------------

  SOURCE

---------------------------------------------------------------------------- */
int StartSRCSegments(void) {
  srcsegment_t *srcseg;
#ifdef WIN32
  int i;
#endif

  for (srcseg = srcsegments; srcseg; srcseg = srcseg->next) {
#ifdef WIN32
    srcseg->thread = (HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessSRCSegments, (void *)srcseg, 0, &i);
    if (!srcseg->thread) {
#endif
#ifdef UNIX   
    if (pthread_create(&srcseg->thread, NULL, &ProcessSRCSegments, (void *)srcseg)) {
#endif
      return 0;
    }
  }

  return 1;
}

void StopSRCSegments(void) {
  srcsegment_t *srcseg, *n;
  srcserver_t *srv, *m;

  n = srcsegments;
  while (n) {
    srcseg = n;
    n = srcseg->next;

    if (srcseg->thread) {
      _lockmutex(&srcseg->mutex);
      srcseg->active = 0;
      _unlockmutex(&srcseg->mutex);

#ifdef SUPERDEBUG
      printf("Waiting for: %s\n", srcseg->srvlist);
#endif

#ifdef WIN32
      WaitForSingleObject(srcseg->thread, INFINITE);
      CloseHandle(srcseg->thread);
#endif
#ifdef UNIX
      pthread_join(srcseg->thread, NULL);
#endif
    }

    SaveSRCTransfers(srcseg, 1);

    m = srcseg->srcservers;
    while (m) {
      srv = m;
      m = srv->next;

      FreeSRCAllPlayers(&srv->players);

      FreeSRCServer(srv);
    }

    _destroymutex(&srcseg->mutex);
    CloseMySQL(&srcseg->mysql);

    _free_(srcseg);
  }
}

static void *ProcessSRCSegments(void *param) {
  char rv[_SRC_MAX_MESSAGE_SIZE+1];
  srcsegment_t *srcseg = (srcsegment_t *)param;
  srcserver_t *srv;
  int active = 1;

  WriteLog("Flying: %s\n", srcseg->name);

  while (active) {
    _lockmutex(&srcseg->mutex);
    active = srcseg->active;
    _unlockmutex(&srcseg->mutex);

    for (srv = srcseg->srcservers; srv; srv = srv->next) {
#ifdef SUPERDEBUG
      //printf("ProcessSRCSegments(): %s\n", srv->group);
#endif
      if (!ProcessSRCServer(srv, rv, sizeof(rv))) {
        continue;
      }
      RequestSRCPlayers(srv);
    }

    SaveSRCTransfers(srcseg, 0);
    _sleep(100);
  }

  //clean up
  for (srv = srcseg->srcservers; srv; srv = srv->next) {
    if (srv->state != SOCKETSTATE_CONNECTED) {
      continue;
    }

    //?
  }

#ifdef WIN32
  _endthreadex(0);
#endif
#ifdef UNIX
  pthread_exit(NULL);
#endif
  return 0;
}

srcsegment_t *NewSRCSegment(const int segcount) {
  srcsegment_t *srcseg;

#ifdef SUPERDEBUG
  printf("NewSRCSegment()\n");
#endif

  srcseg = (srcsegment_t *)_malloc_(sizeof(srcsegment_t));
  if (!srcseg) {
    return NULL;
  }

  if (!OpenMySQL(&srcseg->mysql)) {
    _free_(srcseg);
    return NULL;
  }

  _initmutex(&srcseg->mutex);
  srcseg->srcservers = NULL;
  srcseg->thread = 0;
  srcseg->active = 1;
  _strnprintf(srcseg->name, sizeof(srcseg->name), "seg%d", segcount);
#ifdef SUPERDEBUG
  srcseg->srvlist[0] = '\0';
#endif

  srcseg->next = srcsegments;
  srcsegments = srcseg;

  return srcseg;
}

static void SaveSRCTransfers(srcsegment_t *srcseg, int now) {
  //TODO !
}

void ActionSRCServerDisconnect(srcserver_t *srv) {
  MYSQL_STMT *qry;

#ifdef SUPERDEBUG
  printf("ActionSRCServerDisconnect()\n");
#endif

  if (mysql_ping(&srv->segment->mysql)) {
    return;
  }
    
  qry = mysql_stmt_init(&srv->segment->mysql);
  if (qry) {
    const char sql_update_failed_server[] = {"UPDATE serenity_server SET "
                                             "server_numplayers = 0, "
                                             "server_updatefail = server_updatefail + 1, "
                                             "server_consecutivefails = server_consecutivefails + 1, "
                                             "server_lastupdate = CURRENT_TIMESTAMP() "
                                     /* 0 */ "WHERE server_id = ?"};

    if (!mysql_stmt_prepare(qry, sql_update_failed_server, CHARSIZE(sql_update_failed_server))) {
      MYSQL_BIND bind[1];
      memset(&bind, 0, sizeof(bind));

      //server_id
      bind[0].buffer_type = MYSQL_TYPE_LONG;
      bind[0].buffer = (void *)&srv->serverid;
      bind[0].is_unsigned = 1;

      if (!mysql_stmt_bind_param(qry, bind)) {
        if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
          WriteLog("Server Fail - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
        }
      }
#ifdef LOG_ERRORS
    } else {
      WriteLog("Server Fail - Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
    }

    mysql_stmt_close(qry);
#ifdef LOG_ERRORS
  } else {
    WriteLog("Server Fail - Query failed: %s\n", mysql_error(&srv->segment->mysql));
#endif
  }

  qry = mysql_stmt_init(&srv->segment->mysql);
  if (qry) {
    const char sql_update_player_currentserver[] = {"UPDATE src_player SET player_currentserver_id = NULL WHERE player_currentserver_id = ?"};

    if (!mysql_stmt_prepare(qry, sql_update_player_currentserver, CHARSIZE(sql_update_player_currentserver))) {
      MYSQL_BIND bind[1];
      memset(&bind, 0, sizeof(bind));

      //server_id
      bind[0].buffer_type = MYSQL_TYPE_LONG;
      bind[0].buffer = (void *)&srv->serverid;
      bind[0].is_unsigned = 1;

      if (!mysql_stmt_bind_param(qry, bind)) {
        if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
          WriteLog("Server Fail - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
        }
      }
#ifdef LOG_ERRORS
    } else {
      WriteLog("Server Fail - Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
    }

    mysql_stmt_close(qry);
#ifdef LOG_ERRORS
  } else {
    WriteLog("Server Fail - Query failed: %s\n", mysql_error(&srv->segment->mysql));
#endif
  }
}

void ActionSRCUpdatePlayer(srcserver_t *srv, srcplayer_t *player) {
  srcplayer_t *pl;
  int change = 0;

  assert(player);
  if (!player->name[0] || !player->hash[0] || !player->address[0]) {
#ifdef SUPERDEBUG
    printf("ActionSRCUpdatePlayer() [%s] Missing details, abort: [%s] [%s] [%s]\n", srv->group, player->name, player->hash, player->address);
#endif
    return;
  }

  pl = FindSRCPlayerByName(srv, player->name);
  if (!pl) {
    pl = NewSRCPlayer(&srv->players);
    if (!pl) {
      return;
    }
#ifdef SUPERDEBUG
    printf("ActionSRCUpdatePlayer() [%s] New player: (%d) %s [%s / %s]\n", srv->group, player->id, player->name, player->address, player->hash);
#endif

    change = 1;
    pl->flags |= _SRC_PLAYER_JOIN;

  } else {
    if ((pl->id != player->id) || strcmp(pl->hash, player->hash) || strcmp(pl->address, player->address)) {
#ifdef SUPERDEBUG
      printf("ActionSRCUpdatePlayer() [%s] PLAYER %s POLYMORPHED! %d=%d %s=%s %s=%s\n", srv->group, player->name, pl->id, player->id, pl->hash, player->hash, pl->address, player->address);
#endif
      change = 1; //what ?! -- could rejoin between updates
    }
  }

  pl->cycle = player->cycle;

  if (change) {
    //database
    pl->playerid = 0;
    pl->addressid = 0;
    pl->keyhashid = 0;
    //player
    pl->id = player->id;
    strlcpy(pl->name, player->name, sizeof(pl->name));
    strlcpy(pl->hash, player->hash, sizeof(pl->hash));
    strlcpy(pl->address, player->address, sizeof(pl->address));

    if (!strcmp(pl->hash, _SRC_KEYHASH_PENDING)) {
#ifdef SUPERDEBUG
      printf("ActionSRCUpdatePlayer() [%s] Player hash is pending\n", srv->group);
#endif
      pl->flags |= _SRC_PLAYER_HASH_PENDING;
    }
  }
}

static void RequestSRCPlayers(srcserver_t *srv) {
  double t = DoubleTime();

	if ((t - srv->lastreqplayers) >= _SRC_TIME_PLAYERS_UPDATE) {
    srv->lastreqplayers = t;

    RequestSRCPlayersFull(srv);
  }
}

void ActionSRCUpdateAllPlayers(srcserver_t *srv) {
#ifdef SUPERDEBUG
  printf("ActionSRCUpdateAllPlayers(): [%s] %d total players\n", srv->group, srv->serverinfo.numplayers);
#endif

  if (srv->players) {
    mysqlquery_t query;
    srcplayer_t *pl, *n;
    int missing_player_ids = 0;
    int missing_address_ids = 0;
    int missing_hash_ids = 0;
    int joining_players = 0;
    int quitting_players = 0;

    if (mysql_ping(&srv->segment->mysql)) {
#ifdef SUPERDEBUG
      printf("ActionSRCUpdateAllPlayers(): [%s] Ping with no pong!\n", srv->group);
#endif
      return;
    }

    for (pl = srv->players; pl; pl = pl->next) {
      if (pl->cycle != srv->playercycle) {
        pl->flags |= _SRC_PLAYER_QUIT;
        quitting_players++;
      }
      if (pl->flags & _SRC_PLAYER_JOIN) {
        joining_players++;
      }
      if (!pl->playerid) {
        missing_player_ids++;
      }
      if (!pl->addressid) {
        missing_address_ids++;
      }
      if (!pl->keyhashid && !(pl->flags & _SRC_PLAYER_HASH_PENDING)) {
        missing_hash_ids++;
      }
    }

    //lets get us some player ids
    if (missing_player_ids > 0) {
#define _SRC_SQL_SELECT_PLAYER_OTHER_QMARK 1
      const char select_player_sql_prefix[] = {"SELECT player_id FROM src_player WHERE player_name = ?"}; 
      const char select_player_sql_body[] = {""};
      const char select_player_sql_suffix[] = {""};
      const char select_player_sql_separator[] = {""};
      select_src_player_store_t select_player_store;

      select_player_store.srv = srv;
      select_player_store.total_missing = 0;

      query.mysql = &srv->segment->mysql;
      query.sql_prefix = select_player_sql_prefix;
      query.sql_body = select_player_sql_body;
      query.sql_separator = select_player_sql_separator;
      query.sql_suffix = select_player_sql_suffix;
      query.sql_prefix_len = CHARSIZE(select_player_sql_prefix);
      query.sql_body_len = CHARSIZE(select_player_sql_body);
      query.sql_separator_len = CHARSIZE(select_player_sql_separator);
      query.sql_suffix_len = CHARSIZE(select_player_sql_suffix);
      query.body_count = 0;
      query.qmark = 0;
      query.other_qmark = _SRC_SQL_SELECT_PLAYER_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectPlayer", srv->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = srv->players; pl; pl = pl->next) {
          if (pl->playerid) {
            continue;
          }

          select_player_store.player = pl; //keep this here
          SelectSRCPlayerBindParam(&query, &select_player_store);
          if (RunMySQLQuery(&query)) {
            SelectSRCPlayerInitBindResult(&query, &select_player_store);
            if (ResultMySQLQuery(&query)) {
              SelectSRCPlayerProcessResult(&query, &select_player_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionSRCUpdateAllPlayers(): [%s] Missing %d player ids\n", srv->group, select_player_store.total_missing);
#endif

      //2nsert missing players
      if (select_player_store.total_missing > 0) {
#define _SRC_SQL_INSERT_PLAYER_QMARK 2
        const char insert_player_sql_prefix[] = {"INSERT INTO src_player (player_id, player_name, player_lastserver, player_lastseen) VALUES "}; 
        const char insert_player_sql_body[] = {"(0, ?, ?, CURRENT_TIMESTAMP())"};
        const char insert_player_sql_suffix[] = {""};
        const char insert_player_sql_separator[] = {", "};
        src_generic_store_t insert_player_store;

        insert_player_store.srv = srv;

        query.mysql = &srv->segment->mysql;
        query.sql_prefix = insert_player_sql_prefix;
        query.sql_body = insert_player_sql_body;
        query.sql_separator = insert_player_sql_separator;
        query.sql_suffix = insert_player_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_player_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_player_sql_body);
        query.sql_separator_len = CHARSIZE(insert_player_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_player_sql_suffix);
        query.body_count = select_player_store.total_missing;
        query.qmark = _SRC_SQL_INSERT_PLAYER_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertPlayers", srv->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertSRCPlayersBindParam(&query, &insert_player_store);
          if (RunMySQLQuery(&query)) {
            InsertSRCPlayersProcessResult(&query, &insert_player_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionSRCUpdateAllPlayers(): [%s] Got all player ids\n", srv->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = srv->players; pl; pl = pl->next) {
      if (!pl->playerid) {
        printf("ActionSRCUpdateAllPlayers(): [%s] WTF?! player %s is missing an id!\n", srv->group, pl->name);
      }
    }
#endif

    //by now, all players should have an id

    //lets get us some address ids
    if (missing_address_ids > 0) {
#define _SRC_SQL_SELECT_ADDRESS_OTHER_QMARK 2
      const char select_address_sql_prefix[] = {"SELECT address_id FROM src_address WHERE address_player_id = ? AND address_netip = ?"}; 
      const char select_address_sql_body[] = {""};
      const char select_address_sql_suffix[] = {""};
      const char select_address_sql_separator[] = {""};
      select_src_address_store_t select_address_store;

      select_address_store.srv = srv;
      select_address_store.total_missing = 0;

      query.mysql = &srv->segment->mysql;
      query.sql_prefix = select_address_sql_prefix;
      query.sql_body = select_address_sql_body;
      query.sql_separator = select_address_sql_separator;
      query.sql_suffix = select_address_sql_suffix;
      query.sql_prefix_len = CHARSIZE(select_address_sql_prefix);
      query.sql_body_len = CHARSIZE(select_address_sql_body);
      query.sql_separator_len = CHARSIZE(select_address_sql_separator);
      query.sql_suffix_len = CHARSIZE(select_address_sql_suffix);
      query.body_count = 0;
      query.qmark = 0;
      query.other_qmark = _SRC_SQL_SELECT_ADDRESS_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectAddresses", srv->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = srv->players; pl; pl = pl->next) {
          if (!pl->playerid || pl->addressid) {
            continue;
          }
          if (pl->flags & _SRC_PLAYER_INSERTED) { //new player, don't bother selecting a row that will never exist
#ifdef SUPERDEBUG
            //printf("ActionSRCUpdateAllPlayers(): [%s] Skipping address select for %s (they\'re new)\n", srv->group, pl->name);
#endif
            select_address_store.total_missing++;
            continue;
          }

          select_address_store.player = pl; //keep this here
          SelectSRCAddressBindParam(&query, &select_address_store);
          if (RunMySQLQuery(&query)) {
            SelectSRCAddressInitBindResult(&query, &select_address_store);
            if (ResultMySQLQuery(&query)) {
              SelectSRCAddressProcessResult(&query, &select_address_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionSRCUpdateAllPlayers(): [%s] Missing %d address ids\n", srv->group, select_address_store.total_missing);
#endif

      if (select_address_store.total_missing > 0) {
#define _SRC_SQL_INSERT_ADDRESS_QMARK 2
        const char insert_address_sql_prefix[] = {"INSERT INTO src_address (address_id, address_player_id, address_netip, address_lastseen) VALUES "}; 
        const char insert_address_sql_body[] = {"(0, ?, ?, CURRENT_TIMESTAMP())"}; //if this changes, change that define about
        const char insert_address_sql_suffix[] = {""};
        const char insert_address_sql_separator[] = {", "};
        insert_src_address_store_t insert_address_store;

        insert_address_store.srv = srv;

        query.mysql = &srv->segment->mysql;
        query.sql_prefix = insert_address_sql_prefix;
        query.sql_body = insert_address_sql_body;
        query.sql_separator = insert_address_sql_separator;
        query.sql_suffix = insert_address_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_address_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_address_sql_body);
        query.sql_separator_len = CHARSIZE(insert_address_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_address_sql_suffix);
        query.body_count = select_address_store.total_missing;
        query.qmark = _SRC_SQL_INSERT_ADDRESS_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertAddresses", srv->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertSRCAddressBindParam(&query, &insert_address_store);
          if (RunMySQLQuery(&query)) {
            InsertSRCAddressProcessResult(&query, &insert_address_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionSRCUpdateAllPlayers(): [%s] Got all player address ids\n", srv->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = srv->players; pl; pl = pl->next) {
      if (!pl->addressid) {
        printf("ActionSRCUpdateAllPlayers(): [%s] WTF?! player %s is missing their address id!\n", srv->group, pl->name);
      }
    }
#endif

    //lets get us some key hash ids
    if (missing_hash_ids > 0) {
#define _SRC_SQL_SELECT_KEYHASH_OTHER_QMARK 2
      const char select_keyhash_sql_prefix[] = {"SELECT keyhash_id FROM src_keyhash WHERE keyhash_player_id = ? AND keyhash_hash = ?"}; 
      const char select_keyhash_sql_body[] = {""};
      const char select_keyhash_sql_suffix[] = {""};
      const char select_keyhash_sql_separator[] = {""};
      select_src_keyhash_store_t select_keyhash_store;

      select_keyhash_store.srv = srv;
      select_keyhash_store.total_missing = 0;

      query.mysql = &srv->segment->mysql;
      query.sql_prefix = select_keyhash_sql_prefix;
      query.sql_body = select_keyhash_sql_body;
      query.sql_separator = select_keyhash_sql_separator;
      query.sql_suffix = select_keyhash_sql_suffix;
      query.sql_prefix_len = CHARSIZE(select_keyhash_sql_prefix);
      query.sql_body_len = CHARSIZE(select_keyhash_sql_body);
      query.sql_separator_len = CHARSIZE(select_keyhash_sql_separator);
      query.sql_suffix_len = CHARSIZE(select_keyhash_sql_suffix);
      query.body_count = 0;
      query.qmark = 0;
      query.other_qmark = _SRC_SQL_SELECT_KEYHASH_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectKeyhashes", srv->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = srv->players; pl; pl = pl->next) {
          if (!pl->playerid || pl->keyhashid || (pl->flags & _SRC_PLAYER_HASH_PENDING)) {
            continue;
          }
          if (pl->flags & _SRC_PLAYER_INSERTED) {
#ifdef SUPERDEBUG
            //printf("ActionSRCUpdateAllPlayers(): [%s] Skipping keyhash select for %s (they\'re new)\n", srv->group, pl->name);
#endif
            select_keyhash_store.total_missing++;
            continue;
          }

          select_keyhash_store.player = pl; //keep this here
          SelectSRCKeyhashBindParam(&query, &select_keyhash_store);
          if (RunMySQLQuery(&query)) {
            SelectSRCKeyhashInitBindResult(&query, &select_keyhash_store);
            if (ResultMySQLQuery(&query)) {
              SelectSRCKeyhashProcessResult(&query, &select_keyhash_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionSRCUpdateAllPlayers(): [%s] Missing %d keyhashes ids\n", srv->group, select_keyhash_store.total_missing);
#endif

      if (select_keyhash_store.total_missing > 0) {
        const char insert_keyhash_sql_prefix[] = {"INSERT INTO src_keyhash (keyhash_id, keyhash_player_id, keyhash_hash, keyhash_lastseen) VALUES "}; 
#define _SRC_SQL_INSERT_KEYHASH_QMARK 2
        const char insert_keyhash_sql_body[] = {"(0, ?, ?, CURRENT_TIMESTAMP())"}; //if this changes, change that define about
        const char insert_keyhash_sql_suffix[] = {""};
        const char insert_keyhash_sql_separator[] = {", "};
        src_generic_store_t insert_keyhash_store;

        insert_keyhash_store.srv = srv;

        query.mysql = &srv->segment->mysql;
        query.sql_prefix = insert_keyhash_sql_prefix;
        query.sql_body = insert_keyhash_sql_body;
        query.sql_separator = insert_keyhash_sql_separator;
        query.sql_suffix = insert_keyhash_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_keyhash_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_keyhash_sql_body);
        query.sql_separator_len = CHARSIZE(insert_keyhash_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_keyhash_sql_suffix);
        query.body_count = select_keyhash_store.total_missing;
        query.qmark = _SRC_SQL_INSERT_KEYHASH_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertKeyhashes", srv->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertSRCKeyhashBindParam(&query, &insert_keyhash_store);
          if (RunMySQLQuery(&query)) {
            InsertSRCKeyhashProcessResult(&query, &insert_keyhash_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionSRCUpdateAllPlayers(): [%s] Got all player keyhash ids\n", srv->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = srv->players; pl; pl = pl->next) {
      if (!pl->keyhashid && (!(pl->flags & _SRC_PLAYER_HASH_PENDING))) {
        printf("ActionSRCUpdateAllPlayers(): [%s] WTF?! player %s is missing their keyhash id!\n", srv->group, pl->name);
      }
    }
#endif

    //now that everyone has their details, update the players on server table and deal with join and quits

    //joining players
    if (joining_players > 0) {
#define _SRC_SQL_UPDATE_JOIN_PLAYERS_QMARK 1
#define _SRC_SQL_UPDATE_JOIN_PLAYERS_OTHER_QMARK 2
      const char update_joinplayer_sql_prefix[] = {"UPDATE src_player SET player_currentserver_id = ?, player_lastserver = ?, player_lastseen = CURRENT_TIMESTAMP() WHERE player_id IN ("}; 
      const char update_joinplayer_sql_body[] = {"?"};
      const char update_joinplayer_sql_suffix[] = {")"};
      const char update_joinplayer_sql_separator[] = {", "};
      src_generic_store_t update_joinplayer_store;

      update_joinplayer_store.srv = srv;

      query.mysql = &srv->segment->mysql;
      query.sql_prefix = update_joinplayer_sql_prefix;
      query.sql_body = update_joinplayer_sql_body;
      query.sql_separator = update_joinplayer_sql_separator;
      query.sql_suffix = update_joinplayer_sql_suffix;
      query.sql_prefix_len = CHARSIZE(update_joinplayer_sql_prefix);
      query.sql_body_len = CHARSIZE(update_joinplayer_sql_body);
      query.sql_separator_len = CHARSIZE(update_joinplayer_sql_separator);
      query.sql_suffix_len = CHARSIZE(update_joinplayer_sql_suffix);
      query.body_count = joining_players;
      query.qmark = _SRC_SQL_UPDATE_JOIN_PLAYERS_QMARK;
      query.other_qmark = _SRC_SQL_UPDATE_JOIN_PLAYERS_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:InsertJoinPlayer", srv->group);
#endif

      if (OpenMySQLQuery(&query)) {
        UpdateSRCJoinPlayerBindParam(&query, &update_joinplayer_store);
        RunMySQLQuery(&query);
        CloseMySQLQuery(&query);
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionSRCUpdateAllPlayers(): [%s] No one joined\n", srv->group);
#endif
    }

    //parting players
    if (quitting_players > 0) {
#define _SRC_SQL_UPDATE_QUIT_PLAYERS_QMARK 1
      const char update_quitplayer_sql_prefix[] = {"UPDATE src_player SET player_currentserver_id = NULL, player_lastseen = CURRENT_TIMESTAMP() WHERE player_id IN ("}; 
      const char update_quitplayer_sql_body[] = {"?"};
      const char update_quitplayer_sql_suffix[] = {")"};
      const char update_quitplayer_sql_separator[] = {", "};
      src_generic_store_t update_quitplayer_store;

      update_quitplayer_store.srv = srv;

      query.mysql = &srv->segment->mysql;
      query.sql_prefix = update_quitplayer_sql_prefix;
      query.sql_body = update_quitplayer_sql_body;
      query.sql_separator = update_quitplayer_sql_separator;
      query.sql_suffix = update_quitplayer_sql_suffix;
      query.sql_prefix_len = CHARSIZE(update_quitplayer_sql_prefix);
      query.sql_body_len = CHARSIZE(update_quitplayer_sql_body);
      query.sql_separator_len = CHARSIZE(update_quitplayer_sql_separator);
      query.sql_suffix_len = CHARSIZE(update_quitplayer_sql_suffix);
      query.body_count = quitting_players;
      query.qmark = _SRC_SQL_UPDATE_QUIT_PLAYERS_QMARK;
      query.other_qmark = 0;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:DeletePartPlayer", srv->group);
#endif

      if (OpenMySQLQuery(&query)) {
        UpdateSRCQuitPlayerBindParam(&query, &update_quitplayer_store);
        RunMySQLQuery(&query);
        CloseMySQLQuery(&query);
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionSRCUpdateAllPlayers(): [%s] No one quit\n", srv->group);
#endif
    }

    //remove players that quit
    n = srv->players;
    while (n) {
      pl = n;
      n = pl->next;

      pl->flags &= ~_SRC_PLAYER_JOIN;
      pl->flags &= ~_SRC_PLAYER_INSERTED;

      if (pl->flags & _SRC_PLAYER_QUIT) {
#ifdef SUPERDEBUG
        printf("ActionSRCUpdateAllPlayers(): [%s] player (%d)%s left\n", srv->group, pl->id, pl->name);
#endif
        FreeSRCPlayer(pl, &srv->players);
      }
    }
  }
}

// ==================== Select Players ====================
void SelectSRCPlayerBindParam(mysqlquery_t *query, select_src_player_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectSRCPlayerBindParam(): player is %s (%s)\n", store->player->name, store->player->address);
#endif

  //0 - player_name
  query->bindparam[0].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[0].buffer = (void *)store->player->name;
  query->bindparam[0].buffer_length = strlen(store->player->name);

  store->total_missing++;
}

void SelectSRCPlayerInitBindResult(mysqlquery_t *query, select_src_player_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //player_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->player_id;
  store->bindres[0].is_null = &store->is_null;
  store->bindres[0].is_unsigned = 1;

  query->bindres = store->bindres;
}

void SelectSRCPlayerProcessResult(mysqlquery_t *query, select_src_player_store_t *store) {
  assert(store->player);

  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null) {
#ifdef SUPERDEBUG
      printf("SelectSRCPlayerProcessResult(): [%s] WARNING database returned null for player %s\n", store->srv->group, store->player->name);
#endif
      return;
    }

    store->player->playerid = store->player_id;
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectSRCPlayerProcessResult(): [%s] Player %s\'s database id: %llu\n", store->srv->group, store->player->name, store->player->playerid);
#endif
  }
}

// ==================== Insert Players ====================
void InsertSRCPlayersBindParam(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (pl->playerid) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertSRCPlayersBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //0 - player_name
    query->bindparam[(i * query->qmark) + 0].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 0].buffer = (void *)pl->name;
    query->bindparam[(i * query->qmark) + 0].buffer_length = strlen(pl->name);

    //1 - player_lastserver
    query->bindparam[(i * query->qmark) + 2].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 2].buffer = (void *)store->srv->serverinfo.hostname;
    query->bindparam[(i * query->qmark) + 2].buffer_length = strlen(store->srv->serverinfo.hostname);

    i++;
  }
}

void InsertSRCPlayersProcessResult(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  uint64_t playerid = mysql_stmt_insert_id(query->stmt);
  if (!playerid) {
    return;
  }

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (pl->playerid) {
      continue;
    }

    pl->playerid = playerid;
    playerid++;

    pl->flags |= _SRC_PLAYER_INSERTED;

#ifdef SUPERDEBUG
    printf("InsertSRCPlayersProcessResult(): [%s] Player %s was added, new id is: %llu\n", store->srv->group, pl->name, pl->playerid);
#endif
  }
}

// ==================== Select Addresses ====================
void SelectSRCAddressBindParam(mysqlquery_t *query, select_src_address_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectSRCAddressBindParam(): player is %s (%s)\n", store->player->name, store->player->address);
#endif

  //0 - address_player_id
  query->bindparam[0].buffer_type = MYSQL_TYPE_LONGLONG;
  query->bindparam[0].buffer = (void *)&store->player->playerid;
  query->bindparam[0].is_unsigned = 1;

  //1 - address_netip
  query->bindparam[1].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[1].buffer = (void *)store->player->address;
  query->bindparam[1].buffer_length = strlen(store->player->address);

  store->total_missing++;
}

void SelectSRCAddressInitBindResult(mysqlquery_t *query, select_src_address_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //address_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->address_id;
  store->bindres[0].is_null = &store->is_null;
  store->bindres[0].is_unsigned = 1;

  query->bindres = store->bindres;
}


void SelectSRCAddressProcessResult(mysqlquery_t *query, select_src_address_store_t *store) {
  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null) {
#ifdef SUPERDEBUG
      printf("SelectSRCAddressProcessResult(): [%s] WARNING database returned null for address\n", store->srv->group);
#endif
      return;
    }

    assert(store->player);

    store->player->addressid = store->address_id;
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectSRCAddressProcessResult(): [%s] Player %s\'s address database id: %llu\n", store->srv->group, store->player->name, store->player->addressid);
#endif
  }
}

// ==================== Insert Addresses ====================
void InsertSRCAddressBindParam(mysqlquery_t *query, insert_src_address_store_t *store) {
  srcplayer_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

#ifdef SUPERDEBUG
  //printf("InsertSRCAddressBindParam() [%s]\n", store->srv->group);
#endif

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->addressid) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertSRCAddressBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //0 - address_player_id
    query->bindparam[(i * query->qmark) + 0].buffer_type = MYSQL_TYPE_LONGLONG;
    query->bindparam[(i * query->qmark) + 0].buffer = (void *)&pl->playerid;
    query->bindparam[(i * query->qmark) + 0].is_unsigned = 1;

    //1 - address_netip
    query->bindparam[(i * query->qmark) + 1].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 1].buffer = (void *)pl->address;
    query->bindparam[(i * query->qmark) + 1].buffer_length = strlen(pl->address);

    i++;
  }
}

void InsertSRCAddressProcessResult(mysqlquery_t *query, insert_src_address_store_t *store) {
  srcplayer_t *pl;
  uint64_t addressid = mysql_stmt_insert_id(query->stmt);
  if (!addressid) {
    return;
  }

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->addressid) {
      continue;
    }

    pl->addressid = addressid;
    addressid++;

#ifdef SUPERDEBUG
    printf("InsertSRCAddressProcessResult(): [%s] Player %s\'s IP (%s) was added, new id is: %llu\n", store->srv->group, pl->name, pl->address, pl->addressid);
#endif
  }
}

// ==================== Select Keyhashes ====================
void SelectSRCKeyhashBindParam(mysqlquery_t *query, select_src_keyhash_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectSRCKeyhashBindParam(): player is %s (%s)\n", store->player->name, store->player->hash);
#endif

  //0 - keyhash_player_id
  query->bindparam[0].buffer_type = MYSQL_TYPE_LONGLONG;
  query->bindparam[0].buffer = (void *)&store->player->playerid;
  query->bindparam[0].is_unsigned = 1;

  //1 - keyhash_hashnetip
  query->bindparam[1].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[1].buffer = (void *)store->player->hash;
  query->bindparam[1].buffer_length = strlen(store->player->hash);

  store->total_missing++;
}

void SelectSRCKeyhashInitBindResult(mysqlquery_t *query, select_src_keyhash_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //keyhash_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->keyhash_id;
  store->bindres[0].is_null = &store->is_null[0];
  store->bindres[0].is_unsigned = 1;

  query->bindres = store->bindres;
}


void SelectSRCKeyhashProcessResult(mysqlquery_t *query, select_src_keyhash_store_t *store) {
  assert(store->player);

  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null[0]) {
#ifdef SUPERDEBUG
      printf("SelectSRCKeyhashProcessResult(): [%s] WARNING database returned null for keyhash\n", store->srv->group);
#endif
      return;
    }

    store->player->keyhashid = store->keyhash_id;
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectSRCKeyhashProcessResult(): [%s] Player %s\'s keyhash database id: %llu\n", store->srv->group, store->player->name, store->player->keyhashid);
#endif
  }
}

// ==================== Insert Keyhashes ====================
void InsertSRCKeyhashBindParam(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

#ifdef SUPERDEBUG
  //printf("InsertSRCKeyhashBindParam() [%s]\n", store->srv->group);
#endif

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->keyhashid || (pl->flags & _SRC_PLAYER_HASH_PENDING)) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertSRCKeyhashBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //0 - keyhash_player_id
    query->bindparam[(i * query->qmark) + 0].buffer_type = MYSQL_TYPE_LONGLONG;
    query->bindparam[(i * query->qmark) + 0].buffer = (void *)&pl->playerid;
    query->bindparam[(i * query->qmark) + 0].is_unsigned = 1;

    //1 - keyhash_hash
    query->bindparam[(i * query->qmark) + 1].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 1].buffer = (void *)pl->hash;
    query->bindparam[(i * query->qmark) + 1].buffer_length = strlen(pl->hash);

    i++;
  }
}

void InsertSRCKeyhashProcessResult(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  uint64_t keyhashid = mysql_stmt_insert_id(query->stmt);
  if (!keyhashid) {
    return;
  }

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->keyhashid || (pl->flags & _SRC_PLAYER_HASH_PENDING)) {
      continue;
    }

    pl->keyhashid = keyhashid;
    keyhashid++;

#ifdef SUPERDEBUG
    printf("InsertSRCKeyhashProcessResult(): [%s] Player %s\'s keyhash (%s) was added, new id is: %llu\n", store->srv->group, pl->name, pl->hash, pl->keyhashid);
#endif
  }
}

// ==================== Update Server Player (Join) ====================
void UpdateSRCJoinPlayerBindParam(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark) + query->other_qmark;
#endif

#ifdef SUPERDEBUG
  //printf("InsertServerPlayerBindParam() [%s]\n", store->srv->group);
#endif

  //player_currentserver_id
  query->bindparam[i].buffer_type = MYSQL_TYPE_LONG;
  query->bindparam[i].buffer = (void *)&store->srv->serverid;
  query->bindparam[i].is_unsigned = 1;
  i++;

  query->bindparam[i].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[i].buffer = (void *)&store->srv->serverinfo.hostname;
  query->bindparam[i].buffer_length = strlen(store->srv->serverinfo.hostname);
  i++;

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || (!(pl->flags & _SRC_PLAYER_JOIN))) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("UpdateSRCJoinPlayerBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //player_id
    query->bindparam[i].buffer_type = MYSQL_TYPE_LONGLONG;
    query->bindparam[i].buffer = (void *)&pl->playerid;
    query->bindparam[i].is_unsigned = 1;

    i++;
  }
}

// ==================== Update Server Player (Quit) ====================
void UpdateSRCQuitPlayerBindParam(mysqlquery_t *query, src_generic_store_t *store) {
  srcplayer_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark) + query->other_qmark;
#endif

#ifdef SUPERDEBUG
  //printf("DeleteServerPlayerBindParam() [%s]\n", store->srv->group);
#endif

  for (pl = store->srv->players; pl; pl = pl->next) {
    if (!pl->playerid || (!(pl->flags & _SRC_PLAYER_QUIT))) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("UpdateSRCQuitPlayerBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //serverplayer_player_id
    query->bindparam[i].buffer_type = MYSQL_TYPE_LONGLONG;
    query->bindparam[i].buffer = (void *)&pl->playerid;
    query->bindparam[i].is_unsigned = 1;

    i++;
  }
}

#ifdef SHOW_STATUS
void DisplaySRCSegments(void) {
}
#endif

