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

#include "srcrcon.h"

srcserver_t *NewSRCServer(unsigned int serverid, unsigned short servertype, char *name, char *addr, int port, char *rcon) {
  srcserver_t *srv;

  if (!name || !addr || !port || !rcon) {
    return NULL;
  }

  srv = (srcserver_t *)_malloc_(sizeof(srcserver_t));
  if (!srv) {
    return NULL;
  }

  srv->active = 1;
  srv->state = SOCKETSTATE_FREE;

  memset(&srv->server.sin_zero, '\0', sizeof(srv->server.sin_zero));
  srv->server.sin_family = AF_INET;
  srv->server.sin_addr.s_addr = inet_addr(addr);
  srv->server.sin_port = htons((short)port);

  srv->sock = INVALID_SOCKET;

  strlcpy(srv->group, name, sizeof(srv->group));
  strlcpy(srv->rcon, rcon, sizeof(srv->rcon));

  srv->lastconnect = 0.0;
  srv->lastreqplayers = 0.0;

  srv->startdelay = 0;
  srv->retrydelay = 0;

  srv->sentauth = 0;
  srv->playercycle = 0;

  srv->tcptx = 0;
  srv->tcprx = 0;

  InitSRCServerInfo(&srv->serverinfo);

  srv->serverid = serverid;
  srv->servertype = servertype;

  srv->segment = NULL;
  srv->players = NULL;

  srv->next = NULL;

  return srv;
}

void FreeSRCServer(srcserver_t *srv) {
  CloseSocket(&srv->sock);
  
  _free_(srv);
}

void InitSRCPlayer(srcplayer_t *pl) {
  assert(pl);

  pl->playerid = 0;
  pl->addressid = 0;
  pl->keyhashid = 0;

  pl->id = -1;
  pl->cycle = 0;
  pl->flags = 0;
  pl->name[0] = '\0';
  pl->hash[0] = '\0';
  pl->address[0] = '\0';
}

srcplayer_t *NewSRCPlayer(srcplayer_t **parent) {
  srcplayer_t *pl;

#ifdef SUPERDEBUG
  //printf("NewSRCPlayer()\n");
#endif

  pl = (srcplayer_t *) _malloc_(sizeof(srcplayer_t));
  if (!pl) {
    return NULL;
  }

  InitSRCPlayer(pl);

  if (*parent)
    (*parent)->prev = pl;
  pl->next = *parent;
  pl->prev = NULL;
  *parent = pl;

  return pl;
}

void FreeSRCPlayer(srcplayer_t *pl, srcplayer_t **parent) {
#ifdef SUPERDEBUG
  //printf("FreeSRCPlayer()\n");
#endif

  if (!pl) {
    return;
  }

  if (parent) {
    if (pl->prev) {
      pl->prev->next = pl->next;
    }
    if (pl->next) {
      pl->next->prev = pl->prev;
    }
    if (!pl->prev) {
      *parent = pl->next;
    }
  }

  _free_(pl);
}

void FreeSRCAllPlayers(srcplayer_t **parent) {
  srcplayer_t *pl, *n;

#ifdef SUPERDEBUG
  printf("FreeSRCAllPlayers()\n");
#endif

  assert(parent);

  n = *parent;
  while (n) {
    pl = n;
    n = n->next;

    FreeSRCPlayer(pl, NULL);
  }

  *parent = NULL;
}

void InitSRCServerInfo(srcserverinfo_t *si) {
  assert(si);

  si->hostname[0] = '\0';
  si->version[0] = '\0';
  si->currentmap[0] = '\0';

  si->numplayers = 0;
  si->maxplayers = 0;
}

