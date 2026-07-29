// Copyright (c) Red Alert 4 project. Native C++ Asset Importer Commandlet.
#include "RA4ContentImportCommandlet.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "RA4DataAssets.h"

URA4ContentImportCommandlet::URA4ContentImportCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URA4ContentImportCommandlet::Main(const FString& Params)
{
    UE_LOG(LogTemp, Display, TEXT("=== RA4 Content Import Commandlet Starting ==="));

    FString PythonScriptPath = FPaths::ProjectDir() / TEXT("Tools/Editor/parse_content_bible.py");
    FString Executable = TEXT("/opt/homebrew/bin/python3");
    if (!FPaths::FileExists(Executable))
    {
        Executable = TEXT("/usr/bin/python3");
    }
    if (!FPaths::FileExists(Executable))
    {
        Executable = TEXT("python3");
    }
    FString Arguments = FString::Printf(TEXT("\"%s\""), *PythonScriptPath);

    int32 ReturnCode = -1;
    FString Output;
    FString ErrorOutput;

    bool bSuccess = FPlatformProcess::ExecProcess(*Executable, *Arguments, &ReturnCode, &Output, &ErrorOutput);

    if (!bSuccess || ReturnCode != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Python Bible Parser Failed (Exit Code %d):\n%s\n%s"), ReturnCode, *Output, *ErrorOutput);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Python Bible Parser Succeeded!\n%s"), *Output);

    // Read normalized JSON
    FString JsonPath = FPaths::ProjectDir() / TEXT("Content/RA4/Data/Generated/ra4_content.normalized.json");
    FString JsonRaw;
    if (!FFileHelper::LoadFileToString(JsonRaw, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load normalized JSON at %s"), *JsonPath);
        return 1;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonRaw);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize normalized JSON!"));
        return 1;
    }

    int32 UnitsCreated = 0;
    int32 BuildingsCreated = 0;
    int32 AbilitiesCreated = 0;
    int32 WeaponsCreated = 0;

    // 1. Generate 78 Unit Data Assets
    const TArray<TSharedPtr<FJsonValue>>* UnitsArray;
    if (RootObject->TryGetArrayField(TEXT("units"), UnitsArray))
    {
        for (const TSharedPtr<FJsonValue>& UnitValue : *UnitsArray)
        {
            TSharedPtr<FJsonObject> UnitObj = UnitValue->AsObject();
            if (!UnitObj.IsValid()) continue;

            FString UnitId = UnitObj->GetStringField(TEXT("id"));
            FString UnitName = UnitObj->GetStringField(TEXT("name_ru"));
            int32 Cost = UnitObj->GetIntegerField(TEXT("cost"));
            int32 BuildTime = UnitObj->GetIntegerField(TEXT("build_time_sec"));
            int32 CommandLimit = UnitObj->GetIntegerField(TEXT("command_limit"));
            int32 HP = UnitObj->GetIntegerField(TEXT("hp"));
            FString ArmorType = UnitObj->GetStringField(TEXT("armor_type"));
            double Speed = UnitObj->GetNumberField(TEXT("speed"));
            double TargetDPS = UnitObj->GetNumberField(TEXT("target_dps"));

            FString PackagePath = TEXT("/Game/RA4/Data/Generated/Units/DA_Unit_") + UnitId;
            UPackage* Package = CreatePackage(*PackagePath);
            Package->FullyLoad();

            URA4UnitDefinition* UnitAsset = NewObject<URA4UnitDefinition>(Package, *FString(TEXT("DA_Unit_") + UnitId), RF_Public | RF_Standalone);
            if (UnitAsset)
            {
                UnitAsset->UnitName = FText::FromString(UnitName);
                UnitAsset->UnitTag = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Unit.%s"), *UnitId)), false);
                UnitAsset->Cost = Cost;
                UnitAsset->BuildTimeSeconds = BuildTime;
                UnitAsset->CommandLimit = CommandLimit;
                UnitAsset->MaxHealth = HP;
                UnitAsset->ArmorTypeTag = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Armor.%s"), *ArmorType)), false);
                UnitAsset->MaxSpeed = static_cast<float>(Speed);
                UnitAsset->TargetDPS = static_cast<float>(TargetDPS);

                UnitAsset->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(UnitAsset);

                FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                SaveArgs.SaveFlags = SAVE_NoError;

                UPackage::SavePackage(Package, UnitAsset, *PackageFileName, SaveArgs);
                UnitsCreated++;
            }
        }
    }

    // 2. Generate Building Data Assets
    const TArray<TSharedPtr<FJsonValue>>* BuildingsArray;
    if (RootObject->TryGetArrayField(TEXT("buildings"), BuildingsArray))
    {
        for (const TSharedPtr<FJsonValue>& BuildingValue : *BuildingsArray)
        {
            TSharedPtr<FJsonObject> BuildingObj = BuildingValue->AsObject();
            if (!BuildingObj.IsValid()) continue;

            FString BuildingId = BuildingObj->GetStringField(TEXT("id"));
            FString BuildingName = BuildingObj->GetStringField(TEXT("name_ru"));
            int32 Cost = BuildingObj->GetIntegerField(TEXT("cost"));
            int32 BuildTime = BuildingObj->GetIntegerField(TEXT("build_time_sec"));
            int32 PowerOutput = BuildingObj->GetIntegerField(TEXT("power_produced"));
            int32 PowerConsumed = BuildingObj->GetIntegerField(TEXT("power_consumed"));
            int32 CommandLimitProvided = BuildingObj->GetIntegerField(TEXT("command_limit_provided"));

            FString PackagePath = TEXT("/Game/RA4/Data/Generated/Buildings/DA_Building_") + BuildingId;
            UPackage* Package = CreatePackage(*PackagePath);
            Package->FullyLoad();

            URA4BuildingDefinition* BuildingAsset = NewObject<URA4BuildingDefinition>(Package, *FString(TEXT("DA_Building_") + BuildingId), RF_Public | RF_Standalone);
            if (BuildingAsset)
            {
                BuildingAsset->BuildingName = FText::FromString(BuildingName);
                BuildingAsset->BuildingTag = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Building.%s"), *BuildingId)), false);
                BuildingAsset->Cost = Cost;
                BuildingAsset->BuildTimeSeconds = BuildTime;
                BuildingAsset->PowerOutput = PowerOutput;
                BuildingAsset->PowerConsumption = PowerConsumed;
                BuildingAsset->CommandLimitProvided = CommandLimitProvided;

                BuildingAsset->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(BuildingAsset);

                FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                SaveArgs.SaveFlags = SAVE_NoError;

                UPackage::SavePackage(Package, BuildingAsset, *PackageFileName, SaveArgs);
                BuildingsCreated++;
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("=== Native C++ Asset Importer Completed Successfully! ==="));
    UE_LOG(LogTemp, Display, TEXT("Created %d Unit .uasset Primary Data Assets"), UnitsCreated);
    UE_LOG(LogTemp, Display, TEXT("Created %d Building .uasset Primary Data Assets"), BuildingsCreated);

    return 0;
}
