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

#include "bf2rcon.h"

bf2server_t *NewBF2Server(unsigned int serverid, unsigned short servertype, char *name, char *addr, int port, char *rcon) {
  bf2server_t *bf2;

  if (!name || !addr || !port || !rcon) {
    return NULL;
  }

  bf2 = (bf2server_t *)_malloc_(sizeof(bf2server_t));
  if (!bf2) {
    return NULL;
  }

  memset(&bf2->server.sin_zero, '\0', sizeof(bf2->server.sin_zero));
  bf2->server.sin_family = AF_INET;
  bf2->server.sin_addr.s_addr = inet_addr(addr);
  bf2->server.sin_port = htons((short)port);

  strlcpy(bf2->group, name, sizeof(bf2->group));
#ifdef BF2CCEXT
  bf2->rcon[0] = '\0';
#else
  strlcpy(bf2->rcon, rcon, sizeof(bf2->rcon));
#endif
  bf2->active = 1;
  bf2->sock = INVALID_SOCKET;
  bf2->state = SOCKETSTATE_FREE;
  bf2->lastconnect = 0.0;
  bf2->lastreqchat = 0.0;
  bf2->lastreqplayers = 0.0;
  bf2->lastmapchange = 0.0;
  //bf2->changed = 0;
  bf2->reqstart = 0;
  bf2->reqend = 0;
  bf2->sentwelcome = 0;
  bf2->block = NULL;
  bf2->blocklen = 0;
  bf2->tcptx = 0;
  bf2->tcprx = 0;

  strlcpy(bf2->adminname, _BF2_DEFAULT_ADMIN_NAME, sizeof(bf2->adminname));
  bf2->startdelay = 0;
  bf2->retrydelay = _BF2_DEFAULT_RETRY_DELAY;

#ifdef NVEDEXT
  bf2->nvedext = 0;
  bf2->pbshotplayer = NULL;
  bf2->pbshot = NULL;
  bf2->pbshotsize = 0;
  bf2->pbshotrecv = 0;
#endif

  InitBF2ServerInfo(&bf2->serverinfo);

  bf2->serverid = serverid;
  bf2->servertype = servertype;

  bf2->sentpbalive = 0;
  bf2->lastpbstream = 0;
  bf2->playercycle = 0;
#ifdef NVEDEXT
  bf2->dirgroup[0] = '\0';
  bf2->lastshotname[0] = '\0';
#endif

  bf2->segment = NULL;
  bf2->players = NULL;

  bf2->next = NULL;

  return bf2;
}

void FreeBF2Server(bf2server_t *bf2) {
  if (bf2->block) {
    _free_(bf2->block);
  }

#ifdef NVEDEXT
  if (bf2->pbshot) {
    fclose(bf2->pbshot);
  }
#endif

  CloseSocket(&bf2->sock);
  
  _free_(bf2);
}

int ProcessBF2Server(bf2server_t *bf2, char *rv, int rvsz) {
  int i, j, sz;

#ifdef SUPERDEBUG
  //printf("ProcessBF2Server()\n");
#endif

  if (!ServerBF2Connect(bf2)) {
    return 0;
  }

#ifdef SUPERDEBUG
  //printf("connected!!!\n");
#endif

  if (SocketWait(bf2->sock, _BF2_SOCKET_WAIT) == -1) {
    return 0;
  }

  sz = recv(bf2->sock, rv, rvsz - 1, 0);
  if (sz == -1) {
    return 1;
  }
  rv[sz] = '\0';

  bf2->tcprx += sz;

#if 0
  printf("ProcessBF2Server(): [%d]", sz);
  for (i = 0; i < sz; i++) {
    printf("%c", rv[i] < 32 ? '.' : rv[i]);
  }
  printf("\n");
#endif

  j = 0;
  i = 0;

  for (; i < sz; i++) {
    if (rv[i] == '\x04') {
      ParseBF2(bf2, rv + j, i - j + 1);
      j = i + 1;
    }
  }

  if (j != sz) {
    ParseBF2(bf2, rv + j, sz - j);
  }

  return 1;
}

