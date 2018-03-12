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
extern server_t *servers;
extern server_type_t server_type[SUPPORTED_GAMES];

/*
 * graphServers - updates the usage graphs
 * This code is totally insane. INSANE. The basic idea is that we record the number of players on a server to a
   file every time that the server is updated then make a graph image. Well, that's the idea anyway. I spent far
   too much time getting this right.
  
   Graph File Format:

     Header:
     Byte - Description
      0   -  Header ID, so we can reject bogus files
      1   -  Version of the graph file, incase I decide to add something later and it breaks
      2   -  Time the graph was last updated (1 of 4 byte long)
      3   -  Time the graph was last updated (2 of 4 byte long)
      4   -  Time the graph was last updated (3 of 4 byte long)
      5   -  Time the graph was last updated (4 of 4 byte long)
      
     Data:
      Each byte represents a line on the graph
      360 bytes for day
      360 bytes for week
      360 bytes for month
      360 bytes for year

   Code:
   The code is a bit of a mess, but it works so I don't question its authority.
   It might be possible to have the script creation inside a loop becuase there is repeated code there.
   For a rainy day.
 */
void graphServersPHP(void)
{
  server_t *s;
  char st[MAX_FILE_PATH+MAX_PHP_PARAMETERS+1];
  char name[MAX_FILE_PATH+1];
  unsigned char buf[GRAPH_FILE_SIZE];
  FILE *fp;
	time_t current;
  time_t last;
	struct tm *now;
  int i, j;
  double d;

  if (!info->use_graphs)
    return;

	time(&current);
	now = localtime(&current);
  mkdir(info->graph_dir);
  escape(info->graph_dir, sizeof(info->graph_dir)); //move me

  if (!now)
  {
    echo(DEBUG_FATAL, "graphServers: bad date\n");
    return;
  }

  for (s = servers; s; s = s->next)
  {
    if (!s->got_reply)
      continue;

    //203.999.929.699_65535.graph
    _strnprintf(name, sizeof(name), "%s/serenity_%s_%d.graph", DATA_DIRECTORY, s->ip, s->game_port);
    fp = fopen(name, "rb");
    if (!fp)
    {
      //new graph
      fp = fopen(name, "wb");
      if (!fp)
      {
        echo(DEBUG_INFO, "graphServers: couldn\'t open graph file %s\n", name);
        continue;
      }

      memset(buf, '\0', sizeof(buf));
      buf[GRAPH_HEADER_ID] = GRAPH_ID;
      buf[GRAPH_HEADER_VERSION] = GRAPH_VERSION;
      writelong(&buf[GRAPH_HEADER_TIME_DAY], current);
      writelong(&buf[GRAPH_HEADER_TIME_WEEK], current);
      writelong(&buf[GRAPH_HEADER_TIME_MONTH], current);
      writelong(&buf[GRAPH_HEADER_TIME_YEAR], current);

      //fix me - just memset this - done
      /*for (i = 0; i < GRAPH_SIZE; i++)
        buf[GRAPH_DAY_OFFSET + i] = 0;

      for (i = 0; i < GRAPH_SIZE; i++)
        buf[GRAPH_WEEK_OFFSET + i] = 0;

      for (i = 0; i < GRAPH_SIZE; i++)
        buf[GRAPH_MONTH_OFFSET + i] = 0;

      for (i = 0; i < GRAPH_SIZE; i++)
        buf[GRAPH_YEAR_OFFSET + i] = 0;*/

      fwrite(buf, 1, sizeof(buf), fp);
      fclose(fp);
    }
    //
    fp = fopen(name, "rb");
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t open graph file %s\n", name);
      continue;
    }

    fread(buf, 1, sizeof(buf), fp); //read entire file

    freopen(name, "wb", fp);
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t reopen graph file %s\n", name);
      continue;
    }
  
    if ((buf[GRAPH_HEADER_ID] != GRAPH_ID) || (buf[GRAPH_HEADER_VERSION] != GRAPH_VERSION))
    {
      echo(DEBUG_INFO, "graphServers: bad header/version\n");
      continue;
    }

    //depending on how long since last update, move the graph lines left
    //day
    last = readlong(&buf[GRAPH_HEADER_TIME_DAY]);
    d = difftime(current, last) / 60;
    i = (int)(d / GRAPH_DAY_XPOS);
    shiftleft(buf, GRAPH_DAY_OFFSET, i, (unsigned char)s->num_players);
    if (i > 0)
      writelong(&buf[GRAPH_HEADER_TIME_DAY], current);

    //week
    last = readlong(&buf[GRAPH_HEADER_TIME_WEEK]);
    d = difftime(current, last) / 60;
    i = (int)(d / GRAPH_WEEK_XPOS);
    shiftleft(buf, GRAPH_WEEK_OFFSET, i, (unsigned char)s->num_players);
    if (i > 0)
      writelong(&buf[GRAPH_HEADER_TIME_WEEK], current);

    //month
    last = readlong(&buf[GRAPH_HEADER_TIME_MONTH]);
    d = difftime(current, last) / 60;
    i = (int)(d / GRAPH_MONTH_XPOS);
    shiftleft(buf, GRAPH_MONTH_OFFSET, i, (unsigned char)s->num_players);
    if (i > 0)
      writelong(&buf[GRAPH_HEADER_TIME_MONTH], current);

    //year
    last = readlong(&buf[GRAPH_HEADER_TIME_YEAR]);
    d = difftime(current, last) / 60;
    i = (int)(d / GRAPH_YEAR_XPOS);
    shiftleft(buf, GRAPH_YEAR_OFFSET, i, (unsigned char)s->num_players);
    if (i > 0)
      writelong(&buf[GRAPH_HEADER_TIME_YEAR], current);

    fwrite(buf, 1, sizeof(buf), fp);
    fclose(fp);

