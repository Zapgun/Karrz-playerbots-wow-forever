# RP chat plan — bots talking in character

Status: 2026-11-25 — **step 1 (SQL migration) done + item 6 (guild→bridge) implemented**:

- `sql/playerbot_v2/9003_bot_text_pools.sql` moves every hardcoded say/party/guild line
  verbatim into `playerbot_v2_bot_text`; code prefers rows, builtin pools stay as fallback.
  On PR branch `feature/bot-chat-text-pools` → fork `Zapgun/Karrz-playerbots-wow-forever`.
- Guild chat now routes to the generation paths: `ReactGuildModel` (a real player's /g
  line gets a `chat_in` `channel:"guild"` or a built-in `BotChatModel` `Channel::Guild`
  answer — named bot first, else highest-rank, same per-guild cooldown; switch:
  `Playerbot.ChatModel.Guild`, default 1) and every self-initiated `GuildLine` site +
  the join welcome go through `SayGuild`/`SayGuildLine` → `BotBridge::QueueMoment`
  `channel:"guild"` when a real guild member is online (else the canned line, as before).
  `BotBridge::Say` gained the `guild` → `GuildChatIntent` route; `Moment` carries a
  channel; `GuildMomentListener` requires a real online guild member or the moment drops.
  Keys carried are guild_* / banter_* / say_kindness_* / guild_join_welcome. IC rewriting
  of the lines and the OOC `( )` mixing are separate follow-ups (they change behavior —
  the 9003 migration deliberately does not, so it can go upstream as-is).

Numbering convention (learned the hard way): **fork-local migrations use 9xxx**
so they always sort after anything Karrz ships (upstream 0065+ can never
collide). When Karrz takes one upstream it gets renumbered into the 0xxx
sequence — happened before: our 9001/9002 rpg-names pair became upstream's
0063/0064. Applied rows survive because INSERT IGNORE is keyed on text, not
version.

Goal: say, party and guild chat read as **in-character** (IC). The occasional
out-of-character aside is fine and wanted — wrapped in `( )` brackets, RP-server
convention. Every other channel (whispers, General/Trade/LFG, raid callouts,
yell, emotes) stays as it is.

Decisions so far:

- The hardcoded C++ line pools should move into the shared bot-text SQL tables
  and be picked through the existing `BotTextMgr` path — one text system, not two.
- The Ollama bot bridge is in use on the realm, so generated lines are a wanted
  part of this (not a fallback curiosity).

## How bot speech works today

Two sources produce every line.

### Fixed pools

DB — `{SHARED}.playerbot_v2_bot_text` (+ `_chance`, `weight`), seeded by
`sql/playerbot_v2/0019_bot_texts.sql`, `0022`, `0041`, `0042`. Loaded once by
`Bot/BotTextMgr.cpp`; a line is picked per key (`hello`, `broadcast_*`, `reply`,
`suggest_*`, `emote_reply_*`, `taunt`, `loot`, `aoe`, command acks), weighted by
`weight`, `%placeholders` filled by the caller. English only — the ike3
`text_loc` columns were dropped in 0019, so moving a hardcoded pool to SQL loses
whatever ruRU translation `Util/BotLocaleText.cpp` / `BotLocaleRu.cpp` carried
for it (guild chat has a translated block there).

Hardcoded C++ pools (the ones this plan moves to SQL):

