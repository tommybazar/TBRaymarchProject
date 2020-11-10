// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Rendering/RaymarchTypes.h"

#include "RaymarchClipPlane.generated.h"

class ARaymarchVolume;

UCLASS()
class RAYMARCHER_API ARaymarchClipPlane : public AActor
{
	GENERATED_BODY()

public:
	/// Default constructor.
	ARaymarchClipPlane();

	/// Static mesh component to visualize the clipping plane.
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StaticMeshComponent;

	/// Gets current position and Up vector
	FClippingPlaneParameters GetCurrentParameters() const;
};