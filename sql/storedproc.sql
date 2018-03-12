DELIMITER //

CREATE PROCEDURE UpdateBF2Player (
    IN _server_id INT UNSIGNED,
    IN _server_type SMALLINT UNSIGNED,
    IN _player_name VARCHAR(32),
    IN _player_hash VARCHAR(32),
    IN _player_guid VARCHAR(32),
    IN _player_netip VARCHAR(16),
    IN _player_pid VARCHAR(12),
    IN _player_score INT,
    IN _player_deaths INT,
    IN _player_team VARCHAR(8),
    IN _player_ping SMALLINT UNSIGNED
  )
BEGIN
  DECLARE _player_id BIGINT UNSIGNED;
  DECLARE _keyhash_id BIGINT UNSIGNED;
  DECLARE _netip_id BIGINT UNSIGNED;
  DECLARE _playerkeyhash_id BIGINT UNSIGNED;
  DECLARE _playeraddress_id BIGINT UNSIGNED;
  DECLARE _pbguid_id BIGINT UNSIGNED;
  DECLARE _serverplayer_server_id INT UNSIGNED;

  -- KEYHASH
  SELECT keyhash_id FROM serenity_keyhash
    WHERE keyhash_type = _server_type AND keyhash_hash = _player_hash
    INTO _keyhash_id;
  IF _keyhash_id IS NULL THEN
    INSERT INTO serenity_keyhash (keyhash_id, keyhash_type, keyhash_hash)
      VALUES (0, _server_type, _player_hash);
    SET _keyhash_id := LAST_INSERT_ID();
    -- select 'added keyhash';
  END IF;

  -- PBGUID
  IF NOT _player_guid IS NULL THEN
    SELECT pbguid_id FROM serenity_pbguid
      WHERE pbguid_type = _server_type AND pbguid_guid = _player_guid
      INTO _pbguid_id;
    IF _pbguid_id IS NULL THEN
      INSERT INTO serenity_pbguid (pbguid_id, pbguid_type,
        pbguid_guid, pbguid_keyhash_id)
        VALUES (0, _server_type, _player_guid, _keyhash_id);
    END IF;
  END IF;
      
  -- IP ADDRESS
  SELECT address_id FROM serenity_address
      WHERE address_netip = _player_netip
      INTO _netip_id;
  IF _netip_id IS NULL THEN
    INSERT INTO serenity_address (address_id, address_netip, address_domain)
      VALUES (0, _player_netip, '');
    SET _netip_id := LAST_INSERT_ID();
    -- select 'added ip';
  END IF;

  -- PLAYER
  SELECT player_id FROM serenity_player AS pl
      INNER JOIN (serenity_server AS sv) ON
      pl.player_server_id = sv.server_id AND
      pl.player_name = _player_name AND
      sv.server_type = _server_type
      INTO _player_id;
  IF _player_id IS NULL THEN
    INSERT INTO serenity_player (player_id, player_server_id, player_name,
      player_pid, player_score, player_deaths, player_team, player_ping,
      player_lastseen)
      VALUES (0, _server_id, _player_name, _player_pid, _player_score,
      _player_deaths, _player_team, _player_ping, CURRENT_TIMESTAMP());
    SET _player_id := LAST_INSERT_ID();
    -- select 'added player';
  ELSE
    UPDATE serenity_player SET player_server_id = _server_id,
      player_score = _player_score, player_deaths = _player_deaths,
      player_team = _player_team, player_ping = _player_ping,
      player_lastseen = CURRENT_TIMESTAMP()
      WHERE player_id = _player_id;
  END IF;

  -- KEYHASH -> PLAYER
  SELECT playerkeyhash_id FROM serenity_playerkeyhash
      WHERE playerkeyhash_keyhash_id = _keyhash_id AND
      playerkeyhash_player_id = _player_id
      INTO _playerkeyhash_id;
  IF _playerkeyhash_id IS NULL THEN
    INSERT INTO serenity_playerkeyhash (playerkeyhash_id,
      playerkeyhash_player_id, playerkeyhash_keyhash_id,
      playerkeyhash_lastseen)
      VALUES (0, _player_id, _keyhash_id, CURRENT_TIMESTAMP());
    -- select 'added keyhash -> player';
  ELSE
    UPDATE serenity_playerkeyhash SET
      playerkeyhash_lastseen = CURRENT_TIMESTAMP()
      WHERE playerkeyhash_id = _playerkeyhash_id;
  END IF;

  -- IP -> PLAYER
  SELECT playeraddress_id FROM serenity_playeraddress
      WHERE playeraddress_address_id = _netip_id AND
      playeraddress_player_id = _player_id
      INTO _playeraddress_id;
  IF _playeraddress_id IS NULL THEN
    INSERT INTO serenity_playeraddress (playeraddress_id,
      playeraddress_player_id, playeraddress_address_id,
      playeraddress_lastseen)
      VALUES (0, _player_id, _netip_id, CURRENT_TIMESTAMP());
    -- select 'added ip -> player';
  ELSE
    UPDATE serenity_playeraddress SET
      playeraddress_lastseen = CURRENT_TIMESTAMP()
      WHERE playeraddress_id = _playeraddress_id;
  END IF;

  -- PLAYER ON SERVER
  SELECT serverplayer_server_id FROM serenity_serverplayer
      WHERE serverplayer_server_id = _server_id AND
      serverplayer_player_id = _player_id
      INTO _serverplayer_server_id;
  IF _serverplayer_server_id IS NULL THEN
    INSERT INTO serenity_serverplayer (serverplayer_server_id,
      serverplayer_player_id)
      VALUES (_server_id, _player_id);
  END IF;
END;
//