int ProcessSRCServer(srcserver_t *srv, char *rv, int rvsz) {
  int sz, pktsz, id, cmd;
  int runaway = 0;
  char *p;

#ifdef SUPERDEBUG
  //printf("ProcessSRCServer()\n");
#endif

  if (!ServerSRCConnect(srv)) {
    return 0;
  }

#ifdef SUPERDEBUG
  //printf("connected!!!\n");
#endif

  if (SocketWait(srv->sock, _SRC_SOCKET_WAIT) == -1) {
    return 0;
  }
  
  //packet size
  sz = recv(srv->sock, (char *) &pktsz, sizeof(int), 0);
  if (sz == -1) {
    return 1;
  }
  srv->tcprx += sz;

  if ((pktsz < 10) || (pktsz >= rvsz)) {
    return 1;
  }

  //id
  sz = recv(srv->sock, (char *) &id, sizeof(int),0);
  if (sz == -1) {
    return 1;
  }
  srv->tcprx += sz;

  //cmd
  pktsz -= sz;
  sz = recv(srv->sock, (char *) &cmd, sizeof(int), 0);
  if (sz == -1) {
    return 1;
  }
  srv->tcprx += sz;

  pktsz -= sz;
  p = rv;

  while (pktsz) {
    sz = recv(srv->sock, p, pktsz, 0);
    if (sz == -1) {
      return 1;
    }
    srv->tcprx += sz;

    rv[sz] = '\0';
    pktsz -= sz; 
    p += sz;

    if (runaway++ > 500) {
#ifdef _DEBUG
    printf("ProcessSRCServer() [%s] RUNAWAY ON %d\n", srv->group, pktsz);
#endif
      return 1;
    }
  }

#ifdef _DEBUG
  printf("ProcessSRCServer() [%s] <-- id: %d, cmd: %d, sz: %d\n", srv->group, id, cmd, sz);
  //printf("[%d] %s\n", sz, rv);
#endif

  switch (id) {
    case _SRC_SOCKET_AUTH_ID:
      PRAuthReply(srv, cmd, 1);
      break;

    case _SRC_SOCKET_ERROR_ID:
      PRAuthReply(srv, cmd, 0);
      break;

    case _SRC_SOCKET_UPDATE_STATUS_ID:
      PRStatusReply(srv, cmd, rv, sz);
      break;

    default:
#ifdef _DEBUG
      printf("unknown cmd: %d\n", cmd);
#endif
      break;
  }

  return 1;
}

static void PRAuthReply(srcserver_t *srv, int cmd, int success) {
  if (cmd != _SRC_SERVERDATA_AUTH_RESPONSE) {
    return; //junk packet
  }

  if (!success) {
    EndSRCConnection(srv);
    srv->state = SOCKETSTATE_FAILEDCONNECT;
#ifdef _DEBUG
    printf("PRAuthReply() [%s] bad rcon\n", srv->group);
#endif
    return;
  }

  srv->state = SOCKETSTATE_CONNECTED;
  srv->lastconnect = 0.0;
  srv->lastreqplayers = DoubleTime() + srv->startdelay;

#ifdef _DEBUG
  printf("PRAuthReply() [%s] we\'re good to go (delay of %d)\n", srv->group, srv->startdelay);
  RequestSRCPlayersFull(srv);
#endif
}

