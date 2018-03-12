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

function searchbox()
{
  print '<H1 CLASS="hdra">Player Search</H1>';
  print '<FORM METHOD="GET" ACTION="player.php">';
  print '<TABLE CLASS="tblm"><TR><TD CLASS="tbla">';
  print 'Name: ';
  $c = (isset($_GET['player'])) ? $_GET['player'] : '';
  print '<INPUT TYPE="TEXT" NAME="player" VALUE="' . $c . '" SIZE="32" MAXLENGTH="32"> ';
  print '<INPUT TYPE="SUBMIT" VALUE="Search">';
  print '</TD></TR></TABLE>';
  print '</FORM>';
}

function elapsed($time)
{
  $diff = time() - $time;
  $daysDiff = floor($diff / 60 / 60 / 24);
  $diff -= $daysDiff * 60 * 60 * 24;
  $hrsDiff = floor($diff / 60 / 60);
  $diff -= $hrsDiff * 60 * 60;
  $minsDiff = floor($diff / 60);
  $diff -= $minsDiff * 60;
  $secsDiff = $diff;

  $e = NULL;
  if ($daysDiff) { $e = $e . $daysDiff . 'D'; }
  if ($hrsDiff) {
    if ($e) { $e = $e . ' '; }
    $e = $e . $hrsDiff . 'h';
  }
  if ($minsDiff) {
    if ($e) { $e = $e . ' '; }
    $e = $e . $minsDiff . 'm';
  }
  if ($secsDiff) {
    if ($e) { $e = $e . ' '; }
    $e = $e . $secsDiff . 's';
  }
  if (!$e) { $e = 'now'; }

  return ($e);
}

function textsdate($datein)
{
  // Function to return an english short version of a date passed to it
  // e.g. 2003-10-24 20:00:00 becomes 'Fri 24/10/03 20:00'
    
  list($year,$month,$day,$hour,$minute,$second) = sscanf($datein,"%4s-%2s-%2s %2s:%2s:%2s");
  $unxdte = mktime($hour, $minute, $second, $month, $day, $year);
  $retdate = date("D d/m/y H:i", $unxdte);
    
  return ($retdate);
}

function units($size)
{
  $kb = 1024;
  $mb = 1024 * $kb;
  $gb = 1024 * $mb;
  $tb = 1024 * $gb;

  if ($size < $kb) { return $size . "B"; }
  else if ($size < $mb) { return round($size / $kb, 0) . "KB"; }
  else if ($size < $gb) { return round($size / $mb, 0) . "MB"; }
  else if ($size < $tb) { return round($size / $gb, 0) . "GB"; }
  else { return round($size / $tb, 2) . "TB"; }
}

function alert($type, $pagetitle, $message)
{
  if ($type & 1)
  {
    include('head.php');
  }
  if ($type & 2)
  {
    print '<H2 CLASS="hdrf">' . $message . '</H2>';
  }
  else if ($type & 4)
  {
    print '<H2 CLASS="hdrg">' . $message . '</H2>';
  }
  include('tail.php');
}
?>