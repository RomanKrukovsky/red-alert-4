#include "Managers/FAIEconomyManager.h"
#include "AI/AAIDirector.h"

FAIEconomyManager::FAIEconomyManager()
{
    ResourceAmounts.Add(FGameplayTag::RequestGameplayTag("Resource.Credits"), 5000.0f);
    ResourceAmounts.Add(FGameplayTag::RequestGameplayTag("Resource.Ore"), 0.0f);
    ResourceAmounts.Add(FGameplayTag::RequestGameplayTag("Resource.Power"), 0.0f);
    ResourceAmounts.Add(FGameplayTag::RequestGameplayTag("Resource.PowerCapacity"), 0.0f);

    IncomeRates.Add(FGameplayTag::RequestGameplayTag("Resource.Credits"), 0.0f);
    IncomeRates.Add(FGameplayTag::RequestGameplayTag("Resource.Ore"), 0.0f);
    IncomeRates.Add(FGameplayTag::RequestGameplayTag("Resource.Power"), 0.0f);

    ExpenseRates.Add(FGameplayTag::RequestGameplayTag("Resource.Credits"), 0.0f);
    ExpenseRates.Add(FGameplayTag::RequestGameplayTag("Resource.Power"), 0.0f);

    HarvesterCount = 0;
    RefineryCount = 0;
    PowerPlantCount = 0;
}

void FAIEconomyManager::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;
}

void FAIEconomyManager::Shutdown()
{
    Director = nullptr;
}

void FAIEconomyManager::Update(float DeltaTime)
{
    UpdateTimer += DeltaTime;

    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.0f;

        RecalculateIncome();
        RecalculateExpenses();

        float NetIncome = TotalIncome - TotalExpenses;
        ResourceAmounts.FindOrAdd(FGameplayTag::RequestGameplayTag("Resource.Credits")) += NetIncome * UpdateInterval;

        float Credits = ResourceAmounts.FindRef(FGameplayTag::RequestGameplayTag("Resource.Credits"));
        if (Credits < 0.0f)
        {
            Credits = 0.0f;
            ResourceAmounts[FGameplayTag::RequestGameplayTag("Resource.Credits")] = 0.0f;
        }
    }
}

void FAIEconomyManager::AddIncome(const FGameplayTag& ResourceType, float Amount)
{
    IncomeRates.FindOrAdd(ResourceType) += Amount;
}

void FAIEconomyManager::AddExpense(const FGameplayTag& ResourceType, float Amount)
{
    ExpenseRates.FindOrAdd(ResourceType) += Amount;
}

bool FAIEconomyManager::CanAfford(const FGameplayTag& ResourceType, float Amount) const
{
    float Available = ResourceAmounts.FindRef(ResourceType);
    return Available >= Amount;
}

bool FAIEconomyManager::Spend(const FGameplayTag& ResourceType, float Amount)
{
    float& Available = ResourceAmounts.FindOrAdd(ResourceType);
    if (Available >= Amount)
    {
        Available -= Amount;
        return true;
    }
    return false;
}

void FAIEconomyManager::Earn(const FGameplayTag& ResourceType, float Amount)
{
    ResourceAmounts.FindOrAdd(ResourceType) += Amount;
}

float FAIEconomyManager::GetResourceAmount(const FGameplayTag& ResourceType) const
{
    return ResourceAmounts.FindRef(ResourceType);
}

float FAIEconomyManager::GetIncomeRate(const FGameplayTag& ResourceType) const
{
    return IncomeRates.FindRef(ResourceType);
}

float FAIEconomyManager::GetExpenseRate(const FGameplayTag& ResourceType) const
{
    return ExpenseRates.FindRef(ResourceType);
}

float FAIEconomyManager::GetNetIncome(const FGameplayTag& ResourceType) const
{
    return IncomeRates.FindRef(ResourceType) - ExpenseRates.FindRef(ResourceType);
}

int32 FAIEconomyManager::GetHarvesterCount() const
{
    return HarvesterCount;
}

int32 FAIEconomyManager::GetRefineryCount() const
{
    return RefineryCount;
}

int32 FAIEconomyManager::GetPowerPlantCount() const
{
    return PowerPlantCount;
}

bool FAIEconomyManager::IsPowerPositive() const
{
    float Power = ResourceAmounts.FindRef(FGameplayTag::RequestGameplayTag("Resource.Power"));
    float PowerCapacity = ResourceAmounts.FindRef(FGameplayTag::RequestGameplayTag("Resource.PowerCapacity"));
    return Power < PowerCapacity;
}

