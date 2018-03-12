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

  if (!$_GET['ip'] || !$_GET['port'])
  {
    alert(3, 'Serenity - No server', 'Missing server details'); //todo: perhaps we can display all servers for info we have
    return;    
  }

  $ip = addslashes($_GET['ip']);
  $port = intval($_GET['port']);

  $sstr = 'SELECT server_id, server_type, server_version, server_hostname, server_ip, server_gameport, server_queryport, ' .
          'server_mapname, server_gametype, server_gameid, server_numplayers, server_maxplayers, server_bytesrecieved, ' .
          'server_bytessent, server_trashed, server_updatesuccess, server_updatefail, ' .
          'server_consecutivefails, server_lastupdate ' .
          "FROM serenity_server WHERE server_ip = '" . $ip . "' AND server_gameport = " . $port;

  $sqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());
  $rows = mysql_num_rows($sqry);

  if (!$rows)
  {
    alert(3, 'Serenity - Not found', 'Server not found: ' . $ip . ':' . $port);
    return;
  }

  $qres = mysql_fetch_array($sqry, MYSQL_ASSOC);
  $sid = $qres['server_id'];

  $pagetitle = 'Serenity - ' . $qres['server_hostname'] . ' (' . $ip . ':' . $port . ')'; 
  include('head.php');

  print '<H1 CLASS="hdra">' . $qres['server_hostname'] . ' - ' . $qres['server_ip'] . ':' . $qres['server_gameport'] . ' (' . $qres['server_queryport'] . ')' . '</H1>';
  print '<TABLE CLASS="tblm" WIDTH="50%">';
  print '<TR><TD CLASS="tbla">Map</TD><TD CLASS="tblb">' . ucwords(strtolower($qres['server_mapname'])) . '</TD></TR>';
  print '<TR><TD CLASS="tbla">Players</TD><TD CLASS="tblb">' . $qres['server_numplayers'] . ' / ' . $qres['server_maxplayers'] . '</TD></TR>';
  print '<TR><TD CLASS="tbla">Mod (Version)</TD><TD CLASS="tblb">' . $qres['server_gameid'] . ' (' . $qres['server_version'] . ')' . '</TD></TR>';
  if (!$qres['server_consecutivefails'])
  {
    print '<TR><TD CLASS="tbla">Last Update</TD><TD CLASS="tblb">' . textsdate($qres['server_lastupdate']) . ' - ' . elapsed(strtotime($qres['server_lastupdate'])) . ' ago' . '</TD></TR>';
  }
  else
  {
    print '<TR><TD CLASS="tbla">Last Update (Fails)</TD><TD CLASS="tblb">' . textsdate($qres['server_lastupdate']) . ' - ' . elapsed(strtotime($qres['server_lastupdate'])) . ' ago (' . $qres['server_consecutivefails'] . ')' . '</TD></TR>';
  }
  print '</TABLE>';

  if ($qres['server_type'] == 1) //FIX FIX FIX ME..
  {
    $sstr = 'SELECT player_id, player_name, player_score, player_frags, player_deaths, player_team, player_ping, player_connect, player_userid, player_keyhash, keyhash_hash ' .
            'FROM serenity_player, serenity_keyhash WHERE serenity_player.player_server = ' . $sid . ' AND ' . "serenity_player.player_lastseen = '" . $qres['server_lastupdate'] . "' AND serenity_player.player_keyhash = serenity_keyhash.keyhash_id " .
            'ORDER BY player_score DESC';
  }
  else
  {
    $sstr = 'SELECT player_id, player_name, player_score, player_frags, player_deaths, player_team, player_ping, player_connect, player_userid, player_keyhash ' .
            'FROM serenity_player WHERE serenity_player.player_server = ' . $sid . ' AND ' . "serenity_player.player_lastseen = '" . $qres['server_lastupdate'] . "' " .
            'ORDER BY player_score DESC';
  }
  $sqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

  if (mysql_num_rows($sqry))
  {
    print '<H2 CLASS="hdrc">Players</H2>';

    print '<TABLE CLASS="tblm" WIDTH="80%">';
    print '<TR>';
    print '<TD CLASS="tblc">Player Name</TD>';

    if ((($webserver & 1) && ($prefserver & 1)) && ($prefoption & 16))
    {
      print '<TD CLASS="tblc">Score</TD>';
    }
    else
    {
      if (($webserver & 1) && ($prefserver & 1)) { print '<TD CLASS="tblc">Score</TD>'; }
      if (($webserver & 2) && ($prefserver & 2)) { print '<TD CLASS="tblc">Frags</TD>'; }
      if (($webserver & 4) && ($prefserver & 4)) { print '<TD CLASS="tblc">Deaths</TD>'; }
    }
    if (($webserver & 8) && ($prefserver & 8)) { print '<TD CLASS="tblc">Ping</TD>'; }
    if (($webserver & 16) && ($prefserver & 16)) { print '<TD CLASS="tblc">Team</TD>'; }
    if (($webserver & 32) && ($prefserver & 32)) { print '<TD CLASS="tblc">User ID</TD>'; }
    if (($webserver & 64) && ($prefserver & 64)) { print '<TD CLASS="tblc">Connect</TD>'; }
    if (($webserver & 128) && ($prefserver & 128)) { print '<TD CLASS="tblc">Keyhash</TD>'; }
    print '</TR>';

    while ($qres = mysql_fetch_array($sqry, MYSQL_ASSOC))
    {
      if (!$qres['player_keyhash'])
      {
        $qres['keyhash_hash'] = '(none)';
      }
      print '<TR>';
      print '<TD CLASS="tbla">' . $qres['player_name'] . '</TD>';

      if ((($webserver & 1) && ($prefserver & 1)) && ($prefoption & 16))
      {
        print '<TD CLASS="tbla">' . $qres['player_score'] . ' (' . $qres['player_frags'] . '/' . $qres['player_deaths'] . ')</TD>';
      }
      else
      {
        if (($webserver & 1) && ($prefserver & 1)) { print '<TD CLASS="tbla">' . $qres['player_score'] . '</TD>'; }
        if (($webserver & 2) && ($prefserver & 2)) { print '<TD CLASS="tbla">' . $qres['player_frags'] . '</TD>'; }
        if (($webserver & 4) && ($prefserver & 4)) { print '<TD CLASS="tbla">' . $qres['player_deaths'] . '</TD>'; }
      }
      if (($webserver & 8) && ($prefserver & 8)) { print '<TD CLASS="tbla">' . $qres['player_ping'] . '</TD>'; }
      if (($webserver & 16) && ($prefserver & 16)) { print '<TD CLASS="tbla">' . $qres['player_team'] . '</TD>'; }
      if (($webserver & 32) && ($prefserver & 32)) { print '<TD CLASS="tbla">' . $qres['player_userid'] . '</TD>'; }
      if (($webserver & 64) && ($prefserver & 64)) { print '<TD CLASS="tbla">' . $qres['player_connect'] . '</TD>'; }
      if (($webserver & 128) && ($prefserver & 128))
      {
        $c = ($prefoption & 32) ? 'tblp' : 'tbla';
        print '<TD CLASS="' . $c . '">' . $qres['keyhash_hash'] . '</TD>';
      }
      print '</TR>';
    }
    print '</TABLE>';
  }
  else
  {
    print '<H2 CLASS="hdrg">No Players</H2>';
  }

  if (($weboption & 8) && ($prefoption & 8))
  {
    $sstr = 'SELECT rule_key, rule_value FROM serenity_rule WHERE rule_server = ' . $sid . ' ORDER BY rule_key';
    $sqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

    if (mysql_num_rows($sqry))
    {
      print '<H2 CLASS="hdrc">Rules</H2>';

      print '<TABLE CLASS="tblm" WIDTH="50%">';
      print '<TR>';
      print '<TD CLASS="tblc">Key</TD>';
      print '<TD CLASS="tblc">Value</TD>';
      print '</TR>';
      while ($qres = mysql_fetch_array($sqry, MYSQL_ASSOC))
      {
        print '<TR>';
        print '<TD CLASS="tbla">' . $qres['rule_key'] . '</TD>';
        if ($qres['rule_key'])
        {
          print '<TD CLASS="tbla">' . $qres['rule_value'] . '</TD>';
        }
        else
        {
          print '<TD CLASS="tbla">0</TD>';
        }
        print '</TR>';
      }
      print '</TABLE>';
    }
  }

  if (($weboption & 4) && ($prefoption & 4))
  {
    $graph_day = $graph_dir . '/serenity_' . $ip . '_' . $port . '_day.png';
    $graph_week = $graph_dir . '/serenity_' . $ip . '_' . $port . '_week.png';
    $graph_month = $graph_dir . '/serenity_' . $ip . '_' . $port . '_month.png';
    $graph_year = $graph_dir . '/serenity_' . $ip . '_' . $port . '_year.png';
    if (file_exists($graph_day) && file_exists($graph_week) && file_exists($graph_month) && file_exists($graph_year))
    {
      print '<H2 CLASS="hdrc">Usage</H2>';
      print '<TABLE CLASS="maintable">';
      print '<TR><TD CLASS="tbla" ALIGN="CENTER"><IMG SRC="' . $graph_day . '"><BR>Day</TD></TR>';
      print '<TR><TD CLASS="tbla" ALIGN="CENTER"><IMG SRC="' . $graph_week . '"><BR>Week</TD></TR>';
      print '<TR><TD CLASS="tbla" ALIGN="CENTER"><IMG SRC="' . $graph_month . '"><BR>Month</TD></TR>';
      print '<TR><TD CLASS="tbla" ALIGN="CENTER"><IMG SRC="' . $graph_year . '"><BR>Year</TD></TR>';
      print '</TABLE>';
    }
  }

  include('tail.php');
?>
