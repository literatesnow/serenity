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

#ifdef UNIX
#include <pthread.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#include <direct.h>
#endif
#ifdef USE_MYSQL
#include <mysql.h>
#endif
#include "util.h"
#include "import.h"
#include "graph.h"

#define VERSION "1.019"
#define HEADER_TITLE "Serenity"
#define HEADER_AUTHOR "Chris Cuthbertson"
#define HEADER_EMAIL "blip@zeocen.net"
#define HEADER_COPYRIGHT "Copyright 2004"

#define MAX_NAME 32
#define MAX_KEYHASH 32
#define MAX_IP 16
#define MAX_PORT 5
#define MAX_HOSTNAME 32
#define MAX_MAPNAME 32
#define MAX_GAMETYPE 32
#define MAX_GAMEID 32
#define MAX_RCON 32
#define MAX_KEY 64 //change in rule table too
#define MAX_VALUE 64
#define MAX_QUERY 32
#define MAX_TEAM 6
#define MAX_VERSION 5
#define MAX_RECIEVE 2048
#define MAX_PACKET 16384
#define MAX_FILENAME 256
#define MAX_DATETIME 19
#define MAX_QUERY_LENGTH 32
#define MAX_ESCAPE_LENGTH 128
#define MAX_MYSQL_QUERY 2048
#define MAX_MYSQL_DATATBASE 32
#define MAX_MYSQL_USER 16
#define MAX_MYSQL_PASS 16
#define MAX_FILE_PATH 256
#define MAX_PHP_PARAMETERS 32
#define MAX_PORT_RETRY 500
#define MAX_PROGRAM_RUNTIME 45 //program kills itself if over this time (seconds)
#define MAX_RUNTIME 3 //low, high, last program runtime
#define MAX_PORT_SEARCH 9
#define MAX_PORT_CIELING 65535

#define RUNTIME_LOW 0
#define RUNTIME_HIGH 1
#define RUNTIME_LAST 2

#define SERENITY_FILE_DEBUG "serenity_debug"
#define SERENITY_FILE_IMPORT "serenity_import"
#define SERENITY_FILE_UPDATE "serenity_update"
#define SERENITY_FILE_SERVERS "serenity_servers"
#define SERENITY_FILE_SERVERLIST "serenity_serverlist"
#define SERENITY_FILE_CREATEDB "serenity_createdb"

#define SUPPORTED_GAMES 2

#define BF1942_GAME 1
#define BF1942_QUERY "\\info\\players\\"
#define BF1942_QUERY_LENGTH 14
#define BF1942_PORT 23000
#define BF1942_PLAYER_DATA 7

#define BFVIET_GAME 2
#define BFVIET_QUERY "\xFE\xFD\x01\x01\x01\x01\x01\xFF\xFF\xFF"
#define BFVIET_QUERY_LENGTH 10
#define BFVIET_PORT 23000
#define BFVIET_PLAYER_DATA 7

#define DEBUG_FLOOD 3
#define DEBUG_INFO 2
#define DEBUG_FATAL 1
#define DEBUG_NONE 0

#define DATA_DIRECTORY "./data"

#define DEFAULT_LOCAL_PORT 12005
#define DEFAULT_TIMEOUT 5
#define DEFAULT_QUERY_RETRY 3
#define DEFAULT_QUERY_MAX 0

#define FINISH_NORMAL 0
#define FINISH_CLEANUP 1

#define MYSQL_SERVER_PORT 3306

#define MYSQL_INFO_DEFAULT_USER "admin"
#define MYSQL_INFO_DEFAULT_PASS "21232f297a57a5a743894a0e4a801fc3" //md5 of the word: admin

#define MYSQL_INFO_TABLE "CREATE TABLE IF NOT EXISTS serenity_info (" \
                            "info_id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                            "info_version VARCHAR(8) NOT NULL, " \
                            "info_noreply INT UNSIGNED NOT NULL DEFAULT 0," \
                            "info_badreply INT UNSIGNED NOT NULL DEFAULT 0," \
                            "info_lowruntime SMALLINT UNSIGNED NOT NULL DEFAULT 0, " \
                            "info_highruntime SMALLINT UNSIGNED NOT NULL DEFAULT 0, " \
                            "info_lastruntime SMALLINT UNSIGNED NOT NULL DEFAULT 0, " \
                            "info_firstrun DATETIME NOT NULL, " \
                            "info_lastrun DATETIME NOT NULL, " \
                            "info_webuser VARCHAR(32) NOT NULL, " \
                            "info_webpass VARCHAR(32) NOT NULL, " \
                            "info_webindex INT UNSIGNED NOT NULL, " \
                            "info_webserver INT UNSIGNED NOT NULL, " \
                            "info_webplayer INT UNSIGNED NOT NULL, " \
                            "info_weboptions INT UNSIGNED NOT NULL, " \
                            "CONSTRAINT info_pk PRIMARY KEY (info_id)" \
                         ")"

