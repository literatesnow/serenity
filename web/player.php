<?php
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

  include('init.php');
  include('common.php');

  $pagetitle = 'Serenity - Player Search';

  if ($_GET['player'])
  {
    $pagetitle .= ': ' . $_GET['player'];
  }
  include('head.php');

  if (!($weboption & 1))
  {
    alert(4, NULL, 'Search is disabled');
    return;   
  }

  searchbox();

  if (!$_GET['player'])
  {
    include('tail.php');
    return;
  }
 
  $sstr = "SELECT player_id, player_type, player_name, player_keyhash FROM serenity_player WHERE player_name LIKE '%" . $_GET['player'] . "%' " .
          'ORDER BY player_score DESC';
  $spqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());
  $rows = mysql_num_rows($spqry);
  if (!$rows)
  {
    alert(2, NULL, 'No Match');
    return;
  }

  if ($rows > 250)
  {
    alert(2, NULL, 'Over 250 matches');
    return;
  }

  $aliases = '';
  $nonalias = '';
  $aliasfound = 0;
  $nonaliasfound = 0;

  while ($qpres = mysql_fetch_array($spqry, MYSQL_ASSOC)) //for every match...
  {  
    if ($qpres['player_type'] == 1) //FIX FIX FIX
    {
      $sstr = 'SELECT player_id, player_name, player_score, player_frags, player_deaths, player_team, player_ping, player_userid, player_connect, player_lastseen, player_keyhash, keyhash_hash, server_hostname ' .
              'FROM serenity_player, serenity_server, serenity_keyhash WHERE serenity_player.player_keyhash = ' . $qpres['player_keyhash'] . ' AND serenity_player.player_type = ' . $qpres['player_type'] . ' AND serenity_player.player_server = serenity_server.server_id AND serenity_player.player_keyhash = serenity_keyhash.keyhash_id ' .
              'ORDER BY player_score DESC';
    }
    else
    {
      $sstr = 'SELECT player_id, player_name, player_score, player_frags, player_deaths, player_team, player_ping, player_userid, player_connect, player_lastseen, player_keyhash, server_hostname ' .
              'FROM serenity_player, serenity_server WHERE serenity_player.player_id = ' . $qpres['player_id'] . ' AND serenity_player.player_server = serenity_server.server_id ' .
              'ORDER BY player_score DESC';
    }
    $sqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

    if (!mysql_num_rows($sqry))
    {
      Continue;
    }

    if (mysql_num_rows($sqry) > 1)
    {
      $aliasfound = 1;

      $i = 0;
      while ($qres = mysql_fetch_array($sqry, MYSQL_ASSOC))
      {
        if ($qres['player_id'] == $qpres['player_id'])
        {
          continue;
        }

        if ($qpres['player_type'] != 1) //FIX FIX
        {
          $qres['keyhash_hash'] = '(none)';
        }

        if ($i)
        {
          $aliases .= '<TR><TD></TD>';
        }
        else
        {
          $aliases .= '<TR><TD CLASS="tbla">' . $qpres['player_name'] . '</TD>';
          $i = 1;
        }
/*      
        $aliases .= '<TD CLASS="tblb">' . $qres['player_name'] . '</TD>';
                    '<TD CLASS="tbla">' . $qres['player_score'] . ' (' . $qres['player_frags'] . '/' . $qres['player_deaths'] . ')</TD>' .
                    '<TD CLASS="tblb">' . $qres['player_score'] . '</TD>' .
                    '<TD CLASS="tbla">' . $qres['player_frags'] . '</TD>' .
                    '<TD CLASS="tblb">' . $qres['player_deaths'] . '</TD>' .
                    '<TD CLASS="tbla">' . $qres['player_team'] . '</TD>' .
                    '<TD CLASS="tblb">' . $qres['player_ping'] . '</TD>' .
                    '<TD CLASS="tbla">' . $qres['server_hostname'] . '</TD>' .
                    '<TD CLASS="tblb">' . $qres['player_lastseen'] . ' (' . elapsed(strtotime($qres['player_lastseen'])) . ')</TD>' .
                    '<TD CLASS="tbla">' . $qres['keyhash_hash'] . '</TD>' .
                    '</TR>';
*/
        $aliases .= '<TD CLASS="tblb">' . $qres['player_name'] . '</TD>';

        if ((($webplayer & 1) && ($prefplayer & 1)) && ($prefoption & 16))
        {
          if (($webplayer & 1) && ($prefplayer & 1)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_score'] . ' (' . $qres['player_frags'] . '/' . $qres['player_deaths'] . ')</TD>'; }
        }
        else
        {
          if (($webplayer & 1) && ($prefplayer & 1)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_score'] . '</TD>'; }
          if (($webplayer & 2) && ($prefplayer & 2)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_frags'] . '</TD>'; }
          if (($webplayer & 4) && ($prefplayer & 4)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_deaths'] . '</TD>'; }
        }
        if (($webplayer & 8) && ($prefplayer & 8)) { $aliases .= '<TD CLASS="tbla">' . $qres['server_hostname'] . '</TD>'; }
        if (($webplayer & 16) && ($prefplayer & 16)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_ping'] . '</TD>'; }
        if (($webplayer & 32) && ($prefplayer & 32)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_team'] . '</TD>'; }
        if (($webplayer & 64) && ($prefplayer & 64)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_userid'] . '</TD>'; }
        if (($webplayer & 128) && ($prefplayer & 128)) { $aliases .= '<TD CLASS="tbla">' . $qres['player_connect'] . '</TD>'; }
        if (($webplayer & 256) && ($prefplayer & 256)) { $aliases .= '<TD CLASS="tbla">' . textsdate($qres['player_lastseen']) . ' (' . elapsed(strtotime($qres['player_lastseen'])) . ')</TD>'; }
        if (($webplayer & 512) && ($prefplayer & 512))
        {
          $c = ($prefoption & 32) ? 'tblp' : 'tbla';
          $aliases .= '<TD CLASS="' . $c . '">' . $qres['keyhash_hash'] . '</TD>';
        }
        $aliases .= '</TR>';
      }
    }
    else
    {
      $nonaliasfound = 1;

      $qres = mysql_fetch_array($sqry, MYSQL_ASSOC);

      if ($qpres['player_type'] != 1) //FIX FIX
      {
        $qres['keyhash_hash'] = '';
      }
/*
      $nonalias .= '<TR>' .
                   '<TD CLASS="tbla">' . $qres['player_name'] . '</TD>' .
                   '<TD CLASS="tblb">' . $qres['player_score'] . ' (' . $qres['player_frags'] . '/' . $qres['player_deaths'] . ')</TD>' .
                   '<TD CLASS="tbla">' . $qres['player_score'] . '</TD>' .
                   '<TD CLASS="tblb">' . $qres['player_frags'] . '</TD>' .
                   '<TD CLASS="tbla">' . $qres['player_deaths'] . '</TD>' .
                   '<TD CLASS="tblb">' . $qres['player_team'] . '</TD>' .
                   '<TD CLASS="tbla">' . $qres['player_ping'] . '</TD>' .
                   '<TD CLASS="tblb">' . $qres['server_hostname'] . '</TD>' .
                   '<TD CLASS="tbla">' . $qres['player_lastseen'] . ' (' . elapsed(strtotime($qres['player_lastseen'])) . ')</TD>' .
                   '<TD CLASS="tblb">' . $qres['keyhash_hash'] . '</TD>' .
                   '</TR>';
*/
      $nonalias .= '<TD CLASS="tblb">' . $qres['player_name'] . '</TD>';

      if ((($webplayer & 1) && ($prefplayer & 1)) && ($prefoption & 16))
      {
        if (($webplayer & 1) && ($prefplayer & 1)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_score'] . ' (' . $qres['player_frags'] . '/' . $qres['player_deaths'] . ')</TD>'; }
      }
      else
      {
        if (($webplayer & 1) && ($prefplayer & 1)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_score'] . '</TD>'; }
        if (($webplayer & 2) && ($prefplayer & 2)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_frags'] . '</TD>'; }
        if (($webplayer & 4) && ($prefplayer & 4)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_deaths'] . '</TD>'; }
      }
      if (($webplayer & 8) && ($prefplayer & 8)) { $nonalias .= '<TD CLASS="tbla">' . $qres['server_hostname'] . '</TD>'; }
      if (($webplayer & 16) && ($prefplayer & 16)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_ping'] . '</TD>'; }
      if (($webplayer & 32) && ($prefplayer & 32)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_team'] . '</TD>'; }
      if (($webplayer & 64) && ($prefplayer & 64)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_userid'] . '</TD>'; }
      if (($webplayer & 128) && ($prefplayer & 128)) { $nonalias .= '<TD CLASS="tbla">' . $qres['player_connect'] . '</TD>'; }
      if (($webplayer & 256) && ($prefplayer & 256)) { $nonalias .= '<TD CLASS="tbla">' . textsdate($qres['player_lastseen']) . ' (' . elapsed(strtotime($qres['player_lastseen'])) . ')</TD>'; }
      if (($webplayer & 0512) && ($prefplayer & 512))
      { 
        $c = ($prefoption & 32) ? 'tblp' : 'tbla';
        $nonalias .= '<TD CLASS="' . $c . '">' . $qres['keyhash_hash'] . '</TD>';
      }
      $nonalias .= '</TR>';
    }
  }
  if (!$aliasfound)
  {
    print '<H2 CLASS="hdrg">No matching aliased players</H2>';
  }
  else
  {
    print '<H2 CLASS="hdrc">Matching with alias</H2>';

    print '<TABLE CLASS="tblm" WIDTH="100%">';
/*
    print '<TR>';
    print '<TD CLASS="tblc">Player Name</TD>';
    print '<TD CLASS="tblc">Alias</TD>';
    print '<TD CLASS="tblc">Score</TD>';
    print '<TD CLASS="tblc">Frags</TD>';
    print '<TD CLASS="tblc">Deaths</TD>';
    print '<TD CLASS="tblc">Team</TD>';
    print '<TD CLASS="tblc">Ping</TD>';
    print '<TD CLASS="tblc">Server</TD>';
    print '<TD CLASS="tblc">Last Seen</TD>';
    print '<TD CLASS="tblc">Keyhash</TD>';
    print '</TR>';
*/
    print '<TR>';
    print '<TD CLASS="tblc">Player Name</TD>';
    print '<TD CLASS="tblc">Alias</TD>';
    if ((($webplayer & 1) && ($prefplayer & 1)) && ($prefoption & 16))
    {
      print '<TD CLASS="tblc">Score</TD>';
    }
    else
    {
      if (($webplayer & 1) && ($prefplayer & 1)) { print '<TD CLASS="tblc">Score</TD>'; }
      if (($webplayer & 2) && ($prefplayer & 2)) { print '<TD CLASS="tblc">Frags</TD>'; }
      if (($webplayer & 4) && ($prefplayer & 4)) { print '<TD CLASS="tblc">Deaths</TD>'; }
    }
    if (($webplayer & 8) && ($prefplayer & 8)) { print '<TD CLASS="tblc">Server</TD>'; }
    if (($webplayer & 16) && ($prefplayer & 16)) { print '<TD CLASS="tblc">Ping</TD>'; }
    if (($webplayer & 32) && ($prefplayer & 32)) { print '<TD CLASS="tblc">Team</TD>'; }
    if (($webplayer & 64) && ($prefplayer & 64)) { print '<TD CLASS="tblc">ID</TD>'; }
    if (($webplayer & 128) && ($prefplayer & 128)) { print '<TD CLASS="tblc">Connect</TD>'; }
    if (($webplayer & 256) && ($prefplayer & 256)) { print '<TD CLASS="tblc">Last Seen</TD>'; }
    if (($webplayer & 512) && ($prefplayer & 512)) { print '<TD CLASS="tblc">Keyhash</TD>'; }
    print '</TR>';
    print $aliases;
    print '</TABLE>';
  }
  if (!$nonaliasfound)
  {
    print '<H2 CLASS="hdrg">No matching nonaliased players</H2>';
  }
  else
  {
    print '<H2 CLASS="hdrc">Matching without alias</H2>';

    print '<TABLE CLASS="tblm" WIDTH="100%">';
/*
    print '<TR>';
    print '<TD CLASS="tblc">Player Name</TD>';
    print '<TD CLASS="tblc">Score</TD>';
    print '<TD CLASS="tblc">Frags</TD>';
    print '<TD CLASS="tblc">Deaths</TD>';
    print '<TD CLASS="tblc">Team</TD>';
    print '<TD CLASS="tblc">Ping</TD>';
    print '<TD CLASS="tblc">Server</TD>';
    print '<TD CLASS="tblc">Last Seen</TD>';
    print '<TD CLASS="tblc">Keyhash</TD>';
    print '</TR>';
*/
    print '<TD CLASS="tblc">Player Name</TD>';
    if ((($webplayer & 1) && ($prefplayer & 1)) && ($prefoption & 16))
    {
      print '<TD CLASS="tblc">Score</TD>';
    }
    else
    {
      if (($webplayer & 1) && ($prefplayer & 1)) { print '<TD CLASS="tblc">Score</TD>'; }
      if (($webplayer & 2) && ($prefplayer & 2)) { print '<TD CLASS="tblc">Frags</TD>'; }
      if (($webplayer & 4) && ($prefplayer & 4)) { print '<TD CLASS="tblc">Deaths</TD>'; }
    }
    if (($webplayer & 8) && ($prefplayer & 8)) { print '<TD CLASS="tblc">Server</TD>'; }
    if (($webplayer & 16) && ($prefplayer & 16)) { print '<TD CLASS="tblc">Ping</TD>'; }
    if (($webplayer & 32) && ($prefplayer & 32)) { print '<TD CLASS="tblc">Team</TD>'; }
    if (($webplayer & 64) && ($prefplayer & 64)) { print '<TD CLASS="tblc">ID</TD>'; }
    if (($webplayer & 128) && ($prefplayer & 128)) { print '<TD CLASS="tblc">Connect</TD>'; }
    if (($webplayer & 256) && ($prefplayer & 256)) { print '<TD CLASS="tblc">Last Seen</TD>'; }
    if (($webplayer & 512) && ($prefplayer & 512)) { print '<TD CLASS="tblc">Keyhash</TD>'; }
    print '</TR>';
    print $nonalias;
    print '</TABLE>';
  }

  include('tail.php');
?>