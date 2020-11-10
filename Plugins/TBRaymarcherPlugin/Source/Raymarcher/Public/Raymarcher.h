// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRaymarcherModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
