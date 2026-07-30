#include "Managers/FAIBasePlanner.h"
#include "AI/AAIDirector.h"
#include "Engine/World.h"
#include "GameFramework/Volume.h"
#include "CollisionQueryParams.h"

FAIBasePlanner::FAIBasePlanner()
{
}

void FAIBasePlanner::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;
    GenerateBuildSpots();
}

void FAIBasePlanner::Shutdown()
{
    Director = nullptr;
    BuildSpots.Empty();
    BuildQueue.Empty();
}

void FAIBasePlanner::Update(float DeltaTime)
{
    UpdateTimer += DeltaTime;
    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;
        ProcessBuildQueue(DeltaTime);
    }
}

void FAIBasePlanner::RequestStructure(const FGameplayTag& StructureTag, float Priority, const FVector& PreferredLocation)
{
    FBuildOrder Order;
    Order.StructureTag = StructureTag;
    Order.Priority = Priority;
    Order.PreferredLocation = PreferredLocation;
    Order.RequestTime = Director ? Director->GetWorld()->GetTimeSeconds() : 0.0f;

    BuildQueue.Add(Order);
    BuildQueue.Sort([](const FBuildOrder& A, const FBuildOrder& B) {
        if (A.Priority != B.Priority) return A.Priority > B.Priority;
        return A.RequestTime < B.RequestTime;
    });

    UE_LOG(LogTemp, Log, TEXT("RAAI BasePlanner: Structure requested %s (Priority: %.1f)"), *StructureTag.ToString(), Priority);
}

void FAIBasePlanner::CancelStructureRequest(const FGameplayTag& StructureTag)
{
    BuildQueue.RemoveAll([&StructureTag](const FBuildOrder& Order) {
        return Order.StructureTag.MatchesTag(StructureTag);
    });
}

void FAIBasePlanner::OnStructureBuilt(AActor* Structure, const FGameplayTag& StructureTag)
{
    StructuresByType.FindOrAdd(StructureTag).AddUnique(Structure);
    StructureCounts.FindOrAdd(StructureTag)++;

    if (Director)
    {
        Director->EconomyManager.OnStructureBuilt(StructureTag);
    }

    UE_LOG(LogTemp, Log, TEXT("RAAI BasePlanner: Structure built %s"), *StructureTag.ToString());
}

void FAIBasePlanner::OnStructureDestroyed(AActor* Structure, const FGameplayTag& StructureTag)
{
    StructuresByType.FindOrAdd(StructureTag).Remove(Structure);
    StructureCounts.FindOrAdd(StructureTag)--;

    if (Director)
    {
        Director->EconomyManager.OnStructureDestroyed(StructureTag);
    }

    UE_LOG(LogTemp, Log, TEXT("RAAI BasePlanner: Structure destroyed %s"), *StructureTag.ToString());
}

bool FAIBasePlanner::HasStructure(const FGameplayTag& StructureTag) const
{
    return StructureCounts.FindRef(StructureTag) > 0;
}

int32 FAIBasePlanner::GetStructureCount(const FGameplayTag& StructureTag) const
{
    return StructureCounts.FindRef(StructureTag);
}

TArray<AActor*> FAIBasePlanner::GetStructures(const FGameplayTag& StructureTag) const
{
    if (const TArray<AActor*>* Found = StructuresByType.Find(StructureTag))
    {
        return *Found;
    }
    return TArray<AActor*>();
}

TArray<FBuildSpot> FAIBasePlanner::GetAvailableBuildSpots(const FGameplayTag& StructureTag) const
{
    TArray<FBuildSpot> AvailableSpots;
    for (const FBuildSpot& Spot : BuildSpots)
    {
        if (Spot.bIsOccupied) continue;
        if (CanBuildAt(Spot.Location, StructureTag))
        {
            AvailableSpots.Add(Spot);
        }
    }
    AvailableSpots.Sort([this, &StructureTag](const FBuildSpot& A, const FBuildSpot& B) {
        return EvaluateBuildSpot(A, StructureTag) > EvaluateBuildSpot(B, StructureTag);
    });
    return AvailableSpots;
}

