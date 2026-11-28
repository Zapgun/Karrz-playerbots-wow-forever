# Agent notes — Karrz playerbots (WoW Forever)

## Build / deploy

- Source of truth for bot work is this tree's `src/modules/PlayerbotV2/`; sources are
  mirrored into `F:\_dev\ForeverCore\src\modules\PlayerbotV2` before building.
- Upstream core headers (e.g. `Weather.h`) live only in the ForeverCore tree.
- `F:\_dev\ForeverCore\build-worldserver.ps1`: sync → MSBuild `worldserver.vcxproj`
  RelWithDebInfo x64 `-maxcpucount:15` → timestamped backup of the live exe → copy to
  `F:\Games\ForeverBots\server\`. Live conf overrides: `server\worldserver.conf.d\zz-9-local.conf`.
- **Never kill/restart worldserver yourself** — `WoWForeverLauncher.exe` owns its lifecycle and a
  hard kill loses unsaved player state. After deploying, ask the user to restart it via the launcher.
- This build has `BUILD_TESTING=OFF`; module pure-code checks can be verified with a
  scratch MSVC C++20 harness instead (`/std:c++20 /EHsc`).

## Future plans

### Possible migration to TortoiseBots (wait and see)

Karrz is looking at porting **TortoiseBots** (`F:\_dev\TortoiseBots`,
github.com/TortoiseBots/TortoiseBots — built for Tortoise WoW 1.18.1, ike3/cmangos lineage)
to replace PlayerbotV2. Decision deferred — revisit when the port effort firms up.

Surveyed (2026-11): TortoiseBots deliberately removed **all** autonomous bot chatter —
migration `data/sql/world/20261102113000_world.sql` deleted every `broadcast_*`,
`suggest_*`, `reply`, join/LFG/WTB text row (upstream PR #1474). No bot-to-bot
conversations, no ambient `/say`, no gatherings equivalent, no LLM/bridge hook.
Remaining speech is functional `TellPlayer` answers to commanders, plus 4 mildly
in-character `rp_missing_reagent_*` lines. Our `Playerbot.World.Gathering.TalkStyle=rp`
work (RpScripts, TalkEnv/FillCtx, RP model prompt — this tree, deployed to live) has
**no counterpart there** and would be lost in a migration.

If the migration goes ahead, plan is to re-add ambient RP talk as a **dedicated
addon/module of its own** rather than inside the bot core:

- Nuance/cadence dial — a level setting for how much chatter (e.g. off / sparse /
  normal / chatty), not just on/off.
- Candidate content to carry over: `Fleet/Gathering.h` `RpScripts()` table (33
  gated exchanges — weather, race, night, Defias/Westfall politics), `FillCtx`
  placeholders ({weather}/{race}/{home}/{leader}/{enemy}), TalkEnv construction,
  and the in-character `sys` prompt shape for the chat-model path.
- TortoiseBots already has usable plumbing: `ai_playerbot_texts` table still loaded,
  `BOT_TEXT2` placeholder substitution, race/class/team at `TellPlayer` time.

### Town-errands enhancements withdrawn (2026-11-27)

The local enhancement series on upstream's town-errands feature (craft-station
visits with tools in hand, work audio, guaranteed-occupancy fixtures,
facing-aware queueing, longer bank/AH stops, give_item/learn_spell intents)
was reverted out of this tree (commit 233dcd471e) and the ForeverCore trunk —
feature creep, structural complexity, unresolved bugs, and the same
wait-and-see reasoning as the TortoiseBots migration above. The plain upstream
errands feature remains live. The work is preserved on `archive/town-errands`
here, `zap/town-errands` in the ForeverCore fork, and the closed upstream PR 408;
upstream PR 408 was closed unmerged so Karrz never took the enhancements.

### sql-changes world-fix series is temporary (per user, 2026-11-27)

The `sql-changes/0001`-`0076` world/auth/hotfixes fixes (the series that went up
as upstream PR 396, closed unmerged) are **temporary local patches**: things were
broken, so they were fixed locally while waiting. A corrected world database is
coming from Karrz — when it lands, re-evaluate the series against it instead of
re-applying blindly. Each file's `bak_*` backup tables and undo lines are the way
out. Do not invest in re-upstreaming them.

### AGENTS.md stays untracked (user decision, 2026-11-27)

This file is deliberately kept out of version control. Do not commit it and do
not offer to — the user has decided. Treat it as local-only operational memory.
