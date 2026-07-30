#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FRAAIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FRAAIModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FRAAIModule>("RAAI");
    }

    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("RAAI");
    }
};