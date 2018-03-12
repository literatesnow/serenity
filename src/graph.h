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

#define GRAPH_ID 204
#define GRAPH_VERSION 49

#define GRAPH_HEADER_ID 0
#define GRAPH_HEADER_VERSION 1
#define GRAPH_HEADER_TIME_DAY 2
#define GRAPH_HEADER_TIME_WEEK 6 
#define GRAPH_HEADER_TIME_MONTH 10
#define GRAPH_HEADER_TIME_YEAR 14
#define GRAPH_HEADER_SIZE 18

#define GRAPH_DAY_OFFSET (GRAPH_HEADER_SIZE)
#define GRAPH_WEEK_OFFSET (GRAPH_HEADER_SIZE + GRAPH_SIZE)
#define GRAPH_MONTH_OFFSET (GRAPH_HEADER_SIZE + (GRAPH_SIZE * 2))
#define GRAPH_YEAR_OFFSET (GRAPH_HEADER_SIZE + (GRAPH_SIZE * 3))

#define GRAPH_WIDTH 440 //xxx 320
#define GRAPH_HEIGHT 101
#define GRAPH_AREA_WIDTH 360 //xxx 240
#define GRAPH_AREA_HEIGHT 68

#define GRAPH_AREA_X 50
#define GRAPH_AREA_Y 81
#define GRAPH_YDIVLINES 5
#define GRAPH_YDIVPOS (GRAPH_AREA_HEIGHT / (GRAPH_YDIVLINES - 1))
#define GRAPH_LEFT_TEXT "Players"
#define GRAPH_FONT "lucon.ttf"
#define GRAPH_FONT_SIZE 7
#define GRAPH_FONT_ANGLE 90

#define GRAPH_DAY_XPOS ((60 * 24) / GRAPH_AREA_WIDTH)
#define GRAPH_WEEK_XPOS ((60 * 24 * 7) / GRAPH_AREA_WIDTH)
#define GRAPH_MONTH_XPOS ((60 * 24 * 30) / GRAPH_AREA_WIDTH)
#define GRAPH_YEAR_XPOS ((60 * 24 * 360) / GRAPH_AREA_WIDTH)

#define GRAPH_DAY_XLINES (24 + 1)
#define GRAPH_WEEK_XLINES (7 + 1)
#define GRAPH_MONTH_XLINES (4 + 1)
#define GRAPH_YEAR_XLINES (12 + 1)

#define GRAPH_SIZE 360 //xxx 240
#define GRAPH_FILE_SIZE ((GRAPH_SIZE * 4) + GRAPH_HEADER_SIZE)

void graphServersPHP(void);

long readlong(unsigned char *s);
void writelong(unsigned char *s, int val);

void shiftleft(unsigned char *s, int offset, int div, unsigned char value);