int ParseBF2(bf2server_t *bf2, char *rv, int sz) {
  int r = 0;

  if (!bf2->block) {
#ifdef NVEDEXT
    if (bf2->nvedext) {
      if (bf2->pbshotsize) {
        PRProcessFile(bf2, rv, sz);
        return 1;
      }

      //hackity hack for file oob
      if (rv[0] == _BF2_PROTOCOL_PBSHOT) {
        if (sz > 1) {
          rv[sz - 1] = '\0';
          bf2->pbshotsize = atoi(rv + 1);
#ifdef SUPERDEBUG
          printf("ParseBF2(): New shot: %s %d bytes\n", bf2->group, bf2->pbshotsize);
#endif
        }
        return 1;

      } else if (rv[0] == _BF2_PROTOCOL_PBSHOT_ERROR) {
#ifdef SUPERDEBUG
        printf("ParseBF2(): Shot error: [%s]: %s\n", bf2->group, rv + 1);
#endif
        ActionBF2PlayerShotDone(bf2, 0);
        return 1;
      }
    }
#endif

    //hackity hack for oob
    if (sz == 2 && rv[0] > ' ' && rv[1] == '\x04') {
      //map changed -- meh it's send more than once
      if (rv[0] == '3') {
        double d = DoubleTime();

        //dup map change ignore
        if ((d - bf2->lastmapchange) > 10.0) {
#ifdef _DEBUG
          printf("ParseBF2(): [%s] MAP CHANGED\n", bf2->group);
#endif
          bf2->lastmapchange = d;
#ifdef PARANOID
          //TODO: Exploit check
          //bf2->serverinfo.iskarkand = 0;
          //bf2->lastexploitcheck = 0.0;
#endif

          RequestBF2ServerInfo(bf2);
        }
      }
      return 1;
    }
  }

  if (bf2->reqstart == bf2->reqend) {
#ifdef _DEBUG
    printf("ParseBF2(): [%s] ignore [%d] >>%s\n", bf2->group, sz, rv);
#endif
    return 0;
  }

  r = bf2->requests[bf2->reqstart];

  if ((bf2->block && rv[sz - 1] == '\x04') || (r != _BF2_PACKET_WELCOME && sz > 1 && rv[sz - 1] != '\x04')) {
    char *p = (char *)_realloc_(bf2->block, bf2->blocklen + sz + 1);

    if (p) {
      memcpy(p + bf2->blocklen, rv, sz);
      bf2->blocklen += sz;
      bf2->block = p;
      bf2->block[bf2->blocklen] = '\0';

      if (rv[sz - 1] != '\x04') {
        return 1;
      } else {
        rv = bf2->block;
        sz = bf2->blocklen;
      }
    }
  }

#ifdef SUPERDEBUG
  //printf("ParseBF2(): [%s] type - %d (next: %d)\n", bf2->group, r, bf2->reqstart+1 != bf2->reqend ? bf2->reqstart+1 : -1);
#endif

  RemoveTopRequest(bf2);

  /*
  {
    int i;

    printf(">>>>>>>>>>>>>>>>>>>>RV DUMP:%d<<<<<<<<<<<<<<<<<<<<\n", sz);
      for (i = 0; i < sz; i++)
        printf("%c", rv[i] < 32 ? '.' : rv[i]);
      printf("\n");
    printf(">>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<\n");
  }
  */

  switch (r) {
    case _BF2_PACKET_RAW:
      break;

    case _BF2_PACKET_HIDE:
      break;

    case _BF2_PACKET_WELCOME:
      PRWelcome(bf2, rv, sz);
      break;

    case _BF2_PACKET_ISLOGIN:
      PRLoginReply(bf2, rv, sz);
      break;
      
    case _BF2_PACKET_SERVERINFO:
      PRServerInfo(bf2, rv, sz);
      break;

    /*
    case _BF2_PACKET_SIMPLEPLAYERS:
      PRPlayersSimple(bf2, rv, sz);
      break;
    */

    case _BF2_PACKET_FULLPLAYERS:
      PRPlayersFull(bf2, rv, sz);
      break;

    /*
    case _BF2_PACKET_SERVERCHAT:
      PRServerChat(bf2, rv, sz);
      break;
    */

    case _BF2_PACKET_SETGAMEPORT:
      PRSetPort(&bf2->serverinfo.gameport, rv, sz);
      break;

    case _BF2_PACKET_SETQUERYPORT:
      PRSetPort(&bf2->serverinfo.queryport, rv, sz);
      break;

#ifdef NVEDEXT
    case _BF2_PACKET_NVEDEXTVERSION:
      PRSetNvedExt(bf2, rv, sz);
      break;
#endif

#ifdef _DEBUG
    case _BF2_PACKET_PRINT:
      dump(rv, sz);
      break;
#endif

    default:
#ifdef _DEBUG
      printf("Unknown packet type!: %d\n", r);
#endif
      break;
  }

  if (bf2->block) {
    _free_(bf2->block);
    bf2->block = NULL;
    bf2->blocklen = 0;
  }

  return 1;
}

