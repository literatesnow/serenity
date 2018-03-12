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

#include "serenity.h"
#include "global.h"

serenity_t ne;
bf2segment_t *bf2segments;
srcsegment_t *srcsegments;
//resolver_t *resolve;

int main(int argc, char *argv[]) {
  int r;

#ifdef UNIX
  if (!ServiceStartup(&r, argc, argv)) {
    return r;
  }
#endif

  do {
    r = Init(argv[0]);

    if (r == EXIT_SUCCESS) {
      Frame();
    }

    Finish();
  } while (r == EXIT_SUCCESS && (ne.state == _NVE_STATUS_RESTART));

#ifdef SUPERDEBUG
  printf("\nPress ENTER to exit\n");
  getchar();
#endif

  return r;
}

#ifdef UNIX
static int ServiceStartup(int *ret, int argc, char *argv[]) {
#ifdef SUPERDEBUG
  printf("ServiceStartup()\n");
#endif

  if ((argc == 2) && !strcmp(argv[1], "--service")) {
    pid_t pid, sid;
    
    pid = fork();
    if (pid < 0) {
      *ret = EXIT_FAILURE; //fork failed
      return 0;
    }
    if (pid > 0) {
      *ret = EXIT_SUCCESS; //exit parent
      return 0;
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
      *ret = EXIT_FAILURE;
      return 0;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    ne.runservice = 1;
  }

  return 1;
}
#endif

static int Init(char *path) {
  config_t cfg;
  char buf[1024];
  const char *p;
  int segs = 0;
  int i;
#ifdef WIN32
  WSADATA wsaData;
#endif

#ifdef SUPERDEBUG
  printf("Init()\n");
#endif

  bf2segments = NULL;
  srcsegments = NULL;
  //resolve = NULL;

  ne.state = _NVE_STATUS_ACTIVE;
  ne.runservice = 0;

  ne.mysql_address = NULL;
  ne.mysql_port = DEFAULT_MYSQL_PORT;
  ne.mysql_user = NULL;
  ne.mysql_pass = NULL;
  ne.mysql_database = NULL;

  ne.admin_name = NULL;
  ne.internal_address = NULL;
  ne.external_address = NULL;
  ne.reconnect_delay = 0;
  ne.start_port = DEFAULT_LISTEN_PORT;
  ne.end_port = 0;
  ne.wdir = workingdir(path);
  ne.log_dir = NULL;

  ne.lastsave = 0.0;
  ne.laststatus = 0.0;
  ne.lastresolve = 0.0;

	config_init(&cfg);

  _strnprintf(buf, sizeof(buf), "%s%s", ne.wdir, DEFAULT_CONFIG_FILE);
  if (!config_load_file(&cfg, buf)) { //config_read_file
    WriteLog("Error: Failed to read %s on line %d: %s\n", DEFAULT_CONFIG_FILE, cfg.error_line, cfg.error_text);
    return EXIT_FAILURE;
  }

  //mysql
  if ((p = config_lookup_string(&cfg, "mysql.address"))) {
    ne.mysql_address = _strdup_(p);
  }
  if ((i = config_lookup_int(&cfg, "mysql.port"))) {
    ne.mysql_port = i;
  }
  if ((p = config_lookup_string(&cfg, "mysql.user"))) {
    ne.mysql_user = _strdup_(p);
  }
  if ((p = config_lookup_string(&cfg, "mysql.pass"))) {
    ne.mysql_pass = _strdup_(p);
  }
  if ((p = config_lookup_string(&cfg, "mysql.database"))) {
    ne.mysql_database = _strdup_(p);
  }

  //option
  if ((p = config_lookup_string(&cfg, "option.admin_name"))) {
    ne.admin_name = _strdup_(p);
  }
  if ((p = config_lookup_string(&cfg, "option.internal_address"))) {
    ne.internal_address = _strdup_(p);
  }
  if ((p = config_lookup_string(&cfg, "option.external_address"))) {
    ne.external_address = _strdup_(p);
  }
  if ((i = config_lookup_int(&cfg, "option.reconnect_delay"))) {
    ne.reconnect_delay = i;
  }
  if ((i = config_lookup_int(&cfg, "option.start_port"))) {
    ne.start_port = i;
  }
  if ((i = config_lookup_int(&cfg, "option.end_port"))) {
    ne.end_port = i;
  }
  if ((p = config_lookup_string(&cfg, "option.log_directory"))) {
    ne.log_dir = _strdup_(p);
    if (ne.log_dir) {
      _strnprintf(buf, sizeof(buf), "%s%s", ne.wdir, ne.log_dir);
#ifdef WIN32
      if (mkdir(buf) == -1) {
#endif
#ifdef UNIX
      if (mkdir(buf, UNIX_NEWDIR) == -1) {
#endif
        if (errno != EEXIST) {
          WriteLog("Failed to create directory: %s\n", buf);
          return EXIT_FAILURE;
        }
      }
    }
  }

#ifdef NVEDEXT
  //screenshots
  if ((p = config_lookup_string(&cfg, "option.screenshot_directory"))) {
    ne.ss_dir = _strdup_(p);
    if (ne.ss_dir) {
      _strnprintf(buf, sizeof(buf), "%s%s", ne.wdir, ne.ss_dir);
#ifdef WIN32
      if (mkdir(buf) == -1) {
#endif
#ifdef UNIX
      if (mkdir(buf, UNIX_NEWDIR) == -1) {
#endif
        if (errno != EEXIST) {
          WriteLog("Failed to create directory: %s\n", buf);
          return EXIT_FAILURE;
        }
      }
    }
  }

  //pbscanner
  if ((p = config_lookup_string(&cfg, "option.pbscan_command"))) {
    ne.pbscan_command = _strdup_(p);
  }
#endif
  
	config_destroy(&cfg);

  WriteLog("%s v%s - %s %s (%s)\n\n", _NVE_HEADER_TITLE, _NVE_VERSION, _NVE_HEADER_COPYRIGHT, _NVE_HEADER_AUTHOR, _NVE_HEADER_EMAIL);

  if (!ne.mysql_address) {
    WriteLog("Missing MySQL address\n");
    return EXIT_FAILURE;
  }
  if (!ne.mysql_port) {
    WriteLog("Missing MySQL port\n");
    return EXIT_FAILURE;
  }
  if (!ne.mysql_user) {
    WriteLog("Missing MySQL user name\n");
    return EXIT_FAILURE;
  }
  if (!ne.mysql_pass) {
    WriteLog("Missing MySQL password\n");
    return EXIT_FAILURE;
  }
  if (!ne.mysql_database) {
    WriteLog("Missing MySQL database\n");
    return EXIT_FAILURE;
  }
  if (!ne.internal_address) {
    WriteLog("Missing internal network address\n");
    return EXIT_FAILURE;
  }
  if (!ne.external_address) {
    WriteLog("Missing external network address\n");
    return EXIT_FAILURE;
  }

  if (ne.end_port) {
    if (ne.end_port - ne.start_port <= 0) {
      WriteLog("Error: Invalid port range\n");
      return EXIT_FAILURE;
    }
  }

#ifdef WIN32
#ifdef _DEBUG
  SetMemLeakLogProc(&MemLeakLog);
#endif

  if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)&HandleSignal, TRUE)) {
    WriteLog("Error: Signal handler failed\n");
  }
#endif

#ifdef UNIX
	signal(SIGTERM, &HandleSignal);
  signal(SIGINT, &HandleSignal);
  signal(SIGQUIT, &HandleSignal);
  signal(SIGSEGV, &HandleSignal);
  if (signal(SIGUSR1, &HandleSignal) != SIG_ERR) {
    WriteLog("Signal %d for restart\n", SIGUSR1);
  }
#ifdef SHOW_STATUS
  if (signal(SIGUSR2, &HandleSignal) != SIG_ERR) {
    WriteLog("Signal %d for status\n", SIGUSR2);
  }
#endif
#endif

#ifdef WIN32
  if (WSAStartup(MAKEWORD(1,1), &wsaData)) {
    WriteLog("Error: WSAStartup() failed\n");
    return EXIT_FAILURE;
  }
#endif

  if (!OpenMySQL(&ne.mysql)) {
    return EXIT_FAILURE;
  }

  if (!InsanityCheck()) {
    return EXIT_FAILURE;
  }

  if (!InitBF2()) {
    return EXIT_FAILURE;
  }

  //BF2
  if (!ReadBF2ServerList(&i)) {
    return EXIT_FAILURE;
  }
  segs += i;

  //SRC
  if (!ReadSRCServerList()) {
    return EXIT_FAILURE;
  }

  if ((ne.end_port - ne.start_port) < segs) {
    WriteLog("Error: Not enough ports per segment\n");
    return EXIT_FAILURE;
  }

#ifdef SHOW_STATUS
  DisplayBF2Segments();
  DisplaySRCSegments();
#endif

#ifdef _DEBUG
  printf("using base dir: %s\n", ne.wdir);
  printf("using log dir: %s%s [%s]\n", ne.wdir, ne.log_dir, ne.log_dir);
  printf("using ss dir: %s%s [%s]\n", ne.wdir, ne.ss_dir, ne.ss_dir);
  printf("using pb scan: %s\n", ne.pbscan_command);
#endif

#ifdef _DEBUG
  printf("Push ENTER to continue\n");
  getchar();
#endif

  if (!StartBF2Segments()) {
    return EXIT_FAILURE;
  }

  if (!StartSRCSegments()) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static void Frame(void) {

#ifdef SUPERDEBUG
  printf("Frame()\n");
#endif

  WriteLog("Startup complete\n");

  while (ne.state == _NVE_STATUS_ACTIVE) {
    UpdateStatus();
    //ResolveAddresses();
    _sleep(1000);
  }

#ifdef SUPERDEBUG
  printf("Frame() exited with state %d\n", ne.state);
#endif
}

static void Finish(void) {
#ifdef SUPERDEBUG
  printf("Finish()\n");
#endif

  //FreeResolver(1);

  //BF2
  StopBF2Segments();

  //SRC
  StopSRCSegments();

  CloseMySQL(&ne.mysql);

  FinishBF2();

  if (ne.mysql_address) {
    _free_(ne.mysql_address);
  }
  if (ne.mysql_user) {
    _free_(ne.mysql_user);
  }
  if (ne.mysql_pass) {
    _free_(ne.mysql_pass);
  }
  if (ne.mysql_database) {
    _free_(ne.mysql_database);
  }

  //option
  if (ne.admin_name) {
    _free_(ne.admin_name);
  }
  if (ne.internal_address) {
    _free_(ne.internal_address);
  }
  if (ne.external_address) {
    _free_(ne.external_address);
  }
#ifdef NVEDEXT
  if (ne.ss_dir) {
    _free_(ne.ss_dir);
  }
  if (ne.pbscan_command) {
    _free_(ne.pbscan_command);
  }
#endif
  if (ne.log_dir) {
    _free_(ne.log_dir);
  }

#ifdef WIN32
  WSACleanup();
#endif

#ifdef _DEBUG
#ifdef WIN32
  MemLeakCheckAndFree();
#endif
#endif

  WriteLog("Shutdown complete\n");
}

static int ReadBF2ServerList(int *segs) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  int segcount = 0;
  unsigned int total;
  int r = 0;

#define MYSQL_QUERY_SERVERLIST_ID 0
#define MYSQL_QUERY_SERVERLIST_NAME 1
#define MYSQL_QUERY_SERVERLIST_TYPE 2
#define MYSQL_QUERY_SERVERLIST_NETIP 3
#define MYSQL_QUERY_SERVERLIST_RCONPORT 4
#define MYSQL_QUERY_SERVERLIST_RCON 5

  const char sql_select_server[] = {"SELECT server_id, server_name, server_type, server_netip, server_rconport, server_rcon"
                                     " FROM serenity_server"
                                     " WHERE server_name IS NOT NULL"
                                       " AND server_rconport IS NOT NULL"
                                       " AND server_rcon IS NOT NULL"
                                       " AND server_type = " SERVERTYPE_BATTLEFIELD2};

  const char sql_update_player_currentserver[] = {"UPDATE bf2_player SET player_currentserver_id = NULL WHERE player_currentserver_id IS NOT NULL"};
                                       
#ifdef SUPERDEBUG
  printf("ReadBF2ServerList()\n");
#endif

  if (mysql_ping(&ne.mysql)) {
    return 0;
  }

  if (mysql_real_query(&ne.mysql, sql_select_server, CHARSIZE(sql_select_server))) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(&ne.mysql));
