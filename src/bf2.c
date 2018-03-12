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

#include "bf2.h"

extern serenity_t ne;
extern bf2segment_t *bf2segments;

pcre *pbheaderpattern;
pcre *pbplayerpattern;
pcre *pbscreenpattern;

/* ----------------------------------------------------------------------------

  BATTLEFIELD 2

---------------------------------------------------------------------------- */

int InitBF2(void) {
  const char *pcreerr;
  int i;

  pbheaderpattern = pcre_compile(_BF2_PBSTREAM_HEADER, 0, &pcreerr, &i, NULL);
  if (!pbheaderpattern) {
#ifdef _DEBUG
    WriteLog("Error: compilation failed at offset %d: %s\n", i, pcreerr);
#endif
    assert(0);
    return 0;
  }

  pbplayerpattern = pcre_compile(_BF2_PBSTREAM_PLAYER_PATTERN, 0, &pcreerr, &i, NULL);
  if (!pbplayerpattern) {
#ifdef _DEBUG
    WriteLog("Error: compilation failed at offset %d: %s\n", i, pcreerr);
#endif
    assert(0);
    return 0;
  }

#ifdef NVEDEXT
  pbscreenpattern = pcre_compile(_BF2_PBSTREAM_AUTOSHOT_PATTERN, 0, &pcreerr, &i, NULL);
  if (!pbscreenpattern) {
#ifdef _DEBUG
    WriteLog("Error: compilation failed at offset %d: %s\n", i, pcreerr);
#endif
    assert(0);
    return 0;
  }
#endif

  return 1;
}

void FinishBF2(void) {
  if (pbheaderpattern) {
    pcre_free(pbheaderpattern);
  }

  if (pbplayerpattern) {
    pcre_free(pbplayerpattern);
  }

#ifdef NVEDEXT
  if (pbscreenpattern) {
    pcre_free(pbscreenpattern);
  }
#endif
}

