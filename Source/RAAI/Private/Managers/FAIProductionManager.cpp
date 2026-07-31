#include "Managers/FAIProductionManager.h"
#include "AI/AAIDirector.h"

FAIProductionManager::FAIProductionManager()
{
}

void FAIProductionManager::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;
    RallyPoint = Director ? Director->GetActorLocation() : FVector::ZeroVector;
}

void FAIProductionManager::Shutdown()
{
    Director = nullptr;
    ProductionQueue.Empty();
    Producers.Empty();
}

void FAIProductionManager::Update(float DeltaTime)
{
    UpdateTimer += DeltaTime;
    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;
        ProcessQueue(DeltaTime);
    }
}

void FAIProductionManager::RequestUnits(const FGameplayTag& UnitTag, int32 Count, float Priority)
{
    for (int32 i = 0; i < Count; i++)
    {
        FProductionQueueItem Item;
        Item.UnitTag = UnitTag;
        Item.Cost = GetUnitCost(UnitTag);
        Item.BuildTime = GetUnitBuildTime(UnitTag);
        Item.Priority = Priority;
        Item.ProducerTag = GetProducerForUnit(UnitTag);
        Item.Progress = 0.0f;

        ProductionQueue.Add(Item);
    }

    SortQueue();

    UE_LOG(LogTemp, Log, TEXT("RAAI Production: Requested %d x %s (Priority: %.1f)"), Count, *UnitTag.ToString(), Priority);
}

void FAIProductionManager::CancelUnitRequest(const FGameplayTag& UnitTag, int32 Count)
{
    int32 Removed = 0;
    for (int32 i = ProductionQueue.Num() - 1; i >= 0 && Removed < Count; i--)
    {
        if (ProductionQueue[i].UnitTag.MatchesTag(UnitTag))
        {
            ProductionQueue.RemoveAt(i);
            Removed++;
        }
    }
}

void FAIProductionManager::OnUnitCompleted(const FGameplayTag& UnitTag, const FGameplayTag& ProducerTag)
{
    UE_LOG(LogTemp, Log, TEXT("RAAI Production: Unit completed %s from %s"), *UnitTag.ToString(), *ProducerTag.ToString());
}

int32 FAIProductionManager::GetQueuedCount(const FGameplayTag& UnitTag) const
{
    int32 Count = 0;
    for (const FProductionQueueItem& Item : ProductionQueue)
    {
        if (Item.UnitTag.MatchesTag(UnitTag))
        {
            Count++;
        }
    }
    return Count;
}

float FAIProductionManager::GetQueueProgress(const FGameplayTag& UnitTag) const
{
    for (const FProductionQueueItem& Item : ProductionQueue)
    {
        if (Item.UnitTag.MatchesTag(UnitTag))
        {
            return Item.Progress;
        }
    }
    return 0.0f;
}

TArray<FProductionQueueItem> FAIProductionManager::GetProductionQueue() const
{
    return ProductionQueue;
}

void FAIProductionManager::RegisterProducer(const FProducerInfo& Producer)
{
    Producers.AddUnique(Producer);
}

void FAIProductionManager::UnregisterProducer(const FGameplayTag& ProducerTag)
{
    Producers.RemoveAll([&ProducerTag](const FProducerInfo& P) {
        return P.ProducerTag.MatchesTag(ProducerTag);
    });
}

TArray<FProducerInfo> FAIProductionManager::GetAvailableProducers(const FGameplayTag& UnitTag) const
{
    TArray<FProducerInfo> Available;
    for (const FProducerInfo& Producer : Producers)
    {
        if (Producer.bIsActive && Producer.CanProduce.Contains(UnitTag))
        {
            Available.Add(Producer);
        }
    }
    return Available;
}

void FAIProductionManager::SetRallyPoint(const FVector& Location)
{
    RallyPoint = Location;
}

FVector FAIProductionManager::GetRallyPoint() const
{
    return RallyPoint;
}

bool FAIProductionManager::CanProduce(const FGameplayTag& UnitTag) const
{
    for (const FProducerInfo& Producer : Producers)
    {
        if (Producer.bIsActive && Producer.CanProduce.Contains(UnitTag))
        {
            return true;
        }
    }
    return false;
}

float FAIProductionManager::GetEstimatedCompletionTime(const FGameplayTag& UnitTag) const
{
    int32 Queued = GetQueuedCount(UnitTag);
    if (Queued == 0) return 0.0f;

    float TotalTime = 0.0f;
    int32 Count = 0;
    for (const FProductionQueueItem& Item : ProductionQueue)
    {
        if (Item.UnitTag.MatchesTag(UnitTag))
        {
            TotalTime += Item.BuildTime * (1.0f - Item.Progress);
            Count++;
        }
    }
    return Count > 0 ? TotalTime / Count : 0.0f;
}