#define MYSQL_KEYHASH_TABLE "CREATE TABLE IF NOT EXISTS serenity_keyhash (" \
                              "keyhash_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                              "keyhash_type SMALLINT UNSIGNED NOT NULL, " \
                              "keyhash_hash VARCHAR(32) NOT NULL, " \
                              "CONSTRAINT keyhash_pk PRIMARY KEY (keyhash_id), " \
                              "CONSTRAINT keyhash_type_lk FOREIGN KEY (keyhash_type) REFERENCES serenity_servertype (type_id)" \
                            ")"

#define MYSQL_SERVER_TABLE "CREATE TABLE IF NOT EXISTS serenity_server (" \
                              "server_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                              "server_type SMALLINT UNSIGNED NOT NULL, " \
                              "server_ip VARCHAR(16) NOT NULL, " \
                              "server_gameport SMALLINT UNSIGNED NOT NULL, " \
                              "server_queryport SMALLINT UNSIGNED NOT NULL, " \
                              "server_ping SMALLINT UNSIGNED NOT NULL, " \
                              "server_hostname VARCHAR(32) NOT NULL, " \
                              "server_gametype VARCHAR(32) NOT NULL, " \
                              "server_gameid VARCHAR(32) NOT NULL, " \
                              "server_mapname VARCHAR(32) NOT NULL, " \
                              "server_numplayers TINYINT UNSIGNED NOT NULL, " \
                              "server_maxplayers TINYINT UNSIGNED NOT NULL, " \
                              "server_version VARCHAR(5) NOT NULL, " \
                              "server_bytesrecieved BIGINT UNSIGNED NOT NULL DEFAULT 0, " \
                              "server_bytessent BIGINT UNSIGNED NOT NULL DEFAULT 0, " \
                              "server_trashed INT UNSIGNED NOT NULL DEFAULT 0," \
                              "server_updatesuccess INT UNSIGNED NOT NULL, " \
                              "server_updatefail INT UNSIGNED NOT NULL, " \
                              "server_consecutivefails INT UNSIGNED NOT NULL, " \
                              "server_lastupdate DATETIME NOT NULL, " \
                              "CONSTRAINT server_pk PRIMARY KEY (server_id), " \
                              "CONSTRAINT server_type_lk FOREIGN KEY (server_type) REFERENCES serenity_servertype (type_id)" \
                           ")"

#define MYSQL_RULE_TABLE "CREATE TABLE IF NOT EXISTS serenity_rule (" \
                           "rule_id INT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                           "rule_server INT UNSIGNED NOT NULL, " \
                           "rule_key VARCHAR(64) NOT NULL, " \
                           "rule_value VARCHAR(64) NOT NULL, " \
                           "CONSTRAINT rule_pk PRIMARY KEY (rule_id), " \
                           "CONSTRAINT rule_server_lk FOREIGN KEY (rule_server) REFERENCES serenity_server (server_id)" \
                         ")"

#define MYSQL_PLAYER_TABLE "CREATE TABLE IF NOT EXISTS serenity_player (" \
                             "player_id INT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                             "player_server INT UNSIGNED NOT NULL, " \
                             "player_keyhash BIGINT UNSIGNED NOT NULL, " \
                             "player_type SMALLINT UNSIGNED NOT NULL, " \
                             "player_ip VARCHAR(16) NOT NULL, " \
                             "player_name VARCHAR(32) NOT NULL, " \
                             "player_userid SMALLINT NOT NULL, " \
                             "player_score INT NOT NULL, " \
                             "player_frags INT NOT NULL, " \
                             "player_deaths INT NOT NULL, " \
                             "player_team TINYINT UNSIGNED NOT NULL, " \
                             "player_ping SMALLINT UNSIGNED NOT NULL, " \
                             "player_connect INT UNSIGNED NOT NULL, " \
                             "player_lastseen DATETIME NOT NULL, " \
                             "CONSTRAINT player_pk PRIMARY KEY (player_id), " \
                             "CONSTRAINT player_server_lk FOREIGN KEY (player_server) REFERENCES serenity_server (server_id), " \
                             "CONSTRAINT player_keyhash_lk FOREIGN KEY (player_keyhash) REFERENCES serenity_keyhash (keyhash_id)," \
                             "CONSTRAINT player_type_lk FOREIGN KEY (player_type) REFERENCES serenity_servertype (type_id)" \
                           ")"

