/*
SQLyog Enterprise - MySQL GUI v6.0 Beta 4 
Host - 5.0.27 : Database - serenity-v3
*********************************************************************
Server version : 5.0.27
*/


/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

/*Table structure for table `game_address` */

DROP TABLE IF EXISTS `game_address`;

CREATE TABLE `game_address` (
  `address_id` int(20) unsigned NOT NULL auto_increment,
  `address_player_id` int(20) unsigned NOT NULL,
  `address_net_ip` varchar(12) NOT NULL,
  `address_last_seen` int(10) default NULL,
  PRIMARY KEY  (`address_id`),
  KEY `idx_searchables` (`address_player_id`,`address_net_ip`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Table structure for table `game_keyhash` */

DROP TABLE IF EXISTS `game_keyhash`;

CREATE TABLE `game_keyhash` (
  `keyhash_id` int(20) unsigned NOT NULL auto_increment,
  `keyhash_player_id` int(20) unsigned NOT NULL,
  `keyhash_hash` varchar(100) NOT NULL,
  `keyhash_pbguid` varchar(100) default NULL,
  `keyhash_last_seen` int(10) default NULL,
  PRIMARY KEY  (`keyhash_id`),
  KEY `idx_searchables` (`keyhash_player_id`,`keyhash_hash`,`keyhash_pbguid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Table structure for table `game_player` */

DROP TABLE IF EXISTS `game_player`;

CREATE TABLE `game_player` (
  `player_id` int(20) unsigned NOT NULL auto_increment,
  `player_name` varchar(30) NOT NULL,
  `player_pid` varchar(10) default NULL,
  `player_last_seen` int(10) default NULL,
  `player_last_server` varchar(255) default NULL,
  PRIMARY KEY  (`player_id`),
  FULLTEXT KEY `idx_searchables` (`player_name`,`player_pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Table structure for table `game_serverplayer` */

DROP TABLE IF EXISTS `game_serverplayer`;

CREATE TABLE `game_serverplayer` (
  `serverplayer_server_id` int(5) unsigned NOT NULL,
  `serverplayer_player_id` int(20) unsigned NOT NULL,
  KEY `idx_searchables` (`serverplayer_server_id`,`serverplayer_player_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Table structure for table `global_server` */

DROP TABLE IF EXISTS `global_server`;

CREATE TABLE `global_server` (
  `server_id` int(10) unsigned NOT NULL auto_increment,
  `server_type` smallint(5) unsigned NOT NULL,
  `server_name` varchar(16) default NULL,
  `server_netip` varchar(16) NOT NULL,
  `server_gameport` smallint(5) unsigned NOT NULL,
  `server_queryport` smallint(5) unsigned NOT NULL,
  `server_rconport` smallint(5) unsigned default NULL,
  `server_rcon` varchar(64) default NULL,
  `server_adminonly` tinyint(1) default '0',
  `server_hostname` varchar(32) default NULL,
  `server_gametype` varchar(32) default NULL,
  `server_gameid` varchar(32) default NULL,
  `server_mapname` varchar(32) default NULL,
  `server_numplayers` smallint(5) unsigned default NULL,
  `server_maxplayers` smallint(5) unsigned default NULL,
  `server_version` varchar(12) default NULL,
  `server_updatesuccess` int(10) unsigned default NULL,
  `server_updatefail` int(10) unsigned default NULL,
  `server_consecutivefails` int(10) unsigned default '1',
  `server_lastupdate` datetime default NULL,
  `server_bytesreceived` bigint(20) unsigned default '0',
  `server_bytessent` bigint(20) unsigned default '0',
  PRIMARY KEY  (`server_id`),
  KEY `idx_searchables` (`server_type`)
) ENGINE=MyISAM AUTO_INCREMENT=35 DEFAULT CHARSET=latin1;

/*Table structure for table `global_servertype` */

DROP TABLE IF EXISTS `global_servertype`;

CREATE TABLE `global_servertype` (
  `servertype_id` smallint(5) unsigned NOT NULL,
  `servertype_desc` varchar(32) NOT NULL,
  `servertype_shortdesc` varchar(16) NOT NULL,
  `servertype_table_prefix` varchar(10) NOT NULL,
  PRIMARY KEY  (`servertype_id`),
  KEY `idx_searchables` (`servertype_table_prefix`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;