-- Schema for Serenity

-- 7 May 2007
--   First revision for version 3

-- 22nd August 2006
--   Added tables indexes:
--     keyhash_hash_ix
--     pbguid_guid_ix
--     playerkeyhash_player_id_ix
--     playerkeyhash_keyhash_id_ix
--     address_netip_ix
--     playeraddress_player_id_ix
--     playeraddress_address_id_ix
--     player_name_ix
--  Added unique constraint:
--     server_name_un

-- 20th August 2006
--   Added serenity_pbguid table

-- 13th August 2006
--   Added serenity_status table

-- 11th August 2006
--   Added serenity_serverplayer table
--   Added server_bytesreceived, server_bytessent to serenity_server
--   Data type change:
--     serenity_player.player_id -> BIGINT
--     serenity_server.server_id -> INT
--   Column renamed for consistancy:
--     serenity_player.player_server -> player_server_id

-- 7th August 2006
--   Initial

CREATE DATABASE IF NOT EXISTS serenity;
USE serenity;

CREATE TABLE IF NOT EXISTS bf2_address (
  address_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  address_player_id BIGINT UNSIGNED NOT NULL,
  address_netip VARCHAR(16) NOT NULL,
  address_lastseen DATETIME NOT NULL,
  CONSTRAINT address_pk PRIMARY KEY (address_id),
  KEY bf2_address_idx (address_player_id, address_netip)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS bf2_keyhash (
  keyhash_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  keyhash_player_id BIGINT UNSIGNED NOT NULL,
  keyhash_hash VARCHAR(64) NOT NULL,
  keyhash_guid VARCHAR(64) NULL DEFAULT NULL,
  keyhash_lastseen DATETIME NOT NULL,
  CONSTRAINT keyhash_pk PRIMARY KEY (keyhash_id),
  KEY bf2_keyhash_idx (keyhash_player_id, keyhash_hash, keyhash_guid)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS bf2_player (
  player_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  player_name VARCHAR(40) NOT NULL,
  player_pid VARCHAR(16) NOT NULL,
  player_currentserver_id INT UNSIGNED NULL DEFAULT NULL,
  player_lastserver VARCHAR(128) NOT NULL,
  player_lastseen DATETIME NOT NULL,
  CONSTRAINT player_pk PRIMARY KEY (player_id),
  FULLTEXT KEY bf2_player_idx (player_name, player_pid)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS serenity_server (
  server_id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  server_type SMALLINT UNSIGNED NOT NULL,
  server_name VARCHAR(16) NULL,
  server_netip VARCHAR(16) NOT NULL,
  server_gameport SMALLINT UNSIGNED NOT NULL,
  server_queryport SMALLINT UNSIGNED NOT NULL,
  server_rconport SMALLINT UNSIGNED NULL,
  server_rcon VARCHAR(64) NULL,
  server_adminonly TINYINT(1) NOT NULL DEFAULT 0,
  server_hostname VARCHAR(32) NOT NULL,
  server_gametype VARCHAR(32) NOT NULL,
  server_gameid VARCHAR(32) NOT NULL,
  server_mapname VARCHAR(32) NOT NULL,
  server_numplayers SMALLINT UNSIGNED NOT NULL,
  server_maxplayers SMALLINT UNSIGNED NOT NULL,
  server_version VARCHAR(12) NOT NULL,
  server_bytesreceived BIGINT UNSIGNED NOT NULL DEFAULT 0,
  server_bytessent BIGINT UNSIGNED NOT NULL DEFAULT 0,
  server_updatesuccess INT UNSIGNED NOT NULL,
  server_updatefail INT UNSIGNED NOT NULL,
  server_consecutivefails INT UNSIGNED NOT NULL DEFAULT 1,
  server_lastupdate DATETIME NOT NULL,
  CONSTRAINT server_pk PRIMARY KEY (server_id),
  UNIQUE server_name_un USING BTREE (server_name)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS serenity_servertype (
  servertype_id SMALLINT UNSIGNED NOT NULL,
  servertype_desc VARCHAR(32) NOT NULL,
  servertype_shortdesc VARCHAR(16) NOT NULL,
  servertype_tableprefix VARCHAR(12) NOT NULL,
  CONSTRAINT servertype_pk PRIMARY KEY (servertype_id)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS serenity_status (
  status_id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
  status_version VARCHAR(32) NOT NULL,
  status_ping DATETIME NOT NULL,
  CONSTRAINT status_pk PRIMARY KEY (status_id)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

