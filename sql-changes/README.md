# Local SQL changes

Fixes made to this server's databases, kept here so they can be applied again after a reinstall, a database rebuild or a move to another machine.

- Apply in file-name order (`0001_`, `0002_`, ...). The second word of each name is the database (`world`, `characters`, ...).
- Every file backs up the rows it changes into `bak_<number>_*` tables first and has an undo line at the end.
- Files are safe to run more than once.
- Restart worldserver afterwards; spawn changes take effect on load.

Apply one file (Git Bash, server's bundled MySQL on port 3307):

    /f/Games/ForeverBots/mysql/bin/mysql.exe -h127.0.0.1 -P3307 -u<user> -p world < sql-changes/0001_world_misty_pine_ice_elementals.sql

| File | What it does |
|---|---|
| 0001_world_misty_pine_ice_elementals.sql | Misty Pine Refuge: keep the 9 Minor Ice Elementals on the ice (90 s respawn) and 1 Avala; removes 37 grid-placed spawns |
| 0002_world_remove_exact_stack_duplicates.sql | Removes 91 open-world spawns stacked exactly on another spawn of the same creature |
| 0003_world_remove_anocore_wendigo_copies.sql | Dun Morogh: removes 15 wendigo copies the AnoCore restore stacked next to the original Grizzled Den / Shimmer Ridge camps |
| 0004_world_wendigo_camp_elder_replacements.sql | Dun Morogh camp at 42, 48: removes the Wendigo next to each of the 12 Elder Wendigos the quest-data restore added on the same spots, and the misplaced Wendigo Shaman |
| 0005_world_remove_rooftop_elder_wendigo.sql | Dun Morogh camp at 42, 48: removes Elder Wendigo 27153003, which stands on a roof piece |
| 0006_world_finding_warmth_fallen_logs.sql | Finding Warmth: Fallen Logs give 1-3 Logs, one log per spot (38 -> 24), 120 s respawn |
| 0007_world_trim_restore_doubles.sql | Removes 78 AnoCore/quest-data restore spawns doubling existing camps in 10 areas, and a doubled Stormwind Guard at Westbrook Garrison |
| 0008_world_deeprun_tram_car_directions.sql | Deeprun Tram: swaps 4 tram car spawn entries so each track's consist moves the right direction (4 cars previously ran off the map edge) |
| 0009_world_riverpaw_doubles.sql | Elwynn Riverpaws: removes the AnoCore camp overlay at Hogger Hill + the southeast camp (27 pools, 58 spawns) and a baseline pool stacked on another at the same point |
| 0010_world_frostmane_hold_objective.sql | Quest 287 "Frostmane Hold": adds the missing display-only areatrigger objective (ObjectID -1) — without it the explore line is invisible, the panel says "complete", and Senir refuses turn-in |
| 0011_world_shimmerweed_basket_loot.sql | Quest 315 "The Perfect Stout": Shimmerweed Baskets roll 1-2 Shimmerweed instead of always 1 |
| 0012_world_elder_wendigo_skinning.sql | Elder Wendigo + Wendigo Shaman get SkinLootID and a skinning table copied from Wendigo 1135 — they had none, so they weren't skinnable |
| 0013_hotfixes_frosthowl_footwarmers_sellprice.sql | HOTFIXES db: item 281322 Frosthowl's Footwarmers gets SellPrice 42 — all ~2.8k custom items have SellPrice 0 so the server refuses to buy them |
| 0014_hotfixes_custom_item_sellprices.sql | HOTFIXES db: 247 custom items (ID>=270000) get their formula sell price — the exact copper the client tooltip already shows (ItemPriceBase * quality * slot modifier * variance/4). Skips OVERRIDE_GOLD_COST items (authored unsellables), non-equippable tokens, tabards, and dev/DNT items |
| 0015_world_treacherous_cold_poi.sql | Quest 99162 "Treacherous Cold": adds quest_poi/quest_poi_points (markers at each rifle spawn + the quest giver) and SEQUENCED flags on objectives 1-2 — the quest had no map markers and all 3 rifle steps showed at once |
| 0016_world_turnin_poi_markers.sql | Quests 95212 "Never Saddle on Quality" and 95213/95214 "Stolen Blasting Powder": adds the missing quest_poi ObjectiveIndex=-1 hand-in blobs + points at the turn-in NPCs (Veron Amberstill / Quarrymaster Thesten) — the "?" icons never showed on the world map |
| 0017_world_turnin_poi_elwynn_forest.sql | Turn-in ? markers: 38 quests, filed by starting zone elwynn forest (UiMapID 1429) - generated; check GUESS rows |
| 0018_world_turnin_poi_uimap_84.sql | Turn-in ? markers: 74 quests, filed by starting zone uimap 84 (UiMapID 84) - generated; check GUESS rows |
| 0019_world_turnin_poi_uimap_1.sql | Turn-in ? markers: 3 quests, filed by starting zone uimap 1 (UiMapID 1) - generated; check GUESS rows |
| 0020_world_turnin_poi_mulgore.sql | Turn-in ? markers: 29 quests, filed by starting zone mulgore (UiMapID 1412) - generated; check GUESS rows |
| 0021_world_turnin_poi_dun_morogh.sql | Turn-in ? markers: 26 quests, filed by starting zone dun morogh (UiMapID 1426) - generated; check GUESS rows |
| 0022_world_turnin_poi_swamp_of_sorrows.sql | Turn-in ? markers: 26 quests, filed by starting zone swamp of sorrows (UiMapID 1435) - generated; check GUESS rows |
| 0023_world_turnin_poi_the_barrens.sql | Turn-in ? markers: 18 quests, filed by starting zone the barrens (UiMapID 1443) - generated; check GUESS rows |
| 0024_world_turnin_poi_uimap_198.sql | Turn-in ? markers: 18 quests, filed by starting zone uimap 198 (UiMapID 198) - generated; check GUESS rows |
| 0025_world_turnin_poi_stormwind_city.sql | Turn-in ? markers: 3 quests, filed by starting zone stormwind city (UiMapID 1453) - generated; check GUESS rows |
| 0026_world_turnin_poi_tirisfal_glades.sql | Turn-in ? markers: 16 quests, filed by starting zone tirisfal glades (UiMapID 1420) - generated; check GUESS rows |
| 0027_world_turnin_poi_uimap_2521.sql | Turn-in ? markers: 15 quests, filed by starting zone uimap 2521 (UiMapID 2521) - generated; check GUESS rows |
| 0028_world_turnin_poi_stranglethorn_vale.sql | Turn-in ? markers: 15 quests, filed by starting zone stranglethorn vale (UiMapID 1434) - generated; check GUESS rows |
| 0029_world_turnin_poi_orgrimmar.sql | Turn-in ? markers: 5 quests, filed by starting zone orgrimmar (UiMapID 1454) - generated; check GUESS rows |
| 0030_world_turnin_poi_dustwallow_marsh.sql | Turn-in ? markers: 14 quests, filed by starting zone dustwallow marsh (UiMapID 1445) - generated; check GUESS rows |
| 0031_world_turnin_poi_badlands.sql | Turn-in ? markers: 11 quests, filed by starting zone badlands (UiMapID 1418) - generated; check GUESS rows |
| 0032_world_turnin_poi_moonglade.sql | Turn-in ? markers: 12 quests, filed by starting zone moonglade (UiMapID 1450) - generated; check GUESS rows |
| 0033_world_turnin_poi_redridge_mountains.sql | Turn-in ? markers: 11 quests, filed by starting zone redridge mountains (UiMapID 1433) - generated; check GUESS rows |
| 0034_world_turnin_poi_durotar.sql | Turn-in ? markers: 12 quests, filed by starting zone durotar (UiMapID 1411) - generated; check GUESS rows |
| 0035_world_turnin_poi_azshara.sql | Turn-in ? markers: 11 quests, filed by starting zone azshara (UiMapID 1447) - generated; check GUESS rows |
| 0036_world_turnin_poi_wetlands.sql | Turn-in ? markers: 11 quests, filed by starting zone wetlands (UiMapID 1437) - generated; check GUESS rows |
| 0037_world_turnin_poi_the_barrens.sql | Turn-in ? markers: 11 quests, filed by starting zone the barrens (UiMapID 1413) - generated; check GUESS rows |
| 0038_world_turnin_poi_arathi_highlands.sql | Turn-in ? markers: 10 quests, filed by starting zone arathi highlands (UiMapID 1417) - generated; check GUESS rows |
| 0039_world_turnin_poi_blasted_lands.sql | Turn-in ? markers: 10 quests, filed by starting zone blasted lands (UiMapID 1419) - generated; check GUESS rows |
| 0040_world_turnin_poi_uimap_69.sql | Turn-in ? markers: 9 quests, filed by starting zone uimap 69 (UiMapID 69) - generated; check GUESS rows |
| 0041_world_turnin_poi_ashenvale.sql | Turn-in ? markers: 9 quests, filed by starting zone ashenvale (UiMapID 1440) - generated; check GUESS rows |
| 0042_world_turnin_poi_hillsbrad_foothills.sql | Turn-in ? markers: 8 quests, filed by starting zone hillsbrad foothills (UiMapID 1424) - generated; check GUESS rows |
| 0043_world_turnin_poi_tanaris.sql | Turn-in ? markers: 8 quests, filed by starting zone tanaris (UiMapID 1446) - generated; check GUESS rows |
| 0044_world_turnin_poi_loch_modan.sql | Turn-in ? markers: 6 quests, filed by starting zone loch modan (UiMapID 1432) - generated; check GUESS rows |
| 0045_world_turnin_poi_alterac_mountains.sql | Turn-in ? markers: 7 quests, filed by starting zone alterac mountains (UiMapID 1416) - generated; check GUESS rows |
| 0046_world_turnin_poi_silverpine_forest.sql | Turn-in ? markers: 6 quests, filed by starting zone silverpine forest (UiMapID 1421) - generated; check GUESS rows |
| 0047_world_turnin_poi_stonetalon_mountains.sql | Turn-in ? markers: 6 quests, filed by starting zone stonetalon mountains (UiMapID 1442) - generated; check GUESS rows |
| 0048_world_turnin_poi_felwood.sql | Turn-in ? markers: 6 quests, filed by starting zone felwood (UiMapID 1448) - generated; check GUESS rows |
| 0049_world_turnin_poi_ungoro_crater.sql | Turn-in ? markers: 6 quests, filed by starting zone ungoro crater (UiMapID 1449) - generated; check GUESS rows |
| 0050_world_turnin_poi_uimap_36.sql | Turn-in ? markers: 6 quests, filed by starting zone uimap 36 (UiMapID 36) - generated; check GUESS rows |
| 0051_world_turnin_poi_teldrassil.sql | Turn-in ? markers: 6 quests, filed by starting zone teldrassil (UiMapID 1438) - generated; check GUESS rows |
| 0052_world_turnin_poi_desolace.sql | Turn-in ? markers: 5 quests, filed by starting zone desolace (UiMapID 1441) - generated; check GUESS rows |
| 0053_world_turnin_poi_searing_gorge.sql | Turn-in ? markers: 5 quests, filed by starting zone searing gorge (UiMapID 1427) - generated; check GUESS rows |
| 0054_world_turnin_poi_westfall.sql | Turn-in ? markers: 4 quests, filed by starting zone westfall (UiMapID 1436) - generated; check GUESS rows |
| 0055_world_turnin_poi_the_hinterlands.sql | Turn-in ? markers: 4 quests, filed by starting zone the hinterlands (UiMapID 1425) - generated; check GUESS rows |
| 0056_world_turnin_poi_western_plaguelands.sql | Turn-in ? markers: 4 quests, filed by starting zone western plaguelands (UiMapID 1422) - generated; check GUESS rows |
| 0057_world_turnin_poi_uimap_27.sql | Turn-in ? markers: 4 quests, filed by starting zone uimap 27 (UiMapID 27) - generated; check GUESS rows |
| 0058_world_turnin_poi_uimap_2652.sql | Turn-in ? markers: 3 quests, filed by starting zone uimap 2652 (UiMapID 2652) - generated; check GUESS rows |
| 0059_world_turnin_poi_winterspring.sql | Turn-in ? markers: 3 quests, filed by starting zone winterspring (UiMapID 1452) - generated; check GUESS rows |
| 0060_world_turnin_poi_eastern_plaguelands.sql | Turn-in ? markers: 3 quests, filed by starting zone eastern plaguelands (UiMapID 1423) - generated; check GUESS rows |
| 0061_world_turnin_poi_ironforge.sql | Turn-in ? markers: 3 quests, filed by starting zone ironforge (UiMapID 1455) - generated; check GUESS rows |
| 0062_world_turnin_poi_undercity.sql | Turn-in ? markers: 3 quests, filed by starting zone undercity (UiMapID 1458) - generated; check GUESS rows |
| 0063_world_turnin_poi_feralas.sql | Turn-in ? markers: 3 quests, filed by starting zone feralas (UiMapID 1444) - generated; check GUESS rows |
| 0064_world_turnin_poi_burning_steppes.sql | Turn-in ? markers: 2 quests, filed by starting zone burning steppes (UiMapID 1428) - generated; check GUESS rows |
| 0065_world_turnin_poi_silithus.sql | Turn-in ? markers: 2 quests, filed by starting zone silithus (UiMapID 1451) - generated; check GUESS rows |
| 0066_world_turnin_poi_duskwood.sql | Turn-in ? markers: 1 quests, filed by starting zone duskwood (UiMapID 1431) - generated; check GUESS rows |
| 0067_world_turnin_poi_uimap_23.sql | Turn-in ? markers: 1 quests, filed by starting zone uimap 23 (UiMapID 23) - generated; check GUESS rows |
| 0068_world_turnin_poi_uimap_85.sql | Turn-in ? markers: 1 quests, filed by starting zone uimap 85 (UiMapID 85) - generated; check GUESS rows |
| 0069_auth_veteran_adventurer_appearances.sql | AUTH db: grants the Veteran Adventurer bundle to battlenet account 138 — TransmogSet 5914 "Outdoor Wear" + Rucksack back piece (blocks 9715/9736) + the Loyal Companion mount (spell 1309405, Mount.db2 3130). Subset of 0070. Takes effect on relog, no restart |
| 0070_auth_skyborne_pack.sql | AUTH db: complete reusable Skyborne-pack grant — set @acct at top, run for any account. Both mounts (Prideclaw 1309208, Loyal Companion 1309405), both transmog sets (5702 Skyseer, 5914 Outdoor Wear), Rucksack, Windwell toy (280740), 4 pets (Pachimari/Panda/Zergling/Diablo). Idempotent. Tabards 280404/280729 deliberately excluded — physical items needing a mail/quest delivery path (see TODO in file). Relog only |
| 0071_world_scarecrow_immunities.sql | Scarecrow family (36/114/115/480/573): adds vanilla poison immunity — DispelTypeMask 16 (DISPEL_POISON) so Serpent Sting/poison debuffs show "Immune". SchoolMask stays 0 (nature damage lands, correct). Bleed was already immune via mask |
| 0072_world_westfall_sunken_ship_murloc.sql | Westfall Longshore: adds 1 water murloc spawn point at the sunken ship offshore of Slark's beach (~-9598,1327, z -3). Pool 9200150 picks Coastrunner/Raider, 300s respawn, 10yd swim wander. Classic has murlocs in the water at the wreck; none existed below the waterline |
| 0073/0074 | (removed — the RPG bot names shipped upstream and apply themselves as the bot migrations 0063/0064 in sql/playerbot_v2; the seed drafts stay editable in namegen/) |
| 9001_world_stormwind_kids_chase.sql | Stormwind: Donna & William stood at spawn (MovementType=2, no path). Gives Donna the vanilla 77-node chase loop (cMaNGOS creature_movement_template for entry 2533 — Trade District + canal borders of Cathedral Sq/Old Town/Dwarven District) via waypoint_path 800797210; William trails 3.5 yd behind via creature_formations |
