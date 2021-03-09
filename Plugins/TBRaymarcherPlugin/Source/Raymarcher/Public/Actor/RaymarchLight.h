// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Rendering/RaymarchTypes.h"
#include "VR/Grabbable.h"

#include "RaymarchLight.generated.h"

class ARaymarchVolume;

UCLASS()
class RAYMARCHER_API ARaymarchLight : public AActor, public IGrabbable
{
	GENERATED_BODY()

public:
	ARaymarchLight();

	virtual void Tick(float DeltaSeconds) override;

	FDirLightParameters GetCurrentParameters() const;

	FDirLightParameters PreviousTickParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LightIntensity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMeshComponent* StaticMeshComponent;

#if WITH_EDITOR
	// Override ShouldTickIfViewportsOnly to return true, so this also ticks in editor viewports.
	virtual bool ShouldTickIfViewportsOnly() const override;

#endif
};