int ServerBF2Connect(bf2server_t *bf2) {
  int r;

#ifdef SUPERDEBUG
  //printf("ServerBF2Connect()\n");
#endif

  if (bf2->state == SOCKETSTATE_CONNECTED) {
    return 1;
  }

  if ((bf2->state == SOCKETSTATE_DISCONNECTED) || (bf2->state == SOCKETSTATE_FAILEDCONNECT)) {
    double t = DoubleTime();

    if (bf2->lastconnect == 0.0) {
      bf2->lastconnect = t + bf2->retrydelay;
#ifdef SUPERDEBUG
      printf("ServerBF2Connect() [%s] FailedConnect, setting delay\n", bf2->group);
#endif
      return 0;

    } else if (bf2->lastconnect < t) {
#ifdef SUPERDEBUG
      printf("ServerBF2Connect() [%s] Done waiting\n", bf2->group);
#endif
      if (bf2->state == SOCKETSTATE_FAILEDCONNECT) {
#ifdef SUPERDEBUG
        printf("ServerBF2Connect() [%s] Previous failed, disconnected\n", bf2->group);
#endif
        bf2->state = SOCKETSTATE_DISCONNECTED;

        ActionBF2ServerDisconnect(bf2);
      } else {
#ifdef SUPERDEBUG
        printf("ServerBF2Connect() [%s] Previous disconnect, try connect\n", bf2->group);
#endif
      }

    } else {
      _sleep(80);
      //SocketWait(bf2->sock, 100); <- no point socket is dead
      return 0;
    }
  }

  r = ConnectSocket(&bf2->sock, &bf2->server, &bf2->state, &bf2->lastconnect);

  if (bf2->state == SOCKETSTATE_SOCKCONNECTED) {
    double t = DoubleTime();

    if (!bf2->sentwelcome) {
      AddRequest(bf2, _BF2_PACKET_WELCOME);
      bf2->lastconnect = t + bf2->retrydelay;
      bf2->sentwelcome = 1;

      return 1;
    }

    if (bf2->lastconnect < t) {
      EndBF2Connection(bf2);
      return 0;
    }

    return 1;
  }

  return r;
}

void EndBF2Connection(bf2server_t *bf2)
{
#ifdef SUPERDEBUG
  printf("EndBF2Connection()\n");
#endif

  CloseSocket(&bf2->sock);
  RemoveRequests(bf2);
  InitBF2ServerInfo(&bf2->serverinfo);
  bf2->lastconnect = 0.0;
  bf2->lastreqchat = 0.0;
  bf2->lastreqplayers = 0.0;
  bf2->sentwelcome = 0;
  bf2->state = SOCKETSTATE_DISCONNECTED;

#ifdef NVEDEXT
  if (bf2->pbshot) {
    fclose(bf2->pbshot);
  }
  bf2->pbshot = NULL;
  bf2->pbshotsize = 0;
  bf2->pbshotrecv = 0;
#endif

  if (bf2->block) {
    _free_(bf2->block);
  }
  bf2->block = NULL;
  bf2->blocklen = 0;
  
  ActionBF2ServerDisconnect(bf2);
}

void SendBF2Socket(bf2server_t *bf2, char *data, int sz) {
  int r;

  if (sz == -1) {
    sz = strlen(data);
  }

  if (sz <= 0 || bf2->sock == INVALID_SOCKET) {
    return;
  }

#ifdef _DEBUG
  printf("SendBF2Socket() [%s:%d] --> %s", bf2->group, sz, data);
  if (data[sz - 1] != '\n') {
    printf("\n");
  }
#endif

  r = send(bf2->sock, data, sz, 0);

#ifdef _DEBUG
  //START MESS
  printf("SendBF2Socket() [%s] ret:%d err:%d\n", bf2->group, r,
    r == -1 ?
#ifdef WIN32
      WSAGetLastError()
#endif
#ifdef UNIX
      errno
#endif
      : 0
    );
  //END MESS
#endif

  if (r == SOCKET_ERROR) {
    EndBF2Connection(bf2);
    return;
  }
  
  //_lockmutex(&bf2->mutex);
  bf2->tcptx += r;
  //_unlockmutex(&bf2->mutex);
}

/* /////////////////////////////////////////

PRCOMMANDS

///////////////////////////////////////// */

//### Battlefield 2 ModManager Rcon v1.7.
//### Digest seed: aaaaaaaaaaaaaaaa.
static int PRWelcome(bf2server_t *bf2, char *rv, int sz) {
  char rcon[_BF2_MAX_RCON+1];
  char login[32+1];
  char buf[128+1];
  char *p, *seed;

  if (sz < 33) {
    return 0;
  }

  p = strchr(rv, '\n');
  if (p && (p != &rv[sz - 1])) {
    *p++ = '\0';
  } else {
    p = NULL;
  }

  if (!strncmp(rv, "### Battlefield 2", 17)) {
    rv[sz - 1] = '\0';
#ifdef _DEBUG
    dump(rv, sz);
#endif
    if (!p) {
      AddRequest(bf2, _BF2_PACKET_WELCOME);
      return 1;
    }
    rv = p;
  }

  if (strncmp(rv, "### Digest seed", 15)) {
    return 0;
  }

  seed = rv + 17;
  seed[16] = '\0';

//#ifndef _DEBUG
  destroy(bf2->rcon, rcon, sizeof(rcon));
//#else
//  strlcpy(rcon, bf2->rcon, sizeof(rcon));
//#endif
  //strlcpy(buf, seed, sizeof(buf));
  //strlcat(buf, rcon, sizeof(buf));
  _strnprintf(buf, sizeof(buf), "%s%s", seed, rcon);
  md5(buf, login, sizeof(login));

#ifdef PARANOID
  memset(&rcon, '\0', sizeof(rcon));
#endif

  AddRequest(bf2, _BF2_PACKET_ISLOGIN);
  _strnprintf(buf, sizeof(buf), "\x02login %s\n", login);
  SendBF2Socket(bf2, buf, -1);

#ifdef _DEBUG
  dump(rv, sz);
#endif

  return 1;
}