//START GRAPH DAY SCRIPT ************************************************************************************************
    _strnprintf(name, sizeof(name), "serenity_%s_%d_day", s->ip, s->game_port);
    fp = fopen(name, "wt");
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t open script %s\n", name);
      continue;
    }
    fprintf(fp, "<?php \n" \
                "\n" \
                "/* %s v%s - %s %s (%s)\n" \
                " * %s - automagically generated on %s.\n" \
                " */\n",
                HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, name, info->datetime);
    fprintf(fp, "\n" \
                "header(\"Content-type: image/png\");\n\n" \
                "$image = ImageCreate(%d, %d);\n" \
                "$white = ImageColorAllocate($image, 255, 255, 255);\n" \
                "$black = ImageColorAllocate($image, 0, 0, 0);\n" \
                "$lgrey = ImageColorAllocate($image, 192, 192, 192);\n" \
                "$dgrey = ImageColorAllocate($image, 128, 128, 128);\n" \
                "$red = ImageColorAllocate($image, 255, 0, 0);\n",
                GRAPH_WIDTH,
                GRAPH_HEIGHT);

    fprintf(fp, "\n/* vertical grid lines and horizontal text */\n\n");
    //far left vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X, GRAPH_AREA_Y + 1, GRAPH_AREA_X, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);
    //far right vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y + 1, GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);

    i = (int)(now->tm_min / GRAPH_DAY_XPOS);
    j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    j -= (i < 1) ? 0 : i;

    d = now->tm_hour;
    i = 0;

    while (j > GRAPH_AREA_X)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, %s);\n",
                  j,
                  GRAPH_AREA_Y + 1,
                  j,
                  GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1,
                  (i % 6 == 0) ? "$dgrey" : "$lgrey");
      if (i % 6 == 0)
      {
        fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%02.0f:00\", $textheight, $textwidth);\n" \
                    "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%02.0f:00\");\n",
                    GRAPH_FONT_SIZE,
                    GRAPH_FONT,
                    d,

                    GRAPH_FONT_SIZE,
                    j,
                    GRAPH_AREA_Y + GRAPH_FONT_SIZE,
                    GRAPH_FONT,
                    d);
      }
      d = (d - 1 < 0) ? 23 : d - 1;

      j -= GRAPH_AREA_WIDTH/24;
      i++;
    }

    fprintf(fp, "\n/* horizonal grid lines and vertical text */\n\n");
    for (i = 0, j = GRAPH_AREA_Y; i < GRAPH_YDIVLINES; i++, j -= GRAPH_YDIVPOS)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X - 1, j, GRAPH_AREA_X + GRAPH_AREA_WIDTH + 1, j);

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%d\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%d\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))),

                  GRAPH_FONT_SIZE,
                  (int)(GRAPH_AREA_X - (GRAPH_FONT_SIZE * 1.5)),
                  j,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))));
    }

    fprintf(fp, "\n/* graph lines */\n\n");

    for (i = 0; i < GRAPH_SIZE; i++)
    {
      if (buf[GRAPH_DAY_OFFSET + i] > s->max_players)
        buf[GRAPH_DAY_OFFSET + i] = s->max_players;
      if (buf[GRAPH_DAY_OFFSET + i] > 0)
      {
        fprintf(fp, "ImageLine($image, %d, %d, %d, %0.2f, $red);\n",
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - 1,
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - (((float)GRAPH_AREA_HEIGHT / s->max_players) * buf[GRAPH_DAY_OFFSET + i]));
      }
    }

    fprintf(fp, "\n/* text */\n\n");
    fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                "ImageTTFText($image, %d, 90, $textheight, %d+($textwidth/2), $black, \"%s\", \"%s\");\n",
                GRAPH_FONT_SIZE,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT,

                GRAPH_FONT_SIZE,
                GRAPH_HEIGHT / 2,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT);

    fprintf(fp, "\n\nImagePNG($image, \'%s/%s.png\');\nImageDestroy($image);\n",
                info->graph_dir,
                name);

    fprintf(fp, "\nfunction TTFBounds($fontsize, $fontangle, $font, $text, &$height, &$width)\n{\n" \
                "  $box = ImageTTFBBox($fontsize, $fontangle, $font, $text);\n" \
                "  $width = abs($box[2] - $box[0]);\n" \
                "  $height = abs($box[5] - $box[3]);\n}\n");

    fprintf(fp, "?>\n");
    fclose(fp);
    _strnprintf(st, sizeof(st), "%s %s %s", info->php_filepath, info->php_parameters, name);
    if (!run(st, "graphServers"))
      return;
    if (info->unlink)
      unlink(name);