#endif
    return 0;
  }

  res = mysql_store_result(&ne.mysql);
  if (!res) {
    return 0;
  }

  assert(mysql_num_fields(res) == 6);
  total = (unsigned int)mysql_num_rows(res);

  if (total) {
    const int serversper = best_of(total);
    bf2segment_t *bf2seg = NULL;
    bf2server_t *bf2;
    int start = (int)(_BF2_TIME_PLAYERS_UPDATE / total);

    int i = 1;

    if (start < 0 || start > _BF2_TIME_PLAYERS_UPDATE) {
      start = 0;
    }

#ifdef SUPERDEBUG
    printf("servers per segment %d, start inc %d\n", serversper, start);
#endif

    while ((row = mysql_fetch_row(res))) {
      unsigned short servertype = (unsigned short)atoi(row[MYSQL_QUERY_SERVERLIST_TYPE]);
      unsigned int serverid = strtoul(row[MYSQL_QUERY_SERVERLIST_ID], NULL, 10);

      if (!bf2seg) {
        bf2seg = NewBF2Segment(segcount);
        segcount++;
      }
      if (!bf2seg) {
        WriteLog("Failed to create segment\n");
        goto _fail;
      }

#ifdef SUPERDEBUG
      strlcat(bf2seg->srvlist, row[MYSQL_QUERY_SERVERLIST_NAME], sizeof(bf2seg->srvlist));
      strlcat(bf2seg->srvlist, ",", sizeof(bf2seg->srvlist));
#endif

      bf2 = NewBF2Server(serverid, servertype, row[MYSQL_QUERY_SERVERLIST_NAME], row[MYSQL_QUERY_SERVERLIST_NETIP],
                         atoi(row[MYSQL_QUERY_SERVERLIST_RCONPORT]), row[MYSQL_QUERY_SERVERLIST_RCON]);
      if (!bf2) {
        WriteLog("Failed to create BF2 server\n");
        goto _fail;
      }

      if (ne.admin_name) {
        strlcpy(bf2->adminname, ne.admin_name, sizeof(bf2->adminname));
      }
      bf2->retrydelay = ne.reconnect_delay;
#ifdef _DEBUG
      bf2->startdelay = 0;
#else
      bf2->startdelay = (i * start);
#endif

      bf2->segment = bf2seg;
      bf2->next = bf2seg->bf2servers;
      bf2seg->bf2servers = bf2;

#ifdef NVEDEXT
      //create dirs for shots
      if (!InitBF2Screens(bf2)) {
        goto _fail;
      }
#endif

      if (((i % serversper) == 0)) {
        bf2seg = NULL;
      }

      i++;
    }
  }

  r = 1;
