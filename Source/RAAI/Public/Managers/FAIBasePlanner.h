#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "FAIBasePlanner.generated.h"

class AAIDirector;
class AActor;

USTRUCT(BlueprintType)
struct FBuildSpot
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    float Radius = 200.0f;

    UPROPERTY()
    TArray<FGameplayTag> AllowedStructures;

    UPROPERTY()
    bool bIsOccupied = false;

    UPROPERTY()
    AActor* OccupyingStructure = nullptr;

    UPROPERTY()
    float SuitabilityScore = 0.0f;

    UPROPERTY()
    TArray<FGameplayTag> SuitableFor;

    UPROPERTY()
    float Score = 1.0f;

    bool operator==(const FBuildSpot& Other) const
    {
        return Location.Equals(Other.Location);
    }
};

USTRUCT(BlueprintType)
struct FBuildOrder
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag StructureTag;

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FVector PreferredLocation;

    UPROPERTY()
    float Priority = 1.0f;

    UPROPERTY()
    bool bIsUrgent = false;

    UPROPERTY()
    float RequestTime = 0.0f;
};

class RAAI_API FAIBasePlanner
{
public:
    FAIBasePlanner();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();
    void Update(float DeltaTime);

    void RequestStructure(const FGameplayTag& StructureTag, float Priority, const FVector& PreferredLocation = FVector::ZeroVector);
    void CancelStructureRequest(const FGameplayTag& StructureTag);
    void OnStructureBuilt(AActor* Structure, const FGameplayTag& StructureTag);
    void OnStructureDestroyed(AActor* Structure, const FGameplayTag& StructureTag);

    bool HasStructure(const FGameplayTag& StructureTag) const;
    int32 GetStructureCount(const FGameplayTag& StructureTag) const;
    float GetStructureCost(const FGameplayTag& StructureTag) const;
    TArray<AActor*> GetStructures(const FGameplayTag& StructureTag) const;

    TArray<FBuildSpot> GetAvailableBuildSpots(const FGameplayTag& StructureTag) const;
    FVector FindBestBuildLocation(const FGameplayTag& StructureTag) const;

    bool HasAvailableOreField() const;
    bool HasUndefendedKeyLocation() const;

    void RegisterBuildSpot(const FBuildSpot& Spot);
    void ClearBuildSpot(const FVector& Location);

    void SetBaseCenter(const FVector& Center);
    FVector GetBaseCenter() const;

    float GetBuildQueueProgress(const FGameplayTag& StructureTag) const;

private:
    AAIDirector* Director = nullptr;

    TMap<FGameplayTag, TArray<AActor*>> StructuresByType;
    TMap<FGameplayTag, int32> StructureCounts;

    TArray<FBuildSpot> BuildSpots;
    TArray<FBuildOrder> BuildQueue;

    FVector BaseCenter = FVector::ZeroVector;
    float BaseRadius = 1500.0f;

    float UpdateTimer = 0.0f;
    float UpdateInterval = 2.0f;

    void GenerateBuildSpots();
    void ProcessBuildQueue(float DeltaTime);
    float EvaluateBuildSpot(const FBuildSpot& Spot, const FGameplayTag& StructureTag) const;
    bool CanBuildAt(const FVector& Location, const FGameplayTag& StructureTag) const;
    FVector FindNearestBuildSpot(const FVector& PreferredLocation, const FGameplayTag& StructureTag) const;
    void UpdateBuildSpotScores();
};