#define MYSQL_SERVERLIST_TABLE "CREATE TABLE IF NOT EXISTS serenity_serverlist (" \
                                 "list_id INT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                                 "list_ip VARCHAR(16) NOT NULL, " \
                                 "list_gameport SMALLINT UNSIGNED NOT NULL, " \
                                 "list_queryport SMALLINT UNSIGNED NOT NULL, " \
                                 "list_rcon VARCHAR(32) NOT NULL, " \
                                 "list_type SMALLINT UNSIGNED NOT NULL, " \
                                 "CONSTRAINT list_pk PRIMARY KEY (list_id), " \
                                 "CONSTRAINT list_type_lk FOREIGN KEY (list_type) REFERENCES serenity_servertype (type_id)" \
                               ")"

#define MYSQL_SERVERTYPE_TABLE "CREATE TABLE IF NOT EXISTS serenity_servertype (" \
                                 "type_id SMALLINT UNSIGNED NOT NULL, " \
                                 "type_desc VARCHAR(32) NOT NULL, " \
                                 "type_shortdesc VARCHAR(16) NOT NULL, " \
                                 "CONSTRAINT type_pk PRIMARY KEY (type_id)" \
                               ")"

#if 0
#define MYSQL_STATS_TABLE "CREATE TABLE IF NOT EXISTS serenity_stats (" \
                            "stats_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, " \
                            "stats_server BIGINT UNSIGNED NOT NULL, " \
                            "stats_stamp DATETIME NOT NULL, " \
                            "stats_players SMALLINT UNSIGNED NOT NULL, " \
                            "CONSTRAINT stats_pk PRIMARY KEY (stats_id), " \
                            "CONSTRAINT stats_server_lk FOREIGN KEY (stats_server) REFERENCES serenity_server (server_id)" \
                          ")"
#endif

#define MYSQL_TABLE_INFO_ID 0
#define MYSQL_TABLE_INFO_VERSION 1
#define MYSQL_TABLE_INFO_NO_REPLY 2
#define MYSQL_TABLE_INFO_BAD_REPLY 3
#define MYSQL_TABLE_INFO_LOW_RUN_TIME 4
#define MYSQL_TABLE_INFO_HIGH_RUN_TIME 5
#define MYSQL_TABLE_INFO_LAST_RUN_TIME 6
#define MYSQL_TABLE_INFO_FIRST_UPDATE 7
#define MYSQL_TABLE_INFO_LAST_UPDATE 8

#define MYSQL_TABLE_KEYHASH_ID 0
#define MYSQL_TABLE_KEYHASH_TYPE 1
#define MYSQL_TABLE_KEYHASH_HASH 2

#define MYSQL_TABLE_SERVER_ID 0
#define MYSQL_TABLE_SERVER_TYPE 1
#define MYSQL_TABLE_SERVER_IP 2
#define MYSQL_TABLE_SERVER_GAMEPORT 3
#define MYSQL_TABLE_SERVER_QUERYPORT 4
#define MYSQL_TABLE_SERVER_PING 5
#define MYSQL_TABLE_SERVER_HOSTNAME 6
#define MYSQL_TABLE_SERVER_GAMETYPE 7
#define MYSQL_TABLE_SERVER_GAMEID 8
#define MYSQL_TABLE_SERVER_MAPNAME 9
#define MYSQL_TABLE_SERVER_NUMPLAYERS 10
#define MYSQL_TABLE_SERVER_MAXPLAYERS 11
#define MYSQL_TABLE_SERVER_VERSION 12
#define MYSQL_TABLE_SERVER_BYTESRECIEVED 13
#define MYSQL_TABLE_SERVER_BYTESSENT 14
#define MYSQL_TABLE_SERVER_TRASHED 15
#define MYSQL_TABLE_SERVER_UPDATE_SUCCESS 16
#define MYSQL_TABLE_SERVER_UPDATE_FAIL 17
#define MYSQL_TABLE_SERVER_CONSECUTIVE_FAILS 18
#define MYSQL_TABLE_SERVER_LAST_UPDATE 19