static void PRStatusReply(srcserver_t *srv, int cmd, char *rv, int sz) {
  srcserverinfo_t *si = &srv->serverinfo;
  srcplayer_t player;
  char *s, *p, *q;
  int i, j;

  if (cmd != _SRC_SERVERDATA_RESPONSE_VALUE) {
    return;
  }

  srv->playercycle++;

  s = rv;
  for (i = 0; i < 5; i++) {
    p = strchr(s, ':');
    if (!p) {
      return;
    }
    *p++ = '\0';
    while (*p && (*p == ' ')) {
      p++;
    }
    s = strchr(p, '\n');
    if (!s) {
      return;
    }
    *s++ = '\0';

    switch (i) {
      case 0: //hostname: Stuff 2002
        strlcpy(si->hostname, p, sizeof(si->hostname));
        //printf("hostname=%s\n", si->hostname);
        break;

      case 1: //version : 1.0.1.3/14 3331 insecure
        strlcpy(si->version, p, sizeof(si->version));
        //printf("version=%s\n", si->version);
        break;

      case 2: //udp/ip  :  192.168.233.1:27015
        //printf("ip=%s\n", p);
        break;

      case 3: //map     : cp_dustbowl at: 0 x, 0 y, 0 z
        q = strchr(p, ' ');
        if (!q) {
          q = p;
        } else {
          *q = '\0';
        }
        strlcpy(si->currentmap, p, sizeof(si->currentmap));
        //printf("map=%s\n", si->currentmap);
        break;

      case 4: //players : 0 (8 max)
        q = strchr(p, ' ');
        if (!q) {
          si->numplayers = atoi(p);
          si->maxplayers = 0;
          break;
        }
        *q++ = '\0';
        si->numplayers = atoi(p);

        if (*q == '(') {
          q++;
        }
        p = q;

        q = strchr(p, ' ');
        if (q) {
          *q++ = '\0';
        }

        si->maxplayers = atoi(p);
        break;

      default:
        assert(0);
        break;
    }
  }

  //and now for the players
  while (*s && (*s != '#')) {
    s++;
  }

  s = strchr(s, '\n'); //skip the header
  if (!s) {
    return;
  }
  s++;
  i = 0;

  while (i++ < si->numplayers) {
    p = strchr(s, '\n');
    if (!p) {
      break;
    }
    *p++ = '\0';

    InitSRCPlayer(&player);
    player.cycle = srv->playercycle;

    if (*s++ != '#') {
      return;
    }

    /*
    # userid name uniqueid connected ping loss state adr
    #  4 "this is a very long name with !" STEAM_ID_PENDING 07:39 34 0 active 192.168.233.1:27006
    */

    //skip spaces at beginning
    while (*s && (*s == ' ')) {
      s++;
    }

    //id
    q = strchr(s, ' ');
    if (!q) {
      return;
    }
    *q++ = '\0';

    //printf("id=%s\n", s);
    player.id = atoi(s);
    s = q;

    if (*s == '\"') {
      s++;
      q = strchr(s, '\"');
      if (!q) {
        return;
      }
      *q++ = '\0';
      if (*q) {
        q++;
      }

    } else {
      q = strchr(s, ' ');
      if (!q) {
        return;
      }
      *q++ = '\0';
    }

    //printf("name=%s\n", s);
    strlcpy(player.name, s, sizeof(player.name));
    s = q;

    q = strchr(s, ' ');
    if (!q) {
      return;
    }
    *q++ = '\0';

    //printf("hash=%s\n", s);
    strlcpy(player.hash, s, sizeof(player.hash));
    s = q;

    for (j = 0; j < 4; j++) {
      q = strchr(s, ' ');
      if (!q) {
        return;
      }
      s = ++q;
    }

    q = strchr(s, ':');
    if (!q) {
      return;
    }
    *q = '\0';

    //printf("address=%s\n", s);
    strlcpy(player.address, s, sizeof(player.address));

    ActionSRCUpdatePlayer(srv, &player);

    s = p;
  }

  ActionSRCUpdateAllPlayers(srv);
}

int ServerSRCConnect(srcserver_t *srv) {
  int r;

#ifdef SUPERDEBUG
  //printf("ServerSRCConnect()\n");
#endif

  if (srv->state == SOCKETSTATE_CONNECTED) {
    return 1;
  }

  if ((srv->state == SOCKETSTATE_DISCONNECTED) || (srv->state == SOCKETSTATE_FAILEDCONNECT)) {
    double t = DoubleTime();

    if (srv->lastconnect == 0.0) {
      srv->lastconnect = t + srv->retrydelay;
#ifdef SUPERDEBUG
      printf("ServerSRCConnect() [%s] FailedConnect, setting delay\n", srv->group);
#endif
      return 0;

    } else if (srv->lastconnect < t) {
#ifdef SUPERDEBUG
      printf("ServerSRCConnect() [%s] Done waiting\n", srv->group);
#endif
      if (srv->state == SOCKETSTATE_FAILEDCONNECT) {
#ifdef SUPERDEBUG
        printf("ServerSRCConnect() [%s] Previous failed, disconnected\n", srv->group);
#endif
        srv->state = SOCKETSTATE_DISCONNECTED;

        ActionSRCServerDisconnect(srv);
      } else {
#ifdef SUPERDEBUG
        printf("ServerSRCConnect() [%s] Previous disconnect, try connect\n", srv->group);
#endif
      }

    } else {
      _sleep(80);
      //SocketWait(srv->sock, 100); <- no point socket is dead
      return 0;
    }
  }

  r = ConnectSocket(&srv->sock, &srv->server, &srv->state, &srv->lastconnect);

  if (srv->state == SOCKETSTATE_SOCKCONNECTED) {
    double t = DoubleTime();

    if (!srv->sentauth) {
      SendSRCSocket(srv, _SRC_SOCKET_AUTH_ID, _SRC_SERVERDATA_AUTH, srv->rcon, -1);
      srv->lastconnect = t + srv->retrydelay;
      srv->sentauth = 1;

      return 1;
    }

    if (srv->lastconnect < t) {
      EndSRCConnection(srv);
      return 0;
    }

    return 1;
  }

  return r;
}

