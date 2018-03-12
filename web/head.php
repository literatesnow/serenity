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

  $sstr = 'SELECT info_webuser, info_webpass, info_webindex, info_webserver, info_webplayer, info_weboptions FROM serenity_info';
  $stqry = mysql_query($sstr) or die ('Bad query ' . mysql_error());
  $qarr = mysql_fetch_array($stqry, MYSQL_ASSOC);

  $isadmin = 0;
  if (isset($_COOKIE['admin']))
  {
    $cookie = explode(':', $_COOKIE['admin']);
    if (($cookie[0] == $qarr['info_webuser']) && ($cookie[1] == $qarr['info_webpass']))
    {
      $isadmin = 1;
    }
  }

  $webindex = $qarr['info_webindex'];
  $webserver = $qarr['info_webserver'];
  $webplayer = $qarr['info_webplayer'];
  $weboption = $qarr['info_weboptions'];

  if (isset($_COOKIE['pref']))
  {
    $cookie = explode(':', $_COOKIE['pref']);
    $prefindex = $cookie[0];
    $prefserver = $cookie[1];
    $prefplayer = $cookie[2];
    $prefoption = $cookie[3];
  }
  else
  {
    $prefindex = $qarr['info_webindex'];
    $prefserver = $qarr['info_webserver'];
    $prefplayer = $qarr['info_webplayer'];
    $prefoption = $qarr['info_weboptions'];
  }

  //print $webindex . ' ' . $webserver . ' ' . $webplayer . ' ' . $weboption . '<BR>';
  //print $prefindex . ' ' . $prefserver . ' ' . $prefplayer . ' ' . $prefoption . '<BR>';

  if (!$pagetitle)
  {
    $pagetitle = 'Serenity';
  }
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<HTML>
<HEAD>
<TITLE><?php print $pagetitle; ?></TITLE>
<META HTTP-EQUIV="content-type" CONTENT="text/html; charset=iso-8859-1">
<META NAME="author" CONTENT="Chris Cuthbertson">
<LINK REL="stylesheet" HREF="simple.css" TYPE="text/css">
<STYLE TYPE="text/css">@import url(normal.css);</STYLE>
</HEAD>
<BODY>