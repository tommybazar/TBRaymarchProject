// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Actor/RaymarchLight.h"

#include "Actor/RaymarchVolume.h"

ARaymarchLight::ARaymarchLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Tick at the end of frame, so that previous parameters don't get overwritten until then.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light Static Mesh Componenet"));
	StaticMeshComponent->SetupAttachment(RootComponent);
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