//END SCRIPT DAY ********************************************************************************************************

//START GRAPH WEEK SCRIPT ***********************************************************************************************
    _strnprintf(name, sizeof(name), "serenity_%s_%d_week", s->ip, s->game_port);
    fp = fopen(name, "wt");
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t open script %s\n", name);
      continue;
    }
    //header
    fprintf(fp, "<?php \n" \
                "\n" \
                "/* %s v%s - %s %s (%s)\n" \
                " * %s - automagically generated on %s.\n" \
                " */\n",
                HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, name, info->datetime);
    fprintf(fp, "\n" \
                "header(\"Content-type: image/png\");\n\n" \
                "$image = ImageCreate(%d, %d);\n" \
                "$white = ImageColorAllocate($image, 255, 255, 255);\n" \
                "$black = ImageColorAllocate($image, 0, 0, 0);\n" \
                "$lgrey = ImageColorAllocate($image, 192, 192, 192);\n" \
                "$dgrey = ImageColorAllocate($image, 128, 128, 128);\n" \
                "$red = ImageColorAllocate($image, 255, 0, 0);\n",
                GRAPH_WIDTH,
                GRAPH_HEIGHT);

    fprintf(fp, "\n/* vertical grid lines and horizontal text */\n\n");
    //far left vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X, GRAPH_AREA_Y + 1, GRAPH_AREA_X, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);
    //far right vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y + 1, GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);

    //this is the position to start the dividers from, like starting in the middle of a day depending on the time
    //i = (int)(now->tm_min / (60 / GRAPH_DAY_XPOS));
    //i = (int)((now->tm_hour + 1) / (24 / GRAPH_WEEK_XPOS));
    //i = (int)now->tm_min;
    //j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    //j -= (i < 1) ? GRAPH_WEEK_XPOS : i;
    //i = (int)(now->tm_min / GRAPH_WEEK_XPOS);
    //j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    //j -= (i < 1) ? 0 : i;
    //#define GRAPH_DAY_XPOS ((60 * 24) / GRAPH_AREA_WIDTH) // = 6 of 1440 [240/24]
    //#define GRAPH_WEEK_XPOS ((60 * 24 * 7) / GRAPH_AREA_WIDTH) // = 42 of 10080 [240/7]
    //#define GRAPH_MONTH_XPOS ((60 * 24 * 30) / GRAPH_AREA_WIDTH) // = 180 of 43200
    //#define GRAPH_YEAR_XPOS ((60 * 24 * 366) / GRAPH_AREA_WIDTH) // = 2196 of 527040
    i = (int)(((now->tm_hour * 60) + now->tm_min) / GRAPH_WEEK_XPOS); 
    j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    j -= (i < 1) ? 0 : i;

    i = now->tm_wday;

    //horizontal text
    while (j > GRAPH_AREA_X)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, %s);\n",
                  j,
                  GRAPH_AREA_Y + 1,
                  j,
                  GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1,
                  (i % 6 == 0) ? "$dgrey" : "$lgrey");
      switch (i)
      {
        case 0: strlcpy(st, "sun", sizeof(st)); break;
        case 1: strlcpy(st, "mon", sizeof(st)); break;
        case 2: strlcpy(st, "tue", sizeof(st)); break;
        case 3: strlcpy(st, "wed", sizeof(st)); break;
        case 4: strlcpy(st, "thu", sizeof(st)); break;
        case 5: strlcpy(st, "fri", sizeof(st)); break;
        case 6: strlcpy(st, "sat", sizeof(st)); break;
        default: st[0] = '\0';
      }

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + 2, $black, \"%s\", \"%s\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  st,

                  GRAPH_FONT_SIZE,
                  j,
                  GRAPH_AREA_Y + GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  st);
      i = (i - 1 < 0) ? 6 : i - 1;

      j -= GRAPH_AREA_WIDTH/7;
    }

    fprintf(fp, "\n/* horizonal grid lines and vertical text */\n\n");
    for (i = 0, j = GRAPH_AREA_Y; i < GRAPH_YDIVLINES; i++, j -= GRAPH_YDIVPOS)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X - 1, j, GRAPH_AREA_X + GRAPH_AREA_WIDTH + 1, j);

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%d\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%d\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))),

                  GRAPH_FONT_SIZE,
                  (int)(GRAPH_AREA_X - (GRAPH_FONT_SIZE * 1.5)),
                  j,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))));
    }

    //graph lines
    fprintf(fp, "\n/* graph lines */\n\n");

    for (i = 0; i < GRAPH_SIZE; i++)
    {
      if (buf[GRAPH_WEEK_OFFSET + i] > s->max_players)
        buf[GRAPH_WEEK_OFFSET + i] = s->max_players;
      if (buf[GRAPH_WEEK_OFFSET + i] > 0)
      {
        fprintf(fp, "ImageLine($image, %d, %d, %d, %0.2f, $red);\n",
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - 1,
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - (((float)GRAPH_AREA_HEIGHT / s->max_players) * buf[GRAPH_WEEK_OFFSET + i]));
      }
    }

    //text
    fprintf(fp, "\n/* text */\n\n");
    fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                "ImageTTFText($image, %d, 90, $textheight, %d+($textwidth/2), $black, \"%s\", \"%s\");\n",
                GRAPH_FONT_SIZE,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT,

                GRAPH_FONT_SIZE,
                GRAPH_HEIGHT / 2,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT);

    //write image to file
    fprintf(fp, "\n\nImagePNG($image, \'%s/%s.png\');\nImageDestroy($image);\n",
                info->graph_dir,
                name);

    fprintf(fp, "\nfunction TTFBounds($fontsize, $fontangle, $font, $text, &$height, &$width)\n{\n" \
                "  $box = ImageTTFBBox($fontsize, $fontangle, $font, $text);\n" \
                "  $width = abs($box[2] - $box[0]);\n" \
                "  $height = abs($box[5] - $box[3]);\n}\n");

    fprintf(fp, "?>\n");
    fclose(fp);

    //run script
    _strnprintf(st, sizeof(st), "%s %s %s", info->php_filepath, info->php_parameters, name);
    if (!run(st, "graphServers"))
      return;
    if (info->unlink)
      unlink(name);