//4.6.1.32.0.0.strike_at_karkand.dalian_plant.wtf. BF2 Server.MEC.0.200.200.0.US.0.220.220.0.65969.-1.gpm_cq.bf2.(1024, 1024).0.0.0.0.0.175049.290022.0}}.
static int PRServerInfo(bf2server_t *bf2, char *rv, int sz) {
  char *s, *p;
  int i, re = 0;
  bf2serverinfo_t *si;
  int gameport, queryport;

#ifdef _DEBUG
  printf("PRServerInfo() [%s]\n", bf2->group);
#endif

  if (sz <= 2) {
    return 0;
  }

  si = &bf2->serverinfo;
  gameport = si->gameport;
  queryport = si->queryport;

  //init stomps on gameport/queryport which aren't in the server info
  InitBF2ServerInfo(si);

  si->gameport = gameport;
  si->queryport = queryport;

  s = rv;
  for (i = 0; i < 29; i++) {
    p = strchr(s, '\t');
    if (!p) {
#ifdef _DEBUG
      printf("PRServerInfo() [%s] Bailed out at %d\n", bf2->group, i);
#endif
      goto _end;
    }
    *p++ = '\0';

    switch (i) {
      //case 0: strlcpy(si->scriptver, s, sizeof(si->scriptver));
      //        break;

      //case 1: si->state = atoi(s);
      //        break;

      case 2: si->maxplayers = atoi(s);
              break;

      case 3: si->connectedplayers = atoi(s);
              break;

      case 4: si->joiningplayers = atoi(s);
              break;

      case 5: strlcpy(si->currentmap, s, sizeof(si->currentmap));
              //si->iskarkand = !stricmp(si->currentmap, "strike_at_karkand");
              break;

      case 6: strlcpy(si->nextmap, s, sizeof(si->nextmap));
              break;

      case 7: strlcpy(si->hostname, s, sizeof(si->hostname));
              break;

      //team 1
      case 8: strlcpy(si->team1.name, s, sizeof(si->team1.name));
              break;

      //case 9: si->team1.state = atoi(s);
      //        break;

      //case 10: si->team1.starttickets = atoi(s);
      //        break;

      //case 11: si->team1.tickets = atoi(s);
      //        break;

      //case 12: si->team1.ticketrate = atoi(s);
      //        break;

      //team 2
      case 13: strlcpy(si->team2.name, s, sizeof(si->team2.name));
               break;

      //case 14: si->team2.state = atoi(s);
      //         break;

      //case 15: si->team2.starttickets = atoi(s);
      //         break;

      //case 16: si->team2.tickets = atoi(s);
      //         break;

      //case 17: si->team2.ticketrate = atoi(s);
      //         break;

      //maps
      //case 18: si->maptime = atoi(s);
      //         break;

      //case 19: si->mapleft = atoi(s);
      //         break;

      case 20: strlcpy(si->gamemode, s, sizeof(si->gamemode));
               break; 

      case 21: strlcpy(si->moddir, s, sizeof(si->moddir));
               break;

      //case 22: strlcpy(si->worldsize, s, sizeof(si->worldsize));
      //         break;

      //case 23: si->timelimit = atoi(s);
      //         break;

      //case 24: si->autobalance = atoi(s);
      //         break;

      case 25: si->ranked = atoi(s);
               break;

      //case 26: si->team1.players = atoi(s);
      //         break;

      //case 27: si->team2.players = atoi(s);
      //         break;

      //case 28: si->querytime = atoi(s);
      //         break;

      //case 29: si->reserved = atoi(s);
      //         break;

      default: break;
    }

    s = p;
  }

  si->numplayers = si->joiningplayers + si->connectedplayers;

  if (si->numplayers > si->maxplayers) {
#ifdef _DEBUG
    printf("PRServerInfo() PARSE ERROR [%s] %d > %d\n", bf2->group, si->numplayers, si->maxplayers);
#endif
    RemoveRequests(bf2);
    //lets clutch at straws
    if (bf2->block) {
      _free_(bf2->block);
      bf2->block = NULL;
      bf2->blocklen = 0;
    }
  } else {
    ActionBF2ServerSuccess(bf2);
  }

  re = 1;
_end:

  return re;
}

