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

#ifndef _H_BF2
#define _H_BF2

#ifdef WIN32
#include <direct.h>
#endif

#include <pcre.h>

#include "global.h"

#include "bf2rcon.h"
#include "sqlquery.h"

#define _BF2_PBSTREAM_HEADER "LEBRep \"(.+)\" \\w{32} \\|\\d+\\| (.+)"
#define _BF2_PBSTREAM_PLAYER_PATTERN "Player GUID Computed (\\w{31,32})\\(-\\) \\(slot #(\\d+)\\) ((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)):\\d+ (.+)"
#ifdef NVEDEXT
#define _BF2_PBSTREAM_AUTOSHOT_PATTERN "Screenshot .+(pb\\d+.png) successfully received \\(MD5=(\\w{32})\\) from \\d+ (.+) \\[(\\w{32})\\(-\\) ((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)):\\d+\\]"
//dummy: LEBRep "Test Server" 11111111111111111111111111111111 |32| Screenshot c:\pb0000.png successfully received (MD5=11111111111111111111111111111111) from 32 x [11111111111111111111111111111111(-) 127.0.0.1:69]
//this one: LEBRep "Test Server" 11111111111111111111111111111111 |32| Screenshot c:\pb111111.png successfully received (MD5=11111111111111111111111111111111) from 32 Cheater! [11111111111111111111111111111111(-) 127.0.0.1:69]
#endif

#define _BF2_PBSTREAM_OVECCOUNT 18
#define _BF2_MAX_MESSAGE_SIZE 16384

typedef struct select_bf2_players_store_s {
  bf2server_t *bf2;
  bf2player_t *player;
  MYSQL_BIND bindres[1];
  uint64_t player_id;
  my_bool is_null;
  int total_missing;
} select_bf2_player_store_t;

typedef struct select_bf2_address_store_s {
  bf2server_t *bf2;
  bf2player_t *player;
  MYSQL_BIND bindres[1];
  uint64_t address_id;
  uint64_t player_id;
  my_bool is_null;
  int total_missing;
} select_bf2_address_store_t;

typedef struct insert_bf2_address_store_s {
  bf2server_t *bf2;
} insert_bf2_address_store_t;

typedef struct select_bf2_keyhash_store_s {
  bf2server_t *bf2;
  bf2player_t *player;
  MYSQL_BIND bindres[2];
  uint64_t player_id;
  uint64_t keyhash_id;
  char guid[_BF2_MAX_PLAYER_GUID+1];
  my_bool is_null[2];
  int total_missing;
} select_bf2_keyhash_store_t;

typedef struct update_bf2_keyhash_guid_store_s {
  bf2server_t *bf2;
  bf2player_t *player;
} update_bf2_keyhash_guid_store_t;

typedef struct bf2_generic_store_s {
  bf2server_t *bf2;
} bf2_generic_store_t;

int InitBF2(void);
#ifdef NVEDEXT
int InitBF2Screens(bf2server_t *bf2);
#endif
void FinishBF2(void);

int StartBF2Segments(void);
void StopBF2Segments(void);

bf2segment_t *NewBF2Segment(const int segcount);

static void *ProcessBF2Segments(void *param);

#ifdef NVEDEXT
static void ProcessBF2PlayerShots(bf2server_t *bf2);
#endif
static void ProcessPBStream(bf2segment_t *bf2seg, char *rv, int rvsz);
static void CheckBF2PBStream(bf2server_t *bf2);
static void SaveBF2Transfers(bf2segment_t *bf2seg, int now);
static void SendBF2PBCommands(bf2server_t *bf2);
#ifdef NVEDEXT
static void *RunPBScan(void *param);
#endif

static void RequestBF2Players(bf2server_t *bf2);
#ifdef BF2_REQUESTCHAT
static void RequestBF2Chat(bf2server_t *bf2);
#endif

void SelectBF2PlayerBindParam(mysqlquery_t *query, select_bf2_player_store_t *store);
void SelectBF2PlayerInitBindResult(mysqlquery_t *query, select_bf2_player_store_t *store);
void SelectBF2PlayerProcessResult(mysqlquery_t *query, select_bf2_player_store_t *store);
void InsertBF2PlayersBindParam(mysqlquery_t *query, bf2_generic_store_t *store);
void InsertBF2PlayersProcessResult(mysqlquery_t *query, bf2_generic_store_t *store);

void SelectBF2AddressBindParam(mysqlquery_t *query, select_bf2_address_store_t *store);
void SelectBF2AddressInitBindResult(mysqlquery_t *query, select_bf2_address_store_t *store);
void SelectBF2AddressProcessResult(mysqlquery_t *query, select_bf2_address_store_t *store);
void InsertBF2AddressBindParam(mysqlquery_t *query, insert_bf2_address_store_t *store);
void InsertBF2AddressProcessResult(mysqlquery_t *query, insert_bf2_address_store_t *store);

void SelectBF2KeyhashBindParam(mysqlquery_t *query, select_bf2_keyhash_store_t *store);
void SelectBF2KeyhashInitBindResult(mysqlquery_t *query, select_bf2_keyhash_store_t *store);
void SelectBF2KeyhashProcessResult(mysqlquery_t *query, select_bf2_keyhash_store_t *store);
void InsertBF2KeyhashBindParam(mysqlquery_t *query, bf2_generic_store_t *store);
void InsertBF2KeyhashProcessResult(mysqlquery_t *query, bf2_generic_store_t *store);

void UpdateBF2JoinPlayerBindParam(mysqlquery_t *query, bf2_generic_store_t *store);
void UpdateBF2QuitPlayerBindParam(mysqlquery_t *query, bf2_generic_store_t *store);
void UpdateBF2KeyhashGuidBindParam(mysqlquery_t *query, update_bf2_keyhash_guid_store_t *store);

#ifdef SHOW_STATUS
void DisplayBF2Segments(void);
void ShowBF2Status(void);
#endif

#endif // _H_BF2

