// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Actor/VR/VRMotionController.h"
#include "XRMotionControllerBase.h"
#include "Components/WidgetInteractionComponent.h"

AVRMotionController::AVRMotionController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	MotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionControllerComponent->SetupAttachment(RootComponent);
	MotionControllerComponent->SetRelativeLocation(FVector(0, 0, 0));

	ControllerSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ControllerMesh"));
	ControllerSkeletalMeshComponent->SetupAttachment(MotionControllerComponent);
	ControllerSkeletalMeshComponent->SetCanEverAffectNavigation(false);
	ControllerSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WidgetInteractor = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteractor"));
	WidgetInteractor->SetupAttachment(RootComponent);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	CollisionComponent->SetupAttachment(ControllerSkeletalMeshComponent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionProfileName("WorldDynamic");
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AVRMotionController::OnOverlapBegin);
	CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AVRMotionController::OnOverlapEnd);
}

void AVRMotionController::Tick(float DeltaTime)
{
}

void AVRMotionController::SetupInput(UInputComponent* InInputComponent)
{
	if (bIsInRightHand)
	{
		MotionControllerComponent->SetTrackingMotionSource(FXRMotionControllerBase::RightHandSourceId);

		InInputComponent->BindAction("Right_Grip", IE_Pressed, this, &AVRMotionController::OnGripPressed);
		InInputComponent->BindAction("Right_Grip", IE_Released, this, &AVRMotionController::OnGripReleased);
		InInputComponent->BindAction("Right_Trigger", IE_Pressed, this, &AVRMotionController::OnTriggerPressed);

		InInputComponent->BindAxis("Right_Grip_Axis",  this, &AVRMotionController::OnGripAxis);
		InInputComponent->BindAxis("Right_Trigger_Axis",  this, &AVRMotionController::OnTriggerAxis);
	}
	else
	{
		MotionControllerComponent->SetTrackingMotionSource(FXRMotionControllerBase::LeftHandSourceId);

		InInputComponent->BindAction("Left_Grip", IE_Pressed, this, &AVRMotionController::OnGripPressed);
		InInputComponent->BindAction("Left_Grip", IE_Released, this, &AVRMotionController::OnGripReleased);
		InInputComponent->BindAction("Left_Trigger", IE_Pressed, this, &AVRMotionController::OnTriggerPressed);

		InInputComponent->BindAxis("Left_Grip_Axis", this, &AVRMotionController::OnGripAxis);
		InInputComponent->BindAxis("Left_Trigger_Axis", this, &AVRMotionController::OnTriggerAxis);
	}
}

void AVRMotionController::OnGripPressed()
{
	if (HoveredActor)
	{
		HoveredActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		GrabbedActor = HoveredActor;
		HoveredActor = nullptr;
	}
}

void AVRMotionController::OnGripReleased()
{
	if (GrabbedActor)
	{
		GrabbedActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
		HoveredActor = GrabbedActor;
		GrabbedActor = nullptr;
	}
}

void AVRMotionController::OnTriggerAxis(float Axis)
{
}

void AVRMotionController::OnTriggerPressed()
{
	if (WidgetInteractor)
	{
		WidgetInteractor->PressPointerKey(EKeys::LeftMouseButton);
	}
}

void AVRMotionController::OnGripAxis(float Axis)
{
}

void AVRMotionController::OnOverlapBegin(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HoveredActor = OtherActor;
}

void AVRMotionController::OnOverlapEnd(
	class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (HoveredActor == OtherActor)
	{
		HoveredActor = nullptr;
	}
}

void AVRMotionController::OnActorHovered()
{
}

void AVRMotionController::BeginPlay()
{
}
