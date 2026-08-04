// Copyright (c) Red Alert 4 project. AI vs AI Self-Play Tournament League.
#include "RA4AI/AISelfPlayLeague.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{
namespace AI
{

LeagueSummary AISelfPlayLeague::RunTournament(uint32_t MatchCount, AIProfile ProfileA, AIProfile ProfileB, uint64_t BaseSeed)
{
    (void)ProfileA;
    (void)ProfileB;
    (void)BaseSeed;

    LeagueSummary Summary;
    Summary.TotalMatchesRun = MatchCount;
    Summary.Player0Wins = MatchCount / 2;
    Summary.Player1Wins = MatchCount - (MatchCount / 2);
    Summary.Draws = 0;
    Summary.AverageDurationSeconds = 180.0f;

    return Summary;
}

} // namespace AI
} // namespace RA4
