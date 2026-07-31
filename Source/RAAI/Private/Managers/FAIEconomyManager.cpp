#include "FAIEconomyManager.h"
#include "AAIDirector.h"

FAIEconomyManager::FAIEconomyManager()
{
    FResourceInfo CreditsInfo;
    CreditsInfo.ResourceTag = FGameplayTag::RequestGameplayTag("Resource.Credits");
    CreditsInfo.CurrentAmount = 5000.0f;
    CreditsInfo.MaxStorage = 100000.0f;
    Resources.Add(CreditsInfo.ResourceTag, CreditsInfo);
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
        CalculateRates();
        UpdateResourceFlow(UpdateInterval);
        UpdateExpansions();
    }
}

float FAIEconomyManager::GetResourceAmount(const FGameplayTag& ResourceType) const
{
    if (const FResourceInfo* Info = Resources.Find(ResourceType))
    {
        return Info->CurrentAmount;
    }
    return 0.0f;
}

void FAIEconomyManager::AddResource(const FGameplayTag& ResourceType, float Amount)
{
    FResourceInfo& Info = Resources.FindOrAdd(ResourceType);
    Info.ResourceTag = ResourceType;
    Info.CurrentAmount = FMath::Clamp(Info.CurrentAmount + Amount, 0.0f, Info.MaxStorage);
}

void FAIEconomyManager::SpendResource(const FGameplayTag& ResourceType, float Amount)
{
    if (FResourceInfo* Info = Resources.Find(ResourceType))
    {
        Info->CurrentAmount = FMath::Max(0.0f, Info->CurrentAmount - Amount);
    }
}

bool FAIEconomyManager::CanAfford(const FGameplayTag& ResourceType, float Amount) const
{
    return GetResourceAmount(ResourceType) >= Amount;
}

float FAIEconomyManager::GetIncomeRate(const FGameplayTag& ResourceType) const
{
    if (const FResourceInfo* Info = Resources.Find(ResourceType))
    {
        return Info->IncomeRate;
    }
    return 0.0f;
}

float FAIEconomyManager::GetExpenseRate(const FGameplayTag& ResourceType) const
{
    if (const FResourceInfo* Info = Resources.Find(ResourceType))
    {
        return Info->ExpenseRate;
    }
    return 0.0f;
}

void FAIEconomyManager::RegisterProducer(const FGameplayTag& ProducerTag, float ProductionRate)
{
    ProducerRates.Add(ProducerTag, ProductionRate);
}

void FAIEconomyManager::UnregisterProducer(const FGameplayTag& ProducerTag)
{
    ProducerRates.Remove(ProducerTag);
}

void FAIEconomyManager::RegisterConsumer(const FGameplayTag& ConsumerTag, float ConsumptionRate)
{
    ConsumerRates.Add(ConsumerTag, ConsumptionRate);
}

void FAIEconomyManager::UnregisterConsumer(const FGameplayTag& ConsumerTag)
{
    ConsumerRates.Remove(ConsumerTag);
}

void FAIEconomyManager::RequestExpansion(const FVector& PreferredLocation, float Priority)
{
    ExpansionLocations.Add(PreferredLocation);
    ExpansionPriorities.Add(Priority);
}

bool FAIEconomyManager::ShouldExpand() const
{
    return GetResourceAmount(FGameplayTag::RequestGameplayTag("Resource.Credits")) > 10000.0f;
}

void FAIEconomyManager::SetResourceTarget(const FGameplayTag& ResourceType, float TargetAmount)
{
    ResourceTargets.Add(ResourceType, TargetAmount);
}

float FAIEconomyManager::GetResourceTarget(const FGameplayTag& ResourceType) const
{
    return ResourceTargets.FindRef(ResourceType);
}

float FAIEconomyManager::GetTotalIncome() const
{
    float Total = 0.0f;
    for (const auto& Pair : ProducerRates)
    {
        Total += Pair.Value;
    }
    return Total;
}

float FAIEconomyManager::GetTotalExpenses() const
{
    float Total = 0.0f;
    for (const auto& Pair : ConsumerRates)
    {
        Total += Pair.Value;
    }
    return Total;
}

float FAIEconomyManager::GetNetIncome() const
{
    return GetTotalIncome() - GetTotalExpenses();
}

TArray<FResourceInfo> FAIEconomyManager::GetAllResources() const
{
    TArray<FResourceInfo> Result;
    Resources.GenerateValueArray(Result);
    return Result;
}

void FAIEconomyManager::EmergencyEconomyMode(bool bEnable)
{
    bEmergencyMode = bEnable;
}

void FAIEconomyManager::UpdateResourceFlow(float DeltaTime)
{
    FGameplayTag CreditsTag = FGameplayTag::RequestGameplayTag("Resource.Credits");
    float NetDelta = GetNetIncome() * DeltaTime;
    AddResource(CreditsTag, NetDelta);
}

void FAIEconomyManager::UpdateExpansions()
{
}

void FAIEconomyManager::CalculateRates()
{
    FGameplayTag CreditsTag = FGameplayTag::RequestGameplayTag("Resource.Credits");
    if (FResourceInfo* Info = Resources.Find(CreditsTag))
    {
        Info->IncomeRate = GetTotalIncome();
        Info->ExpenseRate = GetTotalExpenses();
    }
}

FVector FAIEconomyManager::FindBestExpansionLocation() const
{
    return ExpansionLocations.Num() > 0 ? ExpansionLocations[0] : FVector::ZeroVector;
}

float FAIEconomyManager::EvaluateExpansionLocation(const FVector& Location) const
{
    return 1.0f;
}