| Pool | Where | Notes |
|---|---|---|
| Guild chat, all of it | `Bot/States/Rules/GuildRules.cpp` kDing/kGreet/kSmalltalk/kRival/kBrag/kPartyChat/kRaidLines/kDungeonLines/kBgLines/kPhrases + EventAnnounceFire + lfm_pulse | Least RP content; meta lines exist ("the new patch feels good", "phase is acting weird", "rated arena later maybe", "honor cap incoming", "/join wsg") |
| Party banter | `BotTextMgr.cpp` `PickBanter` (Ready/Wipe/Thanks/Ouch/BossKill), class/role/politeness/RP sub-pools | "rdy", "ty", "gg", "wp"; already has a `Verbosity::Roleplay` tier with IC-ish lines |
| Say/yell ambient | `AmbientRules.cpp` kindness heal/rez lines (~2222/2288), `YellLfg` fallback | |
| Gatherings + street chats | `Fleet/Gathering.h` `Scripts()` | Best RP style reference already in the tree; keep as style template |
| Channel calls | `Bot/ChannelTalk.h` `LfgCallLine`/`OfferLine`/`GroupSpotLine` | Stays as-is (channel scope) |
| Conversation answers | `Bot/BotConversation.cpp` reply pools | Used by whisper, /say, party, channel answers; feeds several channels, so only some intents go IC |
| Drink / orders / guild invite | `DrinkSay.h`, `GroupOrderText.cpp`, `GuildWhisperInvite.cpp` | |
| Raid | `Raid/RaidVoice.h`, `RaidCallouts`, `RaidBrain` banter | Probably stays OOC (raid comms are OOC even on RP servers) — confirm |

### Generated lines (Ollama / bridge)

- `Playerbot.ChatModel.*` → local Ollama (`Bot/BotChatModel.cpp`); system prompt in
  `BuildSystemPrompt` says "a real player typing in game chat" — gamer register.
  `Playerbot.ChatModel.ExtraPrompt` can steer it IC **with zero code today**.
