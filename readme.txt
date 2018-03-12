Serenity v1.019 - Copyright (C) 2004 Chris Cuthbertson
Email: spawn [at] nisda [dot] net
Web: http://nisda.net
Compiled: v1.019: Nov 2004

Serenity is a game query and player tracker.

The long awaited version 1.019 is finally here. 1.018 got ditched because it just was not
working to my standards. However, 1.019 does not have the full testing or long run time
that 1.017 does, if it's a mission critical operation, use that and endure the crude web
interface. Having said that, any problems in 1.019 will be fixed swiftly as with any of
my releases.

* DIRECTORIES

  bin - Precompiled binaries made on Win32 (serenity.exe) and Debian 2.2 Potato (serenity)
  src - Source code.
  web - Web front end.
  other - Font required for graphs.

* REQUIRES

  - PHP 4.3.9 and above with GD2 graphics library
  - MySQL 4.1 and above

  Serenity may work with an older version of PHP/MySQL, if it does, great.
  GD2 library is only needed to generate graphs, disable graphs if you don't have it.

* SUPPORTED GAMES

  - Battlefield 1942 (full)
  - Battlefield Vietnam (partial)

* COMPILING - WIN32

  If you're bent on using compiling for Win32, MinGW is the easiest.
  MinGW: http://www.mingw.org/
  Download: http://prdownloads.sf.net/mingw/MinGW-3.1.0-1.exe?download

  gcc -o serenity serenity.c game.c import.c util.c graph.c -lwsock32 -DWIN32

  - Required library: wsock32.lib
  - Preprocessor Defines: WIN32
  - Compiles with lcc-win32 (http://www.cs.virginia.edu/~lcc-win32/)

* COMPILING - UNIX/Linux

  The usual compile, needs POSIX threads and a C compiler (gcc as an example).

  gcc -o serenity serenity.c game.c util.c import.c graph.c -DUNIX -pthread

  - Preprocessor Defines: UNIX
  - Requires: POSIX Threads

* INSTALLING

  1. Serenity's binary goes in its own directory
  2. other/lucon.ttf goes in same directory as Serenity (only needed for graphs)
  3. Web *.php goes in a web server directory (/htdocs/serenity)

* SERENITY SETUP

   All avaliable switches and their default in brackets
   -t  [timeout]    timeout time in msecs (5)
   -p  [port]       local port to use (12005)
   -n  [querynum]   maximum number of ports to scan (3)
   -q  [querymax]   maximum number of servers to query at a time 0-all (0)
   -f  [debugfile]  output debug messages to this file (serenity_debug)
   -d  [debuglevel] messages to display: 0-none, 1-fatal, 2-info, 3-flood (1)
   -u  [unlink]     keep or delete temp scripts/files (1)
   -x  [runtime]    maximum program run time (45)
   -hf [php exe]    full path to php (php)
   -hc [php param]  any extra parameters for php
   -ms [graph path] full path to write graph images (./images)
   -mg [use graph]  generate graph images (0)
   -mi [mysql ip]   mysql ip address (127.0.0.1)
   -mp [mysql port] mysql port (3306)
   -md [mysql db]   mysql database name (serenity)
   -mu [mysql user] mysql user name (bob)
   -mw [mysql pass] mysql password (secret)
   -mc              test mysql and create database and tables
   -i               import server list (serenity.bf42/serenity.bfv)

  - Database/table creation (-mc)

  It's a good idea to use debug level 3 (-d 3) until every thing goes.
  These are all on one line.

  Syntax:  serenity -mi <MySQL IP> -mp <MySQL Port> -mu <MySQL User> -mw <MySQL Pass>
                    -md <MySQL Database> -hf <PHP Path> -mc -d 3

  Example: serenity -mi 127.0.0.1 -mp 3306 -mu bob -mw secret -md serenity
                    -hf /usr/php/php -mc -d 3

  - Import servers

  Use the -i switch to import servers. Easiest way is to highlight servers in ASE
  (The All Seeing Eye - http://www.udpsoft.com/eye/) and paste them into the appropriate file.

  Files Serenity searches for files in:
    Battlefield 1942: serenity.bf42
    Battlefield Vietnam: serenity.bfv

  - Update

  Usual operation of Serenity.
  Switches to mess with incase you're getting many timeouts:
    -t [timeout]  timeout time in msecs
    -q [querymax] maximum number of servers to query at a time 0-all
    -x [runtime]  maximum program run time before self destruct

  When you're satisfied that it's going right, set it up to run every minute or so.

  Syntax:  serenity -mi <MySQL IP> -mp <MySQL Port> -mu <MySQL User> -mw <MySQL Pass>
                    -md <MySQL Database> -hf <PHP Path> -ms <Graph Path> -mg 1

  Example: serenity -mi 127.0.0.1 -mp 3306 -mu bob -mw secret -md serenity
                    -hf /usr/php/php -ms /usr/apache/htdocs/serenity/images -mg 1

* WEB INTERFACE SETUP

  Open up init.php and make the necessary changes. Easy!
  Open index.php in your browser and you're set. Default admin user/pass is admin/admin.
  Tested browsers: NN 3.xx (No CSS), 4.xx (Basic CSS), IE 6 (Full CSS), Firefox 1.0 (Full CSS)

* WEB INTERFACE

  In case it's not obvious:

  - admin section
    Here you can disallow access/visibility to _everyone_ who's not logged in as admin.

  - pref section
    Individual preferences, doesn't effect others.

* STUFF

  I decided to change my coding style, simply Because I Can.
  Instead of...
    if (something) {
      do stuff;
      more stuff;
    }
  It's now...
    if (something)
    {
      do stuff;
      more stuff;
    }
  ...which may seem rather odd when comparing version sources.
  Also, my gamer nickname is bliP, which explains all the copyright notices to this seemingly-
  but-not second party.

* LICENCE

  #include "gpl.txt"
  If hadn't noticed all the gpl.txt files scattered everywhere, that's the licence.
  Even the test files couldn't miss the stamp.