void FAIProductionManager::PrioritizeUnit(const FGameplayTag& UnitTag)
{
    for (FProductionQueueItem& Item : ProductionQueue)
    {
        if (Item.UnitTag.MatchesTag(UnitTag))
        {
            Item.Priority += 5.0f;
        }
    }
    SortQueue();
}

void FAIProductionManager::DeprioritizeUnit(const FGameplayTag& UnitTag)
{
    for (FProductionQueueItem& Item : ProductionQueue)
    {
        if (Item.UnitTag.MatchesTag(UnitTag))
        {
            Item.Priority = FMath::Max(0.1f, Item.Priority - 3.0f);
        }
    }
    SortQueue();
}

void FAIProductionManager::ProcessQueue(float DeltaTime)
{
    if (!Director) return;

    for (FProductionQueueItem& Item : ProductionQueue)
    {
        if (Director->EconomyManager.CanAfford(FGameplayTag::RequestGameplayTag("Resource.Credits"), Item.Cost))
        {
            Director->EconomyManager.SpendResource(FGameplayTag::RequestGameplayTag("Resource.Credits"), Item.Cost);

            Item.Progress += DeltaTime / Item.BuildTime;

            if (Item.Progress >= 1.0f)
            {
                FProducerInfo* Producer = FindBestProducer(Item.UnitTag);
                if (Producer)
                {
                    OnUnitCompleted(Item.UnitTag, Producer->ProducerTag);
                    ProductionQueue.RemoveSingle(Item);
                    break;
                }
            }
        }
    }
}

void FAIProductionManager::SortQueue()
{
    ProductionQueue.Sort([](const FProductionQueueItem& A, const FProductionQueueItem& B) {
        if (A.Priority != B.Priority) return A.Priority > B.Priority;
        return A.BuildTime < B.BuildTime;
    });
}

FProducerInfo* FAIProductionManager::FindBestProducer(const FGameplayTag& UnitTag)
{
    FProducerInfo* Best = nullptr;
    float BestSpeed = 0.0f;

    for (FProducerInfo& Producer : Producers)
    {
        if (!Producer.bIsActive) continue;
        if (!Producer.CanProduce.Contains(UnitTag)) continue;

        if (Producer.BuildSpeedMultiplier > BestSpeed)
        {
            BestSpeed = Producer.BuildSpeedMultiplier;
            Best = &Producer;
        }
    }

    return Best;
}

float FAIProductionManager::GetUnitCost(const FGameplayTag& UnitTag) const
{
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Infantry"))) return 100.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Light"))) return 400.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Medium"))) return 700.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"))) return 1200.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Artillery"))) return 800.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.AntiAir"))) return 500.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Harvester"))) return 1400.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Aircraft"))) return 1500.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Transform"))) return 1800.0f;
    return 500.0f;
}

float FAIProductionManager::GetUnitBuildTime(const FGameplayTag& UnitTag) const
{
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Infantry"))) return 10.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Light"))) return 20.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Medium"))) return 30.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"))) return 45.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Artillery"))) return 35.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.AntiAir"))) return 25.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Harvester"))) return 40.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Aircraft"))) return 50.0f;
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Transform"))) return 60.0f;
    return 30.0f;
}

TArray<FGameplayTag> FAIProductionManager::GetUnitPrerequisites(const FGameplayTag& UnitTag) const
{
    TArray<FGameplayTag> Prereqs;

    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank")))
    {
        Prereqs.Add(FGameplayTag::RequestGameplayTag("Structure.WarFactory"));
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Aircraft")))
    {
        Prereqs.Add(FGameplayTag::RequestGameplayTag("Structure.Airfield"));
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Transform")))
    {
        Prereqs.Add(FGameplayTag::RequestGameplayTag("Structure.TechCenter"));
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Infantry")))
    {
        Prereqs.Add(FGameplayTag::RequestGameplayTag("Structure.Barracks"));
    }

    return Prereqs;
}

FGameplayTag FAIProductionManager::GetProducerForUnit(const FGameplayTag& UnitTag) const
{
    if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Infantry")))
    {
        return FGameplayTag::RequestGameplayTag("Structure.Barracks");
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank")))
    {
        return FGameplayTag::RequestGameplayTag("Structure.WarFactory");
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Aircraft")))
    {
        return FGameplayTag::RequestGameplayTag("Structure.Airfield");
    }
    else if (UnitTag.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Harvester")))
    {
        return FGameplayTag::RequestGameplayTag("Structure.WarFactory");
    }

    return FGameplayTag::RequestGameplayTag("Structure.WarFactory");
}