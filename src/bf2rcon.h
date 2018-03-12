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

#ifndef _H_BF2RCON
#define _H_BF2RCON

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

#define _BF2_PACKET_RAW 0
#define _BF2_PACKET_HIDE 1
#define _BF2_PACKET_PRINT 2
#define _BF2_PACKET_WELCOME 3
#define _BF2_PACKET_ISLOGIN 4
#define _BF2_PACKET_LOGGEDIN 5
#define _BF2_PACKET_SIMPLEPLAYERS 6
#define _BF2_PACKET_FULLPLAYERS 7
#define _BF2_PACKET_SERVERINFO 8
#define _BF2_PACKET_SERVERCHAT 9
#define _BF2_PACKET_SETGAMEPORT 10
#define _BF2_PACKET_SETQUERYPORT 11
#define _BF2_PACKET_NVEDEXTVERSION 12

#define _BF2_MAX_QUEUE 20
#define _BF2_MAX_BF2PLAYERS 64

#define _BF2_MAX_ADMIN_NAME 16
#define _BF2_MAX_RCON 32
#define _BF2_MAX_GROUP 16
#define _BF2_MAX_MD5 32
#define _BF2_MAX_VARS 8

#define _BF2_MAX_PLAYERCHAT 6
#define _BF2_MAX_PLAYERPREFIX 5
#define _BF2_MAX_PLAYERCHOP 8
#define _BF2_MAX_SIMPLEPLAYER 5

#define _BF2_SIMPLEPLAYER_ID 0
#define _BF2_SIMPLEPLAYER_NAME 1
#define _BF2_SIMPLEPLAYER_POINT 2
#define _BF2_SIMPLEPLAYER_IP 3
#define _BF2_SIMPLEPLAYER_KEY 4

#define _BF2_SERVERCHAT_PLAYERID 0
#define _BF2_SERVERCHAT_PLAYERNAME 1
#define _BF2_SERVERCHAT_TEAM 2
#define _BF2_SERVERCHAT_CHANNEL 3
#define _BF2_SERVERCHAT_TIME 4
#define _BF2_SERVERCHAT_TEXT 5

#define _BF2_CHARPREFIX_NONE 0
#define _BF2_CHARPREFIX_TEAM 1
#define _BF2_CHARPREFIX_SQUAD 2
#define _BF2_CHARPREFIX_COMMANDER 3
#define _BF2_CHARPREFIX_DEAD 4

#define _BF2_CHANGED_SERVERINFO 1
#define _BF2_CHANGED_FAILCONNECT 2
#define _BF2_CHANGED_PLAYERSFULL 4
#define _BF2_CHANGED_PLAYERCHAT 8

#define _BF2_MAX_TEAM_SHORTNAME 8
#define _BF2_MAX_SCRIPT_VERSION 8
#define _BF2_MAX_MAP_NAME 64
#define _BF2_MAX_HOST_NAME 48
#define _BF2_MAX_GAME_MODE 16
#define _BF2_MAX_MOD_DIR 16
#define _BF2_MAX_WORLD_SIZE 24
#define _BF2_MAX_DIRGROUP 32
#define _BF2_MAX_SEGMENT_NAME 12

#define _BF2_MAX_PLAYER_NAME 64
#define _BF2_MAX_PLAYER_ADDRESS 15
#define _BF2_MAX_PLAYER_HASH 32
#define _BF2_MAX_PLAYER_GUID 32
#define _BF2_MAX_PLAYER_DNS 64
#define _BF2_MAX_PLAYER_PID 16
#define _BF2_MAX_PLAYER_POSITION 32
#define _BF2_MAX_PLAYER_FILENAME 32
#define _BF2_MAX_PLAYER_MD5 32

#define _BF2_TIME_CHAT_UPDATE 20.0
#define _BF2_TIME_PLAYERS_UPDATE 30.0

#define _BF2_DEFAULT_RETRY_DELAY 45
#define _BF2_DEFAULT_ADMIN_NAME "Admin"

#define _BF2_SOCKET_WAIT 15

