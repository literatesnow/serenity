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

  $ip = '127.0.0.1:3306';
  $user = 'user';
  $pw = 'pass';
  $select = 1;
  $graph_dir = './images';

  $db = mysql_connect($ip, $user, $pw);
  if (!$db)
  {
    echo 'Could not connect: ' . mysql_error() . "\n";
    exit(2);
  }
  if ($select)
  {
    if (!mysql_select_db('serenity', $db)) {
      echo 'Could not select: ' . mysql_error() . "\n";
      exit(3);
    }
  }
?>