//END SCRIPT WEEK *******************************************************************************************************

//START GRAPH MONTH SCRIPT **********************************************************************************************
    _strnprintf(name, sizeof(name), "serenity_%s_%d_month", s->ip, s->game_port);
    fp = fopen(name, "wt");
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t open script %s\n", name);
      continue;
    }
    //header
    fprintf(fp, "<?php \n" \
                "\n" \
                "/* %s v%s - %s %s (%s)\n" \
                " * %s - automagically generated on %s.\n" \
                " */\n",
                HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, name, info->datetime);
    fprintf(fp, "\n" \
                "header(\"Content-type: image/png\");\n\n" \
                "$image = ImageCreate(%d, %d);\n" \
                "$white = ImageColorAllocate($image, 255, 255, 255);\n" \
                "$black = ImageColorAllocate($image, 0, 0, 0);\n" \
                "$lgrey = ImageColorAllocate($image, 192, 192, 192);\n" \
                "$dgrey = ImageColorAllocate($image, 128, 128, 128);\n" \
                "$red = ImageColorAllocate($image, 255, 0, 0);\n",
                GRAPH_WIDTH,
                GRAPH_HEIGHT);

    fprintf(fp, "\n/* vertical grid lines and horizontal text */\n\n");
    //far left vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X, GRAPH_AREA_Y + 1, GRAPH_AREA_X, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);
    //far right vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y + 1, GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);

    //this is the position to start the dividers from, like starting in the middle of a day
    //but because month graph is divided into days, we don't need it here
    //on second thoughts, better keep it the same
    //i = (int)(now->tm_hour / (24 / GRAPH_MONTH_XPOS));
    //i = 0;
    //j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    //j -= (i < 1) ? GRAPH_MONTH_XPOS : i;
    //i = (int)(now->tm_min / GRAPH_MONTH_XPOS);
    //j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    //j -= (i < 1) ? 0 : i;
    i = (int)(((now->tm_hour * 60) + now->tm_min) / GRAPH_MONTH_XPOS);
    //i = (int)(((now->tm_mday * 60 * 24) + ((now->tm_hour * 60) + now->tm_min)) / GRAPH_MONTH_XPOS);
    //i = 6;
    //i = ((now->tm_mday * 60 * 24) + (now->tm_hour * 60) + now->tm_min) / GRAPH_MONTH_XPOS;
    j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    j -= (i < 1) ? 0 : i;

    //d = (now->tm_mday - 1 < 1) ? monthdays(now->tm_mday, now->tm_year) : 1;
    d = now->tm_mday;
    i = 0;

    //horizontal text
    while (j > GRAPH_AREA_X)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, %s);\n",
                  j,
                  GRAPH_AREA_Y + 1,
                  j,
                  GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1,
                  (i % 6 == 0) ? "$dgrey" : "$lgrey");

      if (i % 2 == 0)
      {
        fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%02.0f\", $textheight, $textwidth);\n" \
                    "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%02.0f\");\n",
                    GRAPH_FONT_SIZE,
                    GRAPH_FONT,
                    d,

                    GRAPH_FONT_SIZE,
                    j,
                    GRAPH_AREA_Y + GRAPH_FONT_SIZE,
                    GRAPH_FONT,
                    d);
      }
      d = (d - 1 > 0) ? d - 1 : monthdays((now->tm_mon - 1 > 0) ? now->tm_mon - 1 : 11, now->tm_year);
      //d = GRAPH_AREA_WIDTH/30;
      j -= 12;//GRAPH_AREA_WIDTH/30;//monthdays(now->tm_mon, now->tm_year);
      i++;
    }

    fprintf(fp, "\n/* horizonal grid lines and vertical text */\n\n");
    for (i = 0, j = GRAPH_AREA_Y; i < GRAPH_YDIVLINES; i++, j -= GRAPH_YDIVPOS)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X - 1, j, GRAPH_AREA_X + GRAPH_AREA_WIDTH + 1, j);

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%d\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%d\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))),

                  GRAPH_FONT_SIZE,
                  (int)(GRAPH_AREA_X - (GRAPH_FONT_SIZE * 1.5)),
                  j,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))));
    }

    //graph lines
    fprintf(fp, "\n/* graph lines */\n\n");

    for (i = 0; i < GRAPH_SIZE; i++)
    {
      if (buf[GRAPH_MONTH_OFFSET + i] > s->max_players)
        buf[GRAPH_MONTH_OFFSET + i] = s->max_players;
      if (buf[GRAPH_MONTH_OFFSET + i] > 0)
      {
        fprintf(fp, "ImageLine($image, %d, %d, %d, %0.2f, $red);\n",
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - 1,
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - (((float)GRAPH_AREA_HEIGHT / s->max_players) * buf[GRAPH_MONTH_OFFSET + i]));
      }
    }

    //text
    fprintf(fp, "\n/* text */\n\n");
    fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                "ImageTTFText($image, %d, 90, $textheight, %d+($textwidth/2), $black, \"%s\", \"%s\");\n",
                GRAPH_FONT_SIZE,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT,

                GRAPH_FONT_SIZE,
                GRAPH_HEIGHT / 2,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT);

    //write image to file
    fprintf(fp, "\n\nImagePNG($image, \'%s/%s.png\');\nImageDestroy($image);\n",
                info->graph_dir,
                name);

    fprintf(fp, "\nfunction TTFBounds($fontsize, $fontangle, $font, $text, &$height, &$width)\n{\n" \
                "  $box = ImageTTFBBox($fontsize, $fontangle, $font, $text);\n" \
                "  $width = abs($box[2] - $box[0]);\n" \
                "  $height = abs($box[5] - $box[3]);\n}\n");

    fprintf(fp, "?>\n");
    fclose(fp);

    //run script
    _strnprintf(st, sizeof(st), "%s %s %s", info->php_filepath, info->php_parameters, name);
    if (!run(st, "graphServers"))
      return;
    if (info->unlink)
      unlink(name);
