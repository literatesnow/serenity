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

#include "game.h"

extern info_t *info;

server_type_t server_type[SUPPORTED_GAMES] =
{
  {
    BF1942_GAME,
    "BattleField 1942",
    "BF:42",
    14567,
    23000,
    BF1942_QUERY_LENGTH,
    packet1942,
    info1942,
    match1942,
    parse1942,
    player1942,
    "serenity.bf42"
  },
  {
    BFVIET_GAME,
    "BattleField Vietnam",
    "BF:V",
    15567,
    23000,
    BFVIET_QUERY_LENGTH,
    packetViet,
    infoViet,
    matchViet,
    parseViet,
    playerViet,
    "serenity.bfv"
  }
};

/*
 * packetViet - creates a query packet
 */
void packetViet(char *s, int id)
{
  char query[BFVIET_QUERY_LENGTH+1];
  char idnum[6];
  char *p;
  int i;

  strlcpy(query, BFVIET_QUERY, sizeof(query));
  _strnprintf(idnum, sizeof(idnum), "%4d", 1000 + id - info->local_port);
  query[2] = '\0';
  query[3] = idnum[0];
  query[4] = idnum[1];
  query[5] = idnum[2];
  query[6] = idnum[3];

  p = query;
  for (i = 0; i < BFVIET_QUERY_LENGTH; i++)
    *s++ = *p++;
  *s = 0;
}

/*
 * playerViet - checks for valid player, none needed for this game
 */
int playerViet(player_t *player)
{
  return 1;
}

/*
 * parseViet - parses packet reply from the server
 * While Battlefield Vietnam's query protocol is not as buggy as Battlefield 1942, it still sucks.
 */
int parseViet(server_t *server)
{
  char *s, *p;
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  player_t *player;
  int i, j, l, np;
  int pos[7];

  s = server->packet;

  if (*s == '\\') //nice one, retard, you've given us an old protocol packet
    return 0;

  if (*s != '\0') //our header starts with a null
    return 0;

  for (i = 0; i < 7; i++)
    pos[i] = -1;

  //for (i = 0; i < 5; i++) //strip header
  //  s++;
  s += 5;

  //serverinfo
  i = 0;
  while (1)
  {
    i++;
    for (j = 0; j < 2; j++)
    {
      p = (!j) ? key : value;
      while (*s)
	  		*p++ = *s++;
		  *p = 0;

      s++;
    }
    if (!key[0] && !value[0]) //blank key and value ends serverinfo...
      break;

    if (!strcmp(key, "unknown") && !value[0]) //...but there's an unknown key there? what the hell?!
      break;

    if (!strcmp(key, "sv_punkbuster")) //v1.01 - looks like there's no difference between serverinfo and rules. morons.
      break;

    if (!key[0] || !value[0]) //skip blank keys/values
      continue;

    if (!_strcasecmp(key, "hostname"))
      strlcpy(server->host_name, value, sizeof(server->host_name));
    else if (!_strcasecmp(key, "mapname"))
      strlcpy(server->map_name, value, sizeof(server->map_name));
    else if (!_strcasecmp(key, "hostport"))
      server->game_port = atoi(value);
    else if (!_strcasecmp(key, "gametype"))
      strlcpy(server->game_type, value, sizeof(server->game_type));
    else if (!_strcasecmp(key, "game_id"))
      strlcpy(server->game_id, value, sizeof(server->game_id));
    else if (!_strcasecmp(key, "gamever"))
      strlcpy(server->version, value, sizeof(server->version));
    else if (!_strcasecmp(key, "numplayers"))
      server->num_players = atoi(value);
    else if (!_strcasecmp(key, "maxplayers"))
      server->max_players = atoi(value);
  }

  //rules
  i = 0;
  while (1)
  {
    for (j = 0; j < 2; j++)
    {
      p = (!j) ? key : value;
      while (*s)
	  		*p++ = *s++;
		  *p = 0;

      s++;
    }

    if (!key[0] && !value[0]) //blank key and value ends serverinfo...
      break;
    if (!key[0])
      continue;

    if (newRule(server))
    {
      strlcpy(server->rules->key, key, sizeof(server->rules->key));
      strlcpy(server->rules->value, value, sizeof(server->rules->value));
    }
    else
    {
      echo(DEBUG_FATAL, "parseViet: malloc failure for rule (key(%s) value(%s))\n", key, value);
    }
  }
  if (!*s) //ran off the end of the world
    return 1;

  //order of things to come
  np = *s;
  s++;
  l = 0;
  while (*s)
  {
    p = key;
    while (*s)
      *p++ = *s++;
    *p = 0;

    if (!strcmp(key, "player_"))
      pos[0] = l;
    else if (!strcmp(key, "keyhash_"))
      pos[1] = l;   
    else if (!strcmp(key, "deaths_"))
      pos[2] = l;
    else if (!strcmp(key, "kills_"))
      pos[3] = l;
    else if (!strcmp(key, "ping_"))
      pos[4] = l;
    else if (!strcmp(key, "score_"))
      pos[5] = l;
    else if (!strcmp(key, "team_"))
      pos[6] = l;

    l++;
    s++;
  }
  s++;

  //add players
  for (i = 0; i < np; i++)
  {
    if (!newPlayer(server))
    {
      echo(DEBUG_FATAL, "parseViet: malloc failure for player id:%d\n", i);
      continue;
    }
    player = server->players;
    player->end = l;

    for (j = 0; j < l; j++)
    {
      p = value;
      while (*s)
        *p++ = *s++;
      *p = 0;
      s++;

      player->userid = i;
      if (pos[0] == j)
        strlcpy(player->name, value, sizeof(player->name));
      else if (pos[1] == j)
        strlcpy(player->keyhash, value, sizeof(player->keyhash));
      else if (pos[2] == j)
        player->deaths = atoi(value);
      else if (pos[3] == j)
        player->frags = atoi(value);
      else if (pos[4] == j)
        player->ping = atoi(value);
      else if (pos[5] == j)
        player->score = atoi(value);
      else if (pos[6] == j)
        strlcpy(player->team, value, sizeof(player->team));
    }
  }

  return 1;
}

