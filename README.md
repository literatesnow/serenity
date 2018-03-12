# Serenity

Serenity is a player tracker for game servers. Saving the results to a database allows server administrators to search for players based on many criteria.

During its heyday, Serenity helped catch various troublemakers who played on monitored servers:

* Many players who thought they were anonymous by changing name
* One high profile player who was actually writing cheats
* Requesting player's screenshot and detect any cheats in the image (using 3rd party cheat detection)
* ~~Automatically ban players who were inside walls when they weren't supposed to be (moved to [crank](https://github.com/literatesnow/crank))~~

## Features

* Tracks various player information:
    * IP address
    * Keyhash
    * PunkBuster Guid
    * Last seen
    * Last server
* Save player's screenshots and automatically run script against file.
* MySQL database support.

## Game Support

### v1.019

* Battlefield 1942
* Battlefield: Vietnam (partial)

### v1.99

* Battlefield 2
* Source Engine (incomplete)

## Releases

Version|Date|Notes
---|---|---
v1.019|19th November 2004|First public release
v1.99|2008|Last private release

## Source

This is the source code for the last private release and it may not compile or run.
Required libraries are ``libmysql``, ``libpcre`` and ``libconfig`` (see [Third Party Software](#third-party-software)).

### Windows

Compile with Microsoft Visual Studio 6.

### Linux

Compile with GCC using the ``Makefile`` in the [src/](src/) directory.

```bash
  cd src
  make                  # Build debug version
  make BUILD=release    # Build release version
```

## History

Whilst coding a server manager on 28th December 2003, I discovered that Battlefield 1942 servers will give out a player's keyhash when queried. This had apparently been available since the Battlefield v1.6 patch from 17th December 2003. With this information it would be possible to track players, what names they use and which servers they play on.

The only problem was that there was too much traffic required for my home connection to handle as a 32 player server would generate around 4KB of data. Even querying 8 full servers would saturate the connection. Because of this, the idea was put on hold. In April 2004 after mentioning the idea to mpx, he said that it would be very useful and put me through to Dr-J- and subsequently Biggus who had a private server which could be used to run the project on.

Since then, the project evolved to support other games and have more features.

### Third Party Software

* [libmysql](https://dev.mysql.com/downloads/connector/c/)
* [pcre](https://www.pcre.org/)
* [libconfig](https://github.com/hyperrealm/libconfig)
* [strlcpy/strlcat](http://www.gratisoft.us/todd/papers/strlcpy.html)
* [MemLeakCheck](http://tunesmithy.co.uk/memleakcheck/index.htm)