_fail:
  mysql_free_result(res);
  *segs = segcount;

  //clear out player's current server
  if (mysql_real_query(&ne.mysql, sql_update_player_currentserver, CHARSIZE(sql_update_player_currentserver))) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(&ne.mysql));
#endif
    return 0;
  }


#ifdef SUPERDEBUG
  printf("%u bf2 server(s)\n", total);
#endif

  return r;
}

static int ReadSRCServerList(void) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  int segcount = 0;
  unsigned int total;
  int r = 0;

#define MYSQL_QUERY_SERVERLIST_ID 0
#define MYSQL_QUERY_SERVERLIST_NAME 1
#define MYSQL_QUERY_SERVERLIST_TYPE 2
#define MYSQL_QUERY_SERVERLIST_NETIP 3
#define MYSQL_QUERY_SERVERLIST_RCONPORT 4
#define MYSQL_QUERY_SERVERLIST_RCON 5

  const char sql_select_server[] = {"SELECT server_id, server_name, server_type, server_netip, server_rconport, server_rcon"
                                     " FROM serenity_server"
                                     " WHERE server_name IS NOT NULL"
                                       " AND server_rconport IS NOT NULL"
                                       " AND server_rcon IS NOT NULL"
                                       " AND server_type = " SERVERTYPE_SOURCE};

  const char sql_update_player_currentserver[] = {"UPDATE src_player SET player_currentserver_id = NULL WHERE player_currentserver_id IS NOT NULL"};
                                       