void EndSRCConnection(srcserver_t *srv) {
#ifdef SUPERDEBUG
  printf("EndSRCConnection()\n");
#endif

  CloseSocket(&srv->sock);
  InitSRCServerInfo(&srv->serverinfo);

  srv->lastconnect = 0.0;
  srv->lastreqplayers = 0.0;
  srv->state = SOCKETSTATE_DISCONNECTED;
  srv->sentauth = 0;

  ActionSRCServerDisconnect(srv);
}

void SendSRCSocket(srcserver_t *srv, int id, int cmd, char *string1, int sz) {
  int r;
  const char string2[] = {0};

  if (sz == -1) {
    sz = strlen(string1);
  }

  if (sz <= 0 || srv->sock == INVALID_SOCKET) {
    return;
  }

  //our packet size
  //id+cmd+string1+string2
  r = sizeof(int) + sizeof(int) + (sizeof(char) * 2) + sz;

#ifdef _DEBUG
  printf("SendSRCSocket() [%s id:%d cmd:%d s1sz:%d pktsz:%d] --> %s", srv->group, id, cmd, sz, r, string1);
  if (string1[sz - 1] != '\n') {
    printf("\n");
  }
#endif

#ifdef _DEBUG
#ifdef WIN32
#define PRINT_SOCKET_SEND_MESS(srv, r) \
  printf("SendSRCSocket() [%s] ret:%d err:%d\n", srv->group, r, (r == -1) ? WSAGetLastError() : 0);
#endif
#ifdef UNIX
#define PRINT_SOCKET_SEND_MESS(srv, r) \
  printf("SendSRCSocket() [%s] ret:%d err:%d\n", srv->group, r, (r == -1) ? errno : 0);
#endif
#endif

  r = send(srv->sock, (const char *) &r, sizeof(int), 0);
#ifdef _DEBUG
  PRINT_SOCKET_SEND_MESS(srv, r)
#endif
  if (r == SOCKET_ERROR) {
    EndSRCConnection(srv);
    return;
  }
  srv->tcptx += r;

  r = send(srv->sock, (const char *) &id, sizeof(int), 0);
#ifdef _DEBUG
  //PRINT_SOCKET_SEND_MESS(srv, r)
#endif
  if (r == SOCKET_ERROR) {
    EndSRCConnection(srv);
    return;
  }
  srv->tcptx += r;

  r = send(srv->sock, (const char *) &cmd, sizeof(int), 0);
#ifdef _DEBUG
  //PRINT_SOCKET_SEND_MESS(srv, r)
#endif
  if (r == SOCKET_ERROR) {
    EndSRCConnection(srv);
    return;
  }
  srv->tcptx += r;
  
  r = send(srv->sock, string1, sz + sizeof(char), 0);
#ifdef _DEBUG
  //PRINT_SOCKET_SEND_MESS(srv, r)
#endif
  if (r == SOCKET_ERROR) {
    EndSRCConnection(srv);
    return;
  }
  srv->tcptx += r;

  r = send(srv->sock, string2, sizeof(string2), 0);
#ifdef _DEBUG
  //PRINT_SOCKET_SEND_MESS(srv, r)
#endif
  if (r == SOCKET_ERROR) {
    EndSRCConnection(srv);
    return;
  }
  srv->tcptx += r;
}

void RequestSRCPlayersFull(srcserver_t *srv) {
  SendSRCSocket(srv, _SRC_SOCKET_UPDATE_STATUS_ID, _SRC_SERVERDATA_EXECCOMMAND, "status", -1);
}

srcplayer_t *FindSRCPlayerByName(srcserver_t *srv, char *name) {
  srcplayer_t *pl;

  if (!name) {
    return NULL;
  }

  for (pl = srv->players; pl; pl = pl->next) {
    if (!strcmp(pl->name, name)) {
      return pl;
    }
  }

  return NULL;
}

