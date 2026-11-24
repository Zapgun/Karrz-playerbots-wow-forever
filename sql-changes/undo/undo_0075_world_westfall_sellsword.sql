-- undo_0075_world_westfall_sellsword.sql — removes the Westfall Sellsword
-- Captain added by 0075_world_westfall_sellsword.sql (WORLD db).

DELETE FROM creature WHERE guid = 61100300;
DELETE FROM creature_template_gossip WHERE CreatureID = 6110030;
DELETE FROM creature_template_model WHERE CreatureID = 6110030;
DELETE FROM creature_template WHERE entry = 6110030;
