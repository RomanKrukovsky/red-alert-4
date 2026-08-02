// Copyright (c) Red Alert 4 project.

#include "RA4UIViewModelRegistry.h"

void URA4UIViewModelRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RegistryMap.Reset();
}

void URA4UIViewModelRegistry::Deinitialize()
{
    RegistryMap.Reset();
    Super::Deinitialize();
}

FString URA4UIViewModelRegistry::MakeRegistryKey(TSubclassOf<URA4ViewModelBase> Class, FName InstanceName) const
{
    if (!Class)
    {
        return FString();
    }
    return FString::Printf(TEXT("%s_%s"), *Class->GetName(), *InstanceName.ToString());
}

void URA4UIViewModelRegistry::RegisterViewModel(URA4ViewModelBase* ViewModel, FName InstanceName)
{
    if (!ViewModel)
    {
        return;
    }

    FString Key = MakeRegistryKey(ViewModel->GetClass(), InstanceName);
    if (!Key.IsEmpty())
    {
        RegistryMap.Add(Key, ViewModel);
    }
}

URA4ViewModelBase* URA4UIViewModelRegistry::GetViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName) const
{
    if (!ViewModelClass)
    {
        return nullptr;
    }

    FString Key = MakeRegistryKey(ViewModelClass, InstanceName);
    if (const TObjectPtr<URA4ViewModelBase>* Found = RegistryMap.Find(Key))
    {
        return *Found;
    }

    return nullptr;
}

URA4ViewModelBase* URA4UIViewModelRegistry::GetOrCreateViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName)
{
    if (!ViewModelClass)
    {
        return nullptr;
    }

    if (URA4ViewModelBase* Existing = GetViewModel(ViewModelClass, InstanceName))
    {
        return Existing;
    }

    URA4ViewModelBase* NewViewModel = NewObject<URA4ViewModelBase>(this, ViewModelClass);
    RegisterViewModel(NewViewModel, InstanceName);
    return NewViewModel;
}

bool URA4UIViewModelRegistry::UnregisterViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName)
{
    if (!ViewModelClass)
    {
        return false;
    }

    FString Key = MakeRegistryKey(ViewModelClass, InstanceName);
    return RegistryMap.Remove(Key) > 0;
}

void URA4UIViewModelRegistry::ClearRegistry()
{
    RegistryMap.Reset();
}
