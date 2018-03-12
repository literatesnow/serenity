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

#ifndef _H_SRCRCON
#define _H_SRCRCON

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <process.h>
#endif
#ifdef UNIX
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <my_global.h>
#include <mysql.h>

#include "sockets.h"
#include "util.h"
#include "md5.h"

#ifdef WIN32
#include "MemLeakCheck.h"
#endif

#define _SRC_MAX_PLAYER_NAME 64
#define _SRC_MAX_PLAYER_ADDRESS 15
#define _SRC_MAX_PLAYER_HASH 32

#define _SRC_MAX_SEGMENT_NAME 12
#define _SRC_MAX_GROUP 16
#define _SRC_MAX_RCON 32

#define _SRC_TIME_PLAYERS_UPDATE 30.0

#define _SRC_SOCKET_WAIT 15

#define _SRC_SOCKET_AUTH_ID 10
#define _SRC_SOCKET_UPDATE_STATUS_ID 20
#define _SRC_SOCKET_ERROR_ID -1

#define _SRC_SERVERDATA_AUTH 3
#define _SRC_SERVERDATA_EXECCOMMAND 2
#define _SRC_SERVERDATA_RESPONSE_VALUE 0
#define _SRC_SERVERDATA_AUTH_RESPONSE 2

#define _SRC_MAX_HOST_NAME 48
#define _SRC_MAX_VERSION 40
#define _SRC_MAX_MAP_NAME 64

#define _SRC_PLAYER_JOIN 1
#define _SRC_PLAYER_QUIT 2
#define _SRC_PLAYER_INSERTED 4
#define _SRC_PLAYER_HASH_PENDING 8

typedef struct srcplayer_s {
  //database
  uint64_t playerid;
  uint64_t addressid;
  uint64_t keyhashid;
  //player
  char name[_SRC_MAX_PLAYER_NAME+1];
  char hash[_SRC_MAX_PLAYER_HASH+1];
  char address[_SRC_MAX_PLAYER_ADDRESS+1];
  int id;
  int flags;
  int cycle;

  struct srcplayer_s *next;
  struct srcplayer_s *prev;
} srcplayer_t;

typedef struct srcserverinfo_s {
  char hostname[_SRC_MAX_HOST_NAME+1];
  char version[_SRC_MAX_VERSION+1];
  char currentmap[_SRC_MAX_MAP_NAME+1];
  int numplayers;
  int maxplayers;
} srcserverinfo_t;

typedef struct srcserver_s {
  int active;
  int state;
  struct sockaddr_in server;
  int sock;

  char group[_SRC_MAX_GROUP+1];
  char rcon[_SRC_MAX_RCON+1];

  unsigned int tcptx; //sent
  unsigned int tcprx; //received

  double lastconnect;
  double lastreqplayers;

  int startdelay;
  int retrydelay;

  int sentauth;
  int playercycle;

  struct srcserverinfo_s serverinfo;

  //database
  unsigned int serverid;
  unsigned short servertype;

  struct srcsegment_s *segment;
  struct srcplayer_s *players;

  struct srcserver_s *next;
} srcserver_t;

typedef struct srcsegment_s {
  MYSQL mysql;
  MUTEX mutex;
  int active;
  srcserver_t *srcservers;
  char name[_SRC_MAX_SEGMENT_NAME+1];
#ifdef SUPERDEBUG
  char srvlist[256+1];
#endif
#ifdef UNIX
  pthread_t thread;
#endif
#ifdef WIN32
  HANDLE thread;
#endif
  struct srcsegment_s *next;
} srcsegment_t;

srcserver_t *NewSRCServer(unsigned int serverid, unsigned short servertype, char *name, char *addr, int port, char *rcon);
void FreeSRCServer(srcserver_t *srv);

void InitSRCServerInfo(srcserverinfo_t *si);

void InitSRCPlayer(srcplayer_t *pl);
srcplayer_t *NewSRCPlayer(srcplayer_t **parent);
void FreeSRCPlayer(srcplayer_t *pl, srcplayer_t **parent);
void FreeSRCAllPlayers(srcplayer_t **parent);

int ProcessSRCServer(srcserver_t *srv, char *rv, int rvsz);
int ServerSRCConnect(srcserver_t *srv);
void EndSRCConnection(srcserver_t *srv);

void SendSRCSocket(srcserver_t *srv, int id, int cmd, char *string1, int sz);

static void PRAuthReply(srcserver_t *srv, int cmd, int success);
static void PRStatusReply(srcserver_t *srv, int cmd, char *rv, int sz);

void RequestSRCPlayersFull(srcserver_t *srv);

srcplayer_t *FindSRCPlayerByName(srcserver_t *srv, char *name);

//user made functions
extern void ActionSRCServerSuccess(srcserver_t *srv);
extern void ActionSRCServerDisconnect(srcserver_t *srv);
extern void ActionSRCUpdatePlayer(srcserver_t *srv, srcplayer_t *player);
extern void ActionSRCUpdateAllPlayers(srcserver_t *srv);
extern void ActionSRCConnect(srcserver_t *srv);
extern void ActionSRCConnectTimeout(srcserver_t *srv);
//

#endif //_H_SRCRCON