static int PRPlayersFull(bf2server_t *bf2, char *rv, int sz) {
  char *s, *p, *q, *r;
  int i, j, re = -1;
  bf2player_t player;

#ifdef _DEBUG
  printf("PRPlayersFull() [%s]\n", bf2->group);
#endif

  bf2->playercycle++;

  q = rv;
  i = 0;

  while (*q && (*q != '\x04') && (i < _BF2_MAX_BF2PLAYERS)) {
    r = strchr(q, '\r');
    if (!r) {
      goto _end;
    }
    *r++ = '\0';

    s = q;
    InitBF2Player(&player);
    player.cycle = bf2->playercycle;

    for (j = 0; j < 43; j++) {
      p = strchr(s, '\t');
      if (!p) {
        goto _end;
      }
      *p++ = '\0';

      switch (j) {
        case 0: player.id = atoi(s);
                break;

        case 1: strlcpy(player.name, s, sizeof(player.name));
                break;

        /*
        case 2: player.team = atoi(s);
                break;

        case 3: player.ping = atoi(s);
                break;

        case 8: player.isalive = atoi(s);
                break;
        */

        case 10: strlcpy(player.pid, s, sizeof(player.pid));
                 break;

        case 18: strlcpy(player.address, s, sizeof(player.address));
                 break;

        /*
        case 36: player.deaths = atoi(s);
                 break;

        case 37: player.score = atoi(s);
                 break;
        */

        //case 40: strlcpy(player.position, s, sizeof(player.position));
        //         break;

        case 42: strlcpy(player.hash, s, sizeof(player.hash));
                 break;

        default: break;
      }

      s = p;
    }

#ifdef _DEBUG
    //printf("[%s] %d: %s (%s/%s)\n", bf2->group, i, player.name, player.hash, player.address);
#endif

    ActionBF2UpdatePlayer(bf2, &player);

    i++;
    q = r;
  }

#ifdef _DEBUG
  //printf("%d players\n", i);
#endif

  bf2->serverinfo.numplayers = i;

  ActionBF2UpdateAllPlayers(bf2);

  re = i;
_end:

  //bf2server->lastexploitcheck = DoubleTime();
  //bf2server->playerschecked = i;

  return re;
}

#ifdef _BF2_REQUESTCHAT
static int PRServerChat(bf2server_t *bf2, char *rv, int sz) {
  char *ch[_BF2_MAX_PLAYERCHAT];
  char *pfx[_BF2_MAX_PLAYERPREFIX] = {"", "<TEAM>", "<SQUAD>", "<COMMANDER>", "*DEAD*"};
  char chop[_BF2_MAX_PLAYERCHOP+1];
  char *chat, *prefix;
  char *s, *p, *q;
  int i, dead, dis, id;
  bf2serverinfo_t *si;

  si = &bf2->serverinfo;
  s = rv;

#ifdef _DEBUG
  printf("PRServerChat() [%s]\n", bf2->group);
#endif

  while (*s && *s != '\n') {
    q = strchr(s, '\r');
    if (!q) {
      return 0;
    }
    *q++ = '\0';
    
    for (i = 0; i < 6; i++) {
      if (!s) {
        return 0;
      }

      p = strchr(s, '\t');
      if (p) {
        *p++ = '\0';
      }

      switch (i) {
        case 0: ch[_BF2_SERVERCHAT_PLAYERID] = s; break;
        case 1: ch[_BF2_SERVERCHAT_PLAYERNAME] = s; break;
        case 2: ch[_BF2_SERVERCHAT_TEAM] = s; break;
        case 3: ch[_BF2_SERVERCHAT_CHANNEL] = s; break;
        case 4: ch[_BF2_SERVERCHAT_TIME] = s; break;
        case 5: ch[_BF2_SERVERCHAT_TEXT] = s; break;
      }

      s = p;
    }

    i = atoi(ch[_BF2_SERVERCHAT_TEAM]);

    if (i == 1) {
      ch[_BF2_SERVERCHAT_TEAM] = si->team1.name;
    } else if (i == 2) {
      ch[_BF2_SERVERCHAT_TEAM] = si->team2.name;
    }

    dead = 0;
    i = 0;

    if (!strncmp(ch[_BF2_SERVERCHAT_TEXT], "HUD_TEXT_CHAT_TEAM", 18)) {
      prefix = pfx[_BF2_CHARPREFIX_TEAM];
      chat = ch[_BF2_SERVERCHAT_TEXT] + 18;
    }
    else if (!strncmp(ch[_BF2_SERVERCHAT_TEXT], "HUD_TEXT_CHAT_SQUAD", 19)) {
      prefix = pfx[_BF2_CHARPREFIX_SQUAD];
      chat = ch[_BF2_SERVERCHAT_TEXT] + 19;
    }
    else if (!strncmp(ch[_BF2_SERVERCHAT_TEXT], "HUD_TEXT_CHAT_COMMANDER", 23)) {
      prefix = pfx[_BF2_CHARPREFIX_COMMANDER];
      chat = ch[_BF2_SERVERCHAT_TEXT] + 23;
    }
    else {
      prefix = pfx[_BF2_CHARPREFIX_NONE];
      chat = ch[_BF2_SERVERCHAT_TEXT];
    }

    if (!strncmp(chat, "HUD_CHAT_DEADPREFIX", 19)) {
      dead = 1;
      chat += 19;
    }
    else if (!strncmp(chat, "*§1DEAD§0*", 10)) {
      dead = 1;
      chat += 10;
    }

    while (*chat && *chat == ' ') {
      i++;
      chat++;
    }

    _strnprintf(chop, sizeof(chop), " (%d)", i);

    id = atoi(ch[_BF2_SERVERCHAT_PLAYERID]);

    if (id == -1) { //admin
      //dis = FilterBF2Message(chat);
      dis = 1; //TODO: message filters
    } else {
      dis = 1;
    }

    if (dis) {
      //TODO: do something with chat
      /*
      SayBF2(MSG_PRIVMSG, va("-"IRC_BOLD"%s"IRC_BOLD"- %s %s%s"IRC_BOLD"%s"IRC_BOLD"%s: %s", //%s%s%s",
          bf2server->group,
          //ch[SERVERCHAT_TIME],
          ch[SERVERCHAT_TEAM],
          //
          (dead ? pfx[CHARPREFIX_DEAD] : pfx[CHARPREFIX_NONE]),
          prefix,
          ch[SERVERCHAT_PLAYERNAME],
          (i) ? chop : pfx[CHARPREFIX_NONE],
          chat
          //hostmask ? " "IRC_BOLD"["IRC_BOLD : "",
          //hostmask ? hostmask : "",
          //hostmask ? IRC_BOLD"]"IRC_BOLD : ""
          )
        );
      */
    }
#ifdef _DEBUG
    else
    {
      printf("********message rejected********: %s\n", chat);
    }
#endif

    s = ++q;
  }

  return 1;
}
#endif

