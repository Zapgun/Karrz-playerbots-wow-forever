-- 0075_world_westfall_sellsword.sql
-- WORLD database. Apply with: mysql ... <your world db> < this file
--
-- Adds the missing Westfall Sellsword Captain — the Alliance-side hire
-- network (entries 6110000..6110029, script npc_fb_hire_broker) covers
-- Elwynn/Dun Morogh/Loch Modan/Redridge/Duskwood/Hillsbrad/STW/SW but
-- skips Westfall entirely.
--
--   Murgen Flintlock (entry 6110030), inside the Sentinel Hill tower
--   where the People's Militia officers stand:
--     map 0  x -10509.899  y 1026.5471  z 60.5227  orientation 0.527376
--   (exact player .gps, captured in place)
--
-- Template is a straight clone of entry 6110000 (Brannock Ironhale,
-- Goldshire) — same faction 12, npcflag 1, broker script, flags_extra 2,
-- dwarf model 1598. No npc_vendor rows: the shop is driven by
-- npc_fb_hire_broker itself.
--
-- creature guid convention for this series: entry * 10 (6110000 -> 61100000).
--
-- Idempotent (INSERT ... SELECT only creates the rows once; guarded by
-- the DELETE at the top). New spawns load at map/cell load — restart
-- worldserver (or reload the cell) to see him.
--
-- Undo: run sql-changes/undo/undo_0075_world_westfall_sellsword.sql

DELETE FROM creature WHERE guid = 61100300;
DELETE FROM creature_template_gossip WHERE CreatureID = 6110030;
DELETE FROM creature_template_model WHERE CreatureID = 6110030;
DELETE FROM creature_template WHERE entry = 6110030;

INSERT INTO creature_template
    (entry, KillCredit1, KillCredit2, name, femaleName, subname, TitleAlt,
     IconName, RequiredExpansion, VignetteID, faction, npcflag, speed_walk,
     speed_run, scale, Classification, dmgschool, BaseAttackTime,
     RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags,
     unit_flags2, unit_flags3, family, trainer_class, type, VehicleId,
     AIName, MovementType, ExperienceModifier, RacialLeader, movementId,
     WidgetSetID, WidgetSetUnitConditionID, RegenHealth,
     CreatureImmunitiesId, flags_extra, ScriptName, StringId, VerifiedBuild)
SELECT 6110030, KillCredit1, KillCredit2, 'Murgen Flintlock', femaleName,
     subname, TitleAlt, IconName, RequiredExpansion, VignetteID, faction,
     npcflag, speed_walk, speed_run, scale, Classification, dmgschool,
     BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance,
     unit_class, unit_flags, unit_flags2, unit_flags3, family,
     trainer_class, type, VehicleId, AIName, MovementType,
     ExperienceModifier, RacialLeader, movementId, WidgetSetID,
     WidgetSetUnitConditionID, RegenHealth, CreatureImmunitiesId,
     flags_extra, ScriptName, StringId, VerifiedBuild
FROM creature_template WHERE entry = 6110000;

INSERT INTO creature_template_model
    (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
VALUES (6110030, 0, 1598, 1, 1, 0);

-- Without a gossip row the client is told the NPC has no gossip and strips the
-- talk cursor (same fix as the 39 brokers in
-- sql/custom/world/2026_11_01_20_world_forever_hire_broker_gossip.sql).
-- Menu 6110000 is the shared broker menu; npc_fb_hire_broker builds the options.
INSERT INTO creature_template_gossip
    (CreatureID, MenuID, VerifiedBuild)
VALUES (6110030, 6110000, 0);

INSERT INTO creature
    (guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags,
     PhaseId, PhaseGroup, terrainSwapMap, modelid, equipment_id,
     position_x, position_y, position_z, orientation, spawntimesecs,
     wander_distance, currentwaypoint, curHealthPct, MovementType,
     npcflag, unit_flags, unit_flags2, unit_flags3, ScriptName, StringId,
     VerifiedBuild)
VALUES
    (61100300, 6110030, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0,
     -10509.899, 1026.5471, 60.5227, 0.527376, 300,
     0, 0, 100, 0,
     NULL, NULL, NULL, NULL, '', NULL, 0);