float FAIEconomyManager::GetPowerRatio() const
{
    float Power = ResourceAmounts.FindRef(FGameplayTag::RequestGameplayTag("Resource.Power"));
    float PowerCapacity = ResourceAmounts.FindRef(FGameplayTag::RequestGameplayTag("Resource.PowerCapacity"));
    if (PowerCapacity <= 0.0f) return 1.0f;
    return Power / PowerCapacity;
}

void FAIEconomyManager::OnHarvesterBuilt()
{
    HarvesterCount++;
    RecalculateIncome();
}

void FAIEconomyManager::OnHarvesterLost()
{
    HarvesterCount = FMath::Max(0, HarvesterCount - 1);
    RecalculateIncome();
}

void FAIEconomyManager::OnRefineryBuilt()
{
    RefineryCount++;
    RecalculateIncome();
}

void FAIEconomyManager::OnRefineryLost()
{
    RefineryCount = FMath::Max(0, RefineryCount - 1);
    RecalculateIncome();
}

void FAIEconomyManager::OnPowerPlantBuilt()
{
    PowerPlantCount++;
    float PowerPerPlant = 100.0f;
    float& Capacity = ResourceAmounts.FindOrAdd(FGameplayTag::RequestGameplayTag("Resource.PowerCapacity"));
    Capacity += PowerPerPlant;
}

void FAIEconomyManager::OnPowerPlantLost()
{
    PowerPlantCount = FMath::Max(0, PowerPlantCount - 1);
    float PowerPerPlant = 100.0f;
    float& Capacity = ResourceAmounts.FindOrAdd(FGameplayTag::RequestGameplayTag("Resource.PowerCapacity"));
    Capacity = FMath::Max(0.0f, Capacity - PowerPerPlant);
}

void FAIEconomyManager::OnStructureBuilt(const FGameplayTag& StructureTag)
{
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Refinery")))
    {
        OnRefineryBuilt();
    }
    else if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.PowerPlant")))
    {
        OnPowerPlantBuilt();
    }
    else if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Barracks")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.WarFactory")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Airfield")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.TechCenter")))
    {
        AddExpense(FGameplayTag::RequestGameplayTag("Resource.Power"), 50.0f);
    }
}

void FAIEconomyManager::OnStructureDestroyed(const FGameplayTag& StructureTag)
{
    if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Refinery")))
    {
        OnRefineryLost();
    }
    else if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.PowerPlant")))
    {
        OnPowerPlantLost();
    }
    else if (StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Barracks")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.WarFactory")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.Airfield")) ||
             StructureTag.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.TechCenter")))
    {
        AddExpense(FGameplayTag::RequestGameplayTag("Resource.Power"), -50.0f);
    }
}

void FAIEconomyManager::RecalculateIncome()
{
    float OrePerHarvesterPerSecond = 25.0f;
    float RefineryEfficiency = FMath::Min(1.0f, float(HarvesterCount) / FMath::Max(1, RefineryCount * 2));
    IncomeRates[FGameplayTag::RequestGameplayTag("Resource.Ore")] = HarvesterCount * OrePerHarvesterPerSecond * RefineryEfficiency;
    IncomeRates[FGameplayTag::RequestGameplayTag("Resource.Credits")] = IncomeRates.FindRef(FGameplayTag::RequestGameplayTag("Resource.Ore")) * 0.5f;
}

void FAIEconomyManager::RecalculateExpenses()
{
    TotalExpenses = 0.0f;
    for (auto& Pair : ExpenseRates)
    {
        TotalExpenses += Pair.Value;
    }
}

void FAIEconomyManager::RequestExpansion(const FVector& Location, float Priority)
{
    PendingExpansions.Add(Location);
    UE_LOG(LogTemp, Log, TEXT("RAAI Economy: Expansion requested at %s (Priority: %.1f)"), *Location.ToString(), Priority);
}

bool FAIEconomyManager::HasPendingExpansion() const
{
    return PendingExpansions.Num() > 0;
}

FVector FAIEconomyManager::GetNextExpansionLocation() const
{
    if (PendingExpansions.Num() > 0)
    {
        return PendingExpansions[0];
    }
    return FVector::ZeroVector;
}

void FAIEconomyManager::OnExpansionCompleted(const FVector& Location)
{
    PendingExpansions.Remove(Location);
}