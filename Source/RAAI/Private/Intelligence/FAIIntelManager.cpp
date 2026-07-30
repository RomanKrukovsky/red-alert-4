#include "Intelligence/FAIIntelManager.h"
#include "AI/AAIDirector.h"
#include "Engine/World.h"

FAIIntelManager::FAIIntelManager()
{
}

void FAIIntelManager::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;
}

void FAIIntelManager::Shutdown()
{
    Director = nullptr;
    Sightings.Empty();
    KnownStructures.Empty();
    ProbabilisticEstimates.Empty();
    ThreatMap.Empty();
}

void FAIIntelManager::Update(float DeltaTime)
{
    UpdateTimer += DeltaTime;

    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;

        DecayConfidence(DeltaTime * UpdateInterval);
        UpdateProbabilisticEstimates(DeltaTime * UpdateInterval);
        UpdateThreatMap();
        PruneOldSightings(120.0f);
        MergeSightings();
    }
}

void FAIIntelManager::ReportEnemySighting(const FVector& Location, const FGameplayTag& UnitType, int32 Count, float Confidence)
{
    if (Confidence < 0.3f) return;

    FEnemySighting Sighting;
    Sighting.Location = Location;
    Sighting.UnitType = UnitType;
    Sighting.Count = Count;
    Sighting.Confidence = Confidence;
    Sighting.Timestamp = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;
    Sighting.LastSeenTime = Sighting.Timestamp;

    Sightings.Add(Sighting);

    UpdateProbabilisticEstimate(Sighting);
    UE_LOG(LogTemp, Log, TEXT("RAAI Intel: Enemy sighting %s x%d at %s (Conf: %.2f)"), *UnitType.ToString(), Count, *Location.ToString(), Confidence);
}

void FAIIntelManager::ReportEnemyStructure(const FVector& Location, const FGameplayTag& StructureType, float Confidence)
{
    if (Confidence < 0.3f) return;

    for (FEnemyStructure& Known : KnownStructures)
    {
        if (Known.StructureType.MatchesTag(StructureType) && (Known.Location - Location).Size() < 200.0f)
        {
            Known.Confidence = FMath::Max(Known.Confidence, Confidence);
            return;
        }
    }

    FEnemyStructure Structure;
    Structure.Location = Location;
    Structure.StructureType = StructureType;
    Structure.Confidence = Confidence;
    Structure.DiscoveryTime = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;

    KnownStructures.Add(Structure);

    UE_LOG(LogTemp, Log, TEXT("RAAI Intel: Enemy structure %s at %s (Conf: %.2f)"), *StructureType.ToString(), *Location.ToString(), Confidence);
}

void FAIIntelManager::ReportEnemyUnitDestroyed(const FVector& Location, const FGameplayTag& UnitType)
{
    for (FProbabilisticEstimate& Estimate : ProbabilisticEstimates)
    {
        if (Estimate.UnitType.MatchesTag(UnitType) && (Estimate.EstimatedLocation - Location).Size() < 300.0f)
        {
            Estimate.Probability *= 0.5f;
            Estimate.EstimatedCount = FMath::Max(0, Estimate.EstimatedCount - 1);
            break;
        }
    }
}

void FAIIntelManager::ReportEnemyStructureDestroyed(const FVector& Location, const FGameplayTag& StructureType)
{
    for (FEnemyStructure& Structure : KnownStructures)
    {
        if (Structure.StructureType.MatchesTag(StructureType) && (Structure.Location - Location).Size() < 200.0f)
        {
            Structure.bIsDestroyed = true;
            Structure.DestructionTime = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;
            Structure.Confidence = 0.0f;
            break;
        }
    }
}

TArray<FEnemySighting> FAIIntelManager::GetRecentSightings(float TimeWindow) const
{
    float CurrentTime = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;
    float Cutoff = CurrentTime - TimeWindow;

    TArray<FEnemySighting> Recent;
    for (const FEnemySighting& Sighting : Sightings)
    {
        if (Sighting.LastSeenTime > Cutoff)
        {
            Recent.Add(Sighting);
        }
    }
    return Recent;
}

TArray<FEnemyStructure> FAIIntelManager::GetKnownEnemyStructures() const
{
    TArray<FEnemyStructure> Active;
    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (!Structure.bIsDestroyed && Structure.Confidence > 0.2f)
        {
            Active.Add(Structure);
        }
    }
    return Active;
}

TArray<FProbabilisticEstimate> FAIIntelManager::GetProbabilisticEstimates() const
{
    TArray<FProbabilisticEstimate> Active;
    for (const FProbabilisticEstimate& Estimate : ProbabilisticEstimates)
    {
        if (Estimate.Probability > 0.2f)
        {
            Active.Add(Estimate);
        }
    }
    return Active;
}

TArray<FThreatAssessment> FAIIntelManager::GetThreatMap() const
{
    return ThreatMap;
}

int32 FAIIntelManager::GetKnownEnemyUnitCount(const FGameplayTag& UnitType) const
{
    int32 Total = 0;
    for (const FProbabilisticEstimate& Estimate : ProbabilisticEstimates)
    {
        if (Estimate.UnitType.MatchesTag(UnitType))
        {
            Total += FMath::RoundToInt(Estimate.EstimatedCount * Estimate.Probability);
        }
    }
    return Total;
}

int32 FAIIntelManager::GetKnownEnemyStructureCount(const FGameplayTag& StructureType) const
{
    int32 Total = 0;
    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (Structure.StructureType.MatchesTag(StructureType) && !Structure.bIsDestroyed)
        {
            Total++;
        }
    }
    return Total;
}

bool FAIIntelManager::IsEnemyPowerDown() const
{
    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.PowerPlant")) &&
            !Structure.bIsDestroyed && Structure.Confidence > 0.5f)
        {
            return false;
        }
    }
    return GetKnownEnemyStructureCount(FGameplayTag::RequestGameplayTag("Structure.PowerPlant")) == 0;
}

float FAIIntelManager::GetEnemyPressureLevel() const
{
    float Pressure = 0.0f;
    for (const FThreatAssessment& Threat : ThreatMap)
    {
        Pressure += Threat.ThreatLevel;
    }
    return FMath::Clamp(Pressure / 10.0f, 0.0f, 1.0f);
}

float FAIIntelManager::GetBaseWeaknessScore() const
{
    float Weakness = 0.0f;

    int32 PowerPlants = GetKnownEnemyStructureCount(FGameplayTag::RequestGameplayTag("Structure.PowerPlant"));
    int32 AA = GetKnownEnemyStructureCount(FGameplayTag::RequestGameplayTag("Structure.Defense.AA"));
    int32 Turrets = GetKnownEnemyStructureCount(FGameplayTag::RequestGameplayTag("Structure.Defense.Turret"));

    if (PowerPlants <= 1) Weakness += 0.4f;
    if (AA == 0) Weakness += 0.3f;
    if (Turrets <= 2) Weakness += 0.3f;

    return FMath::Clamp(Weakness, 0.0f, 1.0f);
}

bool FAIIntelManager::AreHarvestersUnderAttack() const
{
    for (const FEnemySighting& Sighting : Sightings)
    {
        if (Sighting.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Combat")) &&
            Sighting.Confidence > 0.5f)
        {
            return true;
        }
    }
    return false;
}

bool FAIIntelManager::IsExpansionSafe() const
{
    return GetEnemyPressureLevel() < 0.4f;
}

float FAIIntelManager::GetEnemyProximityToBase() const
{
    if (!Director) return 0.0f;

    FVector BaseCenter = Director->GetActorLocation();
    float MinDist = TNumericLimits<float>::Max();

    for (const FEnemySighting& Sighting : Sightings)
    {
        float Dist = (Sighting.Location - BaseCenter).Size();
        MinDist = FMath::Min(MinDist, Dist);
    }

    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (!Structure.bIsDestroyed)
        {
            float Dist = (Structure.Location - BaseCenter).Size();
            MinDist = FMath::Min(MinDist, Dist);
        }
    }

    if (MinDist == TNumericLimits<float>::Max()) return 0.0f;

    return FMath::Clamp(1.0f - MinDist / 5000.0f, 0.0f, 1.0f);
}

FVector FAIIntelManager::GetLastKnownEnemyBaseLocation() const
{
    FVector Center(0, 0, 0);
    int32 Count = 0;

    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (!Structure.bIsDestroyed && Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.")))
        {
            Center += Structure.Location;
            Count++;
        }
    }

    if (Count > 0)
    {
        return Center / Count;
    }

    return FVector::ZeroVector;
}

TArray<FVector> FAIIntelManager::GetPredictedEnemyExpansionLocations() const
{
    TArray<FVector> Locations;
    FVector EnemyBase = GetLastKnownEnemyBaseLocation();

    if (!EnemyBase.IsZero())
    {
        for (int32 i = 0; i < 4; i++)
        {
            float Angle = (float)i / 4.0f * 2.0f * PI;
            FVector Loc = EnemyBase + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * 1500.0f;
            Locations.Add(Loc);
        }
    }

    return Locations;
}

void FAIIntelManager::ForgetOldIntel(float MaxAge)
{
    PruneOldSightings(MaxAge);
}

void FAIIntelManager::UpdateProbabilisticEstimates(float DeltaTime)
{
    for (FProbabilisticEstimate& Estimate : ProbabilisticEstimates)
    {
        Estimate.Probability = FMath::Max(0.0f, Estimate.Probability - DeltaTime * 0.05f);
        Estimate.UncertaintyRadius += DeltaTime * 50.0f;
    }

    ProbabilisticEstimates.RemoveAll([](const FProbabilisticEstimate& E) {
        return E.Probability < 0.1f;
    });
}

