#include "Actor/VR/VRPawn.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Actor/VR/VRMotionController.h"


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

	if (LeftControllerClass && RightControllerClass)
	{
		RightController = GetWorld()->SpawnActor<AVRMotionController>(RightControllerClass, SpawnParams);
		LeftController = GetWorld()->SpawnActor<AVRMotionController>(LeftControllerClass, SpawnParams);

		FAttachmentTransformRules Rules(
			EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false);

		RightController->AttachToComponent(RootComponent, Rules);
		LeftController->AttachToComponent(RootComponent, Rules);
	
		RightController->SetupInput(InputComponent);
		LeftController->SetupInput(InputComponent);
	}
}

void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