#define MYSQL_TABLE_RULE_ID 0
#define MYSQL_TABLE_RULE_SERVER 1
#define MYSQL_TABLE_RULE_KEY 2
#define MYSQL_TABLE_RULE_VALUE 3

#define MYSQL_TABLE_PLAYER_ID 0
#define MYSQL_TABLE_PLAYER_SERVER 1
#define MYSQL_TABLE_PLAYER_KEYHASH 2
#define MYSQL_TABLE_PLAYER_TYPE 3
#define MYSQL_TABLE_PLAYER_IP 4
#define MYSQL_TABLE_PLAYER_NAME 5
#define MYSQL_TABLE_PLAYER_USERID 6
#define MYSQL_TABLE_PLAYER_SCORE 7
#define MYSQL_TABLE_PLAYER_FRAGS 8
#define MYSQL_TABLE_PLAYER_DEATHS 9
#define MYSQL_TABLE_PLAYER_PING 10
#define MYSQL_TABLE_PLAYER_CONNECT 11
#define MYSQL_TABLE_PLAYER_SKIN 12
#define MYSQL_TABLE_PLAYER_TOPCOLOUR 13
#define MYSQL_TABLE_PLAYER_BOTTOMCOLOUR 14

#define MYSQL_TABLE_SERVERLIST_ID 0
#define MYSQL_TABLE_SERVERLIST_IP 1
#define MYSQL_TABLE_SERVERLIST_GAME_PORT 2
#define MYSQL_TABLE_SERVERLIST_QUERY_PORT 3
#define MYSQL_TABLE_SERVERLIST_RCON 4
#define MYSQL_TABLE_SERVERLIST_TYPE 5

#define MYSQL_TABLE_SERVERTYPE_ID 0
#define MYSQL_TABLE_SERVERTYPE_DESC 1
#define MYSQL_TABLE_SERVERTYPE_SHORTDESC 2

//todo - to save memory, malloc all the char which are set on command line
//       or just point the pointer at the argv array?
typedef struct info_s
{
  unsigned int realtime;
  int program_active;
#ifdef UNIX
  pthread_t active;
#endif
#ifdef WIN32
  HANDLE active;
#endif
  int kill;
  int local_port;
  int debug_level;
  int print_list;
  int query_retry;
  int query_max;
  int total_servers;
  int num_types;
  int query_time;
  int server_id;
  int keyhash_id;
  int no_reply;
  int bad_reply;
  int unlink;
  int max_runtime;
  int runtime[MAX_RUNTIME];
  unsigned int timeout;
  char datetime[MAX_DATETIME+1];
  char mysql_database[MAX_MYSQL_DATATBASE+1];
  char mysql_address[MAX_IP+1];
  char mysql_user[MAX_MYSQL_USER+1];
  char mysql_pass[MAX_MYSQL_PASS+1];
  char php_filepath[MAX_FILE_PATH+1];
  char php_parameters[MAX_PHP_PARAMETERS+1];
#ifdef RRDTOOL
  int use_rrdtool;
  char rrdtool_filepath[MAX_FILE_PATH+1];
  char rrdtool_images[MAX_FILE_PATH+1];
#endif
  char graph_dir[MAX_FILE_PATH+1];
  int use_graphs;
  int mysql_port;
  char debug_file[MAX_FILENAME+1];
  char read_file[MAX_FILENAME+1];
  FILE *log_file;
#ifdef USE_MYSQL
  MYSQL *db;
#endif
} info_t;

typedef struct player_s
{
  int userid;
  char name[MAX_NAME+1];
  char keyhash[MAX_KEYHASH+1];
  int score;
  int frags;
  int deaths;
  int connect;
  int ping;
  char team[MAX_TEAM+1];
  char ip[MAX_IP+1];
  int end;
  struct player_s *next;
} player_t;

typedef struct rule_s
{
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  struct rule_s *next;
} rule_t;