//NvEdExt
#define _BF2_PROTOCOL_PREFIX "\x0FN"
#define _BF2_PROTOCOL_ACK 'A'
#define _BF2_PROTOCOL_SETEVENTS 'B'
#define _BF2_PROTOCOL_PLAYERCHAT 'C'
#define _BF2_PROTOCOL_PLAYERJOIN 'D'
#define _BF2_PROTOCOL_PLAYERDROP 'E'
#define _BF2_PROTOCOL_MAPCHANGE 'F'
#define _BF2_PROTOCOL_PBSHOT 'G'
#define _BF2_PROTOCOL_PBSHOT_ERROR 'H'
#define _BF2_PROTOCOL_VERSION 'I'

#define _BF2_PLAYER_JOIN 1
#define _BF2_PLAYER_QUIT 2
#define _BF2_PLAYER_INSERTED 4
#define _BF2_PLAYER_NEWGUID 8

typedef struct team_s {
  char name[_BF2_MAX_TEAM_SHORTNAME+1];
  //int players;
  //int state;
  //int starttickets;
  //int tickets;
  //int ticketrate;
} team_t;

#ifdef NVEDEXT
typedef struct bf2pbss_s {
  char filename[_BF2_MAX_PLAYER_FILENAME]; //pbXXXXX.png
  char shotfilename[MAX_PATH+1];
  char md5[_BF2_MAX_PLAYER_MD5+1];
  double shottime;
} bf2pbss_t;
#endif

typedef struct bf2player_s {
  //database
  uint64_t playerid;
  uint64_t addressid;
  uint64_t keyhashid;
  //player
  char name[_BF2_MAX_PLAYER_NAME+1];
  char hash[_BF2_MAX_PLAYER_HASH+1];
  char guid[_BF2_MAX_PLAYER_GUID+1];
  char address[_BF2_MAX_PLAYER_ADDRESS+1];
  char pid[_BF2_MAX_PLAYER_PID+1];
  //char position[_BF2_MAX_PLAYER_POSITION+1];
  int id;
  int flags;
  int cycle;

  /*
  int slot;

  int score;
  int deaths;
  int ping;
  int team;
  int isalive;
  */

#ifdef NVEDEXT
  struct bf2pbss_s *pbss;
#endif

  struct bf2player_s *next;
  struct bf2player_s *prev;
} bf2player_t;

typedef struct bf2serverinfo_s {
  //char scriptver[_BF2_MAX_SCRIPT_VERSION+1];
  char currentmap[_BF2_MAX_MAP_NAME+1];
  char nextmap[_BF2_MAX_MAP_NAME+1];
  char hostname[_BF2_MAX_HOST_NAME+1];
  char gamemode[_BF2_MAX_GAME_MODE+1];
  char moddir[_BF2_MAX_MOD_DIR+1];
  int gameport;
  int queryport;
  //char worldsize[_BF2_MAX_WORLD_SIZE+1];
  //int state;
  int connectedplayers;
  int joiningplayers;
  int numplayers;
  int maxplayers;
  //int maptime;
  //int mapleft;
  //int timelimit;
  //int autobalance;
  int ranked;
  //int querytime;
  //int reserved;
  //int iskarkand;
  struct team_s team1;
  struct team_s team2;
} bf2serverinfo_t;

typedef struct bf2server_s {
  int active;
  int state;
  struct sockaddr_in server;
  int sock;
  char group[_BF2_MAX_GROUP+1];
  char rcon[_BF2_MAX_RCON+1];
  int sentwelcome;
  //int changed;
  double lastconnect;
  double lastreqchat;
  double lastreqplayers;
  double lastmapchange;
  char *block;
  int blocklen;
  unsigned int tcptx; //sent
  unsigned int tcprx; //received
#ifdef NVEDEXT
  int nvedext;
  struct bf2player_s *pbshotplayer;
  FILE *pbshot;
  unsigned int pbshotsize;
  unsigned int pbshotrecv;
  char dirgroup[_BF2_MAX_DIRGROUP+1]; //used in shots dir name
  char lastshotname[MAX_PATH]; //used in scan thread
#endif

  int reqstart;
  int reqend;
  int requests[_BF2_MAX_QUEUE];
  int playercycle;

  char adminname[_BF2_MAX_ADMIN_NAME+1];
  int startdelay;
  int retrydelay;

  struct bf2serverinfo_s serverinfo;

  //database
  unsigned int serverid;
  unsigned short servertype;

  //pb
  int sentpbalive;
  double lastpbstream;

  struct bf2segment_s *segment;
  struct bf2player_s *players;

  struct bf2server_s *next;
} bf2server_t;

