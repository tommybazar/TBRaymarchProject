#pragma once

#include "CoreMinimal.h"
#include "VolumeInfo.generated.h"

/// Voxel format of a loaded volume.
UENUM(BlueprintType)
enum class EVolumeVoxelFormat : uint8
{
	// 1 byte
	UnsignedChar = 0,
	SignedChar = 1,
	// 2 bytes
	UnsignedShort = 2,
	SignedShort = 3,
	// 4 bytes
	UnsignedInt = 4,
	SignedInt = 5,
	// 4 bytes float
	Float = 6
	// #TODO maybe double? Unreal materials don't support them anyways...
};


/// Struct for raymarch windowing parameters. These work exactly the same as DICOM window.
USTRUCT(BlueprintType)
struct FWindowingParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Center = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Width = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool LowCutoff = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool HighCutoff = true;

	/** Transforms the 4 values into a FLinear color to be used in materials.**/
	FLinearColor ToLinearColor()
	{
		return FLinearColor(Center, Width, LowCutoff, HighCutoff);
	}
};

/// Contains information about the volume loaded from the Various volumetric data file formats supported.
USTRUCT(BlueprintType)
struct VOLUMETEXTURETOOLKIT_API FVolumeInfo
{
	GENERATED_BODY()
public:
	/// Format of voxels loaded from the volume. Does NOT have to match the actual PixelFormat that the VolumeTexture is stored in!
	/// e.g. this could be UChar and VolumeTexture is saved as a EPixelFormat::Float - so don't use for calculating sizes!
	UPROPERTY(VisibleAnywhere)
	EVolumeVoxelFormat VoxelFormat;

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

	bool bIsCompressed = false;
	int32 CompressedBytes = 0;

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

	static int32 VoxelFormatByteSize(EVolumeVoxelFormat InFormat);

	static bool IsVoxelFormatSigned(EVolumeVoxelFormat InFormat);
};
