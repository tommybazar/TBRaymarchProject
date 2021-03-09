// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/VR/VRPawn.h"

#include "Actor/VR/VRMotionController.h"
#include "HeadMountedDisplayFunctionLibrary.h"

AVRPawn::AVRPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	HMDScene = CreateDefaultSubobject<USceneComponent>(TEXT("VR HMD Scene"));
	SetRootComponent(HMDScene);

	VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VR Camera"));
	VRCamera->SetupAttachment(HMDScene);
}

void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	EVRPlatform VRPlatformType = EVRPlatform::Default;
	FControllerPlatformClasses PlatformClasses;

	if (PerPlatformControllers.Contains(VRPlatformType))
	{
		PlatformClasses = PerPlatformControllers[VRPlatformType];
	}

	if (PlatformClasses.LeftControllerClass && PlatformClasses.RightControllerClass)
	{
		RightController = GetWorld()->SpawnActor<AVRMotionController>(PlatformClasses.RightControllerClass, SpawnParams);
		LeftController = GetWorld()->SpawnActor<AVRMotionController>(PlatformClasses.LeftControllerClass, SpawnParams);

		RightController->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

		RightController->SetupInput(InputComponent);
		LeftController->SetupInput(InputComponent);
	}
}

void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