static int PRLoginReply(bf2server_t *bf2, char *rv, int sz) {
  char buf[128+1];
  double t;

  if (sz == 1 && *rv == '\n') {
    AddRequest(bf2, _BF2_PACKET_ISLOGIN);
    return 1;
  }

  if (sz < 16) {
    return 0;
  }

  if (!strncmp(rv + 15, "failed", 6)) {
    //PRPrint(rv, sz);
    EndBF2Connection(bf2);
    bf2->state = SOCKETSTATE_FAILEDCONNECT;
    return 1;
  }

  if (strncmp(rv + 15, "successful", 10)) {
    return 0;
  }

  bf2->state = SOCKETSTATE_CONNECTED;
  bf2->lastconnect = 0.0;

  t = DoubleTime();
#ifdef _DEBUG
  printf("start delay of: %d\n", bf2->startdelay);
#endif
  bf2->lastreqplayers = t + bf2->startdelay;
  bf2->lastreqchat = t + bf2->startdelay;

  AddRequest(bf2, _BF2_PACKET_PRINT);
  _strnprintf(buf, sizeof(buf), "\x02" "bf2cc setadminname %s\n", bf2->adminname);
  SendBF2Socket(bf2, buf, -1);

  AddRequest(bf2, _BF2_PACKET_HIDE);
  SendBF2Socket(bf2, "\x02" "bf2cc monitor 1\n", -1);

  AddRequest(bf2, _BF2_PACKET_SETGAMEPORT);
  SendBF2Socket(bf2, "\x02" "exec sv.serverPort\n", -1);

  AddRequest(bf2, _BF2_PACKET_SETQUERYPORT);
  SendBF2Socket(bf2, "\x02" "exec sv.gamespyPort\n", -1);

#ifdef NVEDEXT
  AddRequest(bf2, _BF2_PACKET_NVEDEXTVERSION);
  _strnprintf(buf, sizeof(buf), "\x02%s %c\n", _BF2_PROTOCOL_PREFIX, _BF2_PROTOCOL_VERSION);
  SendBF2Socket(bf2, buf, -1);
#endif

  RequestBF2ServerInfo(bf2);

  ActionBF2Connect(bf2);

#ifdef _DEBUG
  dump(rv, sz);
#endif

  return 1;
}

static int PRSetPort(int *port, char *rv, int sz) {
  assert(port);

  if (sz < 3) {
    return 0;
  }

  rv[sz-2] = '\0';

  *port = atoi(rv);

  return 1;
}

#ifdef NVEDEXT
static void PRSetNvedExt(bf2server_t *bf2, char *rv, int sz) {
  if ((sz < 8) || (rv[0] != _BF2_PROTOCOL_VERSION)) {
    return;
  }
  bf2->nvedext = (!strncmp(rv + 1, "NvEdExt", 7));
}