- `Playerbot.Bridge.*` → file bridge to any program (`BOT_BRIDGE.md`,
  `tools/bot_bridge_ollama.py`). Event types: `chat_in` (whisper/party/raid//say),
  `moment` (party broadcasts), `talk` (bot-to-bot /say + General/Trade posts),
  `raid_line`.
- **Guild chat has no model/bridge route at all** — `GuildRules` emits
  `GuildChatIntent` directly. Gap to close for the RP push.

### Channel → source map

| Channel | Sources |
|---|---|
| /say | Gathering/street scripts or ChatModel `AskSay`, `ReactSay` (BotConversation / ike3 `reply`), kindness lines, bridge lines |
| /yell | `YellLfg` (when `Chatter.LfgChannel=0`), `emote_reply_*` say_type 1 |
| party | `hello`/`hello_follow`, `broadcast_*` (level/loot/quest/kill), `PickBanter`, drink say, ready-check why, `React`/`ReactPartyModel` |
| raid | same via `RaidChatIntent` + raid voice/callouts |
| guild | `GuildRules.cpp` pools + `ReactGuild` + `ReactGuildJoin` welcomes |
| channels | `suggest_*` keys (`SpeechTick::Suggest`), `LfgCallLine`, recruit posts, `ChannelChat::OnPlayerLine` — **stays as-is** |
| whisper | `BotConversation`, `reply` rows, `OfferLine`/`GroupSpotLine`, WTB, guild invite — **stays as-is** |

## Design

### 1. Consolidate text into SQL

New text keys for everything currently hardcoded; the emit site calls
`BotTextMgr::Get(key, ph)` instead of the C++ array. Keep the C++ array as
compile-time fallback when the key has no rows (keeps tests + a server with an
unpatched shared DB working).

Proposed key naming — `say_*`, `party_*`, `guild_*` prefixes so scope is
visible in the table. **As shipped in 9003:**

- `guild_ding` (%my_level), `guild_login_greet`, `guild_smalltalk`, `guild_rival`,
  `guild_quest_brag`, `guild_lfm`, `guild_event_announce` (%event_label,
  %mins_until), `guild_tavern_hangout` (%venue), `guild_tavern_chat`,
  `guild_staging_raid`/`_dungeon`/`_bg`/`_generic`, `guild_babble`,
  `guild_join_welcome` (%s = joiner name)
- `banter_ready`, `banter_wipe`, `banter_thanks`, `banter_ouch`, `banter_bosskill`
  — variants as suffix keys (`banter_ready_tank`, `_healer`, `_caster`, `_rp`,
  `banter_wipe_polite`, `_rude`, ...) merged in via `BotTextMgr::PickKeys`
  (reply_type is only meaningful for the `reply` key, so separate keys it is)
- `banter_raid_wipe` (State_Dead whole-raid-wipe line)
- `say_kindness_heal`, `say_kindness_rez`
- `greet_hello`/`hello` and `goodbye` already exist
- still hardcoded, deferred: `PickReply`/`BotConversation`/`ReactGuild` pools
  (shared across whisper/say/party — needs intent-level surgery, not a blind
  move), `DrinkSay`/`ready_why` (per-bot deterministic picks don't map onto
  weighted rows), functional one-liners (pvp help callout, group-invite
  decline), `Gathering.h` scripts (style reference), channel text (out of scope)

Migration conventions to follow (`sql/playerbot_v2/`): next free number,
`-- @target: shared`, `INSERT IGNORE` against the `(name, reply_type, text(200))`
unique key (idempotent, re-runnable), no `;` inside text, schema_version row.

### 2. IC pool + OOC bracket lines

- Write IC lines into the new keys; put OOC asides in the same key with the
  literal `( ... )` already in the text, at low `weight` (~5-10% of the key's
  total weight). Weight-only is statistically right and needs no new columns.
- If we ever want a hard cap ("at most 1 OOC per N lines per bot") a per-line
  `ooc` flag column is the upgrade path — defer unless brackets feel too
  frequent in play.
- Existing DB offenders to prune/rewrite in the same migration (they're
  whispers/replies, but cheap to fix while there): meme `reply` rows
  ("fr fr no cap on a stack", "0/10 would not read again", "based", "swag",
  "dwarf moment"), gamer `broadcast_*` ("RNG is not bad today", "hunter bis").
  `DELETE ... WHERE name=.. AND text=..` like `0041`/`0042`.
- Style source: `Gathering.h` scripts are the register to copy
  ("Lok'tar ogar", "Light keep you all", "this stew is terrible / you made it").
- Personality already gates: `Verbosity::Roleplay` exists; silent/terse bots
  stay quiet; rude/polite sub-pools exist in `PickBanter`. Keep per-bot
  consistency (a rude dwarf should stay a rude dwarf).

### 3. Bridge / chat-model RP

- Short term (no code): `Playerbot.ChatModel.ExtraPrompt` with IC instructions;
  the bridge program's own style rules likewise.
- `BuildSystemPrompt` — add an IC mode (config-gated, e.g.
  `Playerbot.ChatModel.InCharacter = 1`): "answer as your character in Azeroth;
  rarely, an out-of-character aside in parentheses". Same for `talk` scenes.
- Close the guild gap (**user priority**: guild chat is the most canned channel
  and should go through the bridge for variability once the plumbing exists).
  Options:
  a. New `chat_in`/`talk` `channel` value `"guild"` — smallest change; the
     program answers like a party line but it lands in /g.
  b. New `moment` `what` values for guild triggers (ding greet, smalltalk,
     event announce) — mirrors the party-moment path exactly.
  Lean: (a) for `ReactGuild`, (b) for the self-initiated `GuildRules` lines.
  Without a program/model the SQL pool covers it anyway. Probably a follow-up
  to the 9003 upstream PR rather than part of it — bridge plumbing is a bigger
  feature than moving text rows.
- `BotBridge::QueueMoment` already intercepts party moments — reuse that seam
  for guild lines so `GuildRules` asks "SQL line or program line" in one place.

### 4. What stays out of scope

Channels (General/Trade/LFG), whispers, raid comms, yell, emote replies,
command acknowledgements — per goal. Revisit raid banter only if IC raid
chatter is wanted later.

## Work items (rough order)

1. ~~Migration `9003`: new keys + line corpus~~ **done** — verbatim, no IC
   rewrite, no DELETEs (upstream-neutral). `BotTextMgr::PickKeys` added for the
   merged variant picks.
2. ~~`GuildRules.cpp`: replace every `kXxxVariants` array with
   `BotTextMgr::Get(key, ph)`, fallback to builtin pool~~ **done** via the
   `GuildLine` helper; `ReactGuildJoin` welcome moved too. `ReactGuild` still on
   its `PickReply` pools (shared with other channels — see 5).
3. ~~`PickBanter`: `banter_*` keys with suffix variants~~ **done** —
   `PickBanter` tries `PickKeys` first, builtin pools as fallback.
4. ~~`AmbientRules` kindness `/say`, `State_Dead` raid wipe~~ **done**.
   `DrinkSay` / ready-check why-lines deferred (deterministic per-bot picks).
5. `BotConversation.cpp` + `PickReply` (`react_*` keys): identify which intents
   feed say/party/guild vs whisper/channel; move only those to SQL, IC-rewrite
   them, add `( )` OOC rows at low weight. (Shared-code surgery — the pools
   serve several channels.)
6. ~~Bridge: guild `channel` value + `ReactGuild`/`GuildRules` `moment` hooks~~ **done,
   verified in-game** — `Channel::Guild` + `GuildOn()` + `ReactGuildModel` (replies
   via chat_in/bridge or built-in Ollama, named-first then rank, shared guild
   cooldown `Playerbot.ChatModel.GuildCooldownSec`, `Guild` switch default 1); `SayGuild`/`SayGuildLine` `channel:"guild"`
   moments for every self-initiated guild line + join welcome, gated on a real
   member online; `Say` guild route + `GuildMomentListener` in BotBridge; docs in
   BOT_BRIDGE.md and conf.dist. Verified live: every guild moment + chat_in got a
   generated reply routed to /g. Polish found in test: `to`/`to_is_owner` dropped
   for guild moments (ambient — naming the one online member invited constant
   name-dropping; a directed moment names its target in `detail`), and bots may
   now answer each other's /g lines (~1/3 chance, a real member online and the
   ambient cooldown bound it — reply chains die geometrically). Known behavior:
   `MomentsPerMinute` (6, global) caps moment generation — over the cap the bot
   says its fixed line, most visible as canned greets right after a restart /
   before the bridge client reconnects (`g_moments_on` needs `client_alive`).
