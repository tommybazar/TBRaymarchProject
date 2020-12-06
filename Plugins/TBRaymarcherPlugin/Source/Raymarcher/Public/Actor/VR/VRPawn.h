// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Camera/CameraComponent.h"
#include "VRPawn.generated.h"

class AVRMotionController;

UCLASS()
class AVRPawn : public APawn
{
	GENERATED_BODY()
public:
	AVRPawn();

	UPROPERTY(VisibleAnywhere)
	USceneComponent* HMDScene;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* VRCamera;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AVRMotionController> LeftControllerClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AVRMotionController> RightControllerClass;

	AVRMotionController* LeftController;

	AVRMotionController* RightController;

	// Called when the game starts or when spawned.
	virtual void BeginPlay() override;

	// Called every frame.
	virtual void Tick(float DeltaTime) override;

protected:
	bool bRightHanded = true;

	void QuitGame();
};
