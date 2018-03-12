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
  $pagetitle = 'Serenity - Admin';
  include('head.php');
  include('common.php');

  print '<H1 CLASS="hdra">Admin</H1>';

  if (isset($_GET['logout']))
  {
    setcookie('admin', '', time() - 5184000);
    alert(4, NULL, 'Logged out');
    return;
  }

  if (!$isadmin && isset($_POST['l_user']) && isset($_POST['l_pass']))
  {
    if (($_POST['l_user'] != $qarr['info_webuser']) || (md5($_POST['l_pass']) != $qarr['info_webpass']))
    {
      alert(2, NULL, 'Bad user name or password');
      return;
    }
    setcookie('admin', $qarr['info_webuser'] . ':' .  $qarr['info_webpass'], time() + 5184000);
  }
  else if (!$isadmin)
  {
    print '<H2 CLASS="hdrc">Login</H2>';
    print '<FORM METHOD="POST" ACTION="admin.php">';
    print '<TABLE WIDTH="30%">';
    print '<TR><TD CLASS="tbla">Username:</TD><TD><INPUT TYPE="TEXT" NAME="l_user" SIZE="20" MAXLENGTH="32"></TD></TR>';
    print '<TR><TD CLASS="tbla">Password:</TD><TD><INPUT TYPE="PASSWORD" NAME="l_pass" SIZE="20" MAXLENGTH="32"></TD></TR>';
    print '<TR><TD><INPUT TYPE="SUBMIT" VALUE="Submit"></TD></TR>';
    print '</TABLE>';
    print '</FORM>';

    include('tail.php');
    return;
  }

  if (isset($_POST['a_save']))
  {
    $indexfpd = 0;
    if (isset($_POST['i_address'])) { $indexfpd += 1; }
    if (isset($_POST['i_players'])) { $indexfpd += 2; }
    if (isset($_POST['i_map'])) { $indexfpd += 4; }
    if (isset($_POST['i_gametype'])) { $indexfpd += 8; }
    if (isset($_POST['i_version'])) { $indexfpd += 16; }
    if (isset($_POST['i_ok'])) { $indexfpd += 32; }
    if (isset($_POST['i_fails'])) { $indexfpd += 64; }
    if (isset($_POST['i_lastupdated'])) { $indexfpd += 128; }
    if (isset($_POST['i_bytesrx'])) { $indexfpd += 256; }
    if (isset($_POST['i_bytestx'])) { $indexfpd += 512; }
    if (isset($_POST['i_trashed'])) { $indexfpd += 1024; }

    $serverfpd = 0;
    if (isset($_POST['s_score'])) { $serverfpd += 1; }
    if (isset($_POST['s_frags'])) { $serverfpd += 2; }
    if (isset($_POST['s_deaths'])) { $serverfpd += 4; }
    if (isset($_POST['s_ping'])) { $serverfpd += 8; }
    if (isset($_POST['s_team'])) { $serverfpd += 16; }
    if (isset($_POST['s_userid'])) { $serverfpd += 32; }
    if (isset($_POST['s_connect'])) { $serverfpd += 64; }
    if (isset($_POST['s_keyhash'])) { $serverfpd += 128; }

    $playerfpd = 0;
    if (isset($_POST['p_score'])) { $playerfpd += 1; }
    if (isset($_POST['p_frags'])) { $playerfpd += 2; }
    if (isset($_POST['p_deaths'])) { $playerfpd += 4; }
    if (isset($_POST['p_hostname'])) { $playerfpd += 8; }
    if (isset($_POST['p_ping'])) { $playerfpd += 16; }
    if (isset($_POST['p_team'])) { $playerfpd += 32; }
    if (isset($_POST['p_userid'])) { $playerfpd += 64; }
    if (isset($_POST['p_connect'])) { $playerfpd += 128; }
    if (isset($_POST['p_lastseen'])) { $playerfpd += 256; }
    if (isset($_POST['p_keyhash'])) { $playerfpd += 512; }

    $optionfpd = 0;
    if (isset($_POST['o_search'])) { $optionfpd += 1; }
    if (isset($_POST['o_details'])) { $optionfpd += 2; }
    if (isset($_POST['o_usage'])) { $optionfpd += 4; }
    if (isset($_POST['o_rules'])) { $optionfpd += 8; }
    if (isset($_POST['o_combine'])) { $optionfpd += 16; }
    if (isset($_POST['o_tiny'])) { $optionfpd += 32; }

    $sstr = 'UPDATE serenity_info SET ' .
            'info_webindex = ' . $indexfpd . ', ' .
            'info_webserver = ' . $serverfpd . ', ' .
            'info_webplayer = ' . $playerfpd . ', ' .
            'info_weboptions = ' . $optionfpd;

    if (isset($_POST['c_old']) && isset($_POST['c_new']))
    {
      if (md5($_POST['c_old']) == $qarr['info_webpass'])
      {
        $sstr .= ", info_webpass = '" . md5($_POST['c_new']) . "'";
      }
    }

    $sstr .= ' WHERE info_id = 1';
    $stqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());

    alert(4, NULL, 'Settings Saved');
    return;
  }

  //print '<H1 CLASS="hdra">Public Interface</H1>';

  print '<FORM METHOD="POST" ACTION="admin.php">';

  print '<H2 CLASS="hdrc">Front Page</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';
  print '<TR>';
  print '<TD CLASS="tblc">Column Name</TD>';
  print '<TD CLASS="tblc">Visable</TD>';
  print '</TR>';

  $c = ($qarr['info_webindex'] & 1) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Address</TD><TD><INPUT TYPE="checkbox" NAME="i_address"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 2) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Players</TD><TD><INPUT TYPE="checkbox" NAME="i_players"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 4) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Map</TD><TD><INPUT TYPE="checkbox" NAME="i_map"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 8) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Game Type</TD><TD><INPUT TYPE="checkbox" NAME="i_gametype"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 16) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Version</TD><TD><INPUT TYPE="checkbox" NAME="i_version"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 32) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Update OK</TD><TD><INPUT TYPE="checkbox" NAME="i_ok"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 64) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Update Failed</TD><TD><INPUT TYPE="checkbox" NAME="i_fails"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 128) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Last Update</TD><TD><INPUT TYPE="checkbox" NAME="i_lastupdated"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 256) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Bytes Recieved</TD><TD><INPUT TYPE="checkbox" NAME="i_bytesrx"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 512) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Bytes Sent</TD><TD><INPUT TYPE="checkbox" NAME="i_bytestx"' . $c . '></TD></TR>';
  $c = ($qarr['info_webindex'] & 1024) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Trashed Queries</TD><TD><INPUT TYPE="checkbox" NAME="i_trashed"' . $c . '></TD></TR>';
  print '</TABLE>';

  print '<H2 CLASS="hdrc">Single Server</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';
  print '<TR>';
  print '<TD CLASS="tblc">Column Name</TD>';
  print '<TD CLASS="tblc">Visable</TD>';
  print '</TR>';

  $c = ($qarr['info_webserver'] & 1) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Score</TD><TD><INPUT TYPE="checkbox" NAME="s_score"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 2) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Frags</TD><TD><INPUT TYPE="checkbox" NAME="s_frags"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 4) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Deaths</TD><TD><INPUT TYPE="checkbox" NAME="s_deaths"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 8) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Ping</TD><TD><INPUT TYPE="checkbox" NAME="s_ping"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 16) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Team</TD><TD><INPUT TYPE="checkbox" NAME="s_team"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 32) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">User ID</TD><TD><INPUT TYPE="checkbox" NAME="s_userid"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 64) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Connect</TD><TD><INPUT TYPE="checkbox" NAME="s_connect"' . $c . '></TD></TR>';
  $c = ($qarr['info_webserver'] & 128) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Keyhash</TD><TD><INPUT TYPE="checkbox" NAME="s_keyhash"' . $c . '></TD></TR>';
  print '</TABLE>';

  print '<H2 CLASS="hdrc">Player Search</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';
  print '<TR>';
  print '<TD CLASS="tblc">Column Name</TD>';
  print '<TD CLASS="tblc">Visable</TD>';
  print '</TR>';

  $c = ($qarr['info_webplayer'] & 1) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Score</TD><TD><INPUT TYPE="checkbox" NAME="p_score"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 2) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Frags</TD><TD><INPUT TYPE="checkbox" NAME="p_frags"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 4) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Deaths</TD><TD><INPUT TYPE="checkbox" NAME="p_deaths"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 8) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Hostname</TD><TD><INPUT TYPE="checkbox" NAME="p_hostname"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 16) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Ping</TD><TD><INPUT TYPE="checkbox" NAME="p_ping"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 32) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Team</TD><TD><INPUT TYPE="checkbox" NAME="p_team"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 64) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">User ID</TD><TD><INPUT TYPE="checkbox" NAME="p_userid"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 128) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Connect</TD><TD><INPUT TYPE="checkbox" NAME="p_connect"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 256) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Last Seen</TD><TD><INPUT TYPE="checkbox" NAME="p_lastseen"' . $c . '></TD></TR>';
  $c = ($qarr['info_webplayer'] & 512) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla">Keyhash</TD><TD><INPUT TYPE="checkbox" NAME="p_keyhash"' . $c . '></TD></TR>';
  print '</TABLE>';

  print '<H2 CLASS="hdrc">Public Options</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';

  $c = ($qarr['info_weboptions'] & 1) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_search"' . $c . '> Allow player search</TD></TR>';
  $c = ($qarr['info_weboptions'] & 2) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_details"' . $c . '> Show Serenity information on front page</TD></TR>';
  $c = ($qarr['info_weboptions'] & 4) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_usage"' . $c . '> Show usage graphs on server page</TD></TR>';
  $c = ($qarr['info_weboptions'] & 8) ? ' CHECKED' : '';
  print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_rules"' . $c . '> Show rules on server page</TD></TR>';
  //$c = ($qarr['info_weboptions'] & 16) ? ' CHECKED' : '';
  //print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_combine"' . $c . '> Combine score, frags and deaths columns</TD></TR>';
  //$c = ($qarr['info_weboptions'] & 32) ? ' CHECKED' : '';
  //print '<TR><TD CLASS="tbla"><INPUT CLASS="centerrow" TYPE="checkbox" NAME="o_tiny"' . $c . '> Display keyhash in tiny font</TD></TR>';
  print '</TABLE>';

  print '<H2 CLASS="hdrc">Change Admin Password</H2>';
  print '<TABLE CLASS="tblm" WIDTH="30%">';
  print '<TR><TD CLASS="tbla">Old Password:</TD><TD><INPUT TYPE="PASSWORD" NAME="c_old" SIZE="20" MAXLENGTH="32"></TD></TR>';
  print '<TR><TD CLASS="tbla">New Password:</TD><TD><INPUT TYPE="PASSWORD" NAME="c_new" SIZE="20" MAXLENGTH="32"></TD></TR>';
  print '</TABLE>';


  print '<H2 CLASS="hdrc">Apply Changes</H2>';
  print '<INPUT TYPE="hidden" VALUE="1" NAME="a_save">';
  print '<INPUT TYPE="SUBMIT" VALUE="Save"></TD></TABLE>';
  print '</FORM>';

  include('tail.php');

  return;
?>