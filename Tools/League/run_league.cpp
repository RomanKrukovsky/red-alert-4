// Copyright (c) Red Alert 4 project.
//
// Command-line runner for the AI self-play league. Plays real headless matches
// between AI profiles and prints the balance table the production plan calls for.
//
//   ra4_league [matchesPerPairing] [maxMinutes] [baseSeed] [difficulty 0-3]
//
// Output is deterministic for a given argument set: reruns are byte-identical,
// so a diff between two league reports isolates exactly what a balance change did.
#include "RA4AI/AILeague.h"
#include "RA4Core/SimConfig.h"

#include <cstdio>
#include <cstdlib>

using namespace RA4;
using namespace RA4::AI;

int main(int argc, char** argv)
{
    LeagueConfig Config;
    Config.MatchesPerPairing = argc > 1 ? uint32_t(std::atoi(argv[1])) : 2;
    const int MaxMinutes = argc > 2 ? std::atoi(argv[2]) : 8;
    Config.MaxTicksPerMatch = kTicksPerSecond * 60 * (MaxMinutes > 0 ? MaxMinutes : 8);
    Config.BaseSeed = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 20260805ULL;
    if (argc > 4)
    {
        const int D = std::atoi(argv[4]);
        if (D >= 0 && D <= int(AIDifficulty::Expert))
        {
            Config.Difficulty = AIDifficulty(D);
        }
    }

    std::printf("RA4 self-play league: 8 profiles, %u match(es) per ordered pairing,\n"
                "max %d min per match, base seed %llu, difficulty %s\n\n",
                Config.MatchesPerPairing, MaxMinutes,
                static_cast<unsigned long long>(Config.BaseSeed), ToString(Config.Difficulty));

    const LeagueResult Result = AILeague::RunRoundRobin(Config);

    std::printf("%s\n", Result.FormatTable().c_str());

    // Per-profile ladder: wins as either player, over decisive games.
    const AIProfile All[] = {AIProfile::Balanced,       AIProfile::Aggressive,
                             AIProfile::Defensive,      AIProfile::Economic,
                             AIProfile::Rush,           AIProfile::Turtle,
                             AIProfile::AirSuperiority, AIProfile::Guerrilla};
    std::printf("ladder (decisive games only):\n");
    for (AIProfile P : All)
    {
        uint32_t Wins = 0, Games = 0;
        for (const LeagueMatchRecord& R : Result.Matches)
        {
            if (R.bTimedOut || R.Winner == kInvalidPlayer) { continue; }
            if (R.Profiles[0] == P) { ++Games; if (R.Winner == 0) { ++Wins; } }
            if (R.Profiles[1] == P) { ++Games; if (R.Winner == 1) { ++Wins; } }
        }
        std::printf("  %-16s %3u/%3u  (%d%%)\n", ToString(P), Wins, Games,
                    Games ? int((uint64_t(Wins) * 100) / Games) : 50);
    }

    uint32_t TimedOut = 0;
    for (const LeagueMatchRecord& R : Result.Matches) { if (R.bTimedOut) { ++TimedOut; } }
    std::printf("\n%u matches total, %u timed out (draw)\n",
                Result.TotalMatches(), TimedOut);
    return 0;
}