FVector FAIBasePlanner::FindBestBuildLocation(const FGameplayTag& StructureTag) const
{
    TArray<FBuildSpot> Spots = GetAvailableBuildSpots(StructureTag);
    if (Spots.Num() > 0)
    {
        return Spots[0].Location;
    }

    if (!BaseCenter.IsZero())
    {
        return FindNearestBuildSpot(BaseCenter, StructureTag);
    }

    return FVector::ZeroVector;
}

bool FAIBasePlanner::HasAvailableOreField() const
{
    return Director && Director->IntelManager.GetProbabilisticEstimates().FilterByPredicate(
        [](const FProbabilisticEstimate& E) {
            return E.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Resource.OreField"));
        }).Num() > 0;
}

bool FAIBasePlanner::HasUndefendedKeyLocation() const
{
    TArray<FThreatAssessment> Threats = Director ? Director->IntelManager.GetThreatMap() : TArray<FThreatAssessment>();
    for (const FThreatAssessment& Threat : Threats)
    {
        if (Threat.ThreatLevel > 0.5f && Threat.Location.Size() < 2000.0f)
        {
            return true;
        }
    }
    return false;
}

void FAIBasePlanner::RegisterBuildSpot(const FBuildSpot& Spot)
{
    BuildSpots.AddUnique(Spot);
}

void FAIBasePlanner::ClearBuildSpot(const FVector& Location)
{
    BuildSpots.RemoveAll([&Location](const FBuildSpot& Spot) {
        return (Spot.Location - Location).Size() < 100.0f;
    });
}

void FAIBasePlanner::SetBaseCenter(const FVector& Center)
{
    BaseCenter = Center;
}

FVector FAIBasePlanner::GetBaseCenter() const
{
    return BaseCenter;
}

float FAIBasePlanner::GetBuildQueueProgress(const FGameplayTag& StructureTag) const
{
    for (const FBuildOrder& Order : BuildQueue)
    {
        if (Order.StructureTag.MatchesTag(StructureTag))
        {
            return 0.5f;
        }
    }
    return 0.0f;
}

void FAIBasePlanner::GenerateBuildSpots()
{
    BuildSpots.Empty();

    if (!Director) return;

    UWorld* World = Director->GetWorld();
    if (!World) return;

    FVector Center = BaseCenter.IsZero() ? Director->GetActorLocation() : BaseCenter;

    int32 NumRings = 3;
    int32 SpotsPerRing = 8;
    float RingSpacing = 400.0f;

    for (int32 Ring = 1; Ring <= NumRings; Ring++)
    {
        float Radius = Ring * RingSpacing;
        for (int32 i = 0; i < SpotsPerRing; i++)
        {
            float Angle = (float)i / SpotsPerRing * 2.0f * PI;
            FVector Location = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;

            if (CanBuildAt(Location, FGameplayTag()))
            {
                FBuildSpot Spot;
                Spot.Location = Location;
                Spot.bIsOccupied = false;
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.PowerPlant"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.Refinery"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.Barracks"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.WarFactory"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.TechCenter"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.Defense.Turret"));
                Spot.SuitableFor.Add(FGameplayTag::RequestGameplayTag("Structure.Defense.AA"));
                Spot.Score = 1.0f;
                BuildSpots.Add(Spot);
            }
        }
    }

    UpdateBuildSpotScores();
}

