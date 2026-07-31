// Copyright (c) Red Alert 4 project.
#include "RA4AudioImportCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/SoundFactory.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
// Where the raw audio lives on disk, and the package path each source tree imports
// into. Unit voice lines sit outside Content/ entirely (they are generated output,
// kept out of the asset tree), so the mapping is explicit rather than derived.
struct FAudioSourceTree
{
    const TCHAR* SourceDirRelativeToProject;
    const TCHAR* DestPackageRoot;
};

const FAudioSourceTree GAudioTrees[] = {
    {TEXT("Audio/Voice/RU/Soviet/Runtime"), TEXT("/Game/RA4/Audio/Generated/Voice/Soviet")},
    {TEXT("Content/RA4/Audio/EVA/Processed"), TEXT("/Game/RA4/Audio/Generated/EVA")},
    {TEXT("Content/RA4/Audio/Music"), TEXT("/Game/RA4/Audio/Generated/Music")},
};

// Unreal object names cannot contain most punctuation; source files are named things
// like VO_RU_SU_RubezhRifleman_Selected_01.wav, which is already safe, but music and
// EVA files are not guaranteed to be.
FString MakeAssetName(const FString& BaseFileName)
{
    FString Name = BaseFileName;
    for (TCHAR& C : Name)
    {
        const bool bAllowed = FChar::IsAlnum(C) || C == TEXT('_');
        if (!bAllowed)
        {
            C = TEXT('_');
        }
    }
    if (Name.Len() > 0 && FChar::IsDigit(Name[0]))
    {
        Name = TEXT("SW_") + Name;
    }
    return Name;
}
} // namespace

URA4AudioImportCommandlet::URA4AudioImportCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URA4AudioImportCommandlet::Main(const FString& Params)
{
    const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

    int32 TotalImported = 0;
    int32 TotalSkipped = 0;
    int32 TotalFailed = 0;

    for (const FAudioSourceTree& Tree : GAudioTrees)
    {
        const FString SourceDir = FPaths::Combine(ProjectDir, Tree.SourceDirRelativeToProject);
        if (!IFileManager::Get().DirectoryExists(*SourceDir))
        {
            UE_LOG(LogTemp, Warning, TEXT("RA4AudioImport: source directory missing, skipped: %s"), *SourceDir);
            continue;
        }

        TArray<FString> WavFiles;
        IFileManager::Get().FindFilesRecursive(WavFiles, *SourceDir, TEXT("*.wav"), /*Files*/ true,
                                               /*Directories*/ false);
        UE_LOG(LogTemp, Display, TEXT("RA4AudioImport: %d wav files under %s"), WavFiles.Num(), *SourceDir);

        for (const FString& WavPath : WavFiles)
        {
            // Mirror the source folder structure under the destination package root so
            // per-unit directories stay per-unit packages.
            FString RelativePath = WavPath;
            FPaths::MakePathRelativeTo(RelativePath, *(SourceDir + TEXT("/")));
            const FString RelativeDir = FPaths::GetPath(RelativePath);
            const FString AssetName = MakeAssetName(FPaths::GetBaseFilename(WavPath));

            FString PackagePath = FString(Tree.DestPackageRoot);
            if (!RelativeDir.IsEmpty())
            {
                PackagePath += TEXT("/") + RelativeDir;
            }
            const FString FullPackageName = PackagePath + TEXT("/") + AssetName;

            // Idempotent: a rerun must not duplicate or re-encode assets that already
            // exist, because importing a thousand waves is slow.
            if (FPackageName::DoesPackageExist(FullPackageName))
            {
                ++TotalSkipped;
                continue;
            }

            UPackage* Package = CreatePackage(*FullPackageName);
            if (Package == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("RA4AudioImport: cannot create package %s"), *FullPackageName);
                ++TotalFailed;
                continue;
            }
            Package->FullyLoad();

            USoundFactory* Factory = NewObject<USoundFactory>();
            // Headless: there is no one to answer an import prompt.
            Factory->SuppressImportDialogs();
            // One SoundWave per file and nothing else -- a cue per clip would double
            // the asset count for no gain, since playback goes through code.
            Factory->bAutoCreateCue = false;

            bool bCancelled = false;
            UObject* Imported = Factory->ImportObject(USoundWave::StaticClass(), Package, FName(*AssetName),
                                                     RF_Public | RF_Standalone, WavPath, nullptr, bCancelled);
            USoundWave* SoundWave = Cast<USoundWave>(Imported);
            if (SoundWave == nullptr || bCancelled)
            {
                UE_LOG(LogTemp, Error, TEXT("RA4AudioImport: import failed for %s"), *WavPath);
                ++TotalFailed;
                continue;
            }

            SoundWave->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(SoundWave);

            const FString PackageFileName =
                FPackageName::LongPackageNameToFilename(FullPackageName, FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, SoundWave, *PackageFileName, SaveArgs))
            {
                UE_LOG(LogTemp, Error, TEXT("RA4AudioImport: save failed for %s"), *PackageFileName);
                ++TotalFailed;
                continue;
            }

            ++TotalImported;
            if ((TotalImported % 100) == 0)
            {
                UE_LOG(LogTemp, Display, TEXT("RA4AudioImport: %d imported so far"), TotalImported);
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("RA4AudioImport: imported %d, already present %d, failed %d"), TotalImported,
           TotalSkipped, TotalFailed);
    return TotalFailed > 0 ? 1 : 0;
}
