#include "RAAIModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMEFRAME "FRAAIModule"

void FRAAIModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("RAAI Module Started - Red Alert AI System Initialized"));
}

void FRAAIModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("RAAI Module Shutdown"));
}

#undef LOCTEXT_NAMEFRAME

IMPLEMENT_MODULE(FRAAIModule, RAAI)