/*
 * infoViet - check if packet reply was valid
 */
int infoViet(char *s)
{
  if (*s != '\0' || *s == '\\')
    return 0; //we've probably got an old style protocol packet instead *sigh*

  return 1;
}

/*
 * matchViet - matches a port
 */
int matchViet(int port, char *s)
{
  char *p;
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  int i;

  if (*s != '\0')
    return 0;

  //for (i = 0; i < 5; i++)
  //s++;
  s += 5;

  while (1)
  {
    for (i = 0; i < 2; i++)
    {
      p = (!i) ? key : value;
      while (*s)
	  		*p++ = *s++;
		  *p = 0;

      s++;
    }
    if ((!key[0] && !value[0]) || (!strcmp(key, "unknown") && !value[0]) || !strcmp(key, "sv_punkbuster")) //hack and a half
      break;
    if (!key[0] || !value[0])
      continue;

    if (!_strcasecmp(key, "hostport") && port == atoi(value))
      return 1;
  }  

  return 0;
}

/*
 * packet1942 - construct a query packet
 */
void packet1942(char *s, int id)
{
  char query[BF1942_QUERY_LENGTH+1];
  char *p;

  strlcpy(query, BF1942_QUERY, sizeof(query)); //fix strlcpy ? -done
  p = query;

  while (*p)
    *s++ = *p++;
  *s = 0;
}

/*
 * player1942 - check if this player is valid
 */
int player1942(player_t *player)
{
  return (player->end == BF1942_PLAYER_DATA);
}

/*
 * parse1942 - reply packet parser
 * Battlefield 1942's protocol is _hopeless_. I've got no idea what GameSpy were smoking when they designed it.
 * Game developers, I implore you, make your own decent query protocol! (Or steal iD Software's)
 */
