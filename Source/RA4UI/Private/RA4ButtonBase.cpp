// Copyright (c) Red Alert 4 project.

#include "RA4ButtonBase.h"

URA4ButtonBase::URA4ButtonBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URA4ButtonBase::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Additional button styling setup can go here
}
