// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Engine/VolumeTexture.h"
#include "UObject/ObjectMacros.h"

#include "ComputeVolumeTexture.generated.h"

///
/// A volume texture that has its resource created with Tex_UAV
///
/// It would be cleaner to just inherit from UTexture and implement the minimum functionality needed for 
/// UAV-enabled TextureResource3D, but by inheriting from VolumeTexture we get the editor thinking
/// it's just a plain, old VolumeTexture, so we can actually inspect these visually in-editor. 
/// 

UCLASS(hidecategories = (Object, Compositing, ImportSettings))
class UComputeVolumeTexture : public UVolumeTexture
{
	GENERATED_BODY()
public:
	// The only thing we need to override so that a FTexture3DResource gets created with UAV flag.
	virtual FTextureResource* CreateResource() override;

	// Override stuff because these functions are missing ENGINE_API, so they won't link outside the engine module.
	// Implementation is mostly copypasta from VolumeTexture.cpp, only removed some minor things that wouldn't link.

	//~ Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	void UpdateResource();
	virtual FString GetDesc() override;
	uint32 CalcTextureMemorySize(int32 MipCount) const;
	uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;

#if WITH_EDITOR
	/**
	 * Return maximum dimension for this texture type.
	 */
	virtual uint32 GetMaximumDimension() const override;

#endif
	//~ End UObject Interface.

	//
};
