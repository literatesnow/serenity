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

#include "serenity.h"

void packet1942(char *s, int id);
int info1942(char *s);
int match1942(int port, char *s);
int parse1942(server_t *server);
int player1942(player_t *player);

void packetViet(char *s, int id);
int infoViet(char *s);
int matchViet(int port, char *s);
int parseViet(server_t *server);
int playerViet(player_t *player);

//this is cool, dynamic functions, i think it's called function pointers
//really saves on: if (somegame) { do this } else if (some other game) { do that } else if (...
//4am note: these suck for debugging
typedef void (*game_packet)(char *s, int id);
typedef int (*game_info)(char *s);
typedef int (*game_match)(int port, char *s);
typedef int (*game_parse)(server_t *server);
typedef int (*game_player)(player_t *player);

typedef struct server_type_s
{
  int game_type;
  char *game_desc;
  char *game_short_desc;
  int default_game_port;
  int default_query_port;
  int query_length;
  game_packet packet_query;
  game_info info_query;
  game_match match_query;
  game_parse parse_query;
  game_player check_player;
  char *import_file;
} server_type_t;

