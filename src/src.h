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

#ifndef _H_SRC
#define _H_SRC

#include "global.h"

#include "srcrcon.h"
#include "sqlquery.h"

#define _SRC_MAX_MESSAGE_SIZE 16384
#define _SRC_KEYHASH_PENDING "STEAM_ID_PENDING"

typedef struct select_src_players_store_s {
  srcserver_t *srv;
  srcplayer_t *player;
  MYSQL_BIND bindres[1];
  uint64_t player_id;
  my_bool is_null;
  int total_missing;
} select_src_player_store_t;

typedef struct select_src_address_store_s {
  srcserver_t *srv;
  srcplayer_t *player;
  MYSQL_BIND bindres[1];
  uint64_t address_id;
  uint64_t player_id;
  my_bool is_null;
  int total_missing;
} select_src_address_store_t;

typedef struct insert_src_address_store_s {
  srcserver_t *srv;
} insert_src_address_store_t;

typedef struct select_src_keyhash_store_s {
  srcserver_t *srv;
  srcplayer_t *player;
  MYSQL_BIND bindres[2];
  uint64_t player_id;
  uint64_t keyhash_id;
  my_bool is_null[2];
  int total_missing;
} select_src_keyhash_store_t;

typedef struct src_generic_store_s {
  srcserver_t *srv;
} src_generic_store_t;

int StartSRCSegments(void);
void StopSRCSegments(void);

srcsegment_t *NewSRCSegment(const int segcount);

static void *ProcessSRCSegments(void *param);

static void SaveSRCTransfers(srcsegment_t *srcseg, int now);


static void RequestSRCPlayers(srcserver_t *srv);

void SelectSRCPlayerBindParam(mysqlquery_t *query, select_src_player_store_t *store);
void SelectSRCPlayerInitBindResult(mysqlquery_t *query, select_src_player_store_t *store);
void SelectSRCPlayerProcessResult(mysqlquery_t *query, select_src_player_store_t *store);
void InsertSRCPlayersBindParam(mysqlquery_t *query, src_generic_store_t *store);
void InsertSRCPlayersProcessResult(mysqlquery_t *query, src_generic_store_t *store);

void SelectSRCAddressBindParam(mysqlquery_t *query, select_src_address_store_t *store);
void SelectSRCAddressInitBindResult(mysqlquery_t *query, select_src_address_store_t *store);
void SelectSRCAddressProcessResult(mysqlquery_t *query, select_src_address_store_t *store);
void InsertSRCAddressBindParam(mysqlquery_t *query, insert_src_address_store_t *store);
void InsertSRCAddressProcessResult(mysqlquery_t *query, insert_src_address_store_t *store);

void SelectSRCKeyhashBindParam(mysqlquery_t *query, select_src_keyhash_store_t *store);
void SelectSRCKeyhashInitBindResult(mysqlquery_t *query, select_src_keyhash_store_t *store);
void SelectSRCKeyhashProcessResult(mysqlquery_t *query, select_src_keyhash_store_t *store);
void InsertSRCKeyhashBindParam(mysqlquery_t *query, src_generic_store_t *store);
void InsertSRCKeyhashProcessResult(mysqlquery_t *query, src_generic_store_t *store);

void UpdateSRCJoinPlayerBindParam(mysqlquery_t *query, src_generic_store_t *store);
void UpdateSRCQuitPlayerBindParam(mysqlquery_t *query, src_generic_store_t *store);

#ifdef SHOW_STATUS
void DisplaySRCSegments(void);
#endif

#endif //_H_SRC

