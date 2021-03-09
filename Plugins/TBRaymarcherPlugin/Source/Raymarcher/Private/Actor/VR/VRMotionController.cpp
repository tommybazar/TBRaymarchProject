// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/VR/VRMotionController.h"

#include "Actor/VR/Grabbable.h"
#include "Components/WidgetInteractionComponent.h"
#include "XRMotionControllerBase.h"

AVRMotionController::AVRMotionController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	MotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionControllerComponent->SetupAttachment(RootComponent);
	MotionControllerComponent->SetRelativeLocation(FVector(0, 0, 0));

	ControllerStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ControllerMesh"));
	ControllerStaticMeshComponent->SetupAttachment(MotionControllerComponent);
	ControllerStaticMeshComponent->SetCanEverAffectNavigation(false);
	ControllerStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WidgetInteractor = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteractor"));
	WidgetInteractor->SetupAttachment(ControllerStaticMeshComponent);
	WidgetInteractor->OnHoveredWidgetChanged.AddDynamic(this, &AVRMotionController::OnWidgetInteractorHoverChanged);

	WidgetInteractorVisualizer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WidgetInteractorVisualizer"));
	WidgetInteractorVisualizer->SetupAttachment(ControllerStaticMeshComponent);
	WidgetInteractorVisualizer->SetVisibility(false);
	WidgetInteractorVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	CollisionComponent->SetupAttachment(ControllerStaticMeshComponent);
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
		InInputComponent->BindAction("Right_Trigger", IE_Released, this, &AVRMotionController::OnTriggerReleased);

		InInputComponent->BindAxis("Right_Grip_Axis", this, &AVRMotionController::OnGripAxis);
		InInputComponent->BindAxis("Right_Trigger_Axis", this, &AVRMotionController::OnTriggerAxis);
	}
	else
	{
		MotionControllerComponent->SetTrackingMotionSource(FXRMotionControllerBase::LeftHandSourceId);

		InInputComponent->BindAction("Left_Grip", IE_Pressed, this, &AVRMotionController::OnGripPressed);
		InInputComponent->BindAction("Left_Grip", IE_Released, this, &AVRMotionController::OnGripReleased);
		InInputComponent->BindAction("Left_Trigger", IE_Pressed, this, &AVRMotionController::OnTriggerPressed);
		InInputComponent->BindAction("Left_Trigger", IE_Released, this, &AVRMotionController::OnTriggerReleased);

		InInputComponent->BindAxis("Left_Grip_Axis", this, &AVRMotionController::OnGripAxis);
		InInputComponent->BindAxis("Left_Trigger_Axis", this, &AVRMotionController::OnTriggerAxis);
	}
}

void AVRMotionController::OnGripPressed()
{
	if (HoveredActor)
	{
		HoveredActor->OnGrabbed(ControllerStaticMeshComponent);
		GrabbedActor = HoveredActor;
		HoveredActor = nullptr;
	}
}

void AVRMotionController::OnGripReleased()
{
	if (GrabbedActor)
	{
		GrabbedActor->OnReleased();
		HoveredActor = GrabbedActor;
		GrabbedActor = nullptr;
	}
}

void AVRMotionController::OnTriggerAxis(float Axis)
{
	// Update animation
}

void AVRMotionController::OnTriggerPressed()
{
	if (WidgetInteractor)
	{
		WidgetInteractor->PressPointerKey(EKeys::LeftMouseButton);
	}
}

void AVRMotionController::OnTriggerReleased()
{
	if (WidgetInteractor)
	{
		WidgetInteractor->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

void AVRMotionController::OnGripAxis(float Axis)
{
	// Update animation.
}

void AVRMotionController::OnOverlapBegin(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<IGrabbable>(OtherActor))
	{
		HoveredActor = Cast<IGrabbable>(OtherActor);
	}
}

void AVRMotionController::OnOverlapEnd(
	class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (HoveredActor == Cast<IGrabbable>(OtherActor))
	{
		HoveredActor = nullptr;
	}
}

void AVRMotionController::OnWidgetInteractorHoverChanged(UWidgetComponent* Old, UWidgetComponent* New)
{
	// Hide the WidgetInteractorVisualizer when not pointing at a menu.
	if (New)
	{
		WidgetInteractorVisualizer->SetVisibility(false);
	}
	else if (Old)
	{
		WidgetInteractorVisualizer->SetVisibility(true);
	}
}