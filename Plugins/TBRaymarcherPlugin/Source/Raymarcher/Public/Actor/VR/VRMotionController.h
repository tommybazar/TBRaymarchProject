// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "MotionControllerComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetInteractionComponent.h"

#include "VRMotionController.generated.h"

class UMotionControllerComponent;
class USkeletalMeshComponent;

/**
 * Base class for motion controllers in BodyMap.
 */
UCLASS(Abstract)
class AVRMotionController : public AActor
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	AVRMotionController();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	USphereComponent* CollisionComponent;

	/// The motion controller component used to drive this MotionController.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionController | Component")
	UMotionControllerComponent* MotionControllerComponent = nullptr;

	/// Skeletal mesh of the controller.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionController | Component")
	USkeletalMeshComponent* ControllerSkeletalMeshComponent = nullptr;

	UPROPERTY(EditAnywhere)
	UWidgetInteractionComponent* WidgetInteractor;

	/// If true, this controller should go into the right hand.
	UPROPERTY(EditAnywhere)
	bool bIsInRightHand = true;

	/// Animation used on the skeletal mesh, cast to our custom Animation Instance, to drive
	/// button animations.
	UAnimInstance* ControllerAnimation = nullptr;

	/// Sets up this controller's actions to the provided InputComponent.
	virtual void SetupInput(UInputComponent* InInputComponent);

	/// True if the controller grip is pressed.
	bool bIsGripPressed = false;

	/// IMotionControllerHandler interface begin :

	virtual void OnGripPressed();

	virtual void OnGripReleased();

	virtual void OnTriggerAxis(float Axis);

	virtual void OnTriggerPressed();

	virtual void OnGripAxis(float Axis);

	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/// The actor currently hovered by the sphere collision.
	AActor* HoveredActor = nullptr;

	AActor* GrabbedActor = nullptr;

	virtual void OnActorHovered();

protected:
	// A weak pointer to the owning player controller.
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
