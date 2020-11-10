// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

//
// This whole file is methods and definitions copypasted from VolumeTexture.cpp
// THE ONLY difference is the TexCreate_UAV flag passed to the resource when creating the Texture3D resource.
// If Epic kindly added ENGINE_API to the inherited functions in VolumeTexture.h and put the resource definition in a header
// file, this whole ugly exercise wouldn't be necessary, but hey, I'm not complaining. :)
//

#include "ComputeVolumeTexture.h"

#include "Containers/ResourceArray.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "EngineUtils.h"
#include "RenderUtils.h"
#include "TextureResource.h"

extern RHI_API bool GUseTexture3DBulkDataRHI;

class FVolumeTextureBulkData : public FResourceBulkDataInterface
{
public:
	FVolumeTextureBulkData(int32 InFirstMip) : FirstMip(InFirstMip)
	{
		FMemory::Memzero(MipData, sizeof(MipData));
		FMemory::Memzero(MipSize, sizeof(MipSize));
	}

	~FVolumeTextureBulkData()
	{
		Discard();
	}

	const void* GetResourceBulkData() const override
	{
		return MipData[FirstMip];
	}

	uint32 GetResourceBulkDataSize() const override
	{
		return MipSize[FirstMip];
	}

	void Discard() override
	{
		for (int32 MipIndex = 0; MipIndex < MAX_TEXTURE_MIP_COUNT; ++MipIndex)
		{
			if (MipData[MipIndex])
			{
				FMemory::Free(MipData[MipIndex]);
				MipData[MipIndex] = nullptr;
			}
			MipSize[MipIndex] = 0;
		}
	}

	void MergeMips(int32 NumMips)
	{
		check(NumMips < MAX_TEXTURE_MIP_COUNT);

		uint64 MergedSize = 0;
		for (int32 MipIndex = FirstMip; MipIndex < NumMips; ++MipIndex)
		{
			MergedSize += MipSize[MipIndex];
		}

		// Don't do anything if there is nothing to merge
		if (MergedSize > MipSize[FirstMip])
		{
			uint8* MergedAlloc = (uint8*) FMemory::Malloc(MergedSize);
			uint8* CurrPos = MergedAlloc;
			for (int32 MipIndex = FirstMip; MipIndex < NumMips; ++MipIndex)
			{
				if (MipData[MipIndex])
				{
					FMemory::Memcpy(CurrPos, MipData[MipIndex], MipSize[MipIndex]);
				}
				CurrPos += MipSize[MipIndex];
			}

			Discard();

			MipData[FirstMip] = MergedAlloc;
			MipSize[FirstMip] = MergedSize;
		}
	}

	void** GetMipData()
	{
		return MipData;
	}
	uint32* GetMipSize()
	{
		return MipSize;
	}
	int32 GetFirstMip() const
	{
		return FirstMip;
	}

protected:
	void* MipData[MAX_TEXTURE_MIP_COUNT];
	uint32 MipSize[MAX_TEXTURE_MIP_COUNT];
	int32 FirstMip;
};