int StartBF2Segments(void) {
  bf2segment_t *bf2seg;
#ifdef WIN32
  int i;
#endif

  for (bf2seg = bf2segments; bf2seg; bf2seg = bf2seg->next) {
#ifdef WIN32
    bf2seg->thread = (HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessBF2Segments, (void *)bf2seg, 0, &i);
    if (!bf2seg->thread) {
#endif
#ifdef UNIX   
    if (pthread_create(&bf2seg->thread, NULL, &ProcessBF2Segments, (void *)bf2seg)) {
#endif
      return 0;
    }
  }

  return 1;
}

void StopBF2Segments(void) {
  bf2segment_t *bf2seg, *n;
  bf2server_t *bf2, *m;

  n = bf2segments;
  while (n) {
    bf2seg = n;
    n = bf2seg->next;

    if (bf2seg->thread) {
      _lockmutex(&bf2seg->mutex);
      bf2seg->active = 0;
      _unlockmutex(&bf2seg->mutex);

#ifdef SUPERDEBUG
      printf("Waiting for: %s\n", bf2seg->srvlist);
#endif

#ifdef WIN32
      WaitForSingleObject(bf2seg->thread, INFINITE);
      CloseHandle(bf2seg->thread);
#endif
#ifdef UNIX
      pthread_join(bf2seg->thread, NULL);
#endif
    }

    SaveBF2Transfers(bf2seg, 1);

    m = bf2seg->bf2servers;
    while (m) {
      bf2 = m;
      m = bf2->next;

      FreeBF2AllPlayers(&bf2->players);

      FreeBF2Server(bf2);
    }

    _destroymutex(&bf2seg->mutex);
    CloseSocket(&bf2seg->pbsock);
    CloseMySQL(&bf2seg->mysql);

    _free_(bf2seg);
  }
}

static void *ProcessBF2Segments(void *param) {
  char rv[_BF2_MAX_MESSAGE_SIZE+1];
  bf2segment_t *bf2seg = (bf2segment_t *)param;
  bf2server_t *bf2;
  int active = 1;

  WriteLog("Flying: %s\n", bf2seg->name);

  while (active) {
    _lockmutex(&bf2seg->mutex);
    active = bf2seg->active;
    _unlockmutex(&bf2seg->mutex);

    for (bf2 = bf2seg->bf2servers; bf2; bf2 = bf2->next) {
#ifdef SUPERDEBUG
      //printf("ProcessBF2Segments(): %s\n", bf2->group);
#endif
      if (!ProcessBF2Server(bf2, rv, sizeof(rv))) {
        continue;
      }
      RequestBF2Players(bf2);
      //RequestBF2Chat(bf2);
#ifdef NVEDEXT
      ProcessBF2PlayerShots(bf2);
#endif
      CheckBF2PBStream(bf2);
    }

    ProcessPBStream(bf2seg, rv, sizeof(rv));
    SaveBF2Transfers(bf2seg, 0);
    _sleep(100);
  }

  //clean up
  for (bf2 = bf2seg->bf2servers; bf2; bf2 = bf2->next) {
    if (bf2->state != SOCKETSTATE_CONNECTED) {
      continue;
    }

    AddRequest(bf2, _BF2_PACKET_HIDE);
    SendBF2Socket(bf2, "\x02" "exec PB_SV_LogAddr \"\"\n", -1);
  }

#ifdef WIN32
  _endthreadex(0);
#endif
#ifdef UNIX
  pthread_exit(NULL);
#endif
  return 0;
}

static void ProcessPBStream(bf2segment_t *bf2seg, char *rv, int rvsz) {
  int sz;
  struct sockaddr_in addr;
#ifdef WIN32
  unsigned int hsz = sizeof(SOCKADDR);
#endif
#ifdef UNIX
  unsigned int hsz = sizeof(struct sockaddr_in);
#endif

#ifdef WIN32
  sz = recvfrom(bf2seg->pbsock, rv, rvsz - 1, 0, (LPSOCKADDR)&addr, &hsz);
#endif
#ifdef UNIX
  sz = recvfrom(bf2seg->pbsock, rv, rvsz - 1, MSG_DONTWAIT, (struct sockaddr *)&addr, &hsz);
#endif
  if (sz != -1) {
    int ovector[_BF2_PBSTREAM_OVECCOUNT];
    int r;

    rv[sz] = '\0';

#ifdef SUPERDEBUG
    //printf("*** PB[%d] %s\n", sz, rv);
#endif

    r = pcre_exec(pbheaderpattern, NULL, rv, sz, 0, 0, ovector, _BF2_PBSTREAM_OVECCOUNT);
    if (r > 0) {
      bf2server_t *bf2;
      char group[_BF2_MAX_GROUP+1];
      char *p;
      int len;

#define _BF2_PB_HEADER_GROUP 1
#define _BF2_PB_HEADER_TEXT 2

      pcrestrcpy(group, rv, _BF2_PB_HEADER_GROUP, ovector, sizeof(group));
      //src + ovec[2 * pos]

      bf2 = FindBF2Server(group, &addr, bf2seg->bf2servers);
      if (!bf2) {
#ifdef LOG_ERRORS
        WriteLog("PBStream - Bad group: %s\n", group);
#endif
        return;
      }

      bf2->lastpbstream = DoubleTime();
      bf2->sentpbalive = 0;

      //player check
      p = rv + ovector[2 * _BF2_PB_HEADER_TEXT];
      len = ovector[2 * _BF2_PB_HEADER_TEXT + 1] - ovector[2 * _BF2_PB_HEADER_TEXT];

      r = pcre_exec(pbplayerpattern, NULL, p, len, 0, 0, ovector, _BF2_PBSTREAM_OVECCOUNT);
      if (r > 0) {
        bf2player_t *pl;
        char pl_name[_BF2_MAX_PLAYER_NAME+1];
        char pl_address[_BF2_MAX_PLAYER_ADDRESS+1];
        char pl_guid[_BF2_MAX_PLAYER_GUID+1];
        int pl_slot;

#define _BF2_PB_PATTERN_PL_GUID 1
#define _BF2_PB_PATTERN_PL_SLOT 2
#define _BF2_PB_PATTERN_PL_ADDRESS 3
#define _BF2_PB_PATTERN_PL_NAME 4

        pl_slot = pcreatoi(p, _BF2_PB_PATTERN_PL_SLOT , ovector);
        pcrestrcpy(pl_address, p, _BF2_PB_PATTERN_PL_ADDRESS, ovector, sizeof(pl_address));
        pcrestrcpy(pl_name, p, _BF2_PB_PATTERN_PL_NAME, ovector, sizeof(pl_name));
        pcrestrcpy(pl_guid, p, _BF2_PB_PATTERN_PL_GUID, ovector, sizeof(pl_guid));

        pl = FindBF2PlayerByName(bf2, pl_name);
        if (!pl) {
#ifdef LOG_ERRORS
          WriteLog("PBStream Player - Player not found: %s\n", pl_name);
#endif
          return;
        }

        if (strcmp(pl->address, pl_address) || ((pl->id + 1) != pl_slot)) {
#ifdef SUPERDEBUG
          printf("ProcessPBStream(): [%s] WARNING! guid player didn\'t match: %s != %s or %d != %d\n", bf2->group, pl->address, pl_address, pl->id + 1, pl_slot);
#endif
          return;
        }

        if (pl->guid[0]) {
          if (!strcmp(pl->guid, pl_guid)) {
#ifdef SUPERDEBUG
            printf("ProcessPBStream(): [%s] Already have %s\'s guid [%s]\n", bf2->group, pl->name, pl->guid);
#endif
            return;
#ifdef SUPERDEBUG
          } else {
            printf("ProcessPBStream(): [%s] WARNING! player %s had a guid but they didn\'t match! %s != %s\n", bf2->group, pl->name, pl->guid, pl_guid);
#endif
          }
        }

        //this is all we really came for
        pl->flags |= _BF2_PLAYER_NEWGUID;
        strlcpy(pl->guid, pl_guid, sizeof(pl->guid));

#ifdef SUPERDEBUG
        printf("ProcessPBStream(): [%s] Received PBGUID: (%d:%d)%s %s [%s]\n", bf2->group, pl->id, pl_slot, pl->name, pl->address, pl->guid);
#endif
        return;
      }

#ifdef NVEDEXT
      if (bf2->nvedext) {
        r = pcre_exec(pbscreenpattern, NULL, p, len, 0, 0, ovector, _BF2_PBSTREAM_OVECCOUNT);
        if (r > 0) {
          char pl_name[_BF2_MAX_PLAYER_NAME+1];
          char pl_address[_BF2_MAX_PLAYER_ADDRESS+1];
          bf2player_t *pl;

#define _BF2_PB_PATTERN_SS_FILENAME 1
#define _BF2_PB_PATTERN_SS_MD5HASH 2
#define _BF2_PB_PATTERN_SS_PLAYERNAME 3
#define _BF2_PB_PATTERN_SS_GUID 4
#define _BF2_PB_PATTERN_SS_ADDRESS 5

          pcrestrcpy(pl_name, p, _BF2_PB_PATTERN_SS_PLAYERNAME, ovector, sizeof(pl_name));
          pcrestrcpy(pl_address, p, _BF2_PB_PATTERN_SS_ADDRESS, ovector, sizeof(pl_address));

          pl = FindBF2PlayerByName(bf2, pl_name);
          if (!pl) {
#ifdef LOG_ERRORS
            WriteLog("PBStream - Screen Player not found: %s\n", pl_name);
#endif
            return;
          }

          if (strcmp(pl->address, pl_address)) {
#ifdef SUPERDEBUG
            printf("ProcessPBStream(): [%s] WARNING! player didn\'t match: %s != %s\n", bf2->group, pl->address, pl_address);
#endif
            return;
          }

          if (pl->pbss) {
#ifdef SUPERDEBUG
            printf("ProcessPBStream(): [%s] player %s already has an attached shot!\n", bf2->group, pl->name);
#endif
            FreeBF2PBShot(&pl->pbss);
          }

          pl->pbss = NewBF2PBShot();
          if (!pl->pbss) {
            return;
          }

          pcrestrcpy(pl->pbss->filename, p, _BF2_PB_PATTERN_SS_FILENAME, ovector, sizeof(pl->pbss->filename));
          pcrestrcpy(pl->pbss->md5, p, _BF2_PB_PATTERN_SS_MD5HASH, ovector, sizeof(pl->pbss->md5));

#ifdef SUPERDEBUG
          printf("ProcessPBStream(): [%s]: New ScreenShot: %s %s(%s) [MD5:%s]\n", bf2->group, pl->pbss->filename, pl->name, pl->address, pl->pbss->md5);
#endif
          return;
        }
      }
#endif
#ifdef SUPERDEBUG
     //printf("ProcessPBStream(): [%s:%d]: %s\n", bf2->group, sz, rv);
#endif
#ifdef LOG_ERRORS
    } else if (r == PCRE_ERROR_NOMATCH) {
      WriteLog("PBStream - Unknown (%d): %s\n", sz, rv);
#endif
    }

    assert(r);
    //r = _BF2_PBSTREAM_OVECCOUNT / 3;
    //printf("ovector only has room for %d captured substrings\n", r - 1);
  }
}

static void CheckBF2PBStream(bf2server_t *bf2) {
  double t = DoubleTime();

#ifdef SUPERDEBUG
  //printf("CheckBF2PBStream()\n");
#endif

  if (bf2->serverinfo.numplayers && ((t - bf2->lastpbstream) >= TIME_PBSTREAM_CHECK)) {
    if (!bf2->sentpbalive) {
      AddRequest(bf2, _BF2_PACKET_HIDE);
      SendBF2Socket(bf2, "\x02" "exec PB_SV_Ver\n", -1);
    } else {
      SendBF2PBCommands(bf2);
    }

    bf2->sentpbalive = !bf2->sentpbalive;
    bf2->lastpbstream = DoubleTime();
  }
}

static void SendBF2PBCommands(bf2server_t *bf2) {
  char buf[80];
  int r;

#ifdef SUPERDEBUG
  printf("SendBF2PBCommands()\n");
#endif

  AddRequest(bf2, _BF2_PACKET_HIDE);
  r = _strnprintf(buf, sizeof(buf), "\x02" "exec PB_SV_LogUser \"%s\"\n", bf2->group);
  SendBF2Socket(bf2, buf, r);

  AddRequest(bf2, _BF2_PACKET_HIDE);
  r = _strnprintf(buf, sizeof(buf), "\x02" "exec PB_SV_LogPort \"%d\"\n", bf2->segment->pbsteamport);
  SendBF2Socket(bf2, buf, r);

  AddRequest(bf2, _BF2_PACKET_HIDE);
  r = _strnprintf(buf, sizeof(buf), "\x02" "exec PB_SV_LogAddr \"%s\"\n", ne.external_address);
  SendBF2Socket(bf2, buf, r);
}

#ifdef SHOW_STATUS
void DisplayBF2Segments(void) {
  bf2segment_t *bf2seg;
  bf2server_t *bf2;
  int i = 0, j = 0;

  for (bf2seg = bf2segments; bf2seg; bf2seg = bf2seg->next) {
    WriteLog("BF2 %s\n", bf2seg->name);
    for (bf2 = bf2seg->bf2servers; bf2; bf2 = bf2->next) {
      WriteLog("  %s [%s:%d]\n", bf2->group, inet_ntoa(bf2->server.sin_addr), ntohs(bf2->server.sin_port));
      j++;
    }
    i++;
  }

  WriteLog("Loaded %d server%s into %d segment%s\n",
      j,
      j == 1 ? "" : "s",
      i,
      i == 1 ? "" : "s");

  //printf("\nPress ENTER to continue\n");
  //getchar();
}
#endif

static void SaveBF2Transfers(bf2segment_t *bf2seg, int now) {
  double t = DoubleTime();

  if (now || (t - ne.lastsave) >= TIME_SAVE_TRANSFER) {
    MYSQL_STMT *qry;

#ifdef SUPERDEBUG
    printf("SaveBF2Transfers()\n");
#endif

    ne.lastsave = t;

    if (mysql_ping(&bf2seg->mysql)) {
      return;
    }

    qry = mysql_stmt_init(&bf2seg->mysql);
    if (qry) {
      const char update_server[] = {"UPDATE serenity_server SET "
                                /* 0 */ "server_bytesreceived = (server_bytesreceived + ?), "
                                /* 1 */ "server_bytessent = (server_bytessent + ?) "
                                /* 2 */ "WHERE server_id = ?"};

      if (!mysql_stmt_prepare(qry, update_server, sizeof(update_server))) {

        bf2server_t *bf2;

        MYSQL_BIND bind[3];

        for (bf2 = bf2seg->bf2servers; bf2; bf2 = bf2->next) {
          memset(&bind, 0, sizeof(bind));

          //0 - server_bytesreceived
          bind[0].buffer_type = MYSQL_TYPE_LONG;
          bind[0].buffer = (void *)&bf2->tcprx;
          bind[0].is_unsigned = 1;

          //1 - server_bytessent
          bind[1].buffer_type = MYSQL_TYPE_LONG;
          bind[1].buffer = (void *)&bf2->tcptx;
          bind[1].is_unsigned = 1;

          //2 - server_id
          bind[2].buffer_type = MYSQL_TYPE_LONG;
          bind[2].buffer = (void *)&bf2->serverid;
          bind[2].is_unsigned = 1;

          if (!mysql_stmt_bind_param(qry, bind)) {
            if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
              WriteLog("Save Transfer - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
            } else {
              bf2->tcprx = 0;
              bf2->tcptx = 0;
            }
          }

        }
#ifdef LOG_ERRORS
      } else {
        WriteLog("Save Transfer - Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
      }

      mysql_stmt_close(qry);
#ifdef LOG_ERRORS
    } else {
      WriteLog("Save Transfer - Query failed: %s\n", mysql_error(&bf2seg->mysql));
#endif
    }
  }
}

void ActionBF2ServerSuccess(bf2server_t *bf2) {
  MYSQL_STMT *qry;

#ifdef SUPERDEBUG
  printf("ActionBF2ServerSuccess() [%s] %s - %s [%d/%d]\n", bf2->group, bf2->serverinfo.hostname, bf2->serverinfo.currentmap, bf2->serverinfo.numplayers, bf2->serverinfo.maxplayers);
#endif

  if (mysql_ping(&bf2->segment->mysql)) {
    return;
  }
    
  qry = mysql_stmt_init(&bf2->segment->mysql);
  if (qry) {
    const char update_server[] = {"UPDATE serenity_server SET "
                              /* 0 */ "server_hostname = ?, "
                              /* 1 */ "server_gameport = ?, "
                              /* 2 */ "server_queryport = ?, "
                              /* 3 */ "server_gametype = ?, "
                              /* 4 */ "server_gameid = ?, "
                              /* 5 */ "server_mapname = ?, "
                              /* 6 */ "server_numplayers = ?, "
                              /* 7 */ "server_maxplayers = ?, "
                                      "server_updatesuccess = server_updatesuccess + 1, "
                                      "server_consecutivefails = 0, "
                                      "server_lastupdate = CURRENT_TIMESTAMP() "
                              /* 8 */ "WHERE server_id = ?"};

    if (!mysql_stmt_prepare(qry, update_server, sizeof(update_server))) {
      MYSQL_BIND bind[9];
      memset(&bind, 0, sizeof(bind));

      //0 - server_hostname
      bind[0].buffer_type = MYSQL_TYPE_STRING;
      bind[0].buffer = (void *)bf2->serverinfo.hostname;
      bind[0].buffer_length = strlen(bf2->serverinfo.hostname);

      //1 - server_gameport
      bind[1].buffer_type = MYSQL_TYPE_SHORT;
      bind[1].buffer = (void *)&bf2->serverinfo.gameport;
      bind[1].is_unsigned = 1;

      //2 - server_gameport
      bind[2].buffer_type = MYSQL_TYPE_SHORT;
      bind[2].buffer = (void *)&bf2->serverinfo.queryport;
      bind[2].is_unsigned = 1;

      //3 - server_gametype
      bind[3].buffer_type = MYSQL_TYPE_STRING;
      bind[3].buffer = (void *)bf2->serverinfo.gamemode;
      bind[3].buffer_length = strlen(bf2->serverinfo.gamemode);

      //4 - server_gameid
      bind[4].buffer_type = MYSQL_TYPE_STRING;
      bind[4].buffer = (void *)bf2->serverinfo.moddir;
      bind[4].buffer_length = strlen(bf2->serverinfo.moddir);

      //5 - server_mapname
      bind[5].buffer_type = MYSQL_TYPE_STRING;
      bind[5].buffer = (void *)bf2->serverinfo.currentmap;
      bind[5].buffer_length = strlen(bf2->serverinfo.currentmap);

      //6 - server_numplayers
      bind[6].buffer_type = MYSQL_TYPE_SHORT;
      bind[6].buffer = (void *)&bf2->serverinfo.numplayers;
      bind[6].is_unsigned = 1;

      //7 - server_maxplayers
      bind[7].buffer_type = MYSQL_TYPE_SHORT;
      bind[7].buffer = (void *)&bf2->serverinfo.maxplayers;
      bind[7].is_unsigned = 1;

      //8 - server_id
      bind[8].buffer_type = MYSQL_TYPE_LONG;
      bind[8].buffer = (void *)&bf2->serverid;
      bind[8].is_unsigned = 1;

      if (!mysql_stmt_bind_param(qry, bind)) {
        if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
          WriteLog("Server Reply - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
        }
      }
#ifdef LOG_ERRORS
    } else {
      WriteLog("Server Reply -  Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
    }

    mysql_stmt_close(qry);
#ifdef LOG_ERRORS
  } else {
    WriteLog("Server Reply - Query failed: %s\n", mysql_error(&bf2->segment->mysql));
#endif
  }

}

void ActionBF2Connect(bf2server_t *bf2) {
#ifdef SUPERDEBUG
  printf("ActionBF2Connect()\n");
#endif

  FreeBF2AllPlayers(&bf2->players);

  SendBF2PBCommands(bf2);
}

void ActionBF2ServerDisconnect(bf2server_t *bf2) {
  MYSQL_STMT *qry;

#ifdef SUPERDEBUG
  printf("ActionBF2ServerDisconnect()\n");
#endif

  if (mysql_ping(&bf2->segment->mysql)) {
    return;
  }
    
  qry = mysql_stmt_init(&bf2->segment->mysql);
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
      bind[0].buffer = (void *)&bf2->serverid;
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
    WriteLog("Server Fail - Query failed: %s\n", mysql_error(&bf2->segment->mysql));
#endif
  }

  qry = mysql_stmt_init(&bf2->segment->mysql);
  if (qry) {
    const char sql_update_player_currentserver[] = {"UPDATE bf2_player SET player_currentserver_id = NULL WHERE player_currentserver_id = ?"};

    if (!mysql_stmt_prepare(qry, sql_update_player_currentserver, CHARSIZE(sql_update_player_currentserver))) {
      MYSQL_BIND bind[1];
      memset(&bind, 0, sizeof(bind));

      //server_id
      bind[0].buffer_type = MYSQL_TYPE_LONG;
      bind[0].buffer = (void *)&bf2->serverid;
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
    WriteLog("Server Fail - Query failed: %s\n", mysql_error(&bf2->segment->mysql));
#endif
  }
}

void ActionBF2UpdatePlayer(bf2server_t *bf2, bf2player_t *player) {
  bf2player_t *pl;
  int change = 0;

  assert(player);
  if (!player->name[0] || !player->pid[0] || !player->hash[0] || !player->address[0]) {
#ifdef SUPERDEBUG
    printf("ActionBF2UpdatePlayer() [%s] Missing details, abort: [%s] [%s] [%s] [%s]\n", bf2->group, player->name, player->pid, player->hash, player->address);
#endif
    return;
  }

  pl = FindBF2PlayerByName(bf2, player->name);
  if (!pl) {
    pl = NewBF2Player(&bf2->players);
    if (!pl) {
      return;
    }
#ifdef SUPERDEBUG
    printf("ActionBF2UpdatePlayer() [%s] New player: (%d)%s [%s / %s / %s]\n", bf2->group, player->id, player->name, player->pid, player->address, player->hash);
#endif

    change = 1;
    pl->flags |= _BF2_PLAYER_JOIN;

  } else {
    if ((pl->id != player->id) || strcmp(pl->hash, player->hash) || strcmp(pl->address, player->address) || strcmp(pl->pid, player->pid)) {
#ifdef SUPERDEBUG
      printf("ActionBF2UpdatePlayer() [%s] PLAYER %s POLYMORPHED! %d=%d %s=%s %s=%s %s=%s\n", bf2->group, player->name, pl->id, player->id, pl->hash, player->hash, pl->address, player->address, pl->pid, player->pid);
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
    strlcpy(pl->pid, player->pid, sizeof(pl->pid));
  }
}

//22:19 <immorta|_> bulk insert = insert into table (col1, col2) values (val1, val1), (val1, val1), (val1, val1);
void ActionBF2UpdateAllPlayers(bf2server_t *bf2) {
#ifdef SUPERDEBUG
  printf("ActionBF2UpdateAllPlayers(): [%s] %d total players\n", bf2->group, bf2->serverinfo.numplayers);
#endif

  if (bf2->players) {
    mysqlquery_t query;
    bf2player_t *pl, *n;
    int missing_player_ids = 0;
    int missing_address_ids = 0;
    int missing_hash_ids = 0;
    int joining_players = 0;
    int quitting_players = 0;
    int new_guids = 0;
#ifdef SUPERDEBUG
    int shot_backlog = 0;
#endif

    if (mysql_ping(&bf2->segment->mysql)) {
#ifdef SUPERDEBUG
      printf("ActionBF2UpdateAllPlayers(): [%s] Ping with no pong!\n", bf2->group);
#endif
      return;
    }

    for (pl = bf2->players; pl; pl = pl->next) {
      if (pl->cycle != bf2->playercycle) {
        pl->flags |= _BF2_PLAYER_QUIT;
        quitting_players++;
      }
      if (pl->flags & _BF2_PLAYER_JOIN) {
        joining_players++;
      }
      if (pl->flags & _BF2_PLAYER_NEWGUID) {
        new_guids++;
      }
      if (!pl->playerid) {
        missing_player_ids++;
      }
      if (!pl->addressid) {
        missing_address_ids++;
      }
      if (!pl->keyhashid) {
        missing_hash_ids++;
      }
#ifdef NVEDEXT
      if (!bf2->pbshotplayer && pl->pbss) {
        bf2->pbshotplayer = pl;
      }
#endif
#ifdef SUPERDEBUG
      if (pl->pbss) {
        shot_backlog++;
      }
#endif
    }

#ifdef SUPERDEBUG
    printf("ActionBF2UpdateAllPlayers(): [%s] PBShot backlog is: %d\n", bf2->group, shot_backlog);
#endif

    //lets get us some player ids
    if (missing_player_ids > 0) {
#define _BF2_SQL_SELECT_PLAYER_OTHER_QMARK 1
      const char select_player_sql_prefix[] = {"SELECT player_id FROM bf2_player WHERE player_name = ?"}; 
      const char select_player_sql_body[] = {""};
      const char select_player_sql_suffix[] = {""};
      const char select_player_sql_separator[] = {""};
      select_bf2_player_store_t select_player_store;

      select_player_store.bf2 = bf2;
      select_player_store.total_missing = 0;

      query.mysql = &bf2->segment->mysql;
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
      query.other_qmark = _BF2_SQL_SELECT_PLAYER_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectBF2Player", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = bf2->players; pl; pl = pl->next) {
          if (pl->playerid) {
            continue;
          }

          select_player_store.player = pl; //keep this here
          SelectBF2PlayerBindParam(&query, &select_player_store);
          if (RunMySQLQuery(&query)) {
            SelectBF2PlayerInitBindResult(&query, &select_player_store);
            if (ResultMySQLQuery(&query)) {
              SelectBF2PlayerProcessResult(&query, &select_player_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionBF2UpdateAllPlayers(): [%s] Missing %d player ids\n", bf2->group, select_player_store.total_missing);
#endif

      //insert missing players
      if (select_player_store.total_missing > 0) {
#define _BF2_SQL_INSERT_PLAYER_QMARK 3
        const char insert_player_sql_prefix[] = {"INSERT INTO bf2_player (player_id, player_name, player_pid, player_lastserver, player_lastseen) VALUES "}; 
        const char insert_player_sql_body[] = {"(0, ?, ?, ?, CURRENT_TIMESTAMP())"};
        const char insert_player_sql_suffix[] = {""};
        const char insert_player_sql_separator[] = {", "};
        bf2_generic_store_t insert_player_store;

        insert_player_store.bf2 = bf2;

        query.mysql = &bf2->segment->mysql;
        query.sql_prefix = insert_player_sql_prefix;
        query.sql_body = insert_player_sql_body;
        query.sql_separator = insert_player_sql_separator;
        query.sql_suffix = insert_player_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_player_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_player_sql_body);
        query.sql_separator_len = CHARSIZE(insert_player_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_player_sql_suffix);
        query.body_count = select_player_store.total_missing;
        query.qmark = _BF2_SQL_INSERT_PLAYER_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertBF2Players", bf2->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertBF2PlayersBindParam(&query, &insert_player_store);
          if (RunMySQLQuery(&query)) {
            InsertBF2PlayersProcessResult(&query, &insert_player_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] Got all player ids\n", bf2->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = bf2->players; pl; pl = pl->next) {
      if (!pl->playerid) {
        printf("ActionBF2UpdateAllPlayers(): [%s] WTF?! player %s is missing an id!\n", bf2->group, pl->name);
      }
    }
#endif

    //by now, all players should have an id

    //lets get us some address ids
    if (missing_address_ids > 0) {
#define _BF2_SQL_SELECT_ADDRESS_OTHER_QMARK 2
      const char select_address_sql_prefix[] = {"SELECT address_id FROM bf2_address WHERE address_player_id = ? AND address_netip = ?"}; 
      const char select_address_sql_body[] = {""};
      const char select_address_sql_suffix[] = {""};
      const char select_address_sql_separator[] = {""};
      select_bf2_address_store_t select_address_store;

      select_address_store.bf2 = bf2;
      select_address_store.total_missing = 0;

      query.mysql = &bf2->segment->mysql;
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
      query.other_qmark = _BF2_SQL_SELECT_ADDRESS_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectBF2Addresses", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = bf2->players; pl; pl = pl->next) {
          if (!pl->playerid || pl->addressid) {
            continue;
          }
          if (pl->flags & _BF2_PLAYER_INSERTED) { //new player, don't bother selecting a row that will never exist
#ifdef SUPERDEBUG
            //printf("ActionBF2UpdateAllPlayers(): [%s] Skipping address select for %s (they\'re new)\n", bf2->group, pl->name);
#endif
            select_address_store.total_missing++;
            continue;
          }

          select_address_store.player = pl; //keep this here
          SelectBF2AddressBindParam(&query, &select_address_store);
          if (RunMySQLQuery(&query)) {
            SelectBF2AddressInitBindResult(&query, &select_address_store);
            if (ResultMySQLQuery(&query)) {
              SelectBF2AddressProcessResult(&query, &select_address_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionBF2UpdateAllPlayers(): [%s] Missing %d address ids\n", bf2->group, select_address_store.total_missing);
#endif

      if (select_address_store.total_missing > 0) {
#define _BF2_SQL_INSERT_ADDRESS_QMARK 2
        const char insert_address_sql_prefix[] = {"INSERT INTO bf2_address (address_id, address_player_id, address_netip, address_lastseen) VALUES "}; 
        const char insert_address_sql_body[] = {"(0, ?, ?, CURRENT_TIMESTAMP())"}; //if this changes, change that define about
        const char insert_address_sql_suffix[] = {""};
        const char insert_address_sql_separator[] = {", "};
        insert_bf2_address_store_t insert_address_store;

        insert_address_store.bf2 = bf2;

        query.mysql = &bf2->segment->mysql;
        query.sql_prefix = insert_address_sql_prefix;
        query.sql_body = insert_address_sql_body;
        query.sql_separator = insert_address_sql_separator;
        query.sql_suffix = insert_address_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_address_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_address_sql_body);
        query.sql_separator_len = CHARSIZE(insert_address_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_address_sql_suffix);
        query.body_count = select_address_store.total_missing;
        query.qmark = _BF2_SQL_INSERT_ADDRESS_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertBF2Addresses", bf2->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertBF2AddressBindParam(&query, &insert_address_store);
          if (RunMySQLQuery(&query)) {
            InsertBF2AddressProcessResult(&query, &insert_address_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] Got all player address ids\n", bf2->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = bf2->players; pl; pl = pl->next) {
      if (!pl->addressid) {
        printf("ActionBF2UpdateAllPlayers(): [%s] WTF?! player %s is missing their address id!\n", bf2->group, pl->name);
      }
    }
#endif

    //lets get us some key hash ids
    if (missing_hash_ids > 0) {
#define _BF2_SQL_SELECT_KEYHASH_OTHER_QMARK 2
      const char select_keyhash_sql_prefix[] = {"SELECT keyhash_id, keyhash_guid FROM bf2_keyhash WHERE keyhash_player_id = ? AND keyhash_hash = ?"}; 
      const char select_keyhash_sql_body[] = {""};
      const char select_keyhash_sql_suffix[] = {""};
      const char select_keyhash_sql_separator[] = {""};
      select_bf2_keyhash_store_t select_keyhash_store;

      select_keyhash_store.bf2 = bf2;
      select_keyhash_store.total_missing = 0;

      query.mysql = &bf2->segment->mysql;
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
      query.other_qmark = _BF2_SQL_SELECT_KEYHASH_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:SelectBF2Keyhashes", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = bf2->players; pl; pl = pl->next) {
          if (!pl->playerid || pl->keyhashid) {
            continue;
          }
          if (pl->flags & _BF2_PLAYER_INSERTED) {
#ifdef SUPERDEBUG
            //printf("ActionBF2UpdateAllPlayers(): [%s] Skipping keyhash select for %s (they\'re new)\n", bf2->group, pl->name);
#endif
            select_keyhash_store.total_missing++;
            continue;
          }

          select_keyhash_store.player = pl; //keep this here
          SelectBF2KeyhashBindParam(&query, &select_keyhash_store);
          if (RunMySQLQuery(&query)) {
            SelectBF2KeyhashInitBindResult(&query, &select_keyhash_store);
            if (ResultMySQLQuery(&query)) {
              SelectBF2KeyhashProcessResult(&query, &select_keyhash_store);
            }
          }
        }
        CloseMySQLQuery(&query);
      }

#ifdef SUPERDEBUG
      printf("ActionBF2UpdateAllPlayers(): [%s] Missing %d keyhashes ids\n", bf2->group, select_keyhash_store.total_missing);
#endif

      if (select_keyhash_store.total_missing > 0) {
        const char insert_keyhash_sql_prefix[] = {"INSERT INTO bf2_keyhash (keyhash_id, keyhash_player_id, keyhash_hash, keyhash_lastseen) VALUES "}; 
#define _BF2_SQL_INSERT_KEYHASH_QMARK 2
        const char insert_keyhash_sql_body[] = {"(0, ?, ?, CURRENT_TIMESTAMP())"}; //if this changes, change that define about
        const char insert_keyhash_sql_suffix[] = {""};
        const char insert_keyhash_sql_separator[] = {", "};
        bf2_generic_store_t insert_keyhash_store;

        insert_keyhash_store.bf2 = bf2;

        query.mysql = &bf2->segment->mysql;
        query.sql_prefix = insert_keyhash_sql_prefix;
        query.sql_body = insert_keyhash_sql_body;
        query.sql_separator = insert_keyhash_sql_separator;
        query.sql_suffix = insert_keyhash_sql_suffix;
        query.sql_prefix_len = CHARSIZE(insert_keyhash_sql_prefix);
        query.sql_body_len = CHARSIZE(insert_keyhash_sql_body);
        query.sql_separator_len = CHARSIZE(insert_keyhash_sql_separator);
        query.sql_suffix_len = CHARSIZE(insert_keyhash_sql_suffix);
        query.body_count = select_keyhash_store.total_missing;
        query.qmark = _BF2_SQL_INSERT_KEYHASH_QMARK;
        query.other_qmark = 0;

#ifdef SUPERDEBUG
        _strnprintf(query.what, sizeof(query.what), "%s:InsertBF2Keyhashes", bf2->group);
#endif
        if (OpenMySQLQuery(&query)) {
          InsertBF2KeyhashBindParam(&query, &insert_keyhash_store);
          if (RunMySQLQuery(&query)) {
            InsertBF2KeyhashProcessResult(&query, &insert_keyhash_store);
          }
          CloseMySQLQuery(&query);
        }
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] Got all player keyhash ids\n", bf2->group);
#endif
    }

#ifdef SUPERDEBUG
    for (pl = bf2->players; pl; pl = pl->next) {
      if (!pl->keyhashid) {
        printf("ActionBF2UpdateAllPlayers(): [%s] WTF?! player %s is missing their keyhash id!\n", bf2->group, pl->name);
      }
    }
#endif

    //now that everyone has their details, update the players on server table and deal with join and quits

    //joining players
    if (joining_players > 0) {
#define _BF2_SQL_UPDATE_JOIN_PLAYERS_QMARK 1
#define _BF2_SQL_UPDATE_JOIN_PLAYERS_OTHER_QMARK 2
      const char update_joinplayer_sql_prefix[] = {"UPDATE bf2_player SET player_currentserver_id = ?, player_lastserver = ?, player_lastseen = CURRENT_TIMESTAMP() WHERE player_id IN ("}; 
      const char update_joinplayer_sql_body[] = {"?"};
      const char update_joinplayer_sql_suffix[] = {")"};
      const char update_joinplayer_sql_separator[] = {", "};
      bf2_generic_store_t update_joinplayer_store;

      update_joinplayer_store.bf2 = bf2;

      query.mysql = &bf2->segment->mysql;
      query.sql_prefix = update_joinplayer_sql_prefix;
      query.sql_body = update_joinplayer_sql_body;
      query.sql_separator = update_joinplayer_sql_separator;
      query.sql_suffix = update_joinplayer_sql_suffix;
      query.sql_prefix_len = CHARSIZE(update_joinplayer_sql_prefix);
      query.sql_body_len = CHARSIZE(update_joinplayer_sql_body);
      query.sql_separator_len = CHARSIZE(update_joinplayer_sql_separator);
      query.sql_suffix_len = CHARSIZE(update_joinplayer_sql_suffix);
      query.body_count = joining_players;
      query.qmark = _BF2_SQL_UPDATE_JOIN_PLAYERS_QMARK;
      query.other_qmark = _BF2_SQL_UPDATE_JOIN_PLAYERS_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:InsertJoinPlayer", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        UpdateBF2JoinPlayerBindParam(&query, &update_joinplayer_store);
        RunMySQLQuery(&query);
        CloseMySQLQuery(&query);
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] No one joined\n", bf2->group);
#endif
    }

    //parting players
    if (quitting_players > 0) {
#define _BF2_SQL_UPDATE_QUIT_PLAYERS_QMARK 1
      const char update_quitplayer_sql_prefix[] = {"UPDATE bf2_player SET player_currentserver_id = NULL, player_lastseen = CURRENT_TIMESTAMP() WHERE player_id IN ("}; 
      const char update_quitplayer_sql_body[] = {"?"};
      const char update_quitplayer_sql_suffix[] = {")"};
      const char update_quitplayer_sql_separator[] = {", "};
      bf2_generic_store_t update_quitplayer_store;

      update_quitplayer_store.bf2 = bf2;

      query.mysql = &bf2->segment->mysql;
      query.sql_prefix = update_quitplayer_sql_prefix;
      query.sql_body = update_quitplayer_sql_body;
      query.sql_separator = update_quitplayer_sql_separator;
      query.sql_suffix = update_quitplayer_sql_suffix;
      query.sql_prefix_len = CHARSIZE(update_quitplayer_sql_prefix);
      query.sql_body_len = CHARSIZE(update_quitplayer_sql_body);
      query.sql_separator_len = CHARSIZE(update_quitplayer_sql_separator);
      query.sql_suffix_len = CHARSIZE(update_quitplayer_sql_suffix);
      query.body_count = quitting_players;
      query.qmark = _BF2_SQL_UPDATE_QUIT_PLAYERS_QMARK;
      query.other_qmark = 0;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:DeletePartPlayer", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        UpdateBF2QuitPlayerBindParam(&query, &update_quitplayer_store);
        RunMySQLQuery(&query);
        CloseMySQLQuery(&query);
      }
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] No one quit\n", bf2->group);
#endif
    }

    //new guids for a hash
    if (new_guids > 0) {
#define _BF2_SQL_UPDATE_KEYHASH_GUID_OTHER_QMARK 2
      const char update_keyhash_guid_sql_prefix[] = {"UPDATE bf2_keyhash SET keyhash_guid = ? WHERE keyhash_id = ?"};
      const char update_keyhash_guid_sql_body[] = {""};
      const char update_keyhash_guid_sql_suffix[] = {""};
      const char update_keyhash_guid_sql_separator[] = {""};
      update_bf2_keyhash_guid_store_t update_keyhash_guid_store;

      update_keyhash_guid_store.bf2 = bf2;

      query.mysql = &bf2->segment->mysql;
      query.sql_prefix = update_keyhash_guid_sql_prefix;
      query.sql_body = update_keyhash_guid_sql_body;
      query.sql_separator = update_keyhash_guid_sql_separator;
      query.sql_suffix = update_keyhash_guid_sql_suffix;
      query.sql_prefix_len = CHARSIZE(update_keyhash_guid_sql_prefix);
      query.sql_body_len = CHARSIZE(update_keyhash_guid_sql_body);
      query.sql_separator_len = CHARSIZE(update_keyhash_guid_sql_separator);
      query.sql_suffix_len = CHARSIZE(update_keyhash_guid_sql_suffix);
      query.body_count = 0;
      query.qmark = 0;
      query.other_qmark = _BF2_SQL_UPDATE_KEYHASH_GUID_OTHER_QMARK;

#ifdef SUPERDEBUG
      _strnprintf(query.what, sizeof(query.what), "%s:UpdateKeyhashGuids", bf2->group);
#endif

      if (OpenMySQLQuery(&query)) {
        for (pl = bf2->players; pl; pl = pl->next) {
          if (!pl->playerid || !pl->keyhashid || (!(pl->flags & _BF2_PLAYER_NEWGUID))) {
            continue;
          }

          update_keyhash_guid_store.player = pl;
          UpdateBF2KeyhashGuidBindParam(&query, &update_keyhash_guid_store);
          RunMySQLQuery(&query);
        }
        CloseMySQLQuery(&query);
      }
#ifdef SUPERDEBUG
      printf("ActionBF2UpdateAllPlayers(): [%s] Updated %d keyhashes with guids\n", bf2->group, new_guids);
#endif
#ifdef SUPERDEBUG
    } else {
      printf("ActionBF2UpdateAllPlayers(): [%s] No new guids\n", bf2->group);
#endif
    }

    //remove players that quit
    n = bf2->players;
    while (n) {
      pl = n;
      n = pl->next;

      pl->flags &= ~_BF2_PLAYER_JOIN;
      pl->flags &= ~_BF2_PLAYER_INSERTED;
      pl->flags &= ~_BF2_PLAYER_NEWGUID;

      if (pl->flags & _BF2_PLAYER_QUIT) {
#ifdef NVEDEXT
        if (pl->pbss && (bf2->pbshotplayer == pl)) {
#ifdef SUPERDEBUG
          printf("ActionBF2UpdateAllPlayers(): [%s] Discarding player %s\'s shot\n", bf2->group, pl->name);
#endif
          bf2->pbshotplayer = NULL;
        }
#endif
#ifdef SUPERDEBUG
        printf("ActionBF2UpdateAllPlayers(): [%s] player (%d)%s left\n", bf2->group, pl->id, pl->name);
#endif
        FreeBF2Player(pl, &bf2->players);
      }
    }
  }
}

// ==================== Select Players ====================
void SelectBF2PlayerBindParam(mysqlquery_t *query, select_bf2_player_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectBF2PlayerBindParam(): player is %s (%s)\n", store->player->name, store->player->address);
#endif

  //0 - player_name
  query->bindparam[0].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[0].buffer = (void *)store->player->name;
  query->bindparam[0].buffer_length = strlen(store->player->name);

  store->total_missing++;
}

void SelectBF2PlayerInitBindResult(mysqlquery_t *query, select_bf2_player_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //player_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->player_id;
  store->bindres[0].is_null = &store->is_null;
  store->bindres[0].is_unsigned = 1;

  query->bindres = store->bindres;
}

void SelectBF2PlayerProcessResult(mysqlquery_t *query, select_bf2_player_store_t *store) {
  assert(store->player);

  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null) {
#ifdef SUPERDEBUG
      printf("SelectBF2PlayerProcessResult(): [%s] WARNING database returned null for player %s\n", store->bf2->group, store->player->name);
#endif
      return;
    }

    store->player->playerid = store->player_id;
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectBF2PlayerProcessResult(): [%s] Player %s\'s database id: %llu\n", store->bf2->group, store->player->name, store->player->playerid);
#endif
  }
}

// ==================== Insert Players ====================
void InsertBF2PlayersBindParam(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (pl->playerid) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertBF2PlayersBindParam(): WARNING: nearly went off the end of the universe!\n");
      return;
    }
#endif

    //0 - player_name
    query->bindparam[(i * query->qmark) + 0].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 0].buffer = (void *)pl->name;
    query->bindparam[(i * query->qmark) + 0].buffer_length = strlen(pl->name);

    //1 - player_pid
    query->bindparam[(i * query->qmark) + 1].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 1].buffer = (void *)pl->pid;
    query->bindparam[(i * query->qmark) + 1].buffer_length = strlen(pl->pid);

    //2 - player_lastserver
    query->bindparam[(i * query->qmark) + 2].buffer_type = MYSQL_TYPE_STRING;
    query->bindparam[(i * query->qmark) + 2].buffer = (void *)store->bf2->serverinfo.hostname;
    query->bindparam[(i * query->qmark) + 2].buffer_length = strlen(store->bf2->serverinfo.hostname);

    i++;
  }
}

void InsertBF2PlayersProcessResult(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  uint64_t playerid = mysql_stmt_insert_id(query->stmt);
  if (!playerid) {
    return;
  }

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (pl->playerid) {
      continue;
    }

    pl->playerid = playerid;
    playerid++;

    pl->flags |= _BF2_PLAYER_INSERTED;

#ifdef SUPERDEBUG
    printf("InsertBF2PlayersProcessResult(): [%s] Player %s was added, new id is: %llu\n", store->bf2->group, pl->name, pl->playerid);
#endif
  }
}

