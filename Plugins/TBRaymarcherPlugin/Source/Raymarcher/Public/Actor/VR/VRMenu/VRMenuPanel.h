// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "EngineMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

#include "VRMenuPanel.generated.h"

/**
 * Base class for motion controllers.
 */
UCLASS(Abstract)
class AVRMenuPanel : public AActor
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	AVRMenuPanel();
	
	/// Mesh serving as a menu backgrounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent;

	/// Actual menu.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* WidgetComponent;
};