class FTexture3DUAVResource : public FTextureResource
{
public:
	/**
	 * Minimal initialization constructor.
	 * @param InOwner - The UVolumeTexture which this FComputeTexture3DResource represents.
	 */
	FTexture3DUAVResource(UVolumeTexture* InOwner, int32 MipBias)
		: Owner(InOwner)
		, SizeX(InOwner->GetSizeX())
		, SizeY(InOwner->GetSizeY())
		, SizeZ(InOwner->GetSizeZ())
		, CurrentFirstMip(INDEX_NONE)
		, NumMips(InOwner->GetNumMips())
		, PixelFormat(InOwner->GetPixelFormat())
		, TextureSize(0)
		, TextureReference(&InOwner->TextureReference)
		, InitialData(MipBias)
	{
		check(0 < NumMips && NumMips <= MAX_TEXTURE_MIP_COUNT);
		check(0 <= MipBias && MipBias < NumMips);

		TextureName = Owner->GetFName();

		// This one line is the point of this whole file - add TexCreate_UAV to the resource creation flags so we can target the texture
		// in Compute shaders.
		CreationFlags = (Owner->SRGB ? TexCreate_SRGB : 0) | TexCreate_OfflineProcessed | TexCreate_ShaderResource |
						(Owner->bNoTiling ? TexCreate_NoTiling : 0) | TexCreate_UAV;
		SamplerFilter =
			(ESamplerFilter) UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(Owner);

		bGreyScaleFormat = (PixelFormat == PF_G8) || (PixelFormat == PF_BC4);

		//		This is a UAV resource to be used for calculations. We don't store persistent data here.
		//		And also, TryLoadMips again doesn't have ENGINE_API, so it wouldn't link anyways...

		// 		FTexturePlatformData* PlatformData = Owner->PlatformData;
		// 		if (PlatformData && PlatformData->TryLoadMips(MipBias, InitialData.GetMipData() + MipBias, Owner))
		// 		{
		// 			for (int32 MipIndex = MipBias; MipIndex < NumMips; ++MipIndex)
		// 			{
		// 				const FTexture2DMipMap& MipMap = PlatformData->Mips[MipIndex];
		//
		// 				// The bulk data can be bigger because of memory alignment constraints on each slice and mips.
		// 				InitialData.GetMipSize()[MipIndex] = FMath::Max<int32>(MipMap.BulkData.GetBulkDataSize(),
		// 					CalcTextureMipMapSize3D(SizeX, SizeY, SizeZ, (EPixelFormat) PixelFormat, MipIndex));
		// 			}
		// 		}
	}

	/**
	 * Destructor, freeing MipData in the case of resource being destroyed without ever
	 * having been initialized by the rendering thread via InitRHI.
	 */
	~FTexture3DUAVResource()
	{
	}

	/**
	 * Called when the resource is initialized. This is only called by the rendering thread.
	 */
	virtual void InitRHI() override
	{
		INC_DWORD_STAT_BY(STAT_TextureMemory, TextureSize);
		INC_DWORD_STAT_FNAME_BY(LODGroupStatName, TextureSize);

		CurrentFirstMip = InitialData.GetFirstMip();

		// Create the RHI texture.
		{
			FRHIResourceCreateInfo CreateInfo;
			if (GUseTexture3DBulkDataRHI)
			{
				InitialData.MergeMips(NumMips);
				CreateInfo.BulkData = &InitialData;
			}

			const uint32 BaseMipSizeX = FMath::Max<uint32>(SizeX >> CurrentFirstMip, 1);	// BlockSizeX?
			const uint32 BaseMipSizeY = FMath::Max<uint32>(SizeY >> CurrentFirstMip, 1);
			const uint32 BaseMipSizeZ = FMath::Max<uint32>(SizeZ >> CurrentFirstMip, 1);

			CreateInfo.ExtData = Owner->PlatformData ? Owner->PlatformData->GetExtData() : 0;
			Texture3DRHI = RHICreateTexture3D(
				BaseMipSizeX, BaseMipSizeY, BaseMipSizeZ, PixelFormat, NumMips - CurrentFirstMip, CreationFlags, CreateInfo);
			TextureRHI = Texture3DRHI;
		}

		TextureRHI->SetName(TextureName);
		RHIBindDebugLabelName(TextureRHI, *TextureName.ToString());

		if (TextureReference)
		{
			RHIUpdateTextureReference(TextureReference->TextureReferenceRHI, TextureRHI);
		}

		if (!GUseTexture3DBulkDataRHI)
		{
			const int32 BlockSizeX = GPixelFormats[PixelFormat].BlockSizeX;
			const int32 BlockSizeY = GPixelFormats[PixelFormat].BlockSizeY;
			const int32 BlockBytes = GPixelFormats[PixelFormat].BlockBytes;
			ensure(GPixelFormats[PixelFormat].BlockSizeZ == 1);

			for (int32 MipIndex = CurrentFirstMip; MipIndex < NumMips; ++MipIndex)
			{
				const uint8* MipData = (const uint8*) InitialData.GetMipData()[MipIndex];
				if (MipData)
				{
					// Could also access the mips size directly.
					const uint32 MipSizeX = FMath::Max<uint32>(SizeX >> MipIndex, 1);
					const uint32 MipSizeY = FMath::Max<uint32>(SizeY >> MipIndex, 1);
					const uint32 MipSizeZ = FMath::Max<uint32>(SizeZ >> MipIndex, 1);

					const uint32 NumBlockX = (uint32) FMath::DivideAndRoundUp<int32>(MipSizeX, BlockSizeX);
					const uint32 NumBlockY = (uint32) FMath::DivideAndRoundUp<int32>(MipSizeY, BlockSizeY);

					// FUpdateTextureRegion3D UpdateRegion(0, 0, 0, 0, 0, 0, NumBlockX * BlockSizeX, NumBlockY * BlockSizeY,
					// MipSizeZ);
					FUpdateTextureRegion3D UpdateRegion(0, 0, 0, 0, 0, 0, MipSizeX, MipSizeY, MipSizeZ);

					// RHIUpdateTexture3D crashes on some platforms at engine initialization time.
					// The default volume texture end up being loaded at that point, which is a problem.
					// We check if this is really the rendering thread to find out if the engine is initializing.
					RHIUpdateTexture3D(Texture3DRHI, MipIndex - CurrentFirstMip, UpdateRegion, NumBlockX * BlockBytes,
						NumBlockX * NumBlockY * BlockBytes, MipData);
				}
			}
			InitialData.Discard();
		}

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer(SamplerFilter, AM_Wrap, AM_Wrap, AM_Wrap);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}