// ==================== Select Addresses ====================
void SelectBF2AddressBindParam(mysqlquery_t *query, select_bf2_address_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectBF2AddressBindParam(): player is %s (%s)\n", store->player->name, store->player->address);
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

void SelectBF2AddressInitBindResult(mysqlquery_t *query, select_bf2_address_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //address_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->address_id;
  store->bindres[0].is_null = &store->is_null;
  store->bindres[0].is_unsigned = 1;

  query->bindres = store->bindres;
}


void SelectBF2AddressProcessResult(mysqlquery_t *query, select_bf2_address_store_t *store) {
  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null) {
#ifdef SUPERDEBUG
      printf("SelectBF2AddressProcessResult(): [%s] WARNING database returned null for address\n", store->bf2->group);
#endif
      return;
    }

    assert(store->player);

    store->player->addressid = store->address_id;
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectBF2AddressProcessResult(): [%s] Player %s\'s address database id: %llu\n", store->bf2->group, store->player->name, store->player->addressid);
#endif
  }
}

// ==================== Insert Addresses ====================
void InsertBF2AddressBindParam(mysqlquery_t *query, insert_bf2_address_store_t *store) {
  bf2player_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

#ifdef SUPERDEBUG
  //printf("InsertBF2AddressBindParam() [%s]\n", store->bf2->group);
#endif

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->addressid) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertBF2AddressBindParam(): WARNING: nearly went off the end of the universe!\n");
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

