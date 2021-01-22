// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original FractalMarching code).

#pragma once

#include "../Rendering/FractalShaders.h"
#include "CoreMinimal.h"
#include "Math/IntVector.h"

#include "FractalVolume.generated.h"

class UComputeVolumeTexture;

DECLARE_LOG_CATEGORY_EXTERN(LogFractalMarchVolume, Log, All);

UCLASS()
class FRACTALMARCHER_API AFractalVolume : public AActor
{
	GENERATED_BODY()

public:
	/** Sets default values for this actor's properties*/
	AFractalVolume();

	/** Called after the actor is loaded from disk in editor or when spawned in game.
		This is the last action that is performed before BeginPlay.*/
	void PostRegisterAllComponents();

	/** MeshComponent that contains the FractalMarching cube. */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere)
	UVolumeTexture* MandelbulbVolume;

	/** Pointer to the currently used Transfer Function curve.*/
	UCurveLinearColor* CurrentTFCurve = nullptr;

	UFUNCTION(CallInEditor)
	void CalculateMandelbulbSDF();

protected:
	/** Initializes the FractalMarch Resources to work with the provided Data Volume Texture.**/
	void InitializeFractalMarchResources();

	/** Returns the current World parameters of this volume.**/
	FMandelbulbSDFResources MandelbulbResources;

public:
#if WITH_EDITOR
	/** Handles in-editor changes to exposed properties.*/
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	/** Override ShouldTickIfViewportsOnly to return true, so this also ticks in editor viewports.*/
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif	  //#if WITH_EDITOR

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** The base material for volumetric rendering.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UMaterial* LitFractalMarchMaterialBase;

	/** Dynamic material instance for Lit rendering **/
	UPROPERTY(BlueprintReadOnly, Transient)
	UMaterialInstanceDynamic* LitFractalMarchMaterial = nullptr;

	/** If set to true, Mandelbulb will be recomputed on next tick.**/
	bool bRequestedRecompute = false;

	/** The number of steps to take when FractalMarching. This is multiplied by the volume thickness in texture space, so can be
	 * multiplied by anything from 0 to sqrt(3), FractalMarcher will only take exactly this many steps when the path through the
	 * cube is equal to the lenght of it's side. **/
	UPROPERTY(EditAnywhere)
	float FractalMarchingSteps = 150;

	UPROPERTY(EditAnywhere)
	FIntVector MandelbulbDimensions = FIntVector(512, 512, 512);

	/** "World" center of the fractal - the center of the volume will correspond to these coordinates in XYZ. 
	    Used to calculate the fractal at different places. */
	UPROPERTY(EditAnywhere)
	FVector Center = FVector(0,0,0);

	/// The extent of the volume. Smaller extent means more "zoom" into the fractal.
	/// The full volume will encompass a cube ranging from [Center - FVector(Extent/2)] to [Center + Fvector(Extent/2)]
	UPROPERTY(EditAnywhere)
	float Extent = 2;

	/// The power used in calculating the mandelbulb fractal. 2 Results in "3d mandelbrot", 8 is "standard mandelbulb".
	UPROPERTY(EditAnywhere)
	float Power = 8;

	/** Switches to using a new Transfer function curve.**/
	UFUNCTION(BlueprintCallable)
	void SetTFCurve(UCurveLinearColor*& InTFCurve);

	/** Sets all material parameters to the FractalMarching materials. Usually called only after loading a new volume.**/
	void SetAllMaterialParameters();

	/** Sets the number of marching steps.**/
	UFUNCTION(BlueprintCallable)
	void SetFractalMarchSteps(float InFractalMarchingSteps);
};
