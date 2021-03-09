// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineMinimal.h"

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