void InsertBF2AddressProcessResult(mysqlquery_t *query, insert_bf2_address_store_t *store) {
  bf2player_t *pl;
  uint64_t addressid = mysql_stmt_insert_id(query->stmt);
  if (!addressid) {
    return;
  }

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->addressid) {
      continue;
    }

    pl->addressid = addressid;
    addressid++;

#ifdef SUPERDEBUG
    printf("InsertBF2AddressProcessResult(): [%s] Player %s\'s IP (%s) was added, new id is: %llu\n", store->bf2->group, pl->name, pl->address, pl->addressid);
#endif
  }
}

// ==================== Select Keyhashes ====================
void SelectBF2KeyhashBindParam(mysqlquery_t *query, select_bf2_keyhash_store_t *store) {
  assert(store->player);

#ifdef SUPERDEBUG
  //printf("SelectBF2KeyhashBindParam(): player is %s (%s)\n", store->player->name, store->player->hash);
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

void SelectBF2KeyhashInitBindResult(mysqlquery_t *query, select_bf2_keyhash_store_t *store) {
  memset(store->bindres, '\0', sizeof(store->bindres));

  //keyhash_id
  store->bindres[0].buffer_type = MYSQL_TYPE_LONGLONG;
  store->bindres[0].buffer = (char *)&store->keyhash_id;
  store->bindres[0].is_null = &store->is_null[0];
  store->bindres[0].is_unsigned = 1;

  //keyhash_guid
  store->bindres[1].buffer_type = MYSQL_TYPE_STRING;
  store->bindres[1].buffer = (char *)&store->guid;
  store->bindres[1].buffer_length = sizeof(store->guid);
  store->bindres[1].is_null = &store->is_null[1];

  query->bindres = store->bindres;
}


void SelectBF2KeyhashProcessResult(mysqlquery_t *query, select_bf2_keyhash_store_t *store) {
  assert(store->player);

  if (!mysql_stmt_fetch(query->stmt)) {
    if (store->is_null[0]) {
#ifdef SUPERDEBUG
      printf("SelectBF2KeyhashProcessResult(): [%s] WARNING database returned null for keyhash\n", store->bf2->group);
#endif
      return;
    }

    store->player->keyhashid = store->keyhash_id;
    if (!store->is_null[1]) {
      strlcpy(store->player->guid, store->guid, sizeof(store->player->guid));
#ifdef SUPERDEBUG
      printf("SelectBF2KeyhashProcessResult(): [%s] Not looking for %s\'s guid, already have it [%s]\n", store->bf2->group, store->player->name, store->player->guid);
#endif
    } else {
#ifdef SUPERDEBUG
      printf("SelectBF2KeyhashProcessResult(): [%s] Don\'t know about %s\'s guid yet\n", store->bf2->group, store->player->name);
#endif
    }
    store->total_missing--;

#ifdef SUPERDEBUG
    printf("SelectBF2KeyhashProcessResult(): [%s] Player %s\'s keyhash database id: %llu\n", store->bf2->group, store->player->name, store->player->keyhashid);
#endif
  }
}

// ==================== Insert Keyhashes ====================
void InsertBF2KeyhashBindParam(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark);
#endif

#ifdef SUPERDEBUG
  //printf("InsertBF2KeyhashBindParam() [%s]\n", store->bf2->group);
#endif

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->keyhashid) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("InsertBF2KeyhashBindParam(): WARNING: nearly went off the end of the universe!\n");
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