//END SCRIPT MONTH ******************************************************************************************************

//START GRAPH YEAR SCRIPT ***********************************************************************************************
    _strnprintf(name, sizeof(name), "serenity_%s_%d_year", s->ip, s->game_port);
    fp = fopen(name, "wt");
    if (!fp)
    {
      echo(DEBUG_INFO, "graphServers: couldn\'t open script %s\n", name);
      continue;
    }
    //header
    fprintf(fp, "<?php \n" \
                "\n" \
                "/* %s v%s - %s %s (%s)\n" \
                " * %s - automagically generated on %s.\n" \
                " */\n",
                HEADER_TITLE, VERSION, HEADER_COPYRIGHT, HEADER_AUTHOR, HEADER_EMAIL, name, info->datetime);
    fprintf(fp, "\n" \
                "header(\"Content-type: image/png\");\n\n" \
                "$image = ImageCreate(%d, %d);\n" \
                "$white = ImageColorAllocate($image, 255, 255, 255);\n" \
                "$black = ImageColorAllocate($image, 0, 0, 0);\n" \
                "$lgrey = ImageColorAllocate($image, 192, 192, 192);\n" \
                "$dgrey = ImageColorAllocate($image, 128, 128, 128);\n" \
                "$red = ImageColorAllocate($image, 255, 0, 0);\n",
                GRAPH_WIDTH,
                GRAPH_HEIGHT);

    fprintf(fp, "\n/* vertical grid lines and horizontal text */\n\n");
    //far left vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X, GRAPH_AREA_Y + 1, GRAPH_AREA_X, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);
    //far right vertical line
    fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y + 1, GRAPH_AREA_X + GRAPH_SIZE, GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1);

    //this is the position to start the dividers from, like starting in the middle of a day
    //but because year graph is divided into days, we don't need it here
    //on second thoughts, better keep it the same
    i = (int)(((now->tm_mday * 60 * 24) + ((now->tm_hour * 60) + now->tm_min)) / GRAPH_YEAR_XPOS);
    //i = 0;
    //i = (int)(((now->tm_hour * 60) + now->tm_min) / GRAPH_YEAR_XPOS);
    j = GRAPH_AREA_X + GRAPH_AREA_WIDTH;
    j -= (i < 1) ? 0 : i;

    d = now->tm_mon;
    i = 0;

    //horizontal text
    while (j > GRAPH_AREA_X)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, %s);\n",
                  j,
                  GRAPH_AREA_Y + 1,
                  j,
                  GRAPH_AREA_Y - GRAPH_AREA_HEIGHT - 1,
                  (i % 6 == 0) ? "$dgrey" : "$lgrey");

      //if (i % 2 == 0)
      //{
      switch ((int)d)
      {
        case 0: strlcpy(st, "jan", sizeof(st)); break;
        case 1: strlcpy(st, "feb", sizeof(st)); break;
        case 2: strlcpy(st, "mar", sizeof(st)); break;
        case 3: strlcpy(st, "apr", sizeof(st)); break;
        case 4: strlcpy(st, "may", sizeof(st)); break;
        case 5: strlcpy(st, "jun", sizeof(st)); break;
        case 6: strlcpy(st, "jul", sizeof(st)); break;
        case 7: strlcpy(st, "aug", sizeof(st)); break;
        case 8: strlcpy(st, "sep", sizeof(st)); break;
        case 9: strlcpy(st, "oct", sizeof(st)); break;
        case 10: strlcpy(st, "nov", sizeof(st)); break;
        case 11: strlcpy(st, "dec", sizeof(st)); break;
        default: st[0] = '\0';
      }

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + 2, $black, \"%s\", \"%s\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  st,

                  GRAPH_FONT_SIZE,
                  j,
                  GRAPH_AREA_Y + GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  st);
      //}
      d = (d - 1 < 0) ? 11 : d - 1;

      j -= GRAPH_AREA_WIDTH/12;
      i++;
    }

    fprintf(fp, "\n/* horizonal grid lines and vertical text */\n\n");
    for (i = 0, j = GRAPH_AREA_Y; i < GRAPH_YDIVLINES; i++, j -= GRAPH_YDIVPOS)
    {
      fprintf(fp, "ImageLine($image, %d, %d, %d, %d, $black);\n", GRAPH_AREA_X - 1, j, GRAPH_AREA_X + GRAPH_AREA_WIDTH + 1, j);

      fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%d\", $textheight, $textwidth);\n" \
                  "ImageTTFText($image, %d, 0, %d - ($textwidth/2), %d + ($textheight/2), $black, \"%s\", \"%d\");\n",
                  GRAPH_FONT_SIZE,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))),

                  GRAPH_FONT_SIZE,
                  (int)(GRAPH_AREA_X - (GRAPH_FONT_SIZE * 1.5)),
                  j,
                  GRAPH_FONT,
                  (int)(i * ((float)s->max_players / (GRAPH_YDIVLINES - 1))));
    }

    //graph lines
    fprintf(fp, "\n/* graph lines */\n\n");

    for (i = 0; i < GRAPH_SIZE; i++)
    {
      if (buf[GRAPH_YEAR_OFFSET + i] > s->max_players)
        buf[GRAPH_YEAR_OFFSET + i] = s->max_players;
      if (buf[GRAPH_YEAR_OFFSET + i] > 0)
      {
        fprintf(fp, "ImageLine($image, %d, %d, %d, %0.2f, $red);\n",
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - 1,
                    GRAPH_AREA_X + 1 + i,
                    GRAPH_AREA_Y - (((float)GRAPH_AREA_HEIGHT / s->max_players) * buf[GRAPH_YEAR_OFFSET + i]));
      }
    }

    //text
    fprintf(fp, "\n/* text */\n\n");
    fprintf(fp, "TTFBounds(%d, 0, \"%s\", \"%s\", $textheight, $textwidth);\n" \
                "ImageTTFText($image, %d, 90, $textheight, %d + ($textwidth/2), $black, \"%s\", \"%s\");\n",
                GRAPH_FONT_SIZE,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT,

                GRAPH_FONT_SIZE,
                GRAPH_HEIGHT / 2,
                GRAPH_FONT,
                GRAPH_LEFT_TEXT);

    //write image to file
    fprintf(fp, "\n\nImagePNG($image, \'%s/%s.png\');\nImageDestroy($image);\n",
                info->graph_dir,
                name);

    fprintf(fp, "\nfunction TTFBounds($fontsize, $fontangle, $font, $text, &$height, &$width)\n{\n" \
                "  $box = ImageTTFBBox($fontsize, $fontangle, $font, $text);\n" \
                "  $width = abs($box[2] - $box[0]);\n" \
                "  $height = abs($box[5] - $box[3]);\n}\n");

    fprintf(fp, "?>\n");
    fclose(fp);

    //run script
    _strnprintf(st, sizeof(st), "%s %s %s", info->php_filepath, info->php_parameters, name);
    if (!run(st, "graphServers"))
      return;
    if (info->unlink)
      unlink(name);
