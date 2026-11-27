/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "tc_catch2.h"

#include "../../src/modules/PlayerbotV2/Fleet/Gathering.h"

#include <cstring>
#include <set>

using namespace Playerbot::V2::Gathering;

TEST_CASE("Fill replaces the talkers' names, city and zone", "[ClassicGathering]")
{
    CHECK(Fill("{0} met {1} in {city}", "Stormwind", "Elwynn Forest", { "Ada", "Borin" }) == "Ada met Borin in Stormwind");
    CHECK(Fill("{0} and {2} walk {zone}", "Stormwind", "Elwynn Forest", { "Ada" }) == "Ada and friend walk Elwynn Forest");
}

TEST_CASE("Fill replaces the rp placeholders too", "[ClassicGathering]")
{
    FillCtx ctx;
    ctx.city = "Stormwind";
    ctx.zone = "Elwynn Forest";
    ctx.names = { "Ada", "Borin", "Celil" };
    ctx.races = { "human", "dwarf", "night elf" };
    ctx.homes = { "Stormwind", "Ironforge", "Darnassus" };
    ctx.weather = "rain";
    ctx.race = "dwarf";
    ctx.home = "Ironforge";
    ctx.enemy = "the Horde";
    ctx.leader = "Magni";
    CHECK(Fill("{0} of {home} met {1} the {r1} in {city}", ctx) == "Ada of Ironforge met Borin the dwarf in Stormwind");
    CHECK(Fill("{enemy} raiders in {zone}, this {weather}", ctx) == "the Horde raiders in Elwynn Forest, this rain");
    CHECK(Fill("as {leader} wills, far from {h2}", ctx) == "as Magni wills, far from Darnassus");
    CHECK(Fill("a {race} and his {home} pride", ctx) == "a dwarf and his Ironforge pride");
}

TEST_CASE("Every rp placeholder resolves and every gate is sane", "[ClassicGathering]")
{
    static std::set<std::string> const allowed = { "city", "zone", "weather", "race", "home", "enemy", "leader",
                                                   "0", "1", "2", "r0", "r1", "r2", "h0", "h1", "h2" };
    FillCtx full;
    full.city = "Stormwind";
    full.zone = "Elwynn Forest";
    full.names = { "Ada", "Borin", "Celil" };
    full.races = { "human", "dwarf", "night elf" };
    full.homes = { "Stormwind", "Ironforge", "Darnassus" };
    full.weather = "rain";
    full.race = "dwarf";
    full.home = "Ironforge";
    full.enemy = "the Horde";
    full.leader = "Magni";
    for (Script const& s : RpScripts())
    {
        CHECK(s.faction <= 2);
        CHECK(s.talkers >= 2);
        CHECK(s.talkers <= 3);
        CHECK(s.weather <= kWeatherBad);
        for (Line const& ln : s.lines)
        {
            CHECK(ln.who < s.talkers);
            REQUIRE(ln.text[0]);
            CHECK(Fill(ln.text, full).find('{') == std::string::npos);
            for (char const* p = std::strchr(ln.text, '{'); p; p = std::strchr(p + 1, '{'))
            {
                char const* e = std::strchr(p + 1, '}');
                REQUIRE(e != nullptr);
                CHECK(allowed.count(std::string(p + 1, e)) == 1);
            }
        }
    }
    // The stock table keeps its plain shape (no gates).
    for (Script const& s : Scripts())
    {
        CHECK(s.weather == kWeatherAny);
        CHECK(s.race == 0);
        CHECK_FALSE(s.night);
    }
}

TEST_CASE("RpScriptsFor picks by faction and talker count", "[ClassicGathering]")
{
    TalkEnv env;   // clear day, no races of note
    for (std::size_t i : RpScriptsFor(true, 3, false, env))
    {
        Script const& s = RpScripts()[i];
        CHECK(s.faction != 2);
        CHECK(s.talkers <= 3);
        CHECK(s.weather == kWeatherAny);
        CHECK_FALSE(s.night);
        CHECK(s.race == 0);
    }
    for (std::size_t i : RpScriptsFor(false, 3, false, env))
        CHECK(RpScripts()[i].faction != 1);
    // two talkers never get a three-talker exchange
    for (std::size_t i : RpScriptsFor(true, 2, false, env))
        CHECK(RpScripts()[i].talkers <= 2);
}

TEST_CASE("RpScriptsFor's weather, night and race gates open only when the ring fits", "[ClassicGathering]")
{
    TalkEnv env;
    env.weather = kWeatherRain;
    env.night = true;
    env.race_mask = (1u << kRaceDwarf) | (1u << kRaceNightElf);
    std::set<std::uint8_t> weathers, races;
    bool night_seen = false;
    for (std::size_t i : RpScriptsFor(true, 3, false, env))
    {
        Script const& s = RpScripts()[i];
        weathers.insert(s.weather);
        if (s.race) races.insert(s.race);
        night_seen = night_seen || s.night;
    }
    CHECK(weathers.count(kWeatherRain));
    CHECK(weathers.count(kWeatherBad));          // rain counts as "bad weather"
    CHECK(night_seen);
    CHECK(races.count(kRaceDwarf));
    CHECK(races.count(kRaceNightElf));
    // a snow script still doesn't fit a rainy day, and a tauren script doesn't fit a dwarf+elf ring
    for (std::size_t i : RpScriptsFor(true, 3, false, env))
    {
        Script const& s = RpScripts()[i];
        CHECK(s.weather != kWeatherSnow);
        CHECK(s.weather != kWeatherSand);
        CHECK(s.race != kRaceTauren);
    }
    // snow-gated lines appear under snow
    env.weather = kWeatherSnow;
    bool snow_seen = false;
    for (std::size_t i : RpScriptsFor(true, 3, false, env))
        snow_seen = snow_seen || RpScripts()[i].weather == kWeatherSnow;
    CHECK(snow_seen);
}

TEST_CASE("Street chats never get outdoor rp lines", "[ClassicGathering]")
{
    TalkEnv env;
    env.weather = kWeatherRain;
    env.night = true;
    env.race_mask = 0xFFFFFFFFu;
    for (std::size_t i : RpScriptsFor(true, 4, true, env))
        CHECK_FALSE(OutdoorScript(RpScripts()[i]));
}

TEST_CASE("Rp topics cover both sides' politics", "[ClassicGathering]")
{
    for (std::size_t i = 0; i < RpTopicCount(true); ++i)
        CHECK(RpTopic(true, i)[0]);
    for (std::size_t i = 0; i < RpTopicCount(false); ++i)
        CHECK(RpTopic(false, i)[0]);
    for (std::size_t i = 0; i < RpStreetTopicCount(true); ++i)
        CHECK(RpStreetTopic(true, i)[0]);
    for (std::size_t i = 0; i < RpStreetTopicCount(false); ++i)
        CHECK(RpStreetTopic(false, i)[0]);
    // out-of-range indices wrap instead of reading past the tables
    CHECK(RpTopic(true, 9999)[0]);
    CHECK(RpStreetTopic(false, 9999)[0]);
}

TEST_CASE("Every classic race has a home and a leader", "[ClassicGathering]")
{
    for (std::uint8_t r = kRaceHuman; r <= kRaceTroll; ++r)
    {
        CHECK(RaceHome(r)[0]);
        CHECK(RaceLeader(r)[0]);
    }
    CHECK(std::string(RaceHome(kRaceDwarf)) == "Ironforge");
    CHECK(std::string(RaceLeader(kRaceUndead)) == "the Dark Lady");
}
