// Copyright (c) Red Alert 4 project. Commandlet for automated content bible import.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4ContentImportCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4ContentImportCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4ContentImportCommandlet();

    virtual int32 Main(const FString& Params) override;
};