typedef struct server_s
{
  char ip[MAX_IP+1];
  int game_port;
  int query_port;
  char host_name[MAX_HOSTNAME+1];
  char map_name[MAX_MAPNAME+1];
  char game_type[MAX_GAMETYPE+1];
  char game_id[MAX_GAMEID+1];
  char version[MAX_VERSION+1];
  int num_players;
  int max_players;
  char rcon[MAX_RCON+1];
  char packet[MAX_PACKET+1];
  int packet_length;
  int packet_recieved;
  int bytes_recieved;
  int bytes_sent;
  int trashed;
  int update_success;
  int update_fail;
  int update_consecutive;
  int ping;
  int got_reply;
  int id;  
#ifdef UNIX
  pthread_t thread;
#endif
#ifdef WIN32
  HANDLE thread;
#endif
  struct server_s *threadid;
  struct server_type_s *type;
  struct rule_s *rules;
  struct player_s *players;
  struct server_s *next;
} server_t;

typedef struct getsockinfo_s
{
  int id;
  char *txdata;
  int txdatalen;
  char *rxdata;
  int rxdatalen;
  int rx;
  int rxlen;
  int bytes_rx;
  int bytes_tx;
} getsockinfo_t;

int newServer(server_t *server);
int newPlayer(server_t *server);
int newRule(server_t *server);
void *singleQuery(_threadvoid *data);
void *active(_threadvoid *arg);
void parseServers(void);
void printServers(void);
void queryServers(void);
int readServersPHP(void);
void createDbPHP(void);
int updateDbPHP(void);
int init(int argc, char *argv[]);
void finish(int style);
int findServerType(int by, int thing);
void graphServers(void);
void *singleGraph(_threadvoid *data);
void showHelp(void);
void setSockAddr(struct sockaddr_in *udp, char *ip, int port);
int setSock(int *sock, int port);
int getSock(int *sock, struct sockaddr_in *send, getsockinfo_t *sockinfo);

void echo(int level, char *fmt, ...);
void echoc(int level, char *s, int len);
int run(char *sys, char *func);

#ifdef USE_MYSQL
int readServersMySql(void);
void createDbMySql(void);
int updateDbMySql(void);
int percent(info_t *in, server_t *se, player_t *pl, rule_t *re, char *s);
#endif

#ifdef USE_MYSQL
#define MYSQL_QUERY_SERVER_SELECT "SELECT * FROM serenity_server WHERE server_type = %server_type% " \
                                  "AND server_ip = \'%server_ip%\' AND server_gameport = %server_game_port%"

#define MYSQL_QUERY_SERVER_SUCCESS "UPDATE serenity_server SET " \
                                   "server_ping = %server_ping%, " \
                                   "server_hostname = \'%server_host_name%\', " \
                                   "server_gametype = \'%server_game_type%\', " \
                                   "server_gameid = \'%server_game_id%\', " \
                                   "server_mapname = \'%server_map_name%\', " \
                                   "server_numplayers = %server_num_players%, " \
                                   "server_maxplayers = %server_max_players%, " \
                                   "server_version = \'%server_version%\', " \
                                   "server_bytesrecieved = %server_bytes_recieved%, " \
                                   "server_bytessent = %server_bytes_sent%, " \
                                   "server_trashed = %server_trashed%, " \
                                   "server_updatesuccess = %server_update_success%, " \
                                   "server_consecutivefails = 0, " \
                                   "server_lastupdate = \'%info_datetime%\' " \
                                   "WHERE server_type = %server_type% AND " \
                                   "server_ip = \'%server_ip%\' AND " \
                                   "server_gameport = %server_game_port%"

#define MYSQL_QUERY_SERVER_FAILED "UPDATE serenity_server SET " \
                                  "server_updatefail = %server_update_fail%, " \
                                  "server_consecutivefails = %server_update_consecutive% " \
                                  "WHERE server_type = %server_type% AND " \
                                  "server_ip = \'%server_ip%\' AND " \
                                  "server_gameport = %server_game_port%"

