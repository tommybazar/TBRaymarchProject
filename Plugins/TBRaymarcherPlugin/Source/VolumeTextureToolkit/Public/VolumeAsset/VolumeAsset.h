// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WindowingParameters.h"
#include "VolumeInfo.h"

#include "VolumeAsset.Generated.h"

// Sample MHD header
//
// ObjectType = Image
// NDims = 3
// DimSize = 512 512 128
// ElementSpacing = 0.00390625 0.0117188 0.046875
// Position = 0 -4.19925 -4.19785
// ElementType = MET_UCHAR
// ElementNumberOfChannels = 1
// ElementByteOrderMSB = False
// ElementDataFile = preprocessed.raw

/// Delegate that is broadcast when the color curve is changed.
DECLARE_MULTICAST_DELEGATE_OneParam(FCurveAssetChangedDelegate, UCurveLinearColor*);

/// Delegate that is broadcast when the inner volume info is changed.
DECLARE_MULTICAST_DELEGATE(FVolumeInfoChangedDelegate);

UCLASS()
class VOLUMETEXTURETOOLKIT_API UVolumeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/// Volume texture containing the data loaded from the MHD file.
	UPROPERTY(VisibleAnywhere)
	UVolumeTexture* AssociatedTexture;

	/// A color curve that will be used as a transfer function to display this volume.
	UPROPERTY(EditAnywhere)
	UCurveLinearColor* TransferFuncCurve;

	/// Holds the general info about the MHD Volume read from disk.
	UPROPERTY(EditAnywhere)
	FVolumeInfo ImageInfo;

	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	/// Loads a string from file specified by FileName and parses it into a UMHDAsset.
	static FVolumeInfo ParseHeaderToImageInfo(const FString FileName);

	/// Creates a UMHDAsset asset. If bIsTransient is false, SaveFolder and SaveName specify the desired location of the
	/// persistent asset.
	static UVolumeAsset* CreateAndLoadMHDAsset(
		const FString FileName, bool bIsPersistent, FString SaveFolder = "", const FString SaveName = "");

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset. Both will be put into the OutFolder (relative to Content)
	/// and returned in the out parameters. Normalizes the MHD to the whole range of G8 or G16 (if 16bits or more).
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileNormalized(const FString Filename, UVolumeAsset*& OutMHDAsset,
		UVolumeTexture*& OutVolumeTexture, bool bIsPersistent, const FString OutFolder = "");

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset into a R_32_Float texture. This will preserve the values
	/// from the original file (even in shaders) but the texture won't be persistent.
	/// This could be circumvented by packing the 32float into 8bit RGBA and then unpacking it in shaders.
	/// #TODO Or by extending a texture type that CAN serialize a bloody float array...
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileR32F(const FString Filename, UVolumeAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture);

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset into a texture. The data isn't converted in any way
	/// and you'll probably have to rewrite the shaders to work with anything that isn't U8 or U16 out of the box.
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileNoConversion(
		const FString Filename, const FString OutFolder, UVolumeAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture);

#if WITH_EDITOR
	/// Called when the Transfer function curve is changed (as in, a different asset is selected).
	FCurveAssetChangedDelegate OnCurveChanged;

	/// Called when the inside of the volume info change.
	FVolumeInfoChangedDelegate OnImageInfoChanged;

	/// Called when inner structs get changed. Used to notify active volumes about stuff changing inside the ImageInfo struct.
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

	/// Called when a property is changed.
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