	virtual void ReleaseRHI() override
	{
		DEC_DWORD_STAT_BY(STAT_TextureMemory, TextureSize);
		DEC_DWORD_STAT_FNAME_BY(LODGroupStatName, TextureSize);
		if (TextureReference)
		{
			RHIUpdateTextureReference(TextureReference->TextureReferenceRHI, nullptr);
		}
		Texture3DRHI.SafeRelease();
		FTextureResource::ReleaseRHI();
	}

	/** Returns the width of the texture in pixels. */
	uint32 GetSizeX() const override
	{
		return FMath::Max<uint32>(SizeX >> CurrentFirstMip, 1);
	}
	/** Returns the height of the texture in pixels. */
	uint32 GetSizeY() const override
	{
		return FMath::Max<uint32>(SizeY >> CurrentFirstMip, 1);
	}
	/** Returns the depth of the texture in pixels. */
	uint32 GetSizeZ() const override
	{
		return FMath::Max<uint32>(SizeZ >> CurrentFirstMip, 1);
	}

private:
	/** The UVolumeTexture which this resource represents */
	UVolumeTexture* Owner;

#if STATS
	/** The FName of the LODGroup-specific stat */
	FName LODGroupStatName;
#endif
	/** The FName of the texture asset */
	FName TextureName;

	/** Dimension X of the resource	*/
	uint32 SizeX;
	/** Dimension Y of the resource	*/
	uint32 SizeY;
	/** Dimension Z of the resource	*/
	uint32 SizeZ;
	/** The first mip cached in the resource. */
	int32 CurrentFirstMip;
	/** Num of mips of the texture */
	int32 NumMips;
	/** Format of the texture */
	uint8 PixelFormat;
	/** Creation flags of the texture */
	uint32 CreationFlags;
	/** Cached texture size for stats. */
	int32 TextureSize;

	/** The filtering to use for this texture */
	ESamplerFilter SamplerFilter;

	/** A reference to the texture's RHI resource as a texture 3D. */
	FTexture3DRHIRef Texture3DRHI;

	/** */
	FTextureReference* TextureReference;

	FVolumeTextureBulkData InitialData;
};

FTextureResource* UComputeVolumeTexture::CreateResource()
{
	const FPixelFormatInfo& FormatInfo = GPixelFormats[GetPixelFormat()];
	const bool bCompressedFormat = FormatInfo.BlockSizeX > 1;
	const bool bFormatIsSupported =
		FormatInfo.Supported && (!bCompressedFormat || ShaderPlatformSupportsCompression(GMaxRHIShaderPlatform));

	if (GetNumMips() > 0 && GSupportsTexture3D && bFormatIsSupported)
	{
		return new FTexture3DUAVResource(this, GetCachedLODBias());
	}
	else if (GetNumMips() == 0)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s contains no miplevels! Please delete."), *GetFullName());
	}
	else if (!GSupportsTexture3D)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s cannot be created, rhi does not support 3d textures."), *GetFullName());
	}
	else if (!bFormatIsSupported)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s cannot be created, rhi does not support format %s."), *GetFullName(), FormatInfo.Name);
	}
	return nullptr;
}

