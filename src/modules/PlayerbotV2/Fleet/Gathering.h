// Gathering - small groups of world bots hanging out just outside a capital's gate while a real player is near: one ring sits around
// a campfire, one stands together, and they talk to each other in /say now and then, the way the crowd outside Orgrimmar, Ironforge and
// Stormwind looked on a vanilla realm. Pure part: which cities, the layout, the talk scripts and their pacing, the board the idle rule
// reads. The driver is WorldBotSpawner::DriveGatherings (it uses the gate GateDuels found); the rule is idle:gathering
// (Bot/States/Rules/AmbientRules.cpp). With Playerbot.ChatModel on, the lines come from the chat model instead of the scripts
// (BotChatModel::AskSay). Tested in tests/game/ClassicGathering.cpp.
//
// TalkStyle = rp: the talk is in character (RpScripts, RP topics, an in-character model prompt): the talkers speak as people of
// Azeroth - race, homeland, the war, the zone's weather, the Defias outside Stormwind - instead of players chatting about the game.
//
// Street chats (Playerbot.World.Gathering.CityChats): inside a capital, where a real player walks, two to four world bots stand
// together by the bank, the auction house, the inn, a shop or a mailbox, turned to each other, and talk face to face in /say with
// a gesture on each line. Same driver, same talk; their ring numbers start at kStreetRing.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Playerbot::V2::Gathering
{

constexpr std::size_t   kMaxRing        = 6;         // bots in one ring
constexpr std::size_t   kMinRing        = 3;         // fewer than this is no gathering
constexpr float         kSitRadius      = 3.0f;      // yards from the fire to a seat
constexpr float         kStandRadius    = 2.4f;      // a standing group is closer
constexpr std::uint32_t kRestAfterMs    = 600000u;   // a bot that sat at a gathering does something else for a while
constexpr std::uint32_t kLineMinMs      = 6000u;     // time between two lines of one talk: never faster than this
constexpr std::uint32_t kLineMaxMs      = 14000u;

// The gate cities that get gatherings: the big three by default (GateDuels::kCities order), all six with AllCities.
inline bool BigCity(std::size_t gate_city)
{
    return gate_city == 0 || gate_city == 1 || gate_city == 3;   // Stormwind, Ironforge, Orgrimmar
}

struct Pt { float x, y; };

// Where ring `ring` (0 = the campfire, 1 = standing) of a gate lies: between the duel slots of GateDuels::LayoutFor (angles 1 + 2.1k),
// far enough out that a duel does not run through it.
inline Pt RingCenter(float gx, float gy, std::uint32_t ring)
{
    float const th = 2.05f + 2.1f * float(ring), r = 24.0f + 8.0f * float(ring);
    return { gx + std::cos(th) * r, gy + std::sin(th) * r };
}

// Seat `i` of `n` around (cx, cy): evenly round the circle, each facing the middle (the angle to face is returned in `face`).
inline Pt Seat(Pt c, std::size_t i, std::size_t n, float radius, float& face)
{
    float const a = 0.4f + 6.2831853f * float(i) / float(std::max<std::size_t>(n, 1));
    face = std::fmod(a + 3.14159265f, 6.2831853f);
    return { c.x + std::cos(a) * radius, c.y + std::sin(a) * radius };
}

// How many bots each ring gets out of `want` (the per-gate cap): the campfire first, the standing group only when it can hold kMinRing.
inline void SplitRings(std::size_t want, std::size_t& fire, std::size_t& stand)
{
    want = std::min(want, 2 * kMaxRing);
    fire = std::min(kMaxRing, (want + 1) / 2);
    stand = want - fire;
    if (stand < kMinRing) { fire = std::min(kMaxRing, fire + stand); stand = 0; }
    stand = std::min(stand, kMaxRing);
    if (fire < kMinRing) fire = 0;
}

// ---- Street chats -------------------------------------------------------------------------------------------------------------------

constexpr std::uint32_t kStreetRing      = 100u;      // ring numbers from here on are street chats inside a city
constexpr std::size_t   kStreetMin       = 2;         // two people make a chat
constexpr std::size_t   kStreetMax       = 4;
constexpr float         kStreetRadius    = 1.6f;      // closer than the gate's standing group: a few people talking
constexpr float         kStreetFromNpc   = 7.0f;      // the chat stands this far from the bank / auctioneer / mailbox, toward the player
constexpr float         kStreetApart     = 18.0f;     // two chats are at least this far apart
constexpr std::uint32_t kStreetStayMinMs = 180000u;   // a chat lasts 3 to 6 minutes, then they go about their business
constexpr std::uint32_t kStreetStayMaxMs = 360000u;

inline bool IsStreet(std::uint32_t ring) { return ring >= kStreetRing; }

// Where a chat by an NPC at (nx, ny) stands: kStreetFromNpc yards from it toward the player at (px, py) (so it is in the walkway he
// uses, not behind the counter), or along +x when he stands on the NPC.
inline Pt StreetCenter(float nx, float ny, float px, float py)
{
    float dx = px - nx, dy = py - ny;
    float const d = std::sqrt(dx * dx + dy * dy);
    if (d < 1.0f) { dx = 1.0f; dy = 0.0f; }
    else { dx /= d; dy /= d; }
    float const r = std::min(kStreetFromNpc, std::max(d - 2.0f, 3.0f));
    return { nx + dx * r, ny + dy * r };
}

// The gesture that goes with a line said face to face: a question, a laugh, a yes or a no, an exclamation, or plain talking.
constexpr std::uint16_t kGestureTalk = 1, kGestureQuestion = 6, kGestureLaugh = 11, kGestureExclaim = 5, kGestureYes = 273,
                        kGestureNo = 274, kGesturePoint = 25;
inline std::uint16_t GestureFor(std::string const& line)
{
    std::string l;
    for (char c : line) l += char(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    std::string const padded = " " + l + " ";
    auto has = [&](char const* w)
    {
        std::string const word = std::string(" ") + w;
        for (std::size_t at = padded.find(word); at != std::string::npos; at = padded.find(word, at + 1))
        {
            char const after = padded[at + word.size()];
            if (!(after >= 'a' && after <= 'z'))
                return true;
        }
        return false;
    };
    if (has("lol") || has("haha") || has("hah") || has("lmao") || has("rofl") || has("hehe") || l.find("haha") != std::string::npos)
        return kGestureLaugh;
    if (!l.empty() && l.find('?') != std::string::npos)
        return kGestureQuestion;
    if (has("no") || has("nah") || has("nope") || has("never"))
        return kGestureNo;
    if (has("yes") || has("yeah") || has("yep") || has("sure") || has("aye") || has("agreed") || has("true") || has("exactly"))
        return kGestureYes;
    if (has("look") || has("over there") || has("that one") || has("see that"))
        return kGesturePoint;
    if (l.find('!') != std::string::npos)
        return kGestureExclaim;
    return kGestureTalk;
}

// Topics for a chat in the city.
inline char const* const kStreetTopics[] = {
    "auction house prices", "what you are saving gold for", "a mount someone just bought", "where to level next",
    "a dungeon group for tonight", "the bank being too small", "which profession pays", "the city guards",
    "a rare drop someone almost got", "joining a guild", "the enemy faction raiding a town", "an annoying quest with a bad drop rate",
    "the inn's food and ale", "which class is the most fun", "a duel someone just lost", "what you did today",
};
constexpr std::size_t kStreetTopicCount = sizeof(kStreetTopics) / sizeof(kStreetTopics[0]);

// ---- Talk -------------------------------------------------------------------------------------------------------------------------
// A script is a short exchange between two or three people. A line names its speaker (0, 1, 2: the first, second, third talker of the
// ring) and may carry {city}, {zone} (the zone outside the gate), {0} {1} {2} (the talkers' names). `faction`: 0 both, 1 Alliance,
// 2 Horde. `emote`: an emote the speaker plays with the line when standing (0 none).

struct Line { std::uint8_t who; char const* text; std::uint16_t emote; };

// A script's weather gate (RpScripts only): kWeatherBad matches any real weather, the rest are the categories the driver maps the
// zone's WeatherState onto (rain, snow, sandstorm, fog).
constexpr std::uint8_t kWeatherAny = 0, kWeatherRain = 1, kWeatherSnow = 2, kWeatherSand = 3, kWeatherFog = 4, kWeatherBad = 5;

struct Script
{
    std::uint8_t faction;                     // 0 both, 1 Alliance, 2 Horde
    std::uint8_t talkers;                     // people the script needs
    std::vector<Line> lines;
    // Optional gates (RpScripts only; the stock table leaves them zero = always fits)
    std::uint8_t weather = kWeatherAny;       // a kWeather* the talk only makes sense under
    std::uint8_t race = 0;                    // a RACE_* that must be among the talkers; the driver seats it at talker slot 1
    bool night = false;                       // only after dark
};

constexpr std::uint16_t kTalk = 1, kBow = 2, kCheer = 4, kExclaim = 5, kQuestion = 6, kLaugh = 11, kPoint = 25, kYes = 273, kNo = 274;

inline std::vector<Script> const& Scripts()
{
    static std::vector<Script> const s = {
        { 0, 2, { { 0, "anyone know a good spot to grind around my level?", kQuestion },
                  { 1, "depends, what are you, mid 20s?", kTalk },
                  { 0, "26", 0 },
                  { 1, "try the camps in Stranglethorn once you hit 30, till then just quest", kTalk } } },
        { 0, 2, { { 0, "this fire is the only warm thing I've seen all week", kLaugh },
                  { 1, "you've clearly never been to Searing Gorge", kLaugh } } },
        { 0, 3, { { 0, "who's up for a duel after this?", kQuestion },
                  { 1, "not me, I just repaired", kNo },
                  { 2, "I'll fight you {0}, loser buys the next round", kPoint },
                  { 0, "deal", kYes } } },
        { 0, 2, { { 0, "how much did you get your mount for?", kQuestion },
                  { 1, "still saving, 90 gold is no joke", kTalk },
                  { 0, "the auction house is where I lose all mine", kLaugh } } },
        { 0, 3, { { 0, "heard someone saw Onyxia's lair get cleared again", kTalk },
                  { 1, "one day that'll be us", kCheer },
                  { 2, "one day I'll have a full set of blues first", kLaugh } } },
        { 0, 2, { { 0, "long day out there", kTalk },
                  { 1, "aye. sit, rest a bit, the road will still be there", kYes } } },
        { 0, 2, { { 0, "anyone need anything crafted? I've got tailoring", kQuestion },
                  { 1, "could use a few bags if you've got the cloth for it", kTalk },
                  { 0, "bring me the cloth and it's yours", kYes } } },
        { 0, 3, { { 0, "lost my corpse twice in the same cave today", kNo },
                  { 1, "which cave", kQuestion },
                  { 0, "does it matter, they're all full of kobolds", kLaugh },
                  { 2, "you will not take my candle", kLaugh } } },
        { 0, 2, { { 0, "whats your hearthstone set to?", kQuestion },
                  { 1, "here, never bothered to change it", kTalk } } },
        { 0, 2, { { 0, "fishing is so relaxing after a dungeon", kTalk },
                  { 1, "relaxing? I fished for an hour and got one boot", kLaugh } } },
        { 0, 3, { { 0, "Deadmines or Wailing Caverns tonight?", kQuestion },
                  { 1, "whichever has a healer", kLaugh },
                  { 2, "I can heal, sort of", kTalk },
                  { 0, "sort of is good enough", kYes } } },
        { 0, 2, { { 0, "the stars out here are something else", kPoint },
                  { 1, "beats staring at a wall in the inn", kYes } } },
        { 0, 2, { { 0, "what's everyone doing for the weekend", kQuestion },
                  { 1, "farming herbs, as always", kTalk },
                  { 0, "riveting", kLaugh } } },
        { 0, 3, { { 0, "anyone seen a guild recruiting around here", kQuestion },
                  { 1, "they all want you to be 60 already", kNo },
                  { 2, "start your own, we'll join", kCheer } } },
        { 0, 2, { { 0, "you smell like a gnoll", kLaugh },
                  { 1, "spent the morning in a gnoll camp, what's your excuse", kLaugh } } },
        { 0, 2, { { 0, "this stew is terrible", kNo },
                  { 1, "you made it", kPoint },
                  { 0, "and I stand by my words", kLaugh } } },
        { 0, 3, { { 0, "fun fact, I have never once found a green I could use", kTalk },
                  { 1, "of the Monkey, every time", kLaugh },
                  { 2, "I got a Bear one yesterday, on a mage", kLaugh } } },
        { 0, 2, { { 0, "is there anything good in {zone} at all", kQuestion },
                  { 1, "the view, and the road out", kLaugh } } },
        { 0, 2, { { 0, "any tips for a new player?", kQuestion },
                  { 1, "train your weapon skills and never pull more than two", kTalk },
                  { 0, "noted", kYes } } },
        { 0, 3, { { 0, "was that a world boss earlier on the road?", kExclaim },
                  { 1, "just a level 62 elite wandering about", kTalk },
                  { 2, "just, he says", kLaugh } } },
        { 0, 2, { { 0, "{1} did you finish that quest chain yet", kQuestion },
                  { 1, "almost, need ten more of those feathers", kNo },
                  { 0, "the drop rate is criminal", kLaugh } } },
        { 0, 2, { { 0, "good night for a fire", kTalk },
                  { 1, "every night is a good night for a fire", kYes } } },
        // Alliance
        { 1, 2, { { 0, "for the Alliance", kCheer },
                  { 1, "and for a hot meal, ideally", kLaugh } } },
        { 1, 3, { { 0, "the guards in {city} never let me sit on the bench", kNo },
                  { 1, "you sat on it in plate", kLaugh },
                  { 2, "it was a nice bench", kTalk } } },
        { 1, 2, { { 0, "a dwarf told me the best ale is in Ironforge", kTalk },
                  { 1, "a dwarf would say that", kLaugh } } },
        { 1, 2, { { 0, "Defias spotted on the Westfall road again", kExclaim },
                  { 1, "when are they not", kNo } } },
        { 1, 3, { { 0, "Light keep you all", kBow },
                  { 1, "and you, friend", kBow },
                  { 2, "and keep the murlocs away from me", kLaugh } } },
        { 1, 2, { { 0, "anyone heading to Southshore?", kQuestion },
                  { 1, "only if there's a group, I don't trust the Hillsbrad road", kNo } } },
        // Horde
        { 2, 2, { { 0, "Lok'tar", kCheer },
                  { 1, "Lok'tar ogar", kCheer } } },
        { 2, 3, { { 0, "the Crossroads got raided again", kExclaim },
                  { 1, "every time I hearth there", kNo },
                  { 2, "we should make a raid of our own", kPoint } } },
        { 2, 2, { { 0, "good fire. reminds me of the old camps", kTalk },
                  { 1, "strength and honor", kBow } } },
        { 2, 2, { { 0, "Thrall walked by this morning, didn't even look at me", kTalk },
                  { 1, "he's busy, you're level 20", kLaugh } } },
        { 2, 3, { { 0, "anyone going to Tarren Mill?", kQuestion },
                  { 1, "only if I can bring a friend or five", kLaugh },
                  { 2, "count me in", kCheer } } },
        { 2, 2, { { 0, "the Barrens chat is at it again", kNo },
                  { 1, "where is Mankrik's wife", kLaugh } } },
    };
    return s;
}

// A script with lines that only make sense outside the gate, around a fire or under the sky.
inline bool OutdoorScript(Script const& s)
{
    static constexpr char const* kOutdoor[] = { "fire", "stew", "stars", "out here", "road", "the view", "sit, rest" };
    for (Line const& ln : s.lines)
        for (char const* w : kOutdoor)
            if (std::string_view(ln.text).find(w) != std::string_view::npos)
                return true;
    return false;
}

// The scripts a ring of `talkers` people on this side can say; `street`: none of the ones about the fire or the open road.
inline std::vector<std::size_t> ScriptsFor(bool alliance, std::size_t talkers, bool street = false)
{
    std::vector<std::size_t> out;
    auto const& all = Scripts();
    for (std::size_t i = 0; i < all.size(); ++i)
        if (all[i].talkers <= talkers && (all[i].faction == 0 || all[i].faction == (alliance ? 1u : 2u)) && !(street && OutdoorScript(all[i])))
            out.push_back(i);
    return out;
}

// A script line with its placeholders filled. `{0}` `{1}` `{2}` are the talkers' names; the rp context adds `{weather}` (the word for
// the zone's weather, empty when clear), `{race}` `{home}` `{leader}` (the speaker's), `{enemy}` (the other side, "the Alliance" /
// "the Horde") and `{r0}` `{r1}` `{r2}` `{h0}` `{h1}` `{h2}` (each talker's race word and racial capital).
struct FillCtx
{
    std::string city;
    std::string zone;
    std::vector<std::string> names;              // a talker's name per slot
    std::vector<std::string> races;              // a talker's race word per slot ("dwarf")
    std::vector<std::string> homes;              // a talker's racial capital per slot ("Ironforge")
    std::string weather;                         // the word for the zone's weather ("rain"), empty when clear
    std::string race;                            // the speaker's race word
    std::string home;                            // the speaker's racial capital
    std::string enemy;                           // "the Alliance" / "the Horde" - the side the ring is not on
    std::string leader;                          // the speaker's racial leader ("Thrall", "the Dark Lady")
};

inline std::string Fill(char const* text, FillCtx const& ctx)
{
    std::string out;
    for (char const* p = text; *p; ++p)
    {
        if (*p == '{')
        {
            char const* e = p + 1;
            while (*e && *e != '}') ++e;
            if (*e == '}')
            {
                std::string const key(p + 1, e);
                auto pick = [](std::vector<std::string> const& v, std::size_t i, char const* fallback)
                { return i < v.size() && !v[i].empty() ? v[i] : std::string(fallback); };
                if (key == "city") out += ctx.city;
                else if (key == "zone") out += ctx.zone;
                else if (key == "weather") out += ctx.weather;
                else if (key == "race") out += ctx.race;
                else if (key == "home") out += ctx.home;
                else if (key == "enemy") out += ctx.enemy;
                else if (key == "leader") out += ctx.leader;
                else if (key.size() == 1 && key[0] >= '0' && key[0] <= '2')
                    out += pick(ctx.names, std::size_t(key[0] - '0'), "friend");
                else if (key.size() == 2 && key[0] == 'r' && key[1] >= '0' && key[1] <= '2')
                    out += pick(ctx.races, std::size_t(key[1] - '0'), "traveller");
                else if (key.size() == 2 && key[0] == 'h' && key[1] >= '0' && key[1] <= '2')
                    out += pick(ctx.homes, std::size_t(key[1] - '0'), "far away");
                p = e;
                continue;
            }
        }
        out += *p;
    }
    return out;
}

inline std::string Fill(char const* text, std::string const& city, std::string const& zone, std::vector<std::string> const& names)
{
    FillCtx ctx;
    ctx.city = city;
    ctx.zone = zone;
    ctx.names = names;
    return Fill(text, ctx);
}

// The pause before the next line, from the length of the one just said: time to read it, then type a reply. In [kLineMinMs, kLineMaxMs].
inline std::uint32_t LinePause(std::size_t said_bytes, std::uint32_t jitter)
{
    std::uint32_t const ms = 4500u + std::uint32_t(said_bytes) * 80u + (jitter % 3000u);
    return std::clamp(ms, kLineMinMs, kLineMaxMs);
}

// Topics for the chat model, one per conversation.
inline char const* const kTopics[] = {
    "a dungeon run that went badly", "the price of a mount", "the best place to level around your level", "a rare drop someone almost got",
    "professions and what they are good for", "a dragon or world boss rumoured nearby", "the long road you just travelled",
    "which class is the most fun", "a duel someone just lost", "the food cooking on the fire", "the weather and the stars tonight",
    "joining a guild", "the enemy faction raiding a town", "fishing", "an annoying quest with a bad drop rate", "old war stories",
};
constexpr std::size_t kTopicCount = sizeof(kTopics) / sizeof(kTopics[0]);

// ---- Role-play talk (Playerbot.World.Gathering.TalkStyle = rp) -----------------------------------------------------------------------
// The same machinery with the bots speaking in character, as people of Azeroth instead of players talking about the game. A TalkEnv
// (what the ring actually has: the zone's weather, nightfall, the talkers' races) gates the rp scripts; the race ids are SharedDefines'
// RACE_* restated so this header stays pure.

constexpr std::uint8_t kRaceHuman = 1, kRaceOrc = 2, kRaceDwarf = 3, kRaceNightElf = 4, kRaceUndead = 5, kRaceTauren = 6,
                       kRaceGnome = 7, kRaceTroll = 8;

struct TalkEnv
{
    std::uint8_t weather = kWeatherAny;   // the zone's weather right now, a kWeather* category (never kWeatherBad)
    bool night = false;                   // the realm's clock says night (the same hours the chat bridge reports)
    std::uint32_t race_mask = 0;          // 1u << race, for each talker
};

// The racial capital a {home} / {h0}..{h2} placeholder fills in.
inline char const* RaceHome(std::uint8_t race)
{
    switch (race)
    {
        case kRaceHuman:    return "Stormwind";
        case kRaceOrc:      return "Orgrimmar";
        case kRaceDwarf:    return "Ironforge";
        case kRaceNightElf: return "Darnassus";
        case kRaceUndead:   return "the Undercity";
        case kRaceTauren:   return "Thunder Bluff";
        case kRaceGnome:    return "Gnomeregan";
        case kRaceTroll:    return "Sen'jin Village";
        default:            return "";
    }
}

// Who a {leader} placeholder names.
inline char const* RaceLeader(std::uint8_t race)
{
    switch (race)
    {
        case kRaceHuman:    return "the King";
        case kRaceOrc:      return "Thrall";
        case kRaceDwarf:    return "Magni";
        case kRaceNightElf: return "Tyrande";
        case kRaceUndead:   return "the Dark Lady";
        case kRaceTauren:   return "Cairne";
        case kRaceGnome:    return "Mekkatorque";
        case kRaceTroll:    return "Vol'jin";
        default:            return "";
    }
}

// The word a kWeather* category fills {weather} with ("" when there's nothing to talk about).
inline char const* WeatherCatWord(std::uint8_t cat)
{
    switch (cat)
    {
        case kWeatherRain: return "rain";
        case kWeatherSnow: return "snow";
        case kWeatherSand: return "sandstorm";
        case kWeatherFog:  return "fog";
        default:           return "";
    }
}

// The in-character scripts: same shape as Scripts(), plus the optional gates on Script (weather / a required race / night only).
// A race-gated script may address that talker as {1} - the driver seats a talker of the required race at slot 1. {enemy} is the side
// the ring is not on ("the Alliance" at a Horde gate, "the Horde" at an Alliance one).
inline std::vector<Script> const& RpScripts()
{
    static std::vector<Script> const s = {
        // ---- either side ----
        { 0, 2, { { 0, "the road's been kind to no one this week", kTalk },
                  { 1, "roads never are. rest while you can", kYes } } },
        { 0, 2, { { 0, "this {weather} won't let up", kNo },
                  { 1, "good for the fields, bad for the feet", kTalk },
                  { 0, "and worse for the boots", kLaugh } },
          kWeatherBad },
        { 0, 2, { { 0, "rain on the road, rain in the pack", kNo },
                  { 1, "the sky doesn't hate you. it rains on everyone", kLaugh } },
          kWeatherRain },
        { 0, 2, { { 0, "can't see the far side of the fire in this fog", kNo },
                  { 1, "can't see my own boots", kLaugh } },
          kWeatherFog },
        { 0, 2, { { 1, "snow. proper weather, like {home}", kYes },
                  { 0, "you would say that", kLaugh } },
          kWeatherSnow, kRaceDwarf },
        { 0, 2, { { 0, "the stars are out over {zone}", kPoint },
                  { 1, "first clear night in a while", kYes } },
          kWeatherAny, 0, true },
        { 0, 3, { { 0, "I marched in the last war, you know", kTalk },
                  { 1, "who didn't?", kQuestion },
                  { 0, "the young ones. lucky them", kTalk } } },
        { 0, 2, { { 0, "they say a dragon nests in the steppes again", kTalk },
                  { 1, "they say that every winter", kLaugh },
                  { 0, "one winter they'll be right", kPoint } } },
        { 0, 3, { { 0, "haunch of boar, stale bread, water that tastes of the skin", kNo },
                  { 1, "the road's menu", kTalk },
                  { 2, "I'd kill for a tavern table", kLaugh } } },
        { 0, 2, { { 0, "grain's dear this season", kTalk },
                  { 1, "war's not good for prices", kNo },
                  { 0, "war's good for selling swords", kYes } } },
        { 0, 2, { { 0, "my leg aches whenever {weather} rolls in", kTalk },
                  { 1, "your leg predicts the sky now", kLaugh },
                  { 0, "better than the town crier", kYes } },
          kWeatherBad },
        { 0, 2, { { 0, "the quarry pays in silver and dust", kTalk },
                  { 1, "and the dust's the part that keeps", kLaugh } } },
        { 0, 2, { { 0, "{enemy} patrols the border roads again", kExclaim },
                  { 1, "they want what we have. same as always", kNo } } },
        { 0, 2, { { 0, "you're a long way from {h1}, {1}", kTalk },
                  { 1, "I go where the work is", kTalk } } },
        // ---- Alliance ----
        { 1, 2, { { 0, "the Defias took another farm in Westfall", kExclaim },
                  { 1, "and the Guard?", kQuestion },
                  { 0, "the Guard watches the roads to {city}. the farms fend alone", kNo } } },
        { 1, 3, { { 0, "my uncle rebuilt {city} after the war. still waiting on his pay", kTalk },
                  { 1, "the Stonemasons' grievance. no wonder some turned bandit", kNo },
                  { 0, "turned Defias, you mean", kTalk } } },
        { 1, 3, { { 0, "any word of the King?", kQuestion },
                  { 1, "still missing. Fordragon holds the throne for the boy", kTalk },
                  { 2, "and the nobles hold Fordragon", kNo } } },
        { 1, 2, { { 0, "they say the Brotherhood answers to someone inside the Keep", kTalk },
                  { 1, "keep that whisper low. the Stockade's full of men who talked too loud", kNo } } },
        { 1, 3, { { 0, "another wagon of Westfall folk came through the gates today", kTalk },
                  { 1, "the fields fail, the bandits ride, and {city} sends prayers", kNo },
                  { 2, "prayers don't fill bowls", kTalk } } },
        { 1, 2, { { 0, "gnolls on the farms again. kobolds in the mines", kExclaim },
                  { 1, "{zone} was quieter when the watch kept the roads", kNo } } },
        { 1, 3, { { 0, "Light keep you, friends", kBow },
                  { 1, "and you", kBow },
                  { 2, "and keep my boots dry", kLaugh } } },
        { 1, 2, { { 0, "the Horde pushed at Southshore again, they say", kTalk },
                  { 1, "the border towns bleed so {city} sleeps", kNo } } },
        { 1, 2, { { 1, "this {city} ale is thin as creek water", kNo },
                  { 0, "you're spoiled, friend", kLaugh },
                  { 1, "I'm a dwarf. I'm exact", kPoint } },
          kWeatherAny, kRaceDwarf },
        { 1, 2, { { 0, "how do you bear our short years, {1}?", kQuestion },
                  { 1, "I've seen forests grow. a passing war is a season", kTalk } },
          kWeatherAny, kRaceNightElf },
        { 1, 3, { { 1, "we'll take Gnomeregan back one day", kCheer },
                  { 0, "and the troggs will simply leave, will they", kLaugh },
                  { 1, "with enough explosives, anything leaves", kPoint } },
          kWeatherAny, kRaceGnome },
        // ---- Horde ----
        { 2, 3, { { 0, "{enemy} hit the Crossroads again", kExclaim },
                  { 1, "every week now", kNo },
                  { 0, "we answer, or the Barrens is theirs", kTalk } } },
        { 2, 2, { { 0, "the humans sharpen knives in Southshore", kTalk },
                  { 1, "Tarren Mill holds. it always holds", kYes } } },
        { 2, 2, { { 0, "my father died in the camps the humans kept", kTalk },
                  { 1, "the old ones remember, so the young don't have to", kBow } } },
        { 2, 3, { { 0, "the elves still bleed the Warsong in Ashenvale", kTalk },
                  { 1, "and the Warsong still sing", kCheer },
                  { 2, "sing and bleed", kLaugh } } },
        { 2, 3, { { 0, "Thrall says we build now, not burn", kTalk },
                  { 1, "and if {enemy} burns first?", kQuestion },
                  { 0, "then we answer like always", kPoint } } },
        { 2, 3, { { 1, "the Earthmother watch your steps, friends", kBow },
                  { 0, "and yours", kBow },
                  { 1, "she watches the roads too. safer that way", kYes } },
          kWeatherAny, kRaceTauren },
        { 2, 2, { { 1, "I walked Lordaeron before the plague took it", kTalk },
                  { 0, "and now?", kQuestion },
                  { 1, "now the Dark Lady points, and I walk", kTalk } },
          kWeatherAny, kRaceUndead },
        { 2, 2, { { 1, "an axe is honest work. war was honest too", kTalk },
                  { 0, "Durotar teaches patience", kTalk },
                  { 1, "Durotar teaches thirst", kLaugh } },
          kWeatherAny, kRaceOrc },
        { 2, 2, { { 1, "the jungle calls, mon. this dust is no home", kTalk },
                  { 0, "Sen'jin's not far", kTalk },
                  { 1, "not far, not home", kNo } },
          kWeatherAny, kRaceTroll },
    };
    return s;
}

// The rp scripts a ring of `talkers` on this side can say under `env` (weather / night / the races present); `street` keeps outdoor
// lines out of the city.
inline std::vector<std::size_t> RpScriptsFor(bool alliance, std::size_t talkers, bool street, TalkEnv const& env)
{
    std::vector<std::size_t> out;
    auto const& all = RpScripts();
    for (std::size_t i = 0; i < all.size(); ++i)
    {
        Script const& s = all[i];
        if (s.talkers > talkers) continue;
        if (s.faction != 0 && s.faction != (alliance ? 1u : 2u)) continue;
        if (street && OutdoorScript(s)) continue;
        if (s.weather != kWeatherAny && s.weather != env.weather && !(s.weather == kWeatherBad && env.weather != kWeatherAny)) continue;
        if (s.night && !env.night) continue;
        if (s.race && !(env.race_mask & (std::uint32_t(1) << s.race))) continue;
        out.push_back(i);
    }
    return out;
}

// In-character topics for the chat model, one per conversation: the shared set, then the side's own.
inline char const* const kRpTopics[] = {
    "the war between the Alliance and the Horde", "a town the enemy raided not long ago", "the road you travelled to get here",
    "what you miss about home", "a rumour of dragons in the south", "the fallen kingdoms of the north",
    "work, coin and the price of armour", "the cold nights on the road", "old war stories",
    "what the guards won't talk about", "the markets and the price of bread", "strange lights over the hills at night",
};
constexpr std::size_t kRpTopicCount = sizeof(kRpTopics) / sizeof(kRpTopics[0]);
inline char const* const kRpTopicsAlliance[] = {
    "the Defias Brotherhood tightening its hold on Westfall", "the missing king and the nobles who rule Stormwind",
    "the Stonemasons who rebuilt the city and were never paid", "gnolls and kobolds pressing into Elwynn",
    "the Horde raiding across the border",
};
inline char const* const kRpTopicsHorde[] = {
    "the Alliance pushing into the Barrens", "the internment camps the old ones still speak of",
    "the elves holding Ashenvale against the Warsong", "Tarren Mill standing against Southshore",
    "the Warchief's peace and whether it can last",
};
inline std::size_t RpTopicCount(bool alliance)
{
    return kRpTopicCount + (alliance ? sizeof(kRpTopicsAlliance) : sizeof(kRpTopicsHorde)) / sizeof(char const*);
}
inline char const* RpTopic(bool alliance, std::size_t i)
{
    if (i < kRpTopicCount)
        return kRpTopics[i];
    i -= kRpTopicCount;
    return alliance ? kRpTopicsAlliance[i % (sizeof(kRpTopicsAlliance) / sizeof(char const*))]
                    : kRpTopicsHorde[i % (sizeof(kRpTopicsHorde) / sizeof(char const*))];
}
inline char const* const kRpStreetTopics[] = {
    "the market prices and what a meal costs", "the city watch and who they really serve", "rumours from the border towns",
    "the last caravan through the gates", "who owns half the shops in this city", "the war and what it costs folk",
};
constexpr std::size_t kRpStreetTopicCount = sizeof(kRpStreetTopics) / sizeof(kRpStreetTopics[0]);
inline char const* const kRpStreetTopicsAlliance[] = {
    "Westfall's refugees begging in the streets", "the King's disappearance and who really rules", "the Stockade riots",
    "the Defias moving closer to the city",
};
inline char const* const kRpStreetTopicsHorde[] = {
    "the zeppelin fares and who pockets them", "the Alliance spies they say are in the city",
    "the internment camps in living memory", "the warbands riding for the border",
};
inline std::size_t RpStreetTopicCount(bool alliance)
{
    return kRpStreetTopicCount + (alliance ? sizeof(kRpStreetTopicsAlliance) : sizeof(kRpStreetTopicsHorde)) / sizeof(char const*);
}
inline char const* RpStreetTopic(bool alliance, std::size_t i)
{
    if (i < kRpStreetTopicCount)
        return kRpStreetTopics[i];
    i -= kRpStreetTopicCount;
    return alliance ? kRpStreetTopicsAlliance[i % (sizeof(kRpStreetTopicsAlliance) / sizeof(char const*))]
                    : kRpStreetTopicsHorde[i % (sizeof(kRpStreetTopicsHorde) / sizeof(char const*))];
}

// ---- The board the idle rule reads ------------------------------------------------------------------------------------------------

struct Spot { std::uint32_t city = 0, ring = 0; float x = 0, y = 0, z = 0, face = 0; bool sit = false; };

class Board
{
public:
    static Board& Get() { static Board b; return b; }
    void Set(std::uint64_t bot, Spot const& s) { std::lock_guard<std::mutex> g(m_); by_bot_[bot] = s; }
    bool Find(std::uint64_t bot, Spot& out) const
    {
        std::lock_guard<std::mutex> g(m_);
        auto const it = by_bot_.find(bot);
        if (it == by_bot_.end()) return false;
        out = it->second;
        return true;
    }
    bool Has(std::uint64_t bot) const { std::lock_guard<std::mutex> g(m_); return by_bot_.contains(bot); }
    void Clear(std::uint64_t bot) { std::lock_guard<std::mutex> g(m_); by_bot_.erase(bot); }
    void ClearAll() { std::lock_guard<std::mutex> g(m_); by_bot_.clear(); }
private:
    mutable std::mutex m_;
    std::unordered_map<std::uint64_t, Spot> by_bot_;
};

} // namespace Playerbot::V2::Gathering
