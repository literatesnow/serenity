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
  $pagetitle = 'Serenity';
  include('head.php');

  if ($weboption & 1)
  {
    searchbox();
  }

  $sstr = 'SELECT type_id, type_desc FROM serenity_servertype';
  $stqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

  while ($qres = mysql_fetch_array($stqry, MYSQL_ASSOC))
  {
    print '<H1 CLASS="hdra">'. $qres['type_desc'] . '</H1>';

    $sstr = 'SELECT server_id, server_version, server_hostname, server_ip, server_gameport, server_queryport, ' .
            'server_mapname, server_gametype, server_numplayers, server_maxplayers, server_bytesrecieved, ' .
            'server_bytessent, server_trashed, server_updatesuccess, server_updatefail, ' .
            'server_consecutivefails, server_lastupdate ' .
            'FROM serenity_server WHERE server_type = ' . $qres['type_id'] . ' ORDER BY server_consecutivefails ASC, server_numplayers DESC, server_lastupdate DESC';
    $sqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

    if (!mysql_num_rows($sqry))
    {
      continue;
    }

    print '<TABLE CLASS="tblm" WIDTH="100%">';

    print '<TR>';
    print '<TD CLASS="tblc">Hostname</TD>';
    if (($webindex & 1) && ($prefindex & 1)) { print '<TD CLASS="tblc">IP:Port (Query Port)</TD>'; }
    if (($webindex & 2) && ($prefindex & 2)) { print '<TD CLASS="tblc">Players</TD>'; }
    if (($webindex & 4) && ($prefindex & 4)) { print '<TD CLASS="tblc">Map</TD>'; }
    if (($webindex & 8) && ($prefindex & 8)) { print '<TD CLASS="tblc">Game Type</TD>'; }
    if (($webindex & 16) && ($prefindex & 16)) { print '<TD CLASS="tblc">Ver</TD>'; }
    if (($webindex & 32) && ($prefindex & 32)) { print '<TD CLASS="tblc">OK</TD>'; }
    if (($webindex & 64) && ($prefindex & 64)) { print '<TD CLASS="tblc">Fails</TD>'; }
    if (($webindex & 128) && ($prefindex & 128)) { print '<TD CLASS="tblc">Last Update</TD>'; }
    if (($webindex & 256) && ($prefindex & 256)) { print '<TD CLASS="tblc">RX</TD>'; }
    if (($webindex & 512) && ($prefindex & 512)) { print '<TD CLASS="tblc">TX</TD>'; }
    if (($webindex & 1024) && ($prefindex & 1024)) { print '<TD CLASS="tblc">TR</TD>'; }
    print '</TR>';

    while ($qres = mysql_fetch_array($sqry, MYSQL_ASSOC))
    {
      if ($qres['server_consecutivefails'] > 0)
      {
        $qres['server_numplayers'] = 0;
      }
      print '<TR>';
      print '<TD CLASS="tbla">' . '<A HREF="server.php?ip=' . $qres['server_ip'] . '&port=' . $qres['server_gameport'] . '">' . $qres['server_hostname'] . '</A>' . '</TD>';     
      if (($webindex & 1) && ($prefindex & 1)) { print '<TD CLASS="tbla">' . $qres['server_ip'] . ':' . $qres['server_gameport'] . ' (' . $qres['server_queryport'] . ')' . '</TD>'; }
      if (($webindex & 2) && ($prefindex & 2)) { print '<TD CLASS="tbla">' . $qres['server_numplayers'] . ' / ' . $qres['server_maxplayers'] . '</TD>'; }
      if (($webindex & 4) && ($prefindex & 4)) { print '<TD CLASS="tbla">' . ucwords(strtolower($qres['server_mapname'])) . '</TD>'; }
      if (($webindex & 8) && ($prefindex & 8)) { print '<TD CLASS="tbla">' . $qres['server_gametype'] . '</TD>'; }
      if (($webindex & 16) && ($prefindex & 16)) { print '<TD CLASS="tbla">' . $qres['server_version'] . '</TD>'; }
      if (($webindex & 32) && ($prefindex & 32)) { print '<TD CLASS="tbla">' . round(($qres['server_updatesuccess'] / ($qres['server_updatesuccess'] + $qres['server_updatefail']) * 100), 1) . '%' . '</TD>'; }
      if (($webindex & 64) && ($prefindex & 64)) { print '<TD CLASS="tbla">' . $qres['server_consecutivefails'] . '</TD>'; }
      if (($webindex & 128) && ($prefindex & 128)) { print '<TD CLASS="tbla">' . elapsed(strtotime($qres['server_lastupdate'])) . '</TD>'; }
      if (($webindex & 256) && ($prefindex & 256)) { print '<TD CLASS="tbla">' . units($qres['server_bytesrecieved']) . '</TD>'; }
      if (($webindex & 512) && ($prefindex & 512)) { print '<TD CLASS="tbla">' . units($qres['server_bytessent']) . '</TD>'; }
      if (($webindex & 1024) && ($prefindex & 1024)) { print '<TD CLASS="tbla">' . $qres['server_trashed'] . '</TD>'; }
      print '</TR>';
    }
    print '</TABLE>';
  }

  if (($weboption & 2) && ($prefoption & 2))
  {
    $sstr = 'SELECT * FROM serenity_info';
    $sqry = mysql_query($sstr, $db) or die ('Bad query ' . mysql_error());
    $qres = mysql_fetch_array($sqry, MYSQL_ASSOC);

    print '<H1 CLASS="hdra">Serenity</H1>';

    $qres['number_of_players'] = mysql_result(mysql_query('SELECT COUNT(*) FROM serenity_player', $db), 0); 
    $qres['number_of_keyhashes'] = mysql_result(mysql_query('SELECT COUNT(*) FROM serenity_keyhash', $db), 0);
    $qres['number_of_servers'] = mysql_result(mysql_query('SELECT COUNT(*) FROM serenity_server', $db), 0);

    print '<TABLE CLASS="tblm" WIDTH="50%">';
    print '<TR><TD CLASS="tbla">Version</TD><TD CLASS="tblb">' . $qres['info_version'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Servers</TD><TD CLASS="tblb">' . $qres['number_of_servers'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Players</TD><TD CLASS="tblb">' . $qres['number_of_players'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Keyhashes</TD><TD CLASS="tblb">' . $qres['number_of_keyhashes'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">No Reply</TD><TD CLASS="tblb">' . $qres['info_noreply'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Bad Reply</TD><TD CLASS="tblb">' . $qres['info_badreply'] . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Low Run Time</TD><TD CLASS="tblb">' . $qres['info_lowruntime'] . ' seconds' . '</TD></TR>';
    print '<TR><TD CLASS="tbla">High Run Time</TD><TD CLASS="tblb">' . $qres['info_highruntime'] . ' seconds' . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Last Run Time</TD><TD CLASS="tblb">' . $qres['info_lastruntime'] . ' seconds' . '</TD></TR>';
    print '<TR><TD CLASS="tbla">First Run</TD><TD CLASS="tblb">' . textsdate($qres['info_firstrun']) . ' (' . elapsed(strtotime($qres['info_firstrun'])) . ' ago)' . '</TD></TR>';
    print '<TR><TD CLASS="tbla">Last Run</TD><TD CLASS="tblb">' . textsdate($qres['info_lastrun']) . ' (' . elapsed(strtotime($qres['info_lastrun'])) . ' ago)' . '</TD></TR>';
    print '</TABLE>';
  }

  include('tail.php');
?>