// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Camera/CameraComponent.h"
#include "VRMotionController.h"

#include "VRPawn.generated.h"

class AVRMotionController;

UENUM()
enum class EVRPlatform
{
	Oculus = 0,
	Vive = 1,
	Default,
};

/// The 2 classes of VRMotionController to spawn for each platform.
USTRUCT()
struct FControllerPlatformClasses
{
	GENERATED_BODY()
	/// Class of AVRMotionController to spawn for left hand.
	UPROPERTY(EditAnywhere)
	TSubclassOf<AVRMotionController> LeftControllerClass;

	/// Class of AVRMotionController to spawn for right hand.
	UPROPERTY(EditAnywhere)
	TSubclassOf<AVRMotionController> RightControllerClass;
};

/**
 * VR pawn for handling volumetric volumes.
 */
UCLASS() class AVRPawn : public APawn
{
	GENERATED_BODY()
public:
	/// Default constructor.
	AVRPawn();

	/// Root component to attach everything to.
	UPROPERTY(VisibleAnywhere)
	USceneComponent* HMDScene;

	/// VR Camera component.
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* VRCamera;

	/// Contains the classes of controllers to be spawned per each platform.
	UPROPERTY(EditAnywhere)
	TMap<EVRPlatform, FControllerPlatformClasses> PerPlatformControllers;

	/// Controller spawned for the left hand.
	UPROPERTY(VisibleAnywhere)
	AVRMotionController* LeftController;

	/// Controller spawned for the right hand.
	UPROPERTY(VisibleAnywhere)
	AVRMotionController* RightController;

	// Called when the game starts or when spawned.
	virtual void BeginPlay() override;

	// Called every frame.
	virtual void Tick(float DeltaTime) override;
};
