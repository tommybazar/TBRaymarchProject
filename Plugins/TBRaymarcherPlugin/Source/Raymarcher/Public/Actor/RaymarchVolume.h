// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Actor/RaymarchClipPlane.h"
#include "Actor/RaymarchLight.h"
#include "CoreMinimal.h"
#include "Math/IntVector.h"
#include "UObject/UnrealType.h"
#include "VR/Grabbable.h"
#include "VolumeAsset/VolumeAsset.h"

#include "RaymarchVolume.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRaymarchVolume, Log, All);

DECLARE_DYNAMIC_DELEGATE(FOnVolumeLoaded);

UCLASS()
class RAYMARCHER_API ARaymarchVolume : public AActor, public IGrabbable
{
	GENERATED_BODY()

public:
	/** Sets default values for this actor's properties*/
	ARaymarchVolume();

	/** Called after the actor is loaded from disk in editor or when spawned in game.
		This is the last action that is performed before BeginPlay.*/
	virtual void PostRegisterAllComponents() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Updates a single provided light affecting the LightVolume. */
	void UpdateSingleLight(ARaymarchLight* UpdatedLight);

	/** MeshComponent that contains the raymarching cube. */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComponent;

	/** Pointer to the currently used Transfer Function curve.*/
	UCurveLinearColor* CurrentTFCurve = nullptr;

	/** Delegate that is fired whenever a new volume is loaded. Useful if you have any UI showing info about this volume.*/
	FOnVolumeLoaded OnVolumeLoaded;

	/** Sets a new MHDAsset and reinitializes the raymarching resources.*/
	UFUNCTION(BlueprintCallable)
	bool SetMHDAsset(UVolumeAsset* InMHDAsset);

	/** Use faster shader for light calculation. Leads to instability with more lights.*/
	UPROPERTY(EditAnywhere)
	bool bFastShader = true;

	/// Map for storing previous ticks parameters per-light. Used to detect changes.
	UPROPERTY(Transient)
	TMap<ARaymarchLight*, FDirLightParameters> LightParametersMap;

protected:
	/** Initializes the Raymarch Resources to work with the provided Data Volume Texture.**/
	void InitializeRaymarchResources(UVolumeTexture* LoadedTexture);

	/** Called before initializing new Raymarch resources to free all old resources.*/
	void FreeRaymarchResources();

	/** Returns the current World parameters of this volume.**/
	FRaymarchWorldParameters GetWorldParameters();

	/** Updates the world parameters to the current state of the volume and clipping plane**/
	void UpdateWorldParameters();

	/** Recalculates all lights in the LightsArray. **/
	UFUNCTION()
	void ResetAllLights();

public:
#if WITH_EDITOR
	/** Fired when curve gradient is updated.*/
	FDelegateHandle CurveGradientUpdateDelegateHandle;

	/** Fired when curve in the associated MHD file is changed.*/
	FDelegateHandle CurveChangedInMHDDelegateHandle;

	/** Fired when data in the MHD asset is changed.*/
	FDelegateHandle MHDAssetUpdatedDelegateHandle;

	/** Function that is bound to the current MHDAssets OnCurveChanged delegate (in-editor only). Gets fired when the asset's curve
	 * changes.*/
	void OnMHDAssetChangedTF(UCurveLinearColor* Curve);

	/** Function that is bound to the current transfer function color curve and gets fired when that gets changed (e.g. when the
	 * user edits the curve in curve editor. */
	void OnTFColorCurveUpdated(UCurveBase* Curve, EPropertyChangeType::Type ChangeType);

	/** Called when the MHD asset is modified in-editor.*/
	UFUNCTION()
	void OnImageInfoChangedInEditro();

	/** Handles in-editor changes to exposed properties.*/
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	/** Override ShouldTickIfViewportsOnly to return true, so this also ticks in editor viewports.*/
	virtual bool ShouldTickIfViewportsOnly() const override;

#endif	  //#if WITH_EDITOR

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** The loaded MHD asset belonging to this volume*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UVolumeAsset* MHDAsset = nullptr;

	/** Only kept so that we can compare to it when a user changes the MHDAsset. See SetMHDAsset().*/
	UVolumeAsset* OldMHDAsset = nullptr;

	/** The base material for volumetric rendering.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UMaterial* LitRaymarchMaterialBase;

	/** The base material for intensity rendering.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UMaterial* IntensityRaymarchMaterialBase;

	/** Dynamic material instance for Lit rendering **/
	UPROPERTY(BlueprintReadOnly, Transient)
	UMaterialInstanceDynamic* LitRaymarchMaterial = nullptr;

	/** Dynamic material instance for Intensity rendering**/
	UPROPERTY(BlueprintReadOnly, Transient)
	UMaterialInstanceDynamic* IntensityRaymarchMaterial = nullptr;

	/** Cube border mesh - this is just a cube with wireframe borders.**/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CubeBorderMeshComponent = nullptr;

	/** The clipping plane affecting this volume.**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ARaymarchClipPlane* ClippingPlane = nullptr;

	/** An array of lights affecting this volume.**/
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<ARaymarchLight*> LightsArray;

	/** If set to true, lights will be recomputed on next tick.**/
	bool bRequestedRecompute = false;

	/** If set to true, volume will be raymarched with lit raymarching. If false, it will show Pure Intensity Raymarch only.**/
	UPROPERTY(EditAnywhere)
	bool bLitRaymarch = true;

	/** Raymarch Rendering resources. These contain references to the volume texture currently used, the light volume
		currently used, as well as buffers to fasten the light propagation.	**/
	UPROPERTY(EditAnywhere)
	FBasicRaymarchRenderingResources RaymarchResources;

	/** World parameters of the volume from last tick. Used to see if we need to recompute the lighting when volume
		or clipping plane moves. **/
	UPROPERTY(VisibleAnywhere)
	FRaymarchWorldParameters WorldParameters;

	/** The number of steps to take when raymarching. This is multiplied by the volume thickness in texture space, so can be
	 * multiplied by anything from 0 to sqrt(3), Raymarcher will only take exactly this many steps when the path through the cube is
	 * equal to the lenght of it's side. **/
	UPROPERTY(EditAnywhere)
	float RaymarchingSteps = 150;

	/** If true, the light volume texture will be created using R32F format instead of the standard G8. This allows
		Illumination values greater than 1 (over-lighted) to be visible. Comes at the cost of 4x memory consumption and
		noticeably (but not significantly, in the ballpark of 10%) slower illumination calculation and materials.	**/
	UPROPERTY(EditAnywhere)
	bool bLightVolume32Bit = false;

	/** Switches to using a new Transfer function curve.**/
	UFUNCTION(BlueprintCallable)
	void SetTFCurve(UCurveLinearColor* InTFCurve);

	/** Saves the current windowing parameters as default in the MHD Asset.*/
	void SaveCurrentParamsToMHDAsset();

	/** Loads the specified MHD file into the volume. Will also create a transient Float32 MHD file and VolumeTexture that will be
	 * used.**/
	UFUNCTION(BlueprintCallable)
	bool LoadNewFileIntoVolumeTransientR32F(FString FileName);

	/** Loads the specified MHD file into the volume. Will also create a persistent G8/G16 MHD file and VolumeTexture that will be
	used. If the volume is to be persistent, add OutFolder relative to content directory (with a forward slash at the end!).
	**/
	UFUNCTION(BlueprintCallable)
	bool LoadNewFileIntoVolumeNormalized(FString FileName, bool bPersistent, FString OutFolder);

	// 	UFUNCTION(BlueprintCallable)
	// 	void TestReadDICOM(FString FileName);

	/** Sets all material parameters to the raymarching materials. Usually called only after loading a new volume.**/
	void SetAllMaterialParameters();

	/** Sets material Windowing Parameters. Called after changing Window Center or Width.**/
	void SetMaterialVolumeParameters();

	/** Sets material Windowing Parameters. Called after changing Window Center or Width.**/
	void SetMaterialWindowingParameters();

	/** Sets material Clipping Parameters. Called when the clip plane moves relative to the volume. The parameters are to be
	 * provided in Volume-Local space. **/
	void SetMaterialClippingParameters();

	/** API function to get the Min and Max values of the current MHD file.**/
	UFUNCTION(BlueprintPure)
	void GetMinMaxValues(float& Min, float& Max);

	/** Sets window center in the Lit Raymarch Material. **/
	UFUNCTION(BlueprintCallable)
	void SetWindowCenter(const float& Center);

	/** Sets window width in the Lit Raymarch Material. **/
	UFUNCTION(BlueprintCallable)
	void SetWindowWidth(const float& Width);

	/** Enables/disables low cutoff in the Lit Raymarch Material. **/
	UFUNCTION(BlueprintCallable)
	void SetLowCutoff(const bool& LowCutoff);

	/** Enables/disables high cutoff in the Lit Raymarch Material. **/
	UFUNCTION(BlueprintCallable)
	void SetHighCutoff(const bool& HighCutoff);

	/** Switches between Lit and Intensity raymarching.**/
	UFUNCTION(BlueprintCallable)
	void SwitchRenderer(bool bInLitRaymarch);

	/** Switches between Lit and Intensity raymarching.**/
	UFUNCTION(BlueprintCallable)
	void SetRaymarchSteps(float InRaymarchingSteps);
};
