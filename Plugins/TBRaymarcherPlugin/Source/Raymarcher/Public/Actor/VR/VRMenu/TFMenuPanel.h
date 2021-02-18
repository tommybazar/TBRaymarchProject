// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Actor/VR/VRMenu/VRMenuPanel.h"

#include "TFMenuPanel.generated.h"

class UTransferFuncMenu;
class ARaymarchVolume;

/**
 * Base class for motion controllers.
 */
UCLASS(Abstract)
class ATFMenuPanel : public AVRMenuPanel
{
	GENERATED_BODY()
public:

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UTransferFuncMenu> TransferFuncMenuClass;

	UTransferFuncMenu* TransferFuncMenu;

	UPROPERTY(EditAnywhere)
	ARaymarchVolume* ProviderVolume;

	UPROPERTY(EditAnywhere)
	TArray<ARaymarchVolume*> ListenerVolumes;
};