void UComputeVolumeTexture::Serialize(FArchive& Ar)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UComputeVolumeTexture::Serialize"), STAT_VolumeTexture_Serialize, STATGROUP_LoadTime);

	UTexture::Serialize(Ar);

	FStripDataFlags StripFlags(Ar);
	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (bCooked || Ar.IsCooking())
	{
		SerializeCookedPlatformData(Ar);
	}

#if WITH_EDITOR
	if (Ar.IsLoading() && !Ar.IsTransacting() && !bCooked)
	{
		BeginCachePlatformData();
	}
#endif	  // #if WITH_EDITOR
}

void UComputeVolumeTexture::PostLoad()
{
#if WITH_EDITOR
	FinishCachePlatformData();

	if (Source2DTexture && SourceLightingGuid != Source2DTexture->GetLightingGuid())
	{
		UpdateSourceFromSourceTexture();
	}
#endif	  // #if WITH_EDITOR

	UTexture::PostLoad();
}

void UComputeVolumeTexture::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if WITH_EDITOR
	int32 SizeX = Source.GetSizeX();
	int32 SizeY = Source.GetSizeY();
	int32 SizeZ = Source.GetNumSlices();
#else
	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 SizeZ = 0;
#endif

	const FString Dimensions = FString::Printf(TEXT("%dx%dx%d"), SizeX, SizeY, SizeZ);
	OutTags.Add(FAssetRegistryTag("Dimensions", Dimensions, FAssetRegistryTag::TT_Dimensional));
	OutTags.Add(FAssetRegistryTag("Format", GPixelFormats[GetPixelFormat()].Name, FAssetRegistryTag::TT_Alphabetical));

	UTexture::GetAssetRegistryTags(OutTags);
}

void UComputeVolumeTexture::UpdateResource()
{
	// Route to super.
	UTexture::UpdateResource();
}

FString UComputeVolumeTexture::GetDesc()
{
	return FString::Printf(TEXT("Volume: %dx%dx%d [%s]"), GetSizeX(), GetSizeY(), GetSizeZ(), GPixelFormats[GetPixelFormat()].Name);
}

uint32 UComputeVolumeTexture::CalcTextureMemorySize(int32 MipCount) const
{
	uint32 Size = 0;
	if (PlatformData)
	{
		const EPixelFormat Format = GetPixelFormat();
		if (Format != PF_Unknown)
		{
			const uint32 Flags =
				(SRGB ? TexCreate_SRGB : 0) | TexCreate_OfflineProcessed | (bNoTiling ? TexCreate_NoTiling : 0) | TexCreate_UAV;

			uint32 SizeX = 0;
			uint32 SizeY = 0;
			uint32 SizeZ = 0;
			CalcMipMapExtent3D(
				GetSizeX(), GetSizeY(), GetSizeZ(), Format, FMath::Max<int32>(0, GetNumMips() - MipCount), SizeX, SizeY, SizeZ);

			uint32 TextureAlign = 0;
			Size = (uint32) RHICalcTexture3DPlatformSize(SizeX, SizeY, SizeZ, Format, FMath::Max(1, MipCount), Flags,
				FRHIResourceCreateInfo(PlatformData->GetExtData()), TextureAlign);
		}
	}
	return Size;
}

uint32 UComputeVolumeTexture::CalcTextureMemorySizeEnum(ETextureMipCount Enum) const
{
	if (Enum == TMC_ResidentMips || Enum == TMC_AllMipsBiased)
	{
		return CalcTextureMemorySize(GetNumMips() - GetCachedLODBias());
	}
	else
	{
		return CalcTextureMemorySize(GetNumMips());
	}
}

void UComputeVolumeTexture::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	UTexture::GetResourceSizeEx(CumulativeResourceSize);
	CumulativeResourceSize.AddUnknownMemoryBytes(CalcTextureMemorySizeEnum(TMC_ResidentMips));
}
#if WITH_EDITOR
uint32 UComputeVolumeTexture::GetMaximumDimension() const
{
	return GetMax2DTextureDimension();
}
#endif