//END SCRIPT YEAR *******************************************************************************************************
  }
}

/*
 * shiftleft - moves all the graph lines to the left a certain number
 */
void shiftleft(unsigned char *s, int offset, int div, unsigned char value)
{
  //record highest values only
  //if (value > s[offset + GRAPH_SIZE - 1])
  //  s[offset + GRAPH_SIZE - 1] = value;

  //record average
  s[offset + GRAPH_SIZE - 1] = ((value + s[offset + GRAPH_SIZE - 1]) / 2);

  if (div < 1)
    return;

  //dude, check this out - so much better than a double loop, why didn't
  //i think of that the first time?
  if (div < GRAPH_SIZE)
  {
    memcpy(&s[offset], &s[offset + div], GRAPH_SIZE - div);
    memset(&s[offset + GRAPH_SIZE - div], 0, div - 1);
  }
  else
  {
    memset(&s[offset], 0, GRAPH_SIZE - 1);
  }
}

// These next two functions store/restore longs the same way as they are in computer memory - to save space.
// The technique used here is suspiciously familiar to that of a certain game engine.
/*
 * writelong - stores a long in 4 bytes
 */
void writelong(unsigned char *s, int val)
{
	s[0] = val & 0xff;
	s[1] = (val >> 8) & 0xff;
	s[2] = (val >> 16) & 0xff;
	s[3] = val >> 24;
}

/*
 * readlong - returns long from 4 bytes
 */
long readlong(unsigned char *s)
{
	return (s[0] + (s[1] << 8) + (s[2] << 16) + (s[3] << 24));
}