#define MYSQL_QUERY_SERVER_INSERT "INSERT INTO serenity_server VALUES " \
                                  "(0, " \
                                  "%server_type%, " \
                                  "\'%server_ip%\', " \
                                  "%server_game_port%, " \
                                  "%server_query_port%, " \
                                  "%server_ping%, " \
                                  "\'%server_host_name%\', " \
                                  "\'%server_game_type%\', " \
                                  "\'%server_game_id%\', " \
                                  "\'%server_map_name%\', " \
                                  "%server_num_players%, " \
                                  "%server_max_players%, " \
                                  "\'%server_version%\', " \
                                  "%server_bytes_recieved%, " \
                                  "%server_bytes_sent%, " \
                                  "%server_trashed%, " \
                                  "%server_update_success%, " \
                                  "%server_update_fail%, " \
                                  "%server_update_consecutive%, " \
                                  "\'%info_datetime%\')"

#define MYSQL_QUERY_RULE_INSERT "INSERT INTO serenity_rule VALUES " \
                                "(0, " \
                                "%info_server_id%, " \
                                "\'%rule_key%\', " \
                                "\'%rule_value%\')"

#define MYSQL_QUERY_RULE_DELETE "TRUNCATE TABLE serenity_rule"

#define MYSQL_QUERY_KEYHASH_SELECT "SELECT * FROM serenity_keyhash WHERE " \
                                   "keyhash_type = %server_type% AND keyhash_hash = \'%player_keyhash%\'"

#define MYSQL_QUERY_KEYHASH_INSERT "INSERT INTO serenity_keyhash VALUES (0, %server_type%, \'%player_keyhash%\')"

#define MYSQL_QUERY_PLAYER_SELECT "SELECT * FROM serenity_player WHERE " \
                                  "player_type = %server_type% AND player_name = \'%player_name%\'"

#define MYSQL_QUERY_PLAYER_INSERT "INSERT INTO serenity_player VALUES " \
                                  "(0, " \
                                  "%info_server_id%, " \
                                  "%info_keyhash_id%, " \
                                  "%server_type%, " \
                                  "\'%player_ip%\', " \
                                  "\'%player_name%\', " \
                                  "%player_userid%, " \
                                  "%player_score%, " \
                                  "%player_frags%, " \
                                  "%player_deaths%, " \
                                  "%player_team%, " \
                                  "%player_ping%, " \
                                  "%player_connect%, " \
                                  "\'%info_datetime%\')"

#define MYSQL_QUERY_PLAYER_UPDATE "UPDATE serenity_player SET " \
                                  "player_server = %info_server_id%, " \
                                  "player_keyhash = %info_keyhash_id%, " \
                                  "player_ip = \'%player_ip%\', " \
                                  "player_userid = %player_userid%, " \
                                  "player_score = %player_score%, " \
                                  "player_frags = %player_frags%, " \
                                  "player_deaths = %player_deaths%, " \
                                  "player_ping = %player_ping%, " \
                                  "player_connect = %player_connect%, " \
                                  "player_lastseen = \'%info_datetime%\' " \
                                  "WHERE player_type = %server_type% AND player_name = \'%player_name%\'"

#define MYSQL_QUERY_INFO_SELECT "SELECT * FROM serenity_info"

#define MYSQL_QUERY_INFO_UPDATE "UPDATE serenity_info SET " \
                                "info_version = \'%info_version%\', " \
                                "info_noreply = %info_no_reply%, " \
                                "info_badreply = %info_bad_reply%, " \
                                "info_lowruntime = %info_low_runtime%, " \
                                "info_highruntime = %info_high_runtime%, " \
                                "info_lastruntime = %info_last_runtime%, " \
                                "info_lastrun = \'%info_datetime%\'"

#define MYSQL_QUERY_INFO_INSERT "INSERT INTO serenity_info VALUES " \
                                "(0, " \
                                "\'%info_version%\', " \
                                "%info_no_reply%, " \
                                "%info_bad_reply%, " \
                                "%info_low_runtime%, " \
                                "%info_high_runtime%, " \
                                "%info_last_runtime%, " \
                                "\'%info_datetime%\', " \
                                "\'%info_datetime%\')"

#define MYSQL_QUERY_SERVERLIST_INSERT "INSERT INTO serenity_serverlist VALUES " \
                                      "(0, " \
                                      "\'%server_ip%\', " \
                                      "%server_game_port%, " \
                                      "%server_query_port%, " \
                                      "\'\', " \
                                      "%server_type%)"

#define MYSQL_QUERY_SERVER_SELECT "SELECT * FROM serenity_server WHERE server_type = %server_type% " \
                                  "AND server_ip = \'%server_ip%\' AND server_gameport = %server_game_port%"
#endif