7. ChatModel `InCharacter` config + prompt.
8. IC rewrite pass on the 9003 keys themselves (`( )` OOC rows at ~5-10%
   weight) + `DELETE` of worst `reply`/`broadcast_*` offenders — **this** is
   the player-visible change; do it after upstream takes/looks at 9003.
9. Audit pass: play a session, log `[conv]`/`[channel]`/`[guild_whisper]`
   lines, tune `chance`/`weight`.

## Race-specific bot guilds

Worth doing; skeleton exists. Current state:

- `bot_guild_meta.theme` is a free-form slot ('adventurers'; 'crafters'/
  'raiders' planned). `member_cap` is per-guild (75 default).
- `Fleet/BotGuildNamePool.cpp` hardcodes ~30 names/faction; some already
  race-flavored ("Wildhammer Tribe", "Darkspear Rebellion", "Sentinels of
  Teldrassil").
- Founding: `Fleet/BotGuildCharter.cpp` FSM (buy charter → 4 signatures →
  turn in). Recruiting: `GuildRules.cpp` `RecruitNearby` — officers invite
  eligible guildless bots within 30 yd, biased by Social archetype;
  `PlayerbotV2.Guilds.InvitePlayers` gates inviting real players (default 0).
- Events (`Fleet/BotGuildEvent.h`): TavernParty Sat 22:00, Raid Wed+Sat,
  Dungeon Tue+Thu, BG Fri — venue only `kFactionTavern`/`kFactionStaging`
  (SW Pig & Whistle / Org Wyvern's Tail).
- Names: `0063/0064` just added `playerbots_race_names` — lore first names per
  race. A Tauren-named Tauren in a Tauren guild reads IC instantly.

Options:

- **A. Race-locked guilds** (real RP): `bot_guild_meta` gains `race_req` (or
  theme `race:<id>`); name pool gains race-keyed lists; founder election picks
  race first; `RecruitNearby` filters race; event venue table gains
  race-appropriate taverns (Ironforge, Thunder Bluff, Darnassus).
  Cap to 1-2 per faction (Dwarf + Night Elf / Orc + Tauren) — a gnome-only
  guild may never fill from the fleet's race mix. `member_cap` per-guild
  handles smaller rosters.
- **B. Race-flavored mixed guilds**: race names from the existing pool, race-flavored
  guild chat theme, no membership rule. ~80% of the feel, zero roster logic.
- Lean: A for flavor value, but start with the guild-chat/text work; do A's
  schema + name pools in the same pass, gate the recruit/founder changes
  behind a config key (`PlayerbotV2.Guilds.RaceGuilds`).

## Things to verify while implementing

- Exactly which pool `BotChatReactor::ReactGuild` (~1200-1320, 2078) and
  `ReactGuildJoin` pull from today.
- Whether `suggest_*` keys are used anywhere a player hears IC-scoped chat
  (they're channel-scoped — fine) — confirm no party reuse.
- `emote_reply_*` say_type 1 (yell) lines: keep, they're emote answers.
- ruRU: if RU players read these channels, decide whether the new SQL needs a
  locale column revival or `BotLocaleText` keeps translating the builtin
  fallback only.
- `Playerbot.Chatter.Conversation` config — what it gates today.

## File inventory

- Text data: `sql/playerbot_v2/0019_bot_texts.sql`, `0022_bot_text_weight.sql`,
  `0041_bot_text_fixes.sql`, `0042_bot_text_meta_lines.sql`
- Text engine: `Bot/BotTextMgr.{h,cpp}` (Pick/Get/RollChance, BotChatter,
  SpeechTick, PickBanter, broadcasts, greetings)
- Replies/classify: `Bot/BotChatReactor.{h,cpp}`, `Bot/PartyTalk.h`,
  `Bot/BotConversation.{h,cpp}`, `Bot/GuildAskText.h`,
  `Bot/GuildWhisperInvite.cpp`
- Emit/intents: `Bot/BotIntentEmitter.h`, `Bot/BotIntent.h`,
  `Bot/BotIntentExecutor.cpp`
- Guild chatter: `Bot/States/Rules/GuildRules.cpp`,
  `Fleet/BotGuildEventScheduler.{h,cpp}`, `Fleet/BotGuildEvent.h`
- Ambient /say: `Fleet/Gathering.h`, `Fleet/WorldBotSpawner.cpp` (DriveGatherings),
  `Bot/States/Rules/GatheringRules.cpp`, `AmbientRules.cpp`
- Channels: `Bot/ChannelTalk.h`, `Bot/ChannelChat.cpp`
- Generated: `Bot/BotChatModel.{h,cpp}`, `Bot/BotChatModelClient.{h,cpp}`,
  `Bot/BotBridge.{h,cpp}`, `Bot/BotBridgeFormat.{h,cpp}`,
  `tools/bot_bridge_ollama.py`, `BOT_BRIDGE.md`
- Guild lifecycle: `Fleet/BotGuildMgr.{h,cpp}`, `Fleet/BotGuildCharter.{h,cpp}`,
  `Fleet/BotGuildNamePool.{h,cpp}`, `sql/playerbot_v2/0006_bot_guild_meta.sql`
- Personality: `Bot/BotPersonality.{h,cpp}` (`Verbosity::Roleplay` exists)
- Locale: `Util/BotLocaleText.{h,cpp}`, `Util/BotLocaleRu.cpp`
- Config: `conf/playerbot.conf.dist` — `Playerbot.Chatter.*`,
  `Playerbot.ChatModel.*`, `Playerbot.Bridge.*`, `PlayerbotV2.Guilds.*`,
  `Playerbot.World.Gathering.*`
