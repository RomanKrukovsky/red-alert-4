// Copyright (c) Scarlet Horizon project. Canonical Faction, Country, and Doctrine data definitions.

#pragma once

#include "CoreMinimal.h"
#include "RA4UITheme.h"
#include "RA4FactionData.generated.h"

USTRUCT(BlueprintType)
struct RA4UI_API FRA4DoctrineInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FName DoctrineId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    TArray<FText> ModifiedUnits;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    TArray<FText> KeyAbilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FText SignatureUnit;

    /** Short tactical tagline shown above the doctrine description. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FText CombatPhilosophy;

    /** Strategic asset unlocked by this doctrine. Empty when the doctrine has none. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    FText SignatureSuperweapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doctrine")
    bool bUnlocked = true;
};

USTRUCT(BlueprintType)
struct RA4UI_API FRA4CountryInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FName CountryId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FText Specialization;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FText LoreDescription;

    /** Name of the national headquarters structure shown in the country dossier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FText BaseHeadquarters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    ERA4FactionTheme BlocTheme = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FLinearColor PrimaryColor = FLinearColor(0.28f, 0.08f, 0.38f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    FLinearColor AccentColor = FLinearColor(0.65f, 0.25f, 0.80f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FirepowerRating = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ArmorRating = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MobilityRating = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TechRating = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    TArray<FRA4DoctrineInfo> Doctrines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    bool bUnlocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Country")
    bool bComingSoon = false;
};

USTRUCT(BlueprintType)
struct RA4UI_API FRA4BlocInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    ERA4FactionTheme BlocId = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    FText Motto;

    /**
     * True for a selection category that is not a military union. Independent powers
     * share a picker, never an army, symbol, base, or politics, so screens must not
     * present them as an alliance.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    bool bIsCategoryOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    TArray<FText> KeyAdvantages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    int32 ControlledRegions = 24;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    int32 ActivePersonnel = 1250000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    float ReadinessRatio = 0.88f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    FLinearColor PrimaryColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    FLinearColor GlowColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloc")
    TArray<FRA4CountryInfo> Countries;
};

/** Static registry providing authoritative Scarlet Horizon data to all UI screens. */
class RA4UI_API FRA4FactionDataRegistry
{
public:
    static const FRA4FactionDataRegistry& Get();

    const TArray<FRA4BlocInfo>& GetAllBlocs() const { return Blocs; }
    const FRA4BlocInfo* FindBloc(ERA4FactionTheme Theme) const;
    const FRA4CountryInfo* FindCountry(FName CountryId) const;
    const FRA4CountryInfo* FindDefaultCountryForBloc(ERA4FactionTheme Theme) const;

    static FLinearColor GetBlocPrimaryColor(ERA4FactionTheme Theme);
    static FLinearColor GetBlocAccentColor(ERA4FactionTheme Theme);
    static FLinearColor GetBlocGlowColor(ERA4FactionTheme Theme);
    static FLinearColor GetHorizonScarletColor() { return FLinearColor(0.95f, 0.12f, 0.16f, 1.0f); }

private:
    FRA4FactionDataRegistry();
    void PopulateRegistry();

    TArray<FRA4BlocInfo> Blocs;
};