void FAIIntelManager::UpdateThreatMap()
{
    ThreatMap.Empty();

    for (const FEnemySighting& Sighting : Sightings)
    {
        if (Sighting.Confidence < 0.3f) continue;

        float Threat = Sighting.Count * Sighting.Confidence * 0.5f;

        if (Sighting.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy")))
            Threat *= 2.0f;
        else if (Sighting.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Aircraft")))
            Threat *= 1.5f;
        else if (Sighting.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Artillery")))
            Threat *= 1.8f;

        FThreatAssessment Assessment;
        Assessment.Location = Sighting.Location;
        Assessment.ThreatLevel = Threat;
        Assessment.ThreatTypes.Add(Sighting.UnitType);
        Assessment.TimeSinceUpdate = 0.0f;

        ThreatMap.Add(Assessment);
    }

    for (const FEnemyStructure& Structure : KnownStructures)
    {
        if (Structure.bIsDestroyed) continue;
        if (Structure.Confidence < 0.3f) continue;

        float Threat = 0.0f;

        if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Defense.Turret")))
            Threat = 1.0f;
        else if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Defense.AA")))
            Threat = 0.8f;
        else if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.PowerPlant")))
            Threat = 0.5f;
        else if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.WarFactory")))
            Threat = 1.2f;
        else if (Structure.StructureType.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Airfield")))
            Threat = 1.0f;
        else
            Threat = 0.3f;

        Threat *= Structure.Confidence;

        FThreatAssessment Assessment;
        Assessment.Location = Structure.Location;
        Assessment.ThreatLevel = Threat;
        Assessment.ThreatTypes.Add(Structure.StructureType);
        Assessment.TimeSinceUpdate = 0.0f;

        ThreatMap.Add(Assessment);
    }
}

void FAIIntelManager::DecayConfidence(float DeltaTime)
{
    for (FEnemySighting& Sighting : Sightings)
    {
        Sighting.Confidence = FMath::Max(0.0f, Sighting.Confidence - DeltaTime * 0.1f);
    }

    for (FEnemyStructure& Structure : KnownStructures)
    {
        if (!Structure.bIsDestroyed)
        {
            Structure.Confidence = FMath::Max(0.0f, Structure.Confidence - DeltaTime * 0.02f);
        }
    }

    Sightings.RemoveAll([](const FEnemySighting& S) { return S.Confidence < 0.1f; });
    KnownStructures.RemoveAll([](const FEnemyStructure& S) { return S.Confidence < 0.1f && S.bIsDestroyed; });
}

void FAIIntelManager::MergeSightings()
{
    for (int32 i = Sightings.Num() - 1; i > 0; i--)
    {
        for (int32 j = i - 1; j >= 0; j--)
        {
            if (Sightings[i].UnitType.MatchesTag(Sightings[j].UnitType) &&
                (Sightings[i].Location - Sightings[j].Location).Size() < 150.0f)
            {
                Sightings[j].Confidence = FMath::Max(Sightings[j].Confidence, Sightings[i].Confidence);
                Sightings[j].Count = FMath::Max(Sightings[j].Count, Sightings[i].Count);
                Sightings[j].LastSeenTime = FMath::Max(Sightings[j].LastSeenTime, Sightings[i].LastSeenTime);
                Sightings.RemoveAt(i);
                break;
            }
        }
    }
}

void FAIIntelManager::UpdateProbabilisticEstimate(const FEnemySighting& Sighting)
{
    for (FProbabilisticEstimate& Estimate : ProbabilisticEstimates)
    {
        if (Estimate.UnitType.MatchesTag(Sighting.UnitType) &&
            (Estimate.EstimatedLocation - Sighting.Location).Size() < 300.0f)
        {
            Estimate.Probability = FMath::Min(1.0f, Estimate.Probability + Sighting.Confidence * 0.5f);
            Estimate.EstimatedCount = FMath::Max(Estimate.EstimatedCount, Sighting.Count);
            Estimate.EstimatedLocation = (Estimate.EstimatedLocation * 0.7f + Sighting.Location * 0.3f);
            Estimate.UncertaintyRadius = FMath::Max(100.0f, Estimate.UncertaintyRadius * 0.9f);
            return;
        }
    }

    FProbabilisticEstimate NewEstimate;
    NewEstimate.EstimatedLocation = Sighting.Location;
    NewEstimate.Probability = Sighting.Confidence * 0.7f;
    NewEstimate.UnitType = Sighting.UnitType;
    NewEstimate.EstimatedCount = Sighting.Count;
    NewEstimate.UncertaintyRadius = 200.0f;

    ProbabilisticEstimates.Add(NewEstimate);
}

void FAIIntelManager::PruneOldSightings(float MaxAge)
{
    float CurrentTime = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;

    Sightings.RemoveAll([CurrentTime, MaxAge](const FEnemySighting& S) {
        return CurrentTime - S.Timestamp > MaxAge;
    });

    ProbabilisticEstimates.RemoveAll([](const FProbabilisticEstimate& E) {
        return E.Probability < 0.1f;
    });
}