typedef struct bf2segment_s {
  MYSQL mysql;
  MUTEX mutex;
  int active;
  int pbsock;
  int pbsteamport;
  bf2server_t *bf2servers;
  char name[_BF2_MAX_SEGMENT_NAME+1];
#ifdef SUPERDEBUG
  char srvlist[256+1];
#endif
#ifdef UNIX
  pthread_t thread;
#endif
#ifdef WIN32
  HANDLE thread;
#endif
  struct bf2segment_s *next;
} bf2segment_t;

bf2server_t *NewBF2Server(unsigned int serverid, unsigned short servertype, char *name, char *addr, int port, char *rcon);
//int BeginBF2Server(bf2server_t *bf2);
//void DestroyBF2Servers(void);
void FreeBF2Server(bf2server_t *bf2);

void InitBF2Player(bf2player_t *pl);
bf2player_t *NewBF2Player(bf2player_t **parent);
void FreeBF2Player(bf2player_t *pl, bf2player_t **parent);
void FreeBF2AllPlayers(bf2player_t **parent);

#ifdef NVEDEXT
bf2pbss_t *NewBF2PBShot(void);
void FreeBF2PBShot(bf2pbss_t **pbss);
#endif

int ProcessBF2Server(bf2server_t *bf2, char *rv, int rvsz);

//user made functions
extern void ActionBF2ServerSuccess(bf2server_t *bf2);
extern void ActionBF2ServerDisconnect(bf2server_t *bf2);
extern void ActionBF2UpdatePlayer(bf2server_t *bf2, bf2player_t *player);
extern void ActionBF2UpdateAllPlayers(bf2server_t *bf2);
extern void ActionBF2Connect(bf2server_t *bf2);
extern void ActionBF2ConnectTimeout(bf2server_t *bf2);
#ifdef NVEDEXT
extern void ActionBF2PlayerShotDone(bf2server_t *bf2, int success);
#endif
//

int ServerBF2Connect(bf2server_t *bf2);
void EndBF2Connection(bf2server_t *bf2);
int ParseBF2(bf2server_t *bf2, char *rv, int sz);
void SendBF2Socket(bf2server_t *bf2, char *data, int sz);

void RequestBF2ServerInfo(bf2server_t *bf2);
void RequestBF2PlayersFull(bf2server_t *bf2);
void RequestBF2ServerChat(bf2server_t *bf2);
#ifdef NVEDEXT
void RequestBF2ServerFile(bf2server_t *bf2, char *name, int pos);
#endif

void AddRequest(bf2server_t *bf2, int type);
void RemoveTopRequest(bf2server_t *bf2);
void RemoveRequests(bf2server_t *bf2);

void InitBF2ServerInfo(bf2serverinfo_t *si);
//void InitBF2Players(bf2serverinfo_t *si);

static int PRWelcome(bf2server_t *bf2, char *rv, int sz);
static int PRServerInfo(bf2server_t *bf2, char *rv, int sz);
#ifdef BF2_REQUESTCHAT
static int PRServerChat(bf2server_t *bf2, char *rv, int sz);
#endif
static int PRPlayersFull(bf2server_t *bf2, char *rv, int sz);
static int PRLoginReply(bf2server_t *bf2, char *rv, int sz);
static int PRSetPort(int *port, char *rv, int sz);
#ifdef NVEDEXT
static void PRSetNvedExt(bf2server_t *bf2, char *rv, int sz);
static void PRProcessFile(bf2server_t *bf2, char *rv, int sz);
#endif

bf2server_t *FindBF2Server(char *group, struct sockaddr_in *a, bf2server_t *head);
bf2player_t *FindBF2PlayerByName(bf2server_t *bf2, char *name);
bf2player_t *FindBF2PlayerById(bf2server_t *bf2, uint64_t id);

void destroy(char *p, char *dec, int sz);

#ifdef _DEBUG
int dump(char *rv, int sz);
#endif

#endif //_H_BF2RCON

