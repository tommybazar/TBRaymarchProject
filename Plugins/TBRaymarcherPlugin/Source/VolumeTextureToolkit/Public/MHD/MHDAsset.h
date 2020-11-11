// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WindowingParameters.h"
#include "sstream"
#include "string"

#include "MHDAsset.Generated.h"

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

/// Contains information about the volume loaded from the MHD file.
USTRUCT(BlueprintType)
struct VOLUMETEXTURETOOLKIT_API FVolumeInfo
{
	GENERATED_BODY()
public:
	// Size of volume in voxels.
	UPROPERTY(VisibleAnywhere)
	FIntVector Dimensions;

	// Size of a voxel in mm.
	UPROPERTY(VisibleAnywhere)
	FVector Spacing;

	// Size of the whole volume in mm (equals VoxelDimensions * Spacing)
	UPROPERTY(VisibleAnywhere)
	FVector WorldDimensions;

	// Default windowing parameters used when this volume is loaded.
	UPROPERTY(EditAnywhere)
	FWindowingParameters DefaultWindowingParameters;

	// If true, the values in the texture have been normalized from [MinValue, MaxValue] to [0, 1] range.
	UPROPERTY(VisibleAnywhere)
	bool bIsNormalized = false;

	// Lowest value voxel in the volume in the original volume (before normalization).
	UPROPERTY(VisibleAnywhere)
	float MinValue = -1000;

	// Highest value voxel in the volume in the original volume (before normalization).
	UPROPERTY(VisibleAnywhere)
	float MaxValue = 3000;

	// Returns the number of bytes needed to store this Volume.
	int64 GetTotalBytes();

	// Returns the number of voxels in this volume.
	int64 GetTotalVoxels();

	// Properties not visible to blueprints (used only when loading)
	bool bIsSigned;
	size_t BytesPerVoxel;

	// Normalizes an input value from the range [MinValue, MaxValue] to [0,1]. Note that values can be outside of the range,
	// e.g. MinValue - (MaxValue - MinValue) will be normalized to -1.
	float NormalizeValue(float InValue)
	{
		if (!bIsNormalized)
		{
			return InValue;
		}
		// Normalize on the range of [Min, Max]
		return ((InValue - MinValue) / (MaxValue - MinValue));
	}

	/// Converts a [0,1] normalized value to [Min, Max] range.
	float DenormalizeValue(float InValue)
	{
		if (!bIsNormalized)
		{
			return InValue;
		}
		return ((InValue * (MaxValue - MinValue)) + MinValue);
	}

	/// Normalizes a range to 0-1 depending on the size of the original data.
	float NormalizeRange(float InRange)
	{
		if (!bIsNormalized)
		{
			return InRange;
		}
		// Normalize the range from [Max - Min] to 1
		return ((InRange) / (MaxValue - MinValue));
	}

	/// Converts a [0,1] normalized range to the range of the original data (e.g. 1 will get converted to (MaxValue - MinValue))
	float DenormalizeRange(float InRange)
	{
		if (!bIsNormalized)
		{
			return InRange;
		}
		// Normalize the range from [Max - Min] to 1
		return (InRange * (MaxValue - MinValue));
	}
};



/// Delegate that is broadcast when the color curve is changed.
DECLARE_MULTICAST_DELEGATE_OneParam(FCurveAssetChangedDelegate, UCurveLinearColor*);

/// Delegate that is broadcast when the inner volume info is changed.
DECLARE_MULTICAST_DELEGATE(FVolumeInfoChangedDelegate);

UCLASS()
class VOLUMETEXTURETOOLKIT_API UMHDAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/// If true, parsing succeeded. If false, this MHDAsset is unusable.
	bool ParseSuccessful;

	/// MHD ElementType in string format. e.g. "MET_UCHAR".
	UPROPERTY(VisibleAnywhere)
	FString ElementType;

	/// Name of the MHD file that was loaded.
	UPROPERTY(VisibleAnywhere)
	FString DataFileName;

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
	bool LoadAndParseMhdFile(const FString FileName);

	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	/// Parses a MHD header loaded in a FString into a UMHDAsset.
	bool ParseFromString(const FString InString);

	/// Returns World Dimensions in mm.
	UFUNCTION(BlueprintPure)
	FVector GetWorldDimensions() const;

	/// Outputs a string with info about this MHD file.
	UFUNCTION(BlueprintPure)
	FString ToString() const;

	/// Creates a UMHDAsset asset. If bIsTransient is false, SaveFolder and SaveName specify the desired location of the
	/// persistent asset.
	static UMHDAsset* CreateAndLoadMHDAsset(
		const FString FileName, bool bIsPersistent, FString SaveFolder = "", const FString SaveName = "");

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset. Both will be put into the OutFolder (relative to Content)
	/// and returned in the out parameters. Normalizes the MHD to the whole range of G8 or G16 (if 16bits or more).
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileNormalized(const FString Filename, UMHDAsset*& OutMHDAsset,
		UVolumeTexture*& OutVolumeTexture, bool bIsPersistent, const FString OutFolder = "");

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset into a R_32_Float texture. This will preserve the values
	/// from the original file (even in shaders) but the texture won't be persistent.
	/// This could be circumvented by packing the 32float into 8bit RGBA and then unpacking it in shaders.
	/// #TODO Or by extending a texture type that CAN serialize a bloody float array...
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileR32F(const FString Filename, UMHDAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture);

	/// Loads the provided .MHD file into a Volume texture and UMHDAsset into a texture. The data isn't converted in any way
	/// and you'll probably have to rewrite the shaders to work with anything that isn't U8 or U16 out of the box.
	UFUNCTION(BlueprintCallable, Category = "MHD Asset")
	static void CreateAssetFromMhdFileNoConversion(
		const FString Filename, const FString OutFolder, UMHDAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture);

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
