// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRA4IntelModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