void InsertBF2KeyhashProcessResult(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  uint64_t keyhashid = mysql_stmt_insert_id(query->stmt);
  if (!keyhashid) {
    return;
  }

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || pl->keyhashid) {
      continue;
    }

    pl->keyhashid = keyhashid;
    keyhashid++;

#ifdef SUPERDEBUG
    printf("InsertBF2KeyhashProcessResult(): [%s] Player %s\'s keyhash (%s) was added, new id is: %llu\n", store->bf2->group, pl->name, pl->hash, pl->keyhashid);
#endif
  }
}

// ==================== Update Server Player (Join) ====================
void UpdateBF2JoinPlayerBindParam(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark) + query->other_qmark;
#endif

#ifdef SUPERDEBUG
  //printf("InsertServerPlayerBindParam() [%s]\n", store->bf2->group);
#endif

  //player_currentserver_id
  query->bindparam[i].buffer_type = MYSQL_TYPE_LONG;
  query->bindparam[i].buffer = (void *)&store->bf2->serverid;
  query->bindparam[i].is_unsigned = 1;
  i++;

  query->bindparam[i].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[i].buffer = (void *)&store->bf2->serverinfo.hostname;
  query->bindparam[i].buffer_length = strlen(store->bf2->serverinfo.hostname);
  i++;

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || (!(pl->flags & _BF2_PLAYER_JOIN))) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("UpdateBF2JoinPlayerBindParam(): WARNING: nearly went off the end of the universe!\n");
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
void UpdateBF2QuitPlayerBindParam(mysqlquery_t *query, bf2_generic_store_t *store) {
  bf2player_t *pl;
  int i = 0;
#ifdef SUPERDEBUG
  int max = (query->body_count * query->qmark) + query->other_qmark;
#endif

#ifdef SUPERDEBUG
  //printf("DeleteServerPlayerBindParam() [%s]\n", store->bf2->group);
#endif

  for (pl = store->bf2->players; pl; pl = pl->next) {
    if (!pl->playerid || (!(pl->flags & _BF2_PLAYER_QUIT))) {
      continue;
    }

#ifdef SUPERDEBUG
    if (i >= max) {
      printf("UpdateBF2QuitPlayerBindParam(): WARNING: nearly went off the end of the universe!\n");
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

// ==================== Update Keyhash to add a Guid ====================
void UpdateBF2KeyhashGuidBindParam(mysqlquery_t *query, update_bf2_keyhash_guid_store_t *store) {
  assert(store->player);
  assert(store->player->guid[0]);
  assert(store->player->keyhashid);

  //keyhash_guid
  query->bindparam[0].buffer_type = MYSQL_TYPE_STRING;
  query->bindparam[0].buffer = (void *)store->player->guid;
  query->bindparam[0].buffer_length = strlen(store->player->guid);

  //keyhash_id
  query->bindparam[1].buffer_type = MYSQL_TYPE_LONGLONG;
  query->bindparam[1].buffer = (void *)&store->player->keyhashid;
  query->bindparam[1].is_unsigned = 1;

#ifdef SUPERDEBUG
  printf("UpdateBF2KeyhashGuidBindParam(): [%s] Player %s\'s guid was found: %s\n", store->bf2->group, store->player->name, store->player->guid);
#endif
}

#ifdef NVEDEXT
void ActionBF2PlayerShotDone(bf2server_t *bf2, int success) {
  bf2player_t *pl = bf2->pbshotplayer;

  if (!pl || !pl->pbss) {
#ifdef SUPERDEBUG
    printf("ActionBF2PlayerShotDone(): %s I don\'t know what to do!\n", bf2->group);
#endif
    return;
  }

#ifdef SUPERDEBUG
  printf("ActionBF2PlayerShotDone(): %s shot \"%s\" (%s) %s\n", bf2->group, pl->pbss->shotfilename, pl->pbss->filename, success ? "successful" : "failed");
#endif

  if (bf2->pbshot) {
    fclose(bf2->pbshot);
  }
  bf2->pbshot = NULL;
  bf2->pbshotrecv = 0;
  bf2->pbshotsize = 0;

  if (success) {
    char *p;
    int i;

    if (ne.pbscan_command) {
#ifdef WIN32
      HANDLE pbscan;
#endif
#ifdef UNIX
      pthread_t pbscan;
      pthread_attr_t attr;
#endif
      strlcpy(bf2->lastshotname, pl->pbss->shotfilename, sizeof(bf2->lastshotname));

#ifdef WIN32
      pbscan = (HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)RunPBScan, (void *)bf2, 0, &i);
      if (!pbscan) {
#endif
#ifdef UNIX
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //yeah detached
      if ((i = pthread_create(&pbscan, &attr, &RunPBScan, (void *)bf2))) {
#endif
#ifdef SUPERDEBUG
        printf("ActionBF2PlayerShotDone(): Failed to create pbscan thread: %d\n", i);
#endif
      }
#ifdef UNIX
      pthread_attr_destroy(&attr);
#endif
    }

    p = pl->pbss->shotfilename;
    i = strlen(p);

    if ((i > 4) && (p[i - 4] == '.')) {
      FILE *fp;

      p[i - 3] = 't';
      p[i - 2] = 'x';
      p[i - 1] = 't';

      fp = fopen(p, "wt");
      if (fp) {
        fprintf(fp, "Player: %s\n", pl->name);
        fprintf(fp, "Address: %s\n", pl->address);
        fprintf(fp, "Hash: %s\n", pl->hash);
        fprintf(fp, "GUID: %s\n", pl->guid);
        fprintf(fp, "Image MD5: %s\n", pl->pbss->md5);
        fprintf(fp, "PB File: %s\n", pl->pbss->filename);
        fclose(fp);
#ifdef SUPERDEBUG
      } else {
        printf("ActionBF2PlayerShotDone(): WARNING: failed to open %s\n", p);
#endif
      }
    }
  }

  bf2->pbshotplayer = NULL;
  _free_(pl->pbss);
  pl->pbss = NULL;
}
#endif

#ifdef NVEDEXT
static void ProcessBF2PlayerShots(bf2server_t *bf2) {
  if (bf2->pbshotplayer) {
    bf2player_t *pl = bf2->pbshotplayer;
    double d = DoubleTime();

    if (bf2->pbshot) {
      if ((d - pl->pbss->shottime) >= TIME_FILE_TIMEOUT) {
        ActionBF2PlayerShotDone(bf2, 0);
      }

    } else {
      char buf[_BF2_MAX_PLAYER_NAME+1];
      char *p;
      struct tm *t;
	    time_t long_time;

      time(&long_time);
	    t = localtime(&long_time);

      strlcpy(buf, pl->name, sizeof(buf));
      p = buf;
      while (*p) {
        if (!isalpha(*p) && !isdigit(*p)) {
          *p = '_';
        }
        p++;
      }

      _strnprintf(pl->pbss->shotfilename, sizeof(pl->pbss->shotfilename), "%s%s%s/%04d%02d%02d_%02d%02d-%s.png",
          ne.wdir, ne.ss_dir, bf2->dirgroup,
          t ? (t->tm_year + 1900) : 0,
          t ? (t->tm_mon + 1) : 0,
          t ? (t->tm_mday) : 0,
          t ? (t->tm_hour) : 0,
          t ? (t->tm_min) : 0,
          buf
          );

      bf2->pbshot = fopen(pl->pbss->shotfilename, "wb");
      if (!bf2->pbshot) {
#ifdef LOG_ERRORS
        WriteLog("Failed to open shot: %s\n", pl->pbss->shotfilename);
#endif
        return;
      }

      pl->pbss->shottime = d;
      RequestBF2ServerFile(bf2, pl->pbss->filename, 0);
    }
  }
}
#endif

#ifdef NVEDEXT
static void *RunPBScan(void *param) {
  bf2server_t *bf2 = (bf2server_t *)param;
  char buf[1024];

#ifdef SUPERDEBUG
  printf("RunPBScan(): %s\n", bf2->lastshotname);
#endif

  if (bf2->lastshotname[0]) {
    _strnprintf(buf, sizeof(buf), "%s%s \"%s\" \"%s\" \"%s\" \"%s\"",
        ne.wdir, ne.pbscan_command,
        ne.wdir, ne.ss_dir, bf2->dirgroup, bf2->lastshotname
        );

#ifdef SUPERDEBUG
    //printf("RunPBScan(): %s System: [%s]\n", bf2->lastshotname, buf);
#endif

    if (system(buf) == -1) {
#ifdef SUPERDEBUG
      printf("RunPBScan() %s FAILED!\n", bf2->lastshotname);
#endif
#ifdef LOG_ERRORS
      WriteLog("Failed to run pbscan: %d\n",
#ifdef WIN32
        GetLastError()
#endif
#ifdef UNIX
        errno
#endif
        );
#endif
    }
  }

#ifdef WIN32
  _endthread();
#endif
#ifdef UNIX
  pthread_exit(NULL);
#endif
  return 0;
}
#endif

static void RequestBF2Players(bf2server_t *bf2) {
  double t = DoubleTime();

	if ((t - bf2->lastreqplayers) >= _BF2_TIME_PLAYERS_UPDATE) {
    bf2->lastreqplayers = t;

    RequestBF2PlayersFull(bf2);
  }
}

#ifdef BF2_REQUESTCHAT
static void RequestBF2Chat(bf2server_t *bf2) {
  double t = DoubleTime();

	if ((t - bf2->lastreqchat) >= _BF2_TIME_CHAT_UPDATE) {
    bf2->lastreqchat = t;

    AddRequest(bf2, _BF2_PACKET_SERVERCHAT);
    SendBF2Socket(bf2, "\x02" "bf2cc clientchatbuffer" "\n", -1); 
	}
}
#endif

#ifdef NVEDEXT
int InitBF2Screens(bf2server_t *bf2) {
  char buf[MAX_PATH+1];
  char *p;

  if (!ne.ss_dir) {
    return 1;
  }

  strlcpy(bf2->dirgroup, bf2->group, sizeof(bf2->dirgroup));

  p = bf2->dirgroup;
  while (*p) {
    if (!isdigit(*p) && !isalpha(*p)) {
      *p = '_';
    }
    p++;
  }

  _strnprintf(buf, sizeof(buf), "%s%s%s", ne.wdir, ne.ss_dir, bf2->dirgroup);
#ifdef WIN32
  if (mkdir(buf) == -1) {
#endif
#ifdef UNIX
  if (mkdir(buf, UNIX_NEWDIR) == -1) {
#endif
    if (errno != EEXIST) {
      WriteLog("Failed to create directory: %s\n", buf);
      return 0;
    }
  }

  return 1;
}
#endif

bf2segment_t *NewBF2Segment(const int segcount) {
  bf2segment_t *bf2seg;
  struct sockaddr_in addr;
  const int pbport = (ne.start_port + segcount);

#ifdef SUPERDEBUG
  printf("NewBF2Segment()\n");
#endif

  bf2seg = (bf2segment_t *)_malloc_(sizeof(bf2segment_t));
  if (!bf2seg) {
    return NULL;
  }

  memset(&(addr.sin_zero), '\0', sizeof(addr.sin_zero));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ne.internal_address);
  addr.sin_port = htons((unsigned short)pbport);
  bf2seg->pbsock = INVALID_SOCKET;

  if (!InitUDPSocket(&bf2seg->pbsock, &addr)) {
    _free_(bf2seg);
#ifdef WIN32
    WriteLog("Failed to init socket: %d\n", WSAGetLastError());
#endif
#ifdef UNIX
    WriteLog("Failed to init socket: %d\n", errno);
#endif
    return NULL;
  }

  if (!OpenMySQL(&bf2seg->mysql)) {
    CloseSocket(&bf2seg->pbsock);
    _free_(bf2seg);
    return NULL;
  }

  _initmutex(&bf2seg->mutex);
  bf2seg->bf2servers = NULL;
  bf2seg->thread = 0;
  bf2seg->active = 1;
  bf2seg->pbsteamport = pbport;
  _strnprintf(bf2seg->name, sizeof(bf2seg->name), "seg%d", segcount);
#ifdef SUPERDEBUG
  bf2seg->srvlist[0] = '\0';
#endif

  bf2seg->next = bf2segments;
  bf2segments = bf2seg;

#ifdef SUPERDEBUG
  printf("NewBF2Segment(): PBListen: %s:%d(%d)\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), pbport);
#endif

  return bf2seg;
}

#ifdef SHOW_STATUS
void ShowBF2Status(void) {
  bf2segment_t *bf2seg;
  bf2server_t *bf2;
  char state[128];
  char srv[128];
  double t;

  t = DoubleTime();

  for (bf2seg = bf2segments; bf2seg; bf2seg = bf2seg->next) {
    _lockmutex(&bf2seg->mutex);

    for (bf2 = bf2seg->bf2servers; bf2; bf2 = bf2->next) {
      srv[0] = '\0';

      switch (bf2->state) {
        case SOCKETSTATE_FAILEDCONNECT: strlcpy(state, "FailedConnect", sizeof(state)); break;
        case SOCKETSTATE_FREE: strlcpy(state, "Free", sizeof(state)); break;
        case SOCKETSTATE_DISCONNECTED: strlcpy(state, "Disconnected", sizeof(state)); break;
        case SOCKETSTATE_BEGINCONNECT: strlcpy(state, "BeginConnect", sizeof(state)); break;
        case SOCKETSTATE_SOCKCONNECTED: strlcpy(state, "SockConnected", sizeof(state)); break;
        case SOCKETSTATE_CONNECTED: strlcpy(state, "Connected", sizeof(state)); break;
        default: strlcpy(state, "Unknown", sizeof(state)); break;
      }

      if (bf2->state == SOCKETSTATE_CONNECTED) {
        _strnprintf(srv, sizeof(srv), "\n    Map:%s (%s) Players:%d/%d Next:%0.2f"
#ifdef NVEDEXT
            " NVE:%d"
#endif
            , //yep there's a comma on this line
            bf2->serverinfo.currentmap,
            bf2->serverinfo.nextmap[0] ? bf2->serverinfo.nextmap : "none",
            bf2->serverinfo.numplayers,
            bf2->serverinfo.maxplayers,
            (_BF2_TIME_PLAYERS_UPDATE - (t - bf2->lastreqplayers))
#ifdef NVEDEXT
            ,bf2->nvedext
#endif
            );
      } else if (bf2->state == SOCKETSTATE_FAILEDCONNECT) {
        _strnprintf(srv, sizeof(srv), " Attempt:%0.2f",
            bf2->lastconnect - t
            );
      }

      WriteLog("-%s- %s:%d State:%s%s\n",
          bf2->group,
          inet_ntoa(bf2->server.sin_addr),
          ntohs(bf2->server.sin_port),
          state,
          srv
        );
    }

    _unlockmutex(&bf2seg->mutex);
  }
}
#endif