#ifdef SUPERDEBUG
  printf("ReadSRCServerList()\n");
#endif

  if (mysql_ping(&ne.mysql)) {
    return 0;
  }

  if (mysql_real_query(&ne.mysql, sql_select_server, CHARSIZE(sql_select_server))) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(&ne.mysql));
#endif
    return 0;
  }

  res = mysql_store_result(&ne.mysql);
  if (!res) {
    return 0;
  }

  assert(mysql_num_fields(res) == 6);
  total = (unsigned int)mysql_num_rows(res);

  if (total) {
    const int serversper = best_of(total);
    srcsegment_t *srcseg = NULL;
    srcserver_t *srv;
    int start = (int)(_SRC_TIME_PLAYERS_UPDATE / total);

    int i = 1;

    if (start < 0 || start > _SRC_TIME_PLAYERS_UPDATE) {
      start = 0;
    }

#ifdef SUPERDEBUG
    printf("servers per segment %d, start inc %d\n", serversper, start);
#endif

    while ((row = mysql_fetch_row(res))) {
      unsigned short servertype = (unsigned short)atoi(row[MYSQL_QUERY_SERVERLIST_TYPE]);
      unsigned int serverid = strtoul(row[MYSQL_QUERY_SERVERLIST_ID], NULL, 10);

      if (!srcseg) {
        srcseg = NewSRCSegment(segcount);
        segcount++;
      }
      if (!srcseg) {
        WriteLog("Failed to create segment\n");
        goto _fail;
      }

#ifdef SUPERDEBUG
      strlcat(srcseg->srvlist, row[MYSQL_QUERY_SERVERLIST_NAME], sizeof(srcseg->srvlist));
      strlcat(srcseg->srvlist, ",", sizeof(srcseg->srvlist));
#endif

      srv = NewSRCServer(serverid, servertype, row[MYSQL_QUERY_SERVERLIST_NAME], row[MYSQL_QUERY_SERVERLIST_NETIP],
                         atoi(row[MYSQL_QUERY_SERVERLIST_RCONPORT]), row[MYSQL_QUERY_SERVERLIST_RCON]);
      if (!srv) {
        WriteLog("Failed to create SRC server\n");
        goto _fail;
      }

      srv->retrydelay = ne.reconnect_delay;
#ifdef _DEBUG
      srv->startdelay = 0;
#else
      srv->startdelay = (i * start);
#endif

      srv->segment = srcseg;
      srv->next = srcseg->srcservers;
      srcseg->srcservers = srv;

      if (((i % serversper) == 0)) {
        srcseg = NULL;
      }

      i++;
    }
  }

  r = 1;