int parse1942(server_t *server)
{
  char *s, *p, *q;
  char key[MAX_KEY+1];
  char value[MAX_VALUE+2];
  char id[16];
  int tr[64];
  player_t *player;
  int i, j, n, b;

  s = server->packet;

  if (!*s)
    return 0;

  n = 0;

  for (i = 0; i < 64; i++)
    tr[0] = -1;

  while (*s)
  {
	  while (*s == '\\')
      s++;
	  if (!*s)
      break;
    
    for (i = 0; i < 2; i++)
    {
      p = (!i) ? key : value;
      while (*s && *s != '\\')
	  		*p++ = *s++;
		  *p = 0;

      if (*s)
		    s++; 
    }

    if (!_strcasecmp(key, "final") || !_strcasecmp(key, "queryid"))
    {
      n++;
      continue;
    }

    if (!key[0] || !value[0]) //hello buggy gamespy protocol!
      continue;

    //sigh
    b = (!_strncasecmp(key, "deaths_", 7) || !_strncasecmp(key, "keyhash_", 8) || !_strncasecmp(key, "kills_", 6) ||
         !_strncasecmp(key, "ping_", 5) || !_strncasecmp(key, "playername_", 11) || !_strncasecmp(key, "score_", 6) ||
         !_strncasecmp(key, "team_", 5)) ? 1 : 0;
    
    if (!b && !_strcasecmp(key, "hostname"))
      strlcpy(server->host_name, value, sizeof(server->host_name));
    else if (!b && !_strcasecmp(key, "mapname"))
      strlcpy(server->map_name, value, sizeof(server->map_name));
    else if (!b && !_strcasecmp(key, "hostport"))
      server->game_port = atoi(value);
    else if (!b && !_strcasecmp(key, "gametype"))
      strlcpy(server->game_type, value, sizeof(server->game_type));
    else if (!b && !_strcasecmp(key, "gameid"))
      strlcpy(server->game_id, value, sizeof(server->game_id));
    else if (!b && !_strcasecmp(key, "version"))
      strlcpy(server->version, value, sizeof(server->version));
    else if (!b && !_strcasecmp(key, "numplayers"))
      server->num_players = atoi(value);
    else if (!b && !_strcasecmp(key, "maxplayers"))
      server->max_players = atoi(value);
    else if (!b)
    {
      if (newRule(server))
      {
        strlcpy(server->rules->key, key, sizeof(server->rules->key));
        strlcpy(server->rules->value, value, sizeof(server->rules->value));
      }
      else
      {
        echo(DEBUG_FATAL, "parse1942: malloc failure for rule (key(%s) value(%s))\n", key, value);
      }
    }

    if (!b)
      continue;

 		p = key;
    q = id;
    while (*p && *p != '_')
      *p++;
    if (*p)
      *p++;
		while (*p)
			*q++ = *p++;
		*q = 0;

    //yuck, i don't like this,
    //what happens is that it returns duplicate players which aren't even on the server and are > num_players
    //so basically any player with an id > num_players gets dumped
    i = atoipz(id);
    if (!i || i > server->num_players) //buggy gamespy protocol, we meet again...
    {
      for (j = 0; j < 64; j++) //this mess means we don't count a trash more than once
      {
        if (i == tr[j])
        {
          j = -1;
          break;
        }
      }
      if (j == -1)
        continue;

      j = 0;
      while (j < 64 && tr[j] == -1)
        j++;
      if (j > 63)
        continue;

      tr[j] = i;
      server->trashed++;
      echo(DEBUG_FLOOD, "parse1942: trashing player id:%d\n", i);
      continue;
    }
    i--;  

    //find player that belongs to this id
    player = server->players;
    while (player)
    {
      if (player->userid == i)
      {
        break;
      }
      player = player->next;
    }

    //new player
    if (!player)
    {
      if (!newPlayer(server))
      {
        echo(DEBUG_FATAL, "parse1942: malloc failure for player id:%d\n", i);
        continue;
      }
      player = server->players;
      player->userid = i;
    }

    if (!_strncasecmp(key, "deaths_", 7))
      player->deaths = atoi(value);
    else if (!_strncasecmp(key, "keyhash_", 8))
      strlcpy(player->keyhash, value, sizeof(player->keyhash));
    else if (!_strncasecmp(key, "kills_", 6))
      player->frags = atoi(value);
    else if (!_strncasecmp(key, "ping_", 5))
      player->ping = atoi(value);
    else if (!_strncasecmp(key, "playername_", 11))
      strlcpy(player->name, value, sizeof(player->name));
    else if (!_strncasecmp(key, "score_", 6))
      player->score = atoi(value);
    else if (!_strncasecmp(key, "team_", 5))
      strlcpy(player->team, value, sizeof(player->team));

    player->end++;
  }

  //we should have (queryid + final keys) == packet_recieved), if not, trash the lot
  return (n == (server->packet_recieved + 1)) ? 1 : 0;
}

/*
 * info1942 - reply packet check
 */
int info1942(char *s)
{
  if (!*s || *s != '\\')
    return 0;

  return 1;
}

/*
 * match1942 - matches a port
 */
int match1942(int port, char *s)
{
  char *p;
  char key[MAX_KEY+1];
  char value[MAX_VALUE+2];
  int i;

  if (!*s)
    return 0;

  while (*s)
  {
	  while (*s == '\\')
      s++;
	  if (!*s)
      break;
    
    for (i = 0; i < 2; i++)
    {
      p = (!i) ? key : value;
      while (*s && *s != '\\')
	  		*p++ = *s++;
		  *p = 0;

      if (*s)
		    s++; 
    }

    if (!key[0] || !value[0])
      continue;

    if (!_strcasecmp(key, "hostport") && port == atoi(value))
      return 1;
  }

  return 0;
}

