// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RA4ViewModelBase.h"
#include "RA4UIViewModelRegistry.generated.h"

/**
 * Registry and lifecycle manager for UI ViewModels.
 * Allows decoupling UI widgets, presenters, and HUD components from ViewModel instantiation logic.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4UIViewModelRegistry : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Registers an existing ViewModel instance under its class type and optional instance tag. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Registry")
    void RegisterViewModel(URA4ViewModelBase* ViewModel, FName InstanceName = NAME_None);

    /** Resolves a registered ViewModel instance by class type and optional instance tag. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Registry")
    URA4ViewModelBase* GetViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName = NAME_None) const;

    /** Retrieves an existing ViewModel instance or constructs a new one if not yet registered. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Registry")
    URA4ViewModelBase* GetOrCreateViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName = NAME_None);

    /** Removes a registered ViewModel. Returns true if an entry was removed. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Registry")
    bool UnregisterViewModel(TSubclassOf<URA4ViewModelBase> ViewModelClass, FName InstanceName = NAME_None);

    /** Clears all registered ViewModels. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Registry")
    void ClearRegistry();

    // --- C++ Templated Helper Methods ---
    template <typename T>
    T* GetViewModelOfClass(FName InstanceName = NAME_None) const
    {
        return Cast<T>(GetViewModel(T::StaticClass(), InstanceName));
    }

    template <typename T>
    T* GetOrCreateViewModelOfClass(FName InstanceName = NAME_None)
    {
        return Cast<T>(GetOrCreateViewModel(T::StaticClass(), InstanceName));
    }

private:
    FString MakeRegistryKey(TSubclassOf<URA4ViewModelBase> Class, FName InstanceName) const;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<URA4ViewModelBase>> RegistryMap;
};