static void PRProcessFile(bf2server_t *bf2, char *rv, int sz) {
  assert(bf2->pbshot);

  fwrite(rv, sizeof(char), sz, bf2->pbshot);
  bf2->pbshotrecv += sz;

#ifdef SUPERDEBUG
  //printf("PRProcessFile(): +%d for %d/%d\n", sz, bf2->pbshotrecv, bf2->pbshotsize);
#endif

  if (bf2->pbshotrecv >= bf2->pbshotsize) {
    ActionBF2PlayerShotDone(bf2, 1);
  }
}
#endif

/* /////////////////////////////////////////

MISC

///////////////////////////////////////// */

bf2server_t *FindBF2Server(char *group, struct sockaddr_in *a, bf2server_t *head) {
  bf2server_t *bf2;

  for (bf2 = head; bf2; bf2 = bf2->next) {
    if (!strcmp(bf2->group, group) && (bf2->server.sin_addr.s_addr == a->sin_addr.s_addr)) {
      return bf2;
    }
  }

  return NULL;
}

bf2player_t *FindBF2PlayerByName(bf2server_t *bf2, char *name) {
  bf2player_t *pl;

  if (!name) {
    return NULL;
  }

  for (pl = bf2->players; pl; pl = pl->next) {
    if (!strcmp(pl->name, name)) {
      return pl;
    }
  }

  return NULL;
}

bf2player_t *FindBF2PlayerById(bf2server_t *bf2, uint64_t id) {
  bf2player_t *pl;

  if (!id) {
    return NULL;
  }

  for (pl = bf2->players; pl; pl = pl->next) {
    if (pl->playerid == id) {
      return pl;
    }
  }

  return NULL;
}

void InitBF2Player(bf2player_t *pl) {
  assert(pl);

  pl->playerid = 0;
  pl->addressid = 0;
  pl->keyhashid = 0;

  pl->id = -1;
  pl->cycle = 0;
  pl->flags = 0;
  pl->name[0] = '\0';
  pl->hash[0] = '\0';
  pl->guid[0] = '\0';
  pl->address[0] = '\0';
  pl->pid[0] = '\0';

#ifdef NVEDEXT
  pl->pbss = NULL;
#endif
}

bf2player_t *NewBF2Player(bf2player_t **parent) {
  bf2player_t *pl;

#ifdef SUPERDEBUG
  //printf("NewBF2Player()\n");
#endif

  pl = (bf2player_t *) _malloc_(sizeof(bf2player_t));
  if (!pl) {
    return NULL;
  }

  InitBF2Player(pl);

  if (*parent)
    (*parent)->prev = pl;
  pl->next = *parent;
  pl->prev = NULL;
  *parent = pl;

  return pl;
}

