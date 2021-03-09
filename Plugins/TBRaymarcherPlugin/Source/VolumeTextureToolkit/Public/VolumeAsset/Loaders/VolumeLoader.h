// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "CoreMinimal.h"
#include "VolumeAsset/VolumeAsset.h"
#include "VolumeAsset/VolumeInfo.h"

#include "VolumeLoader.generated.h"

UINTERFACE(BlueprintType)
class VOLUMETEXTURETOOLKIT_API UVolumeLoader : public UInterface
{
	GENERATED_BODY()
};

/// Interface for all volume loaders.
/// Implement this to make classes that can create UVolumeAssets from arbitrary volumetric formats.
class VOLUMETEXTURETOOLKIT_API IVolumeLoader
{
	GENERATED_BODY()
public:
	virtual FVolumeInfo ParseVolumeInfoFromHeader(FString FileName) = 0;

	virtual UVolumeAsset* ParseFromFile(FString FileName, bool bNormalize = true, bool bConvertToFloat = true) = 0;
};