_fail:
  mysql_free_result(res);

  //clear out player's current server
  if (mysql_real_query(&ne.mysql, sql_update_player_currentserver, CHARSIZE(sql_update_player_currentserver))) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(&ne.mysql));
#endif
    return 0;
  }


#ifdef SUPERDEBUG
  printf("%u src server(s)\n", total);
#endif

  return r;
}

static int InsanityCheck(void) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned int total;
  int i, r = 1;

  const char sql_select_servertypes[] = {"SELECT servertype_id, servertype_shortdesc FROM serenity_servertype ORDER BY servertype_id ASC"};
                                       
#ifdef SUPERDEBUG
  printf("InsanityCheck()\n");
#endif

  if (mysql_ping(&ne.mysql)) {
    return 0;
  }

  if (mysql_real_query(&ne.mysql, sql_select_servertypes, CHARSIZE(sql_select_servertypes))) {
#ifdef LOG_ERRORS
    WriteLog("MySQL Error: %s\n", mysql_error(&ne.mysql));
#endif
    return 0;
  }

  res = mysql_store_result(&ne.mysql);
  if (!res) {
    return 0;
  }

#define MYSQL_QUERY_SERVERTYPE_ID 0
#define MYSQL_QUERY_SERVERTYPE_SHORTDESC 1

  total = (unsigned int)mysql_num_rows(res);

  i = 0;
  while ((row = mysql_fetch_row(res))) {
    switch (i) {
      case 0:
        if (strcmp(row[MYSQL_QUERY_SERVERTYPE_SHORTDESC], "BF2") ||
            strcmp(row[MYSQL_QUERY_SERVERTYPE_ID], SERVERTYPE_BATTLEFIELD2)) {
#ifdef LOG_ERRORS
          WriteLog("Insanity: Server Type 0 is not Battlefield 2\n");
#endif
          r = 0;
        }
        break;
      case 1:
        if (strcmp(row[MYSQL_QUERY_SERVERTYPE_SHORTDESC], "SRC") ||
            strcmp(row[MYSQL_QUERY_SERVERTYPE_ID], SERVERTYPE_SOURCE)) {
#ifdef LOG_ERRORS
          WriteLog("Insanity: Server Type 1 is not Source\n");
#endif
          r = 0;
        }
        break;
      default:
#ifdef LOG_ERRORS
        WriteLog("Insanity: Unknown Server Type: [%s] %s\n",
                 row[MYSQL_QUERY_SERVERTYPE_ID],
                 row[MYSQL_QUERY_SERVERTYPE_SHORTDESC]
                );
#endif
        r = 0;
        break;
    }
    i++;
  }

  mysql_free_result(res);

  return r;
}