void FreeBF2Player(bf2player_t *pl, bf2player_t **parent) {
#ifdef SUPERDEBUG
  //printf("FreeBF2Player()\n");
#endif

  if (!pl) {
    return;
  }

#ifdef NVEDEXT
  if (pl->pbss) {
    _free_(pl->pbss);
  }
#endif

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

void FreeBF2AllPlayers(bf2player_t **parent) {
  bf2player_t *pl, *n;

#ifdef SUPERDEBUG
  printf("FreeBF2AllPlayers()\n");
#endif

  assert(parent);

  n = *parent;
  while (n) {
    pl = n;
    n = n->next;

    FreeBF2Player(pl, NULL);
  }

  *parent = NULL;
}

#ifdef NVEDEXT
bf2pbss_t *NewBF2PBShot(void) {
  bf2pbss_t *pbss;

  pbss = (bf2pbss_t *) _malloc_(sizeof(bf2pbss_t));
  if (!pbss) {
    return NULL;
  }

  pbss->filename[0] = '\0';
  pbss->shotfilename[0] = '\0';
  pbss->md5[0] = '\0';
  pbss->shottime = 0.0;

  return pbss;
}
#endif

#ifdef NVEDEXT
void FreeBF2PBShot(bf2pbss_t **pbss) {
  assert(pbss);

  if (!*pbss) {
    return;
  }
  _free_(*pbss);
  *pbss = NULL;
}
#endif

void RequestBF2ServerInfo(bf2server_t *bf2) {
  AddRequest(bf2, _BF2_PACKET_SERVERINFO);
  SendBF2Socket(bf2, "\x02" "bf2cc si\n", -1);
}

void RequestBF2PlayersFull(bf2server_t *bf2) {
  AddRequest(bf2, _BF2_PACKET_FULLPLAYERS);
  SendBF2Socket(bf2, "\x02" "bf2cc pl" "\n", -1);
}

void RequestBF2ServerChat(bf2server_t *bf2) {
  AddRequest(bf2, _BF2_PACKET_SERVERCHAT);
  SendBF2Socket(bf2, "\x02" "bf2cc clientchatbuffer" "\n", -1);
}

#ifdef NVEDEXT
void RequestBF2ServerFile(bf2server_t *bf2, char *name, int pos) {
  if (bf2->nvedext) {
    char buf[MAX_PATH];

#ifdef SUPERDEBUG
    printf("RequestBF2ServerFile(): requesting %s\n", name);
#endif

    _strnprintf(buf, sizeof(buf), "%s %c %s %d\n", _BF2_PROTOCOL_PREFIX, _BF2_PROTOCOL_PBSHOT, name, pos);

    //AddRequest(bf2, _BF2_PACKET_REQUESTFILE); -- this is OOB not a request!
    SendBF2Socket(bf2, buf, -1);
  }
}
#endif

void AddRequest(bf2server_t *bf2, int type) {
  bf2->requests[bf2->reqend++] = type;
  if (bf2->reqend >= _BF2_MAX_QUEUE)
    bf2->reqend = 0;
}

void RemoveTopRequest(bf2server_t *bf2) {
  bf2->reqstart++;
  if (bf2->reqstart >= _BF2_MAX_QUEUE) {
    bf2->reqstart = 0;
  }
}

void RemoveRequests(bf2server_t *bf2) {
  bf2->reqstart = 0;
  bf2->reqend = 0;
}

void InitBF2ServerInfo(bf2serverinfo_t *si) {
  assert(si);

  //si->scriptver[0] = '\0';
  si->currentmap[0] = '\0';
  si->nextmap[0] = '\0';
  si->hostname[0] = '\0';
  si->gamemode[0] = '\0';
  si->moddir[0] = '\0';
  si->gameport = 0;
  si->queryport = 0;
  //si->worldsize[0] = '\0';
  //si->state = 0;
  si->connectedplayers = 0;
  si->joiningplayers = 0;
  si->numplayers = 0;
  si->maxplayers = 0;
  //si->maptime = 0;
  //si->mapleft = 0;
  //si->timelimit = 0;
  //si->autobalance = 0;
  si->ranked = 0;
  //si->querytime = 0;
  //si->reserved = 0;
  si->team1.name[0] = '\0';
  //si->team1.players = 0;
  //si->team1.state = 0;
  //si->team1.starttickets = 0;
  //si->team1.tickets = 0;
  //si->team1.ticketrate = 0;
  si->team2.name[0] = '\0';
  //si->team2.players = 0;
  //si->team2.state = 0;
  //si->team2.starttickets = 0;
  //si->team2.tickets = 0;
  //si->team2.ticketrate = 0;
  //si->iskarkand = 0;

  //InitBF2Players(si);
}

#if 0
void InitBF2Players(bf2serverinfo_t *si) {
  int i;
  bf2player_t *pl;

  assert(si);

  for (i = 0; i < _BF2_MAX_BF2PLAYERS; i++) {   
    pl = &si->players[i];

    pl->name[0] = '\0';
    pl->address[0] = '\0';
    pl->position[0] = '\0';
    pl->pid[0] = '\0';
    pl->hash[0] = '\0';
    pl->isalive = 0;

    pl->id = -1;
    pl->score = 0;
    pl->deaths = 0;
    pl->ping = 0;
    pl->team = 0;
  }
}
#endif

#define _BF2_MAGIC_OFFSET1 76
#define _BF2_MAGIC_OFFSET2 29

void destroy(char *p, char *dec, int sz) {
  char buf[8];
  char *s, *q;
  int i, j, k, offset;
  union {
    unsigned short h;
    char x[2];
  } a;

  k = 0;
  q = dec;
  offset = 0;

  if (*p) {
    offset = *(p++) - '0';
  }

  while (*p) {
    j = 0;
    s = buf;
    //for (i = 0; i < 6 && *p; *p++, i++) {
    for (i = 0; i < 6 && *p; i++) {
      if (j < sizeof(buf)) {
        //*s++ = *p;
        *s++ = *p;
        j++;
      }
      p++;
    }
    *(s + ((j) ? -1 : 0)) = '\0';

    if (buf[0] && k < sz) {
      a.h = atoi(buf);
      *q++ = a.x[0] - offset - _BF2_MAGIC_OFFSET1;
      *q++ = a.x[1] - _BF2_MAGIC_OFFSET2 - offset;
    }
  }
  *q = '\0';
}

#ifdef _DEBUG
int dump(char *rv, int sz) {
  char *s, *p;

  if (!sz) {
    return 0;
  }

  if (rv[sz - 1] == '\x04') {
    rv[sz - 1] = '\0';
  }

  s = rv;
  while (s && *s) {
    p = strchr(s, '\n');
    if (p) {
      *p++ = '\0';
    }
#ifdef _DEBUG
    printf("%s\n", s);
#endif
    s = p;
  }

  return 1;
}
#endif

