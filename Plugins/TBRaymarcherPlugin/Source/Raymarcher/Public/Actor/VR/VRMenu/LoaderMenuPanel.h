// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Actor/VR/VRMenu/VRMenuPanel.h"

#include "LoaderMenuPanel.generated.h"

class ARaymarchVolume;
class UVolumeLoadMenu;

/**
 * Base class for motion controllers.
 */
UCLASS(Abstract)
class ALoaderMenuPanel : public AVRMenuPanel
{
	GENERATED_BODY()
public:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UVolumeLoadMenu> LoaderMenuClass;

	UVolumeLoadMenu* LoaderMenu;
		
	UPROPERTY(EditAnywhere)
	TArray<ARaymarchVolume*> ListenerVolumes;
};
