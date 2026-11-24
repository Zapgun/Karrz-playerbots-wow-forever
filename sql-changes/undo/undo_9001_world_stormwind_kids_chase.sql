DELETE FROM `waypoint_path_node` WHERE `PathId`=800797210;
DELETE FROM `waypoint_path` WHERE `PathId`=800797210;
DELETE FROM `creature_addon` WHERE `guid`=20079721;
DELETE FROM `creature_formations` WHERE `memberGUID`=20079720;
UPDATE `creature` SET `MovementType`=2 WHERE `guid`=20079720;
