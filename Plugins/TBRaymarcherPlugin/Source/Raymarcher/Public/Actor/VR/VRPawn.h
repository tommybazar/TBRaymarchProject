// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

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
	TMap<EVRPlatform, FControllerPlatformClasses> PerPlatformControllers;

	/// Controller spawned for the left hand.
	AVRMotionController* LeftController;

	/// Controller spawned for the right hand.
	AVRMotionController* RightController;

	// Called when the game starts or when spawned.
	virtual void BeginPlay() override;

	// Called every frame.
	virtual void Tick(float DeltaTime) override;
};