#ifdef SUPERDEBUG
static void ShowDebugStuff(void) {
  double t = DoubleTime();
  //WriteLog("Next resolve: %0.2f\n", TIME_RESOLVE_ADDRESS - (t - ne.lastresolve));
  WriteLog("Next status update: %0.2f\n", TIME_UPDATE_STATUS - (t - ne.laststatus));
}
#endif

static void UpdateStatus(void) {
  double t = DoubleTime();

  if ((t - ne.laststatus) >= TIME_UPDATE_STATUS) {
    MYSQL_STMT *qry;

#ifdef SUPERDEBUG
  printf("UpdateStatus()\n");
#endif

    ne.laststatus = t;

    if (mysql_ping(&ne.mysql)) {
      return;
    }

    qry = mysql_stmt_init(&ne.mysql);
    if (qry) {
      const char update_status[] = {"UPDATE serenity_status SET "
                                /* 0 */ "status_version = ?, "
                                        "status_ping = CURRENT_TIMESTAMP() "
                                        "WHERE status_id = 1"};

      if (!mysql_stmt_prepare(qry, update_status, CHARSIZE(update_status))) {
        MYSQL_BIND bind[1];
        const char status_version[] = {_NVE_STATUS_VERSION};

        memset(&bind, 0, sizeof(bind));
    
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void *)status_version;
        bind[0].buffer_length = CHARSIZE(status_version);

        if (!mysql_stmt_bind_param(qry, bind)) {
          if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
            WriteLog("Status - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
          }
        }
#ifdef LOG_ERRORS
      } else {
        WriteLog("Status - Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
      }

      mysql_stmt_close(qry);
#ifdef LOG_ERRORS
    } else {
        WriteLog("Status - Query failed: %s\n", mysql_error(&ne.mysql));
#endif
    }
  }
}

