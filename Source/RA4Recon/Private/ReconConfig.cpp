// Copyright (c) Red Alert 4 project.
#include "RA4Recon/ReconConfig.h"

#include "RA4Core/Checksum.h"

#include <cmath>

#include "RA4Content/JsonParser.h"

namespace RA4
{
namespace Recon
{

const DistortionProfile* ReconSettings::FindDistortionProfile(const std::string& InName) const
{
    for (const DistortionProfile& P : DistortionProfiles)
    {
        if (P.Name == InName)
        {
            return &P;
        }
    }
    return nullptr;
}

const CommsProfile* ReconSettings::FindCommsProfile(const std::string& InName) const
{
    for (const CommsProfile& P : CommsProfiles)
    {
        if (P.Name == InName)
        {
            return &P;
        }
    }
    return nullptr;
}

namespace
{

// Designers author human-readable fractions ("0.35"); simulation math wants exact
// integers. The conversion happens exactly once, here, so no double ever reaches
// sim state. Rounding is deterministic: decimal literals parse to the same IEEE
// double on every platform, and llround of that is the same integer everywhere.
int32_t ReadPerMille(const Json::Value& Parent, const char* Key, int32_t Default)
{
    const Json::Value* V = Parent.Find(Key);
    if (V == nullptr || !V->IsNumber())
    {
        return Default;
    }
    return int32_t(std::llround(V->AsNumber() * 1000.0));
}

int32_t ReadInt(const Json::Value& Parent, const char* Key, int32_t Default)
{
    const Json::Value* V = Parent.Find(Key);
    if (V == nullptr || !V->IsNumber())
    {
        return Default;
    }
    return V->AsInt();
}

bool ReadBool(const Json::Value& Parent, const char* Key, bool Default)
{
    const Json::Value* V = Parent.Find(Key);
    if (V == nullptr || !V->IsBool())
    {
        return Default;
    }
    return V->AsBool();
}

std::string ReadString(const Json::Value& Parent, const char* Key, const std::string& Default)
{
    const Json::Value* V = Parent.Find(Key);
    if (V == nullptr || !V->IsString())
    {
        return Default;
    }
    return V->AsString();
}

// Category names as authored in JSON. Order must match ObservedCategory.
const char* kCategoryNames[kObservedCategoryCount] = {
    "infantry", "light_vehicle", "heavy_vehicle", "aircraft", "ship", "structure",
};

int32_t CategoryIndexByName(const std::string& Name)
{
    for (int32_t I = 0; I < kObservedCategoryCount; ++I)
    {
        if (Name == kCategoryNames[I])
        {
            return I;
        }
    }
    return -1;
}

void ParseDistortionProfile(const Json::Value& V, DistortionProfile& Out)
{
    Out.Name = ReadString(V, "name", "");

    Out.bClarityEnabled = ReadBool(V, "clarity_enabled", Out.bClarityEnabled);
    Out.MinClarityPerMille = ReadPerMille(V, "min_clarity", Out.MinClarityPerMille);
    Out.ClarityDistanceFalloffPerMille = ReadPerMille(V, "clarity_distance_falloff", Out.ClarityDistanceFalloffPerMille);

    Out.bCountDistortionEnabled = ReadBool(V, "count_distortion_enabled", Out.bCountDistortionEnabled);
    Out.FearCountBiasMaxPerMille = ReadPerMille(V, "fear_count_bias_max", Out.FearCountBiasMaxPerMille);
    Out.CompetenceNoiseMaxPerMille = ReadPerMille(V, "competence_noise_max", Out.CompetenceNoiseMaxPerMille);

    Out.bClassificationErrorEnabled = ReadBool(V, "classification_error_enabled", Out.bClassificationErrorEnabled);

    Out.bPositionErrorEnabled = ReadBool(V, "position_error_enabled", Out.bPositionErrorEnabled);
    Out.PositionErrorMaxTiles = ReadInt(V, "position_error_max_tiles", Out.PositionErrorMaxTiles);

    Out.bOmissionEnabled = ReadBool(V, "omission_enabled", Out.bOmissionEnabled);
    Out.OmissionChanceMaxPerMille = ReadPerMille(V, "omission_chance_max", Out.OmissionChanceMaxPerMille);

    Out.bFabricationEnabled = ReadBool(V, "fabrication_enabled", Out.bFabricationEnabled);
    Out.FabricationChanceMaxPerMille = ReadPerMille(V, "fabrication_chance_max", Out.FabricationChanceMaxPerMille);
    Out.MaxPhantomLifetimeTicks = ReadInt(V, "max_phantom_lifetime_ticks", Out.MaxPhantomLifetimeTicks);

    Out.bSelfReportBiasEnabled = ReadBool(V, "self_report_bias_enabled", Out.bSelfReportBiasEnabled);
    Out.SelfReportLossUnderstatementMaxPerMille =
        ReadPerMille(V, "self_report_loss_understatement_max", Out.SelfReportLossUnderstatementMaxPerMille);
}

void ParseCommsProfile(const Json::Value& V, CommsProfile& Out)
{
    Out.Name = ReadString(V, "name", "");
    Out.OfficerBiasMaxPerMille = ReadPerMille(V, "officer_bias_max", Out.OfficerBiasMaxPerMille);

    const Json::Value* Delays = V.Find("hop_delay_seconds_by_level");
    if (Delays != nullptr && Delays->IsArray())
    {
        Out.HopDelayTicksByLevel.clear();
        for (const Json::Value& D : Delays->AsArray())
        {
            // Authored in seconds, stored in ticks: designers think in seconds,
            // the simulation thinks in ticks, and the boundary is load time.
            const int32_t Ticks = D.IsNumber() ? int32_t(std::llround(D.AsNumber() * 20.0)) : 0;
            Out.HopDelayTicksByLevel.push_back(Ticks);
        }
    }
}

bool ParseConfusionMatrix(const Json::Value& V, ConfusionMatrix& Out, std::vector<std::string>& OutErrors)
{
    const Json::Value* Rows = V.Find("rows");
    if (Rows == nullptr || !Rows->IsObject())
    {
        OutErrors.push_back("confusion_matrix: missing 'rows' object");
        return false;
    }

    for (const auto& [TrueName, RowValue] : Rows->AsObject())
    {
        const int32_t TrueIdx = CategoryIndexByName(TrueName);
        if (TrueIdx < 0)
        {
            OutErrors.push_back("confusion_matrix: unknown category '" + TrueName + "'");
            return false;
        }
        if (!RowValue.IsObject())
        {
            OutErrors.push_back("confusion_matrix: row '" + TrueName + "' is not an object");
            return false;
        }
        for (int32_t I = 0; I < kObservedCategoryCount; ++I)
        {
            Out.PerMille[TrueIdx][I] = 0;
        }
        for (const auto& [ObservedName, Cell] : RowValue.AsObject())
        {
            const int32_t ObservedIdx = CategoryIndexByName(ObservedName);
            if (ObservedIdx < 0)
            {
                OutErrors.push_back("confusion_matrix: unknown category '" + ObservedName + "' in row '" + TrueName + "'");
                return false;
            }
            if (!Cell.IsNumber())
            {
                OutErrors.push_back("confusion_matrix: non-numeric cell " + TrueName + "/" + ObservedName);
                return false;
            }
            Out.PerMille[TrueIdx][ObservedIdx] = int32_t(std::llround(Cell.AsNumber() * 1000.0));
        }
    }
    return true;
}

bool CheckPerMilleRange(int32_t Value, const char* Field, std::vector<std::string>& OutErrors)
{
    if (Value < 0 || Value > 10000) // up to 10x multipliers are legitimate tuning space
    {
        OutErrors.push_back(std::string(Field) + ": out of range [0, 10000] per-mille");
        return false;
    }
    return true;
}

} // namespace

bool ValidateReconSettings(const ReconSettings& Settings, std::vector<std::string>& OutErrors)
{
    const size_t ErrorsBefore = OutErrors.size();

    if (Settings.FindDistortionProfile(Settings.ActiveDistortionProfile) == nullptr)
    {
        OutErrors.push_back("recon_settings: active distortion profile '" + Settings.ActiveDistortionProfile +
                            "' not found");
    }
    if (Settings.FindCommsProfile(Settings.ActiveCommsProfile) == nullptr)
    {
        OutErrors.push_back("recon_settings: active comms profile '" + Settings.ActiveCommsProfile + "' not found");
    }

    for (const DistortionProfile& P : Settings.DistortionProfiles)
    {
        if (P.Name.empty())
        {
            OutErrors.push_back("distortion profile with empty name");
        }
        CheckPerMilleRange(P.MinClarityPerMille, "min_clarity", OutErrors);
        CheckPerMilleRange(P.ClarityDistanceFalloffPerMille, "clarity_distance_falloff", OutErrors);
        CheckPerMilleRange(P.FearCountBiasMaxPerMille, "fear_count_bias_max", OutErrors);
        CheckPerMilleRange(P.CompetenceNoiseMaxPerMille, "competence_noise_max", OutErrors);
        CheckPerMilleRange(P.OmissionChanceMaxPerMille, "omission_chance_max", OutErrors);
        CheckPerMilleRange(P.FabricationChanceMaxPerMille, "fabrication_chance_max", OutErrors);
        CheckPerMilleRange(P.SelfReportLossUnderstatementMaxPerMille, "self_report_loss_understatement_max", OutErrors);
        if (P.PositionErrorMaxTiles < 0)
        {
            OutErrors.push_back(P.Name + ": position_error_max_tiles is negative");
        }
        if (P.MaxPhantomLifetimeTicks <= 0)
        {
            // A phantom with no lifetime bound breaks the guaranteed-refutation
            // contract (§4.5) -- the one rule that keeps players trusting the system.
            OutErrors.push_back(P.Name + ": max_phantom_lifetime_ticks must be positive");
        }
    }

    for (const CommsProfile& P : Settings.CommsProfiles)
    {
        if (P.Name.empty())
        {
            OutErrors.push_back("comms profile with empty name");
        }
        if (P.HopDelayTicksByLevel.empty())
        {
            OutErrors.push_back(P.Name + ": hop_delay_seconds_by_level is empty");
        }
        for (int32_t Delay : P.HopDelayTicksByLevel)
        {
            if (Delay < 0)
            {
                OutErrors.push_back(P.Name + ": negative hop delay");
                break;
            }
        }
    }

    // Every confusion row must sum to exactly 1000 so sampling needs no
    // normalisation and a typo cannot silently skew classification odds.
    for (int32_t Row = 0; Row < kObservedCategoryCount; ++Row)
    {
        int32_t Sum = 0;
        for (int32_t Col = 0; Col < kObservedCategoryCount; ++Col)
        {
            const int32_t Cell = Settings.Confusion.PerMille[Row][Col];
            if (Cell < 0 || Cell > 1000)
            {
                OutErrors.push_back(std::string("confusion_matrix: cell out of [0,1000] in row ") + kCategoryNames[Row]);
            }
            Sum += Cell;
        }
        if (Sum != 1000)
        {
            OutErrors.push_back(std::string("confusion_matrix: row '") + kCategoryNames[Row] +
                                "' sums to " + std::to_string(Sum) + " per-mille, expected 1000");
        }
    }

    if (Settings.RadarRangeTiles < 0)
    {
        OutErrors.push_back("recon_settings: radar_range_tiles is negative");
    }

    const MoraleTuning& MT = Settings.Morale;
    CheckPerMilleRange(MT.DamageMoralePenaltyPerMille, "damage_morale_penalty", OutErrors);
    CheckPerMilleRange(MT.DamageSuppressionPerMille, "damage_suppression", OutErrors);
    CheckPerMilleRange(MT.AllyDeathMoralePenaltyPerMille, "ally_death_morale_penalty", OutErrors);
    CheckPerMilleRange(MT.SuperiorityMoralePenaltyPerTickPerMille, "superiority_morale_penalty_per_tick", OutErrors);
    CheckPerMilleRange(MT.FatiguePerTickUnderFirePerMille, "fatigue_per_tick_under_fire", OutErrors);
    CheckPerMilleRange(MT.MoraleRegenPerTickPerMille, "morale_regen_per_tick", OutErrors);
    CheckPerMilleRange(MT.FatigueRegenPerTickPerMille, "fatigue_regen_per_tick", OutErrors);
    CheckPerMilleRange(MT.SuppressionDecayPerTickPerMille, "suppression_decay_per_tick", OutErrors);
    CheckPerMilleRange(MT.DefaultCompetencePerMille, "default_competence", OutErrors);
    CheckPerMilleRange(MT.ScoutCompetencePerMille, "scout_competence", OutErrors);
    if (MT.AllyDeathRadiusTiles < 0 || MT.SuperiorityRadiusTiles < 0 || MT.OutOfFireDelayTicks < 0)
    {
        OutErrors.push_back("morale_tuning: negative radius or delay");
    }
    if (MT.SuperiorityRatioThresholdPerMille < 1000)
    {
        // A threshold below 1x means being merely EQUAL scares troops; that is a
        // designer mistake, not a tuning choice.
        OutErrors.push_back("morale_tuning: superiority_ratio_threshold below 1.0x");
    }

    const ChainTuning& CT = Settings.Chain;
    CheckPerMilleRange(CT.ReliabilityLossPerHopPerMille, "reliability_loss_per_hop", OutErrors);
    CheckPerMilleRange(CT.BlackoutConfidenceDecayPerSecondPerMille, "blackout_confidence_decay_per_second",
                       OutErrors);
    if (CT.NodeAttachRadiusTiles < 0 || CT.OrphanDelayTicks < 0 || CT.HopsFromNodeToHq < 0)
    {
        OutErrors.push_back("chain_tuning: negative radius, delay or hop count");
    }
    if (CT.CommsLevel < 0)
    {
        OutErrors.push_back("chain_tuning: comms_level is negative");
    }
    else if (const CommsProfile* Active = Settings.FindCommsProfile(Settings.ActiveCommsProfile))
    {
        // A level outside the active profile's ladder would silently read as "no
        // delay", turning a comms downgrade into a free upgrade.
        if (size_t(CT.CommsLevel) >= Active->HopDelayTicksByLevel.size())
        {
            OutErrors.push_back("chain_tuning: comms_level " + std::to_string(CT.CommsLevel) +
                                " is outside profile '" + Active->Name + "' ladder of " +
                                std::to_string(Active->HopDelayTicksByLevel.size()) + " levels");
        }
    }

    const TrackTuning& T = Settings.Tracks;
    if (T.MaxTracksPerPlayer <= 0 || T.MaxTracksPerPlayer > 65536)
    {
        OutErrors.push_back("track_tuning: max_tracks_per_player out of (0, 65536]");
    }
    if (T.MergeRadiusTiles < 0 || T.MergeWindowTicks < 0)
    {
        OutErrors.push_back("track_tuning: negative merge window");
    }
    CheckPerMilleRange(T.ConfidenceDecayPerSecondPerMille, "confidence_decay_per_second", OutErrors);
    CheckPerMilleRange(T.DropBelowConfidencePerMille, "drop_below_confidence", OutErrors);
    CheckPerMilleRange(T.AgreementConfidenceBonusPerMille, "agreement_confidence_bonus", OutErrors);
    if (T.TracksPerTickBudget <= 0 || T.TracksPerTickBudget > T.MaxTracksPerPlayer)
    {
        // Zero would stall the sweep forever (tracks never decay, never GC);
        // above the cap is meaningless and hides tuning mistakes.
        OutErrors.push_back("track_tuning: tracks_per_tick_budget out of (0, max_tracks_per_player]");
    }

    return OutErrors.size() == ErrorsBefore;
}

bool LoadReconSettingsFromJson(const std::string& JsonText, ReconSettings& OutSettings,
                               std::vector<std::string>& OutErrors)
{
    Json::Value Root;
    std::string ParseError;
    if (!Json::Parse(JsonText, Root, ParseError))
    {
        OutErrors.push_back("recon_settings: JSON parse error: " + ParseError);
        return false;
    }
    if (!Root.IsObject())
    {
        OutErrors.push_back("recon_settings: root is not an object");
        return false;
    }

    OutSettings = ReconSettings{};
    OutSettings.bEnabled = ReadBool(Root, "enabled", false);
    OutSettings.ActiveDistortionProfile = ReadString(Root, "active_distortion_profile", OutSettings.ActiveDistortionProfile);
    OutSettings.ActiveCommsProfile = ReadString(Root, "active_comms_profile", OutSettings.ActiveCommsProfile);
    OutSettings.RadarRangeTiles = ReadInt(Root, "radar_range_tiles", OutSettings.RadarRangeTiles);

    if (const Json::Value* Profiles = Root.Find("distortion_profiles"); Profiles != nullptr && Profiles->IsArray())
    {
        for (const Json::Value& P : Profiles->AsArray())
        {
            DistortionProfile Profile;
            ParseDistortionProfile(P, Profile);
            OutSettings.DistortionProfiles.push_back(Profile);
        }
    }

    if (const Json::Value* Profiles = Root.Find("comms_profiles"); Profiles != nullptr && Profiles->IsArray())
    {
        for (const Json::Value& P : Profiles->AsArray())
        {
            CommsProfile Profile;
            ParseCommsProfile(P, Profile);
            OutSettings.CommsProfiles.push_back(Profile);
        }
    }

    if (const Json::Value* Matrix = Root.Find("confusion_matrix"); Matrix != nullptr)
    {
        if (!ParseConfusionMatrix(*Matrix, OutSettings.Confusion, OutErrors))
        {
            return false;
        }
    }

    if (const Json::Value* M = Root.Find("morale_tuning"); M != nullptr && M->IsObject())
    {
        MoraleTuning& T = OutSettings.Morale;
        T.DamageMoralePenaltyPerMille = ReadPerMille(*M, "damage_morale_penalty", T.DamageMoralePenaltyPerMille);
        T.DamageSuppressionPerMille = ReadPerMille(*M, "damage_suppression", T.DamageSuppressionPerMille);
        T.AllyDeathMoralePenaltyPerMille = ReadPerMille(*M, "ally_death_morale_penalty", T.AllyDeathMoralePenaltyPerMille);
        T.AllyDeathRadiusTiles = ReadInt(*M, "ally_death_radius_tiles", T.AllyDeathRadiusTiles);
        T.SuperiorityRatioThresholdPerMille = ReadPerMille(*M, "superiority_ratio_threshold", T.SuperiorityRatioThresholdPerMille);
        T.SuperiorityMoralePenaltyPerTickPerMille = ReadPerMille(*M, "superiority_morale_penalty_per_tick", T.SuperiorityMoralePenaltyPerTickPerMille);
        T.SuperiorityRadiusTiles = ReadInt(*M, "superiority_radius_tiles", T.SuperiorityRadiusTiles);
        T.FatiguePerTickUnderFirePerMille = ReadPerMille(*M, "fatigue_per_tick_under_fire", T.FatiguePerTickUnderFirePerMille);
        T.MoraleRegenPerTickPerMille = ReadPerMille(*M, "morale_regen_per_tick", T.MoraleRegenPerTickPerMille);
        T.FatigueRegenPerTickPerMille = ReadPerMille(*M, "fatigue_regen_per_tick", T.FatigueRegenPerTickPerMille);
        T.SuppressionDecayPerTickPerMille = ReadPerMille(*M, "suppression_decay_per_tick", T.SuppressionDecayPerTickPerMille);
        T.OutOfFireDelayTicks = ReadInt(*M, "out_of_fire_delay_ticks", T.OutOfFireDelayTicks);
        T.DefaultCompetencePerMille = ReadPerMille(*M, "default_competence", T.DefaultCompetencePerMille);
        T.ScoutCompetencePerMille = ReadPerMille(*M, "scout_competence", T.ScoutCompetencePerMille);
    }

    if (const Json::Value* C = Root.Find("chain_tuning"); C != nullptr && C->IsObject())
    {
        ChainTuning& T = OutSettings.Chain;
        T.CommsLevel = ReadInt(*C, "comms_level", T.CommsLevel);
        T.NodeAttachRadiusTiles = ReadInt(*C, "node_attach_radius_tiles", T.NodeAttachRadiusTiles);
        T.OrphanDelayTicks = ReadInt(*C, "orphan_delay_ticks", T.OrphanDelayTicks);
        T.HopsFromNodeToHq = ReadInt(*C, "hops_from_node_to_hq", T.HopsFromNodeToHq);
        T.ReliabilityLossPerHopPerMille = ReadPerMille(*C, "reliability_loss_per_hop", T.ReliabilityLossPerHopPerMille);
        T.BlackoutConfidenceDecayPerSecondPerMille =
            ReadPerMille(*C, "blackout_confidence_decay_per_second", T.BlackoutConfidenceDecayPerSecondPerMille);
    }

    if (const Json::Value* Tracks = Root.Find("track_tuning"); Tracks != nullptr && Tracks->IsObject())
    {
        TrackTuning& T = OutSettings.Tracks;
        T.ConfidenceDecayPerSecondPerMille = ReadPerMille(*Tracks, "confidence_decay_per_second", T.ConfidenceDecayPerSecondPerMille);
        T.ErrorRadiusGrowthTilesPerMinute = ReadInt(*Tracks, "error_radius_growth_tiles_per_minute", T.ErrorRadiusGrowthTilesPerMinute);
        T.StaleAfterTicks = ReadInt(*Tracks, "stale_after_ticks", T.StaleAfterTicks);
        T.DropBelowConfidencePerMille = ReadPerMille(*Tracks, "drop_below_confidence", T.DropBelowConfidencePerMille);
        T.MergeRadiusTiles = ReadInt(*Tracks, "merge_radius_tiles", T.MergeRadiusTiles);
        T.MergeWindowTicks = ReadInt(*Tracks, "merge_window_ticks", T.MergeWindowTicks);
        T.AgreementConfidenceBonusPerMille = ReadPerMille(*Tracks, "agreement_confidence_bonus", T.AgreementConfidenceBonusPerMille);
        T.MaxTracksPerPlayer = ReadInt(*Tracks, "max_tracks_per_player", T.MaxTracksPerPlayer);
        T.TracksPerTickBudget = ReadInt(*Tracks, "tracks_per_tick_budget", T.TracksPerTickBudget);
    }

    return ValidateReconSettings(OutSettings, OutErrors);
}

uint64_t ReconSettings::ComputeSettingsHash() const
{
    // Field order below is the hash contract and follows declaration order.
    // Adding ANY field changes every settings hash -- deliberately: a new
    // tunable is a new ruleset, and old replays must be refused rather than
    // replayed under silently different rules. Strings feed length+bytes.
    const auto FeedString = [](Hash64& InH, const std::string& Str)
    {
        InH.FeedUInt32(uint32_t(Str.size()));
        InH.Feed(Str.data(), Str.size());
    };

    Hash64 H;
    H.FeedBool(bEnabled);
    FeedString(H, ActiveDistortionProfile);
    FeedString(H, ActiveCommsProfile);

    H.FeedUInt32(uint32_t(Tracks.ConfidenceDecayPerSecondPerMille));
    H.FeedUInt32(uint32_t(Tracks.ErrorRadiusGrowthTilesPerMinute));
    H.FeedUInt32(uint32_t(Tracks.StaleAfterTicks));
    H.FeedUInt32(uint32_t(Tracks.DropBelowConfidencePerMille));
    H.FeedUInt32(uint32_t(Tracks.MergeRadiusTiles));
    H.FeedUInt32(uint32_t(Tracks.MergeWindowTicks));
    H.FeedUInt32(uint32_t(Tracks.AgreementConfidenceBonusPerMille));
    H.FeedUInt32(uint32_t(Tracks.MaxTracksPerPlayer));
    H.FeedUInt32(uint32_t(Tracks.TracksPerTickBudget));

    H.FeedUInt32(uint32_t(Chain.CommsLevel));
    H.FeedUInt32(uint32_t(Chain.NodeAttachRadiusTiles));
    H.FeedUInt32(uint32_t(Chain.OrphanDelayTicks));
    H.FeedUInt32(uint32_t(Chain.HopsFromNodeToHq));
    H.FeedUInt32(uint32_t(Chain.ReliabilityLossPerHopPerMille));
    H.FeedUInt32(uint32_t(Chain.BlackoutConfidenceDecayPerSecondPerMille));

    H.FeedUInt32(uint32_t(DistortionProfiles.size()));
    for (const DistortionProfile& P : DistortionProfiles)
    {
        FeedString(H, P.Name);
        H.FeedBool(P.bClarityEnabled);
        H.FeedUInt32(uint32_t(P.MinClarityPerMille));
        H.FeedUInt32(uint32_t(P.ClarityDistanceFalloffPerMille));
        H.FeedBool(P.bCountDistortionEnabled);
        H.FeedUInt32(uint32_t(P.FearCountBiasMaxPerMille));
        H.FeedUInt32(uint32_t(P.CompetenceNoiseMaxPerMille));
        H.FeedBool(P.bClassificationErrorEnabled);
        H.FeedBool(P.bPositionErrorEnabled);
        H.FeedUInt32(uint32_t(P.PositionErrorMaxTiles));
        H.FeedBool(P.bOmissionEnabled);
        H.FeedUInt32(uint32_t(P.OmissionChanceMaxPerMille));
        H.FeedBool(P.bFabricationEnabled);
        H.FeedUInt32(uint32_t(P.FabricationChanceMaxPerMille));
        H.FeedUInt32(uint32_t(P.MaxPhantomLifetimeTicks));
        H.FeedBool(P.bSelfReportBiasEnabled);
        H.FeedUInt32(uint32_t(P.SelfReportLossUnderstatementMaxPerMille));
    }

    H.FeedUInt32(uint32_t(CommsProfiles.size()));
    for (const CommsProfile& P : CommsProfiles)
    {
        FeedString(H, P.Name);
        H.FeedUInt32(uint32_t(P.HopDelayTicksByLevel.size()));
        for (int32_t D : P.HopDelayTicksByLevel)
        {
            H.FeedUInt32(uint32_t(D));
        }
        H.FeedUInt32(uint32_t(P.OfficerBiasMaxPerMille));
    }

    for (int32_t Row = 0; Row < kObservedCategoryCount; ++Row)
    {
        for (int32_t Col = 0; Col < kObservedCategoryCount; ++Col)
        {
            H.FeedUInt32(uint32_t(Confusion.PerMille[Row][Col]));
        }
    }
    return H.Get();
}

} // namespace Recon
} // namespace RA4