void FAIBasePlanner::ProcessBuildQueue(float DeltaTime)
{
    for (int32 i = 0; i < BuildQueue.Num(); i++)
    {
        FBuildOrder& Order = BuildQueue[i];

        if (!Director) continue;

        float Cost = GetStructureCost(Order.StructureTag);
        if (Director->EconomyManager.CanAfford(FGameplayTag::RequestGameplayTag("Resource.Credits"), Cost))
        {
            FVector BuildLocation = Order.PreferredLocation.IsZero() ? FindBestBuildLocation(Order.StructureTag) : Order.PreferredLocation;

            if (!BuildLocation.IsZero())
            {
                Director->EconomyManager.Spend(FGameplayTag::RequestGameplayTag("Resource.Credits"), Cost);
                BuildQueue.RemoveAt(i);
                i--;

                UE_LOG(LogTemp, Log, TEXT("RAAI BasePlanner: Building %s at %s"), *Order.StructureTag.ToString(), *BuildLocation.ToString());
            }
        }
    }
}

float FAIBasePlanner::EvaluateBuildSpot(const FBuildSpot& Spot, const FGameplayTag& StructureTag) const
{
    float Score = Spot.Score;

    if (!Director) return Score;

    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Refinery")))
    {
        Score += 1.0f / FMath::Max(100.0f, (Director->IntelManager.GetProbabilisticEstimates().FilterByPredicate(
            [&Spot](const FProbabilisticEstimate& E) {
                return E.UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Resource.OreField")) &&
                       (E.EstimatedLocation - Spot.Location).Size() < 500.0f;
            }).Num()));
    }

    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Defense.")))
    {
        TArray<FThreatAssessment> Threats = Director->IntelManager.GetThreatMap();
        for (const FThreatAssessment& Threat : Threats)
        {
            float Dist = (Threat.Location - Spot.Location).Size();
            if (Dist < 1500.0f)
            {
                Score += Threat.ThreatLevel * (1.0f - Dist / 1500.0f);
            }
        }
    }

    float DistToCenter = (Spot.Location - BaseCenter).Size();
    Score -= DistToCenter / 5000.0f;

    return Score;
}

bool FAIBasePlanner::CanBuildAt(const FVector& Location, const FGameplayTag& StructureTag) const
{
    if (!Director || !Director->GetWorld()) return false;

    UWorld* World = Director->GetWorld();
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Director);

    FVector Start = Location + FVector(0, 0, 200.0f);
    FVector End = Location - FVector(0, 0, 200.0f);

    FHitResult Hit;
    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
    {
        if (!Hit.bBlockingHit) return true;
    }

    return false;
}

FVector FAIBasePlanner::FindNearestBuildSpot(const FVector& PreferredLocation, const FGameplayTag& StructureTag) const
{
    float BestDist = TNumericLimits<float>::Max();
    FVector BestSpot = FVector::ZeroVector;

    for (const FBuildSpot& Spot : BuildSpots)
    {
        if (Spot.bIsOccupied) continue;
        if (!CanBuildAt(Spot.Location, StructureTag)) continue;

        float Dist = (Spot.Location - PreferredLocation).Size();
        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestSpot = Spot.Location;
        }
    }

    return BestSpot;
}

void FAIBasePlanner::UpdateBuildSpotScores()
{
    for (FBuildSpot& Spot : BuildSpots)
    {
        float MinDist = TNumericLimits<float>::Max();
        for (auto& Pair : StructuresByType)
        {
            for (AActor* Structure : Pair.Value)
            {
                if (Structure)
                {
                    float Dist = (Structure->GetActorLocation() - Spot.Location).Size();
                    MinDist = FMath::Min(MinDist, Dist);
                }
            }
        }
        Spot.Score = 1.0f - FMath::Clamp(MinDist / 3000.0f, 0.0f, 1.0f);
    }
}

float FAIBasePlanner::GetStructureCost(const FGameplayTag& StructureTag) const
{
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.PowerPlant"))) return 500.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Refinery"))) return 1000.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Barracks"))) return 800.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.WarFactory"))) return 1200.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Airfield"))) return 1500.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.TechCenter"))) return 2000.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Defense.Turret"))) return 400.0f;
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Defense.AA"))) return 500.0f;
    return 1000.0f;
}