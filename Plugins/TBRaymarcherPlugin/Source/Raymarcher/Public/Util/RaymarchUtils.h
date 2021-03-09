// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/VolumeTexture.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RHI.h"
#include "RHIResources.h"
#include "Rendering/LightingShaders.h"
#include "Rendering/RaymarchTypes.h"
#include "UObject/ObjectMacros.h"

#include "RaymarchUtils.generated.h"

UCLASS()
class URaymarchUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//
	//
	// Functions for working with a single-channel (just alpha) light volume follow.
	//
	//

	/** Adds a light to light volume. Also works for removing a light by setting bLightAdded to false.*/
	UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	static RAYMARCHER_API void AddDirLightToSingleVolume(const FBasicRaymarchRenderingResources& Resources,
		const FDirLightParameters& LightParameters, const bool Added, const FRaymarchWorldParameters WorldParameters,
		bool& LightAdded, bool bGPUSync = false);

	/** Changes a light in the light volume.	 */
	UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	static RAYMARCHER_API void ChangeDirLightInSingleVolume(FBasicRaymarchRenderingResources& Resources,
		const FDirLightParameters OldLightParameters, const FDirLightParameters NewLightParameters,
		const FRaymarchWorldParameters WorldParameters, bool& LightAdded, bool bGPUSync = false);

	/** Clears a light volume in provided raymarch resources. */
	UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	static RAYMARCHER_API void ClearResourceLightVolumes(FBasicRaymarchRenderingResources Resources, float ClearValue);

	//
	//
	// Functions for handling transfer functions and color curves follow.
	//
	//

	/** Will create a 1D texture representing a default transfer function. This TF is full opacity,
	going from black at 0 to white at 1.*/
	UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	static RAYMARCHER_API void MakeDefaultTFTexture(UTexture2D*& OutTexture);

	/** Will create a 1D texture asset from a ColorCurve. */
	UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	static RAYMARCHER_API void ColorCurveToTexture(UCurveLinearColor* Curve, UTexture2D*& OutTexture);

	//
	//
	// Functions for creating parameter collections follow
	//
	//

	/** Gets volume texture dimension. */
	UFUNCTION(BlueprintPure, Category = "Raymarcher")
	static RAYMARCHER_API void GetVolumeTextureDimensions(UVolumeTexture* Texture, FIntVector& Dimensions);

	/** Transforms a transform to a matrix. */
	UFUNCTION(BlueprintPure, Category = "Raymarcher")
	static RAYMARCHER_API void TransformToMatrix(const FTransform Transform, FMatrix& OutMatrix, bool WithScaling);

	/**
	  Transforms Local (-1 to 1) coords to UV coords (0 to 1) coords. (The values are not clamped to the range).
	*/
	static RAYMARCHER_API void LocalToTextureCoords(FVector LocalCoords, FVector& TextureCoords);

	/**
	  Transforms UV coords (0 to 1) to Local (-1 to 1) coords. (The values are not clamped to the range).
	*/
	static RAYMARCHER_API void TextureToLocalCoords(FVector TextureCoors, FVector& LocalCoords);

	static RAYMARCHER_API void CreateBufferTextures(
		FIntPoint Size, EPixelFormat PixelFormat, OneAxisReadWriteBufferResources& RWBuffers);

	static RAYMARCHER_API void ReleaseOneAxisReadWriteBufferResources(OneAxisReadWriteBufferResources& Buffer);

	// UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	// static RAYMARCHER_API void UpdateVolumeTextureSource(UVolumeTexture* Texture);

	/**
	 Given a volume texture, this will copy the current texture content into the texture source.
	 This causes the current contents of the texture to be saved when you hit the save button,
	 instead of the source content this texture was created with.
	*/
	// UFUNCTION(BlueprintCallable, Category = "Raymarcher")
	// static RAYMARCHER_API void UpdateVolumeTextureSource(UVolumeTexture* VolumeTexture);
};