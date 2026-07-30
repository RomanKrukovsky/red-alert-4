// Copyright (c) Red Alert 4 project.
//
// CommonUI routes input through its own action router, and that router refuses to
// work unless the viewport client derives from UCommonGameViewportClient -- without
// it the log carries "CommonUI Input routing will not function correctly" and widget
// focus, gamepad navigation and back-handling silently misbehave. This class exists
// only to satisfy that requirement; it adds no behaviour of its own.
#pragma once

#include "CoreMinimal.h"
#include "CommonGameViewportClient.h"

#include "RA4GameViewportClient.generated.h"

UCLASS()
class RA4UI_API URA4GameViewportClient : public UCommonGameViewportClient
{
    GENERATED_BODY()
};
