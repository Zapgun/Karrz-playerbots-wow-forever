# namegen — bot name seed drafts

Per-race/sex name data for the V2 bot name generator. The code merged upstream
(ForeverCore PR #394) and ships in the module; the tables and the seed data made
from these drafts apply themselves at server startup as the bot migrations
0063/0064 (sql/playerbot_v2) — the sql-changes/0073+0074 manual-apply files they
replaced are gone. Drafts are small, individually runnable `INSERT IGNORE`
files so they're easy to review and hand-edit; `merge.py` concatenates them
into `merged_data.sql` here in this folder.

To ship extended or edited names: re-run merge.py, then fold the output into a
NEW fork-local 9xxx migration in sql/playerbot_v2 (never edit upstream's 0064)
— same INSERTs, with the table names written `{SHARED}.\`playerbots_race_names\``
etc. as in 0064. INSERT IGNORE makes re-seeding safe; rows already consumed by
bots are untouched either way.

## Files

| Pattern        | Target table                  | Contents |
|----------------|-------------------------------|----------|
| `fn_<race>_m.sql` / `fn_<race>_f.sql` | `playerbots_race_names` | first names for that race+sex (`race` = ChrRaces id, `gender` 0/1) |
| `sn_<race>.sql`     | `playerbots_surnames`, `playerbots_surname_parts` | whole surnames + compound halves (`half` 0=prefix, 1=suffix) |

Race ids: 1 Human, 2 Orc, 3 Dwarf, 4 Night Elf, 5 Undead, 6 Tauren,
7 Gnome, 8 Troll, 95 Skyborne-A (High Order), 96 Skyborne-H (Windshaper).

## Conventions

- First names: max 12 chars (the `name` column and `MAX_PLAYER_NAME` cap).
  Consumed uniquely like `playerbots_names` — a name serves exactly one bot.
  Keep them race-flavoured; avoid famous NPC names (Wrynn, Hellscream, ...).
- Surnames: prefix parts capitalised, suffix parts lowercase ("Iron"+"vein"
  -> "Ironvein"). Never consumed — reuse freely.
- `gender=2` = unisex row. `race=0` = any race (used sparingly).

Re-generate after edits:

    python merge.py
