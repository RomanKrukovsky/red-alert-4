// Copyright (c) Red Alert 4 project. In-Game Cheat Console Widget.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4CheatConsoleWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheatExecuted, const FString&, Message);

UCLASS()
class REDALERT4_API URA4CheatConsoleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URA4CheatConsoleWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "RA4|CheatConsole")
    bool ExecuteCommandText(const FString& InCommandText);

    UFUNCTION(BlueprintCallable, Category = "RA4|CheatConsole")
    void AddLogLine(const FString& Line);

    UFUNCTION(BlueprintCallable, Category = "RA4|CheatConsole")
    const TArray<FString>& GetCommandHistory() const { return HistoryLines; }

    UPROPERTY(BlueprintAssignable, Category = "RA4|CheatConsole")
    FOnCheatExecuted OnCheatExecuted;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|CheatConsole")
    TArray<FString> HistoryLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|CheatConsole")
    int32 MaxHistoryLines = 50;
};
