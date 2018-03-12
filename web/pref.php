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
  $pagetitle = 'Serenity - Preferences';
  include('head.php');
  include('common.php');

  print '<H1 CLASS="hdra">Preferences</H1>';

  if (isset($_POST['r_save']))
  {
    $prefindex = 0;
    if (isset($_POST['i_address'])) { $prefindex += 1; }
    if (isset($_POST['i_players'])) { $prefindex += 2; }
    if (isset($_POST['i_map'])) { $prefindex += 4; }
    if (isset($_POST['i_gametype'])) { $prefindex += 8; }
    if (isset($_POST['i_version'])) { $prefindex += 16; }
    if (isset($_POST['i_ok'])) { $prefindex += 32; }
    if (isset($_POST['i_fails'])) { $prefindex += 64; }
    if (isset($_POST['i_lastupdated'])) { $prefindex += 128; }
    if (isset($_POST['i_bytesrx'])) { $prefindex += 256; }
    if (isset($_POST['i_bytestx'])) { $prefindex += 512; }
    if (isset($_POST['i_trashed'])) { $prefindex += 1024; }

    $prefserver = 0;
    if (isset($_POST['s_score'])) { $prefserver += 1; }
    if (isset($_POST['s_frags'])) { $prefserver += 2; }
    if (isset($_POST['s_deaths'])) { $prefserver += 4; }
    if (isset($_POST['s_ping'])) { $prefserver += 8; }
    if (isset($_POST['s_team'])) { $prefserver += 16; }
    if (isset($_POST['s_userid'])) { $prefserver += 32; }
    if (isset($_POST['s_connect'])) { $prefserver += 64; }
    if (isset($_POST['s_keyhash'])) { $prefserver += 128; }

    $prefplayer = 0;
    if (isset($_POST['p_score'])) { $prefplayer += 1; }
    if (isset($_POST['p_frags'])) { $prefplayer += 2; }
    if (isset($_POST['p_deaths'])) { $prefplayer += 4; }
    if (isset($_POST['p_hostname'])) { $prefplayer += 8; }
    if (isset($_POST['p_ping'])) { $prefplayer += 16; }
    if (isset($_POST['p_team'])) { $prefplayer += 32; }
    if (isset($_POST['p_userid'])) { $prefplayer += 64; }
    if (isset($_POST['p_connect'])) { $prefplayer += 128; }
    if (isset($_POST['p_lastseen'])) { $prefplayer += 256; }
    if (isset($_POST['p_keyhash'])) { $prefplayer += 512; }

    $prefoption = 0;
    //if (isset($_POST['o_search'])) { $prefoption += 1; }
    if (isset($_POST['o_details'])) { $prefoption += 2; }
    if (isset($_POST['o_usage'])) { $prefoption += 4; }
    if (isset($_POST['o_rules'])) { $prefoption += 8; }
    if (isset($_POST['o_combine'])) { $prefoption += 16; }
    if (isset($_POST['o_tiny'])) { $prefoption += 32; }

    $cookie = $prefindex . ':' . $prefserver . ':' . $prefplayer . ':' . $prefoption;
    setcookie('pref', $cookie, time() + 5184000);
    
    alert(4, NULL, 'Settings Saved');
    return;
  }

  //print '<H2 CLASS="hdrc">Public Interface</H2>';

  print '<FORM METHOD="POST" ACTION="pref.php">';

  if ($webindex > 0)
  {
    print '<H2 CLASS="hdrc">Front Page</H2>';
    print '<TABLE CLASS="tblm" WIDTH="30%">';
    print '<TR>';
    print '<TD CLASS="tblc">Column Name</TD>';
    print '<TD CLASS="tblc">Visable</TD>';
    print '</TR>';

    if ((!$isadmin && ($webindex & 1)) || $isadmin)
    {
      $c = ($prefindex & 1) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Address</TD><TD><INPUT TYPE="checkbox" NAME="i_address"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 2)) || $isadmin)
    {
      $c = ($prefindex & 2) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Players</TD><TD><INPUT TYPE="checkbox" NAME="i_players"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 4)) || $isadmin)
    {
      $c = ($prefindex & 4) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Map</TD><TD><INPUT TYPE="checkbox" NAME="i_map"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 8)) || $isadmin)
    {
      $c = ($prefindex & 8) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Game Type</TD><TD><INPUT TYPE="checkbox" NAME="i_gametype"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 16)) || $isadmin)
    {
      $c = ($prefindex & 16) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Version</TD><TD><INPUT TYPE="checkbox" NAME="i_version"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 32)) || $isadmin)
    {
      $c = ($prefindex & 32) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Update OK</TD><TD><INPUT TYPE="checkbox" NAME="i_ok"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 64)) || $isadmin)
    {
      $c = ($prefindex & 64) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Update Failed</TD><TD><INPUT TYPE="checkbox" NAME="i_fails"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 128)) || $isadmin)
    {
      $c = ($prefindex & 128) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Last Update</TD><TD><INPUT TYPE="checkbox" NAME="i_lastupdated"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 256)) || $isadmin)
    {
      $c = ($prefindex & 256) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Bytes Recieved</TD><TD><INPUT TYPE="checkbox" NAME="i_bytesrx"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 512)) || $isadmin)
    {
      $c = ($prefindex & 512) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Bytes Sent</TD><TD><INPUT TYPE="checkbox" NAME="i_bytestx"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webindex & 1024)) || $isadmin)
    {
      $c = ($prefindex & 1024) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Trashed Queries</TD><TD><INPUT TYPE="checkbox" NAME="i_trashed"' . $c . '></TD></TR>';
    }
    print '</TABLE>';
  }

  if ($webserver > 0)
  {
    print '<H2 CLASS="hdrc">Single Server</H2>';
    print '<TABLE CLASS="tblm" WIDTH="30%">';
    print '<TR>';
    print '<TD CLASS="tblc">Column Name</TD>';
    print '<TD CLASS="tblc">Visable</TD>';
    print '</TR>';

    if ((!$isadmin && ($webserver & 1)) || $isadmin)
    {
      $c = ($prefserver & 1) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Score</TD><TD><INPUT TYPE="checkbox" NAME="s_score"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 2)) || $isadmin)
    {
      $c = ($prefserver & 2) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Frags</TD><TD><INPUT TYPE="checkbox" NAME="s_frags"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 4)) || $isadmin)
    {
      $c = ($prefserver & 4) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Deaths</TD><TD><INPUT TYPE="checkbox" NAME="s_deaths"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 8)) || $isadmin)
    {
      $c = ($prefserver & 8) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Ping</TD><TD><INPUT TYPE="checkbox" NAME="s_ping"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 16)) || $isadmin)
    {
      $c = ($prefserver & 16) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Team</TD><TD><INPUT TYPE="checkbox" NAME="s_team"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 32)) || $isadmin)
    {
      $c = ($prefserver & 32) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">User ID</TD><TD><INPUT TYPE="checkbox" NAME="s_userid"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 64)) || $isadmin)
    {
      $c = ($prefserver & 64) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Connect</TD><TD><INPUT TYPE="checkbox" NAME="s_connect"' . $c . '></TD></TR>';
    }  
    if ((!$isadmin && ($webserver & 128)) || $isadmin)
    {
      $c = ($prefserver & 128) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Keyhash</TD><TD><INPUT TYPE="checkbox" NAME="s_keyhash"' . $c . '></TD></TR>';
    }  
    print '</TABLE>';
  }

  if ($webplayer > 0)
  {
    print '<H2 CLASS="hdrc">Player Search</H2>';
    print '<TABLE CLASS="tblm" WIDTH="30%">';
    print '<TR>';
    print '<TD CLASS="tblc">Column Name</TD>';
    print '<TD CLASS="tblc">Visable</TD>';
    print '</TR>';

    if ((!$isadmin && ($webplayer & 1)) || $isadmin)
    {
      $c = ($prefplayer & 1) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Score</TD><TD><INPUT TYPE="checkbox" NAME="p_score"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 2)) || $isadmin)
    {
      $c = ($prefplayer & 2) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Frags</TD><TD><INPUT TYPE="checkbox" NAME="p_frags"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 4)) || $isadmin)
    {
      $c = ($prefplayer & 4) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Deaths</TD><TD><INPUT TYPE="checkbox" NAME="p_deaths"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 8)) || $isadmin)
    {
      $c = ($prefplayer & 8) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Hostname</TD><TD><INPUT TYPE="checkbox" NAME="p_hostname"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 16)) || $isadmin)
    {
      $c = ($prefplayer & 16) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Ping</TD><TD><INPUT TYPE="checkbox" NAME="p_ping"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 32)) || $isadmin)
    {
      $c = ($prefplayer & 32) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Team</TD><TD><INPUT TYPE="checkbox" NAME="p_team"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 64)) || $isadmin)
    {
      $c = ($prefplayer & 64) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">User ID</TD><TD><INPUT TYPE="checkbox" NAME="p_userid"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 128)) || $isadmin)
    {
      $c = ($prefplayer & 128) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Connect</TD><TD><INPUT TYPE="checkbox" NAME="p_connect"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 256)) || $isadmin)
    {
      $c = ($prefplayer & 256) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Last Seen</TD><TD><INPUT TYPE="checkbox" NAME="p_lastseen"' . $c . '></TD></TR>';
    }
    if ((!$isadmin && ($webplayer & 512)) || $isadmin)
    {
      $c = ($prefplayer & 512) ? ' CHECKED' : '';
      print '<TR><TD CLASS="tbla">Keyhash</TD><TD><INPUT TYPE="checkbox" NAME="p_keyhash"' . $c . '></TD></TR>';
    }
    print '</TABLE>';
  }

  print '<H2 CLASS="hdrc">Public Options</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';
  if ((!$isadmin && ($weboption & 2)) || $isadmin)
  {
    $c = ($prefoption & 2) ? ' CHECKED' : '';
    print '<TR><TD CLASS="tbla"><INPUT CLASS="cntr" TYPE="checkbox" NAME="o_details"' . $c . '> Show Serenity information on front page</TD></TR>';
  }
  if ((!$isadmin && ($weboption & 4)) || $isadmin)
  {
    $c = ($prefoption & 4) ? ' CHECKED' : '';
    print '<TR><TD CLASS="tbla"><INPUT CLASS="cntr" TYPE="checkbox" NAME="o_usage"' . $c . '> Show usage graphs on server page</TD></TR>';
  }
  if ((!$isadmin && ($weboption & 8)) || $isadmin)
  {
    $c = ($prefoption & 8) ? ' CHECKED' : '';
    print '<TR><TD CLASS="tbla"><INPUT CLASS="cntr" TYPE="checkbox" NAME="o_rules"' . $c . '> Show rules on server page</TD></TR>';
  }
  //if ((!$isadmin && (($webserver & 1) || ($webplayer & 1))) || $isadmin)
  //{
    $c = ($prefoption & 16) ? ' CHECKED' : '';
    print '<TR><TD CLASS="tbla"><INPUT CLASS="cntr" TYPE="checkbox" NAME="o_combine"' . $c . '> Combine score, frags and deaths columns</TD></TR>';
  //}
  if ((!$isadmin && (($webserver & 128) || ($webplayer & 256))) || $isadmin)
  {
    $c = ($prefoption & 32) ? ' CHECKED' : '';
    print '<TR><TD CLASS="tbla"><INPUT CLASS="cntr" TYPE="checkbox" NAME="o_tiny"' . $c . '> Display keyhash in tiny font</TD></TR>';
  }

  print '</TABLE>';

  print '<H2 CLASS="hdrc">Apply Changes</H2>';

  //print '<TABLE CLASS="tblm" WIDTH="30%"><TR><TD>';
  print '<INPUT TYPE="HIDDEN" VALUE="1" NAME="r_save">';
  print '<INPUT TYPE="SUBMIT" VALUE="Save">';//</TD></TR></TABLE>';
  print '</FORM>';

  include('tail.php');

  return;
?>