#if 0
static void ResolveAddresses(void) {
  double t = DoubleTime();

  if (resolve) {
    int r;

    _lockmutex(&resolve->mutex);
    r = resolve->active;
    _unlockmutex(&resolve->mutex);

    if (!r) {
#ifdef SUPERDEBUG
      printf("ResolveAddresses(): resolve done\n");
#endif
      FreeResolver(0);
    }

    return;
  }

  if ((t - ne.lastresolve) >= TIME_RESOLVE_ADDRESS) {
    int i;
    ne.lastresolve = t;

    assert(!resolve);

    resolve = (resolver_t *)_malloc_(sizeof(resolver_t));
    if (!resolve) {
      return;
    }

    resolve->active = 1;
    _initmutex(&resolve->mutex);

#ifdef WIN32
    resolve->thread = (HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessAddressResolve, (void *)resolve, 0, &i);
    if (!resolve->thread) {
#endif
#ifdef UNIX
    if ((i = pthread_create(&resolve->thread, NULL, &ProcessAddressResolve, (void *)resolve))) {
#endif

#ifdef SUPERDEBUG
      printf("ResolveAddresses(): thread create failed: %d\n", i);
#endif

      _destroymutex(&resolve->mutex);
      _free_(resolve);
      resolve = NULL;
    }
  }
}
#endif

#if 0
static void *ProcessAddressResolve(void *param) {
  resolver_t *resolve = (resolver_t *)param;
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  MYSQL_STMT *qry;
  unsigned int total = 0;
  int r = 0;

#define MYSQL_QUERY_ADDRESS_ID 0
#define MYSQL_QUERY_ADDRESS_NETIP 1

  const char address_query[] = {"SELECT address_id, address_netip"
                                     " FROM serenity_address"
                                     " WHERE address_domain = '' LIMIT 50"};

  const char update_query[] = {"UPDATE serenity_address"
                             /* 1 */ " SET address_domain = ?"
                             /* 2 */ " WHERE address_id = ?"};
  
#ifdef SUPERDEBUG
  printf("ProcessAddressResolve()\n");
#endif

  if (OpenMySQL(&mysql)) {
    uint64_t addressid;
    struct hostent *hp;
    in_addr_t addr;
    char *p;
#ifdef SUPERDEBUG
    int i;
#endif

    do {
#ifdef SUPERDEBUG
      i = 0;
#endif
      if (!mysql_real_query(&mysql, address_query, sizeof(address_query))) {   
        res = mysql_store_result(&mysql);
        if (res) {
          assert(mysql_num_fields(res) == 2);
          total = (int)mysql_num_rows(res);
  
          while ((row = mysql_fetch_row(res))) {
            _sleep(50);
#ifdef WIN32
            addressid = _atoi64(row[MYSQL_QUERY_ADDRESS_ID]);
#endif
#ifdef UNIX
            addressid = strtoull(row[MYSQL_QUERY_ADDRESS_ID], NULL, 10);
#endif
            p = row[MYSQL_QUERY_ADDRESS_NETIP];

            if (!addressid || !p[0]) {
              continue;
            }

            addr = inet_addr(p);
            hp = (struct hostent *)gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
            if (hp && hp->h_name[0]) {
              p = hp->h_name;
            }

#ifdef SUPERDEBUG
            printf("%02d/%02d %s -> %s [%d]\n", i + 1, total, row[MYSQL_QUERY_ADDRESS_NETIP], p,
#ifdef WIN32
              WSAGetLastError()
#endif
#ifdef UNIX
              errno
#endif
              );
            i++;
#endif
            
            qry = mysql_stmt_init(&mysql);
            if (qry) {
              if (!mysql_stmt_prepare(qry, update_query, sizeof(update_query))) {
                MYSQL_BIND bind[2];

                memset(&bind, '\0', sizeof(bind));

                //0 - address_domain
                bind[0].buffer_type = MYSQL_TYPE_STRING;
                bind[0].buffer = (void *)p;
                bind[0].buffer_length = strlen(p);

                //1 - address_id
                bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
                bind[1].buffer = (void *)&addressid;
                bind[1].is_unsigned = 1;

                if (!mysql_stmt_bind_param(qry, bind)) {
                  if (mysql_stmt_execute(qry)) {
#ifdef LOG_ERRORS
                    WriteLog("Resolve - Execute failed: %s\n", mysql_stmt_error(qry));
#endif
                  }
                }
              }
#ifdef LOG_ERRORS
            } else {
              WriteLog("Resolve - Prepare failed: %s\n", mysql_stmt_error(qry));
#endif
            }
          }

          mysql_free_result(res);
        }
      } else {
#ifdef LOG_ERRORS
        WriteLog("Resolve - Query failed: %s\n", mysql_error(&mysql));
#endif
        break;
      }

      _lockmutex(&resolve->mutex);
      r = resolve->active;
      _unlockmutex(&resolve->mutex);

    } while (total && r);

    CloseMySQL(&mysql);
  }

  _lockmutex(&resolve->mutex);
  resolve->active = 0;
  _unlockmutex(&resolve->mutex);

#ifdef WIN32
  _endthreadex(0);
#endif
#ifdef UNIX
  pthread_exit(NULL);
#endif
  return 0;
}
#endif

/* ------------------------------------------------------------------------- */

#if 0
static void FreeResolver(int now) {
  if (!resolve) {
    return;
  }

  if (now) {
    _lockmutex(&resolve->mutex);
    resolve->active = 0;
    _unlockmutex(&resolve->mutex);
  }

#ifdef WIN32
  WaitForSingleObject(resolve->thread, INFINITE);
  CloseHandle(resolve->thread);
#endif
#ifdef UNIX
  pthread_join(resolve->thread, NULL);
#endif

  _destroymutex(&resolve->mutex);

  _free_(resolve);
  resolve = NULL;
}
#endif

static int best_of(int n) {
#define MAX_DIVBY 4
  const int divby[MAX_DIVBY] = {8, 7, 6, 5};
  int i;

  if (n <= divby[MAX_DIVBY - 1]) {
    return divby[MAX_DIVBY - 1];
  }

  for (i = 0; i < MAX_DIVBY; i++) {
    if (fmod(n, divby[i]) == 0.0) {
      return divby[i];
    }
  }
  return best_of(n + 1);
}

#ifdef WIN32
static BOOL HandleSignal(DWORD signal) {
  if (signal == CTRL_C_EVENT) {
#ifdef _DEBUG
    ShowBF2Status();
    ShowDebugStuff();
#endif
    ne.state = _NVE_STATUS_QUIT;
    return 1;
  }

  return 0;
}
#endif

#ifdef UNIX
static void HandleSignal(int signal) {
#ifdef SUPERDEBUG
  printf("HandleSignal() %d\n", signal);
#endif

  switch (signal) {
    case SIGSEGV:
#ifdef SUPERDEBUG
      printf("HandleSignal() EXPLOSiON!\n");
#endif
#ifndef NDEBUG
      exit(EXIT_FAILURE);
#endif
      break;

    case SIGUSR1:
#ifdef SUPERDEBUG
      printf("HandleSignal() restarting\n");
#endif
      ne.state = _NVE_STATUS_RESTART;
      break;

#ifdef SHOW_STATUS
    case SIGUSR2:
      ShowBF2Status();
#ifdef SUPERDEBUG
      ShowDebugStuff();
#endif
      break;
#endif

    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
#ifdef SUPERDEBUG
      printf("HandleSignal() quitting\n");
#endif
      ne.state = _NVE_STATUS_QUIT;
      break;

    default:
      break;
  }
}
#endif

#ifdef _DEBUG
#ifdef WIN32
static void MemLeakLog(char *szmsg) {
  FILE *fp;

#ifdef SUPERDEBUG
  printf("MemLeakLog()\n");
#endif
 
  fp = fopen("leak.txt", "at");
  if (fp) {
    fprintf(fp, "%s", szmsg);
    fclose(fp);
  }
}
#endif
#endif
