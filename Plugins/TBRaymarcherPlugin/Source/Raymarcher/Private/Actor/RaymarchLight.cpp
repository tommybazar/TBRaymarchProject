// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/RaymarchLight.h"

#include "Actor/RaymarchVolume.h"

ARaymarchLight::ARaymarchLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Tick at the end of frame, so that previous parameters don't get overwritten until then.
	PrimaryActorTick.TickGroup = TG_LastDemotable;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light Static Mesh Componenet"));
	SetRootComponent(StaticMeshComponent);
}

void ARaymarchLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PreviousTickParameters = GetCurrentParameters();
}

FDirLightParameters ARaymarchLight::GetCurrentParameters() const
{
	return FDirLightParameters(this->GetActorForwardVector(), LightIntensity);
}

#if WITH_EDITOR

bool ARaymarchLight::ShouldTickIfViewportsOnly() const
{
	return true;
}

#endif