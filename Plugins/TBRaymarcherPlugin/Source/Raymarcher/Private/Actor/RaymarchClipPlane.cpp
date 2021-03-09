// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/RaymarchClipPlane.h"

#include "Actor/RaymarchVolume.h"

ARaymarchClipPlane::ARaymarchClipPlane()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Clip Plane Static Mesh Component"));
	SetRootComponent(StaticMeshComponent);

	// static ConstructorHelpers::FObjectFinder<UStaticMesh> Arrow(TEXT("/Engine/VREditor/TransformGizmo/TranslateArrowHandle"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/ArtTools/RenderToTexture/Meshes/S_1_Unit_Plane"));

	StaticMeshComponent->SetStaticMesh(Plane.Object);
	StaticMeshComponent->SetRelativeScale3D(FVector(200, 200, 200));
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	StaticMeshComponent->SetVisibility(true);
	static ConstructorHelpers::FObjectFinder<UMaterial> CutPlaneMaterial(TEXT("/TBRaymarcherPlugin/Materials/M_CuttingPlane"));

	if (CutPlaneMaterial.Object)
	{
		StaticMeshComponent->SetMaterial(0, CutPlaneMaterial.Object);
	}
}

FClippingPlaneParameters ARaymarchClipPlane::GetCurrentParameters() const
{
	return FClippingPlaneParameters(this->GetActorLocation(), -this->GetActorUpVector());
}
