// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "TextureUtilities.h"

#include "AssetRegistryModule.h"
#include "ComputeVolumeTexture.h"

DEFINE_LOG_CATEGORY(LogTextureUtils);

FString UVolumeTextureToolkit::MakePackageName(FString AssetName, FString FolderName)
{
	if (FolderName.IsEmpty())
	{
		FolderName = "GeneratedTextures";
	}
	return "/Game" / FolderName / AssetName;
}

void UVolumeTextureToolkit::SetVolumeTextureDetails(UVolumeTexture*& VolumeTexture, EPixelFormat PixelFormat, FIntVector Dimensions)
{
	// Newly created Volume textures have this null'd
	if (!VolumeTexture->PlatformData)
	{
		VolumeTexture->PlatformData = new FTexturePlatformData();
	}
	// Set Dimensions and Pixel format.
	VolumeTexture->PlatformData->SizeX = Dimensions.X;
	VolumeTexture->PlatformData->SizeY = Dimensions.Y;
	VolumeTexture->PlatformData->SetNumSlices(Dimensions.Z);
	VolumeTexture->PlatformData->PixelFormat = PixelFormat;
	// Set sRGB and streaming to false.
	VolumeTexture->SRGB = false;
	VolumeTexture->NeverStream = true;
}

void UVolumeTextureToolkit::CreateVolumeTextureMip(
	UVolumeTexture*& VolumeTexture, EPixelFormat PixelFormat, FIntVector Dimensions, uint8* BulkData /*= nullptr*/)
{
	int PixelByteSize = GPixelFormats[PixelFormat].BlockBytes;
	const long long TotalSize = (long long) Dimensions.X * Dimensions.Y * Dimensions.Z * PixelByteSize;

	// Create the one and only mip in this texture.
	FTexture2DMipMap* mip = new FTexture2DMipMap();
	mip->SizeX = Dimensions.X;
	mip->SizeY = Dimensions.Y;
	mip->SizeZ = Dimensions.Z;

	mip->BulkData.Lock(LOCK_READ_WRITE);
	// Allocate memory in the mip and copy the actual texture data inside
	uint8* ByteArray = (uint8*) mip->BulkData.Realloc(TotalSize);

	if (BulkData)
	{
		FMemory::Memcpy(ByteArray, BulkData, TotalSize);
	}
	else
	{
		// If no data is provided, memset to zero
		FMemory::Memset(ByteArray, 0, TotalSize);
	}

	mip->BulkData.Unlock();

	// Newly created Volume textures have this null'd
	if (!VolumeTexture->PlatformData)
	{
		VolumeTexture->PlatformData = new FTexturePlatformData();
	}
	// Add the new MIP to the list of mips.
	VolumeTexture->PlatformData->Mips.Add(mip);
}

bool UVolumeTextureToolkit::CreateVolumeTextureAsset(UVolumeTexture*& OutTexture, FString AssetName, FString FolderName,
	EPixelFormat PixelFormat, FIntVector Dimensions, uint8* BulkData, bool IsPersistent, bool ShouldUpdateResource,
	bool bUAVTargettable)
{
	if (Dimensions.X == 0 || Dimensions.Y == 0 || Dimensions.Z == 0)
	{
		return false;
	}

	FString PackageName = MakePackageName(AssetName, FolderName);
	UPackage* Package = CreatePackage(NULL, *PackageName);
	Package->FullyLoad();

	UVolumeTexture* VolumeTexture = nullptr;

	if (bUAVTargettable)
	{
		VolumeTexture =
			NewObject<UComputeVolumeTexture>((UObject*) Package, FName(*AssetName), RF_Public | RF_Standalone | RF_MarkAsRootSet);
	}
	else
	{
		VolumeTexture =
			NewObject<UVolumeTexture>((UObject*) Package, FName(*AssetName), RF_Public | RF_Standalone | RF_MarkAsRootSet);
	}

	// Prevent garbage collection of the texture
	VolumeTexture->AddToRoot();

	SetVolumeTextureDetails(VolumeTexture, PixelFormat, Dimensions);
	CreateVolumeTextureMip(VolumeTexture, PixelFormat, Dimensions, BulkData);
	CreateVolumeTextureEditorData(VolumeTexture, PixelFormat, Dimensions, BulkData, IsPersistent);

	// Update resource, mark that the folder needs to be rescan and notify editor
	// about asset creation.
	if (ShouldUpdateResource)
	{
		VolumeTexture->UpdateResource();
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(VolumeTexture);
	// Pass out the reference to our brand new texture.
	OutTexture = VolumeTexture;
	return true;
}

bool UVolumeTextureToolkit::UpdateVolumeTextureAsset(UVolumeTexture* VolumeTexture, EPixelFormat PixelFormat, FIntVector Dimensions,
	uint8* BulkData, bool IsPersistent /*= false*/, bool ShouldUpdateResource /*= true*/)
{
	if (!VolumeTexture || (Dimensions.X == 0 || Dimensions.Y == 0 || Dimensions.Z == 0))
	{
		return false;
	}

	SetVolumeTextureDetails(VolumeTexture, PixelFormat, Dimensions);
	CreateVolumeTextureMip(VolumeTexture, PixelFormat, Dimensions, BulkData);
	CreateVolumeTextureEditorData(VolumeTexture, PixelFormat, Dimensions, BulkData, IsPersistent);

	// Update resource, mark the asset package dirty.
	if (ShouldUpdateResource)
	{
		VolumeTexture->UpdateResource();
	}

	// Notify asset manager that this is dirty now.
	VolumeTexture->MarkPackageDirty();
	return true;
}

bool UVolumeTextureToolkit::CreateVolumeTextureEditorData(
	UTexture* Texture, const EPixelFormat PixelFormat, const FIntVector Dimensions, const uint8* BulkData, const bool IsPersistent)
{
	// Handle persistency only if we're in editor
	// These don't exist outside of the editor.
#if WITH_EDITORONLY_DATA
	// Todo - figure out how to tell the Texture Builder to REALLY LEAVE THE
	// BLOODY MIPS ALONE when setting TMGS_LeaveExistingMips and being persistent.
	// Until then, we simply don't support mips on generated textures.
	Texture->MipGenSettings = TMGS_NoMipmaps;

	// CompressionNone assures the texture is actually saved in the format we want and not DXT1.
	Texture->CompressionNone = true;

	// If asset is to be persistent, handle creating the Source structure for it.
	if (IsPersistent)
	{
		// If using a format that's not supported as Source format, fail.
		ETextureSourceFormat TextureSourceFormat = PixelFormatToSourceFormat(PixelFormat);
		if (TextureSourceFormat == TSF_Invalid)
		{
			GEngine->AddOnScreenDebugMessage(
				0, 10, FColor::Red, "Trying to create persistent asset with unsupported pixel format!");
			return false;
		}
		// Otherwise initialize the source struct with our size and bulk data.
		Texture->Source.Init(Dimensions.X, Dimensions.Y, Dimensions.Z, 1, TextureSourceFormat, BulkData);
	}
#endif	  // WITH_EDITORONLY_DATA
	return true;
}

bool UVolumeTextureToolkit::Create2DTextureTransient(UTexture2D*& OutTexture, EPixelFormat PixelFormat, FIntPoint Dimensions,
	uint8* BulkData, TextureAddress TilingX, TextureAddress TilingY)
{
	int BlockBytes = GPixelFormats[PixelFormat].BlockBytes;
	int TotalBytes = Dimensions.X * Dimensions.Y * BlockBytes;

	UTexture2D* TransientTexture = UTexture2D::CreateTransient(Dimensions.X, Dimensions.Y, PixelFormat);
	TransientTexture->AddressX = TilingX;
	TransientTexture->AddressY = TilingY;

	TransientTexture->SRGB = false;
	TransientTexture->NeverStream = true;

	FTexture2DMipMap& Mip = TransientTexture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);

	if (BulkData)
	{
		FMemory::Memcpy(Data, BulkData, TotalBytes);
	}
	else
	{
		FMemory::Memset(Data, 0, TotalBytes);
	}

	Mip.BulkData.Unlock();

	TransientTexture->UpdateResource();
	OutTexture = TransientTexture;
	return true;
}

bool UVolumeTextureToolkit::CreateVolumeTextureTransient(UVolumeTexture*& OutTexture, EPixelFormat PixelFormat,
	FIntVector Dimensions, uint8* BulkData, bool ShouldUpdateResource, bool bUAVTargettable)
{
	UVolumeTexture* VolumeTexture = nullptr;
	if (bUAVTargettable)
	{
		VolumeTexture = NewObject<UComputeVolumeTexture>(GetTransientPackage(), NAME_None, RF_Transient);
	}
	else
	{
		VolumeTexture = NewObject<UVolumeTexture>(GetTransientPackage(), NAME_None, RF_Transient);
	}

	SetVolumeTextureDetails(VolumeTexture, PixelFormat, Dimensions);
	CreateVolumeTextureMip(VolumeTexture, PixelFormat, Dimensions, BulkData);
	
	// Update resource, mark that the folder needs to be rescan and notify editor
	// about asset creation.
	if (ShouldUpdateResource)
	{
		VolumeTexture->UpdateResource();
	}

	OutTexture = VolumeTexture;
	return true;
}

uint8* UVolumeTextureToolkit::LoadRawFileIntoArray(const FString FileName, const int64 BytesToLoad)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	// Try opening as absolute path.
	IFileHandle* FileHandle = PlatformFile.OpenRead(*FileName);

	// If opening as absolute path failed, open as relative to content directory.
	if (!FileHandle)
	{
		FString FullPath = FPaths::ProjectContentDir() + FileName;
		FileHandle = PlatformFile.OpenRead(*FullPath);
	}

	if (!FileHandle)
	{
		UE_LOG(LogTextureUtils, Error, TEXT("Raw file could not be opened."));
		return nullptr;
	}
	else if (FileHandle->Size() < BytesToLoad)
	{
		UE_LOG(LogTextureUtils, Error, TEXT("Raw file is smaller than expected, cannot read volume."));
		delete FileHandle;
		return nullptr;
	}
	else if (FileHandle->Size() > BytesToLoad)
	{
		UE_LOG(LogTextureUtils, Warning,
			TEXT("Raw File is larger than expected,	check your dimensions and pixel format. (nonfatal, but the texture will "
				 "probably be screwed up)"));
	}

	uint8* LoadedArray = new uint8[BytesToLoad];
	FileHandle->Read(LoadedArray, BytesToLoad);
	delete FileHandle;

	return LoadedArray;
}

uint8* UVolumeTextureToolkit::NormalizeArrayByFormat(
	const FString FormatName, uint8* InArray, const int64 ByteSize, float& OutInMin, float& OutInMax)
{
	// #TODO maybe figure out a nice way to get the format->C++ type instead of this if/else monstrosity
	if (FormatName.Equals("MET_CHAR"))
	{
		return ConvertArrayToNormalizedArray<int8, uint8>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_UCHAR"))
	{
		return ConvertArrayToNormalizedArray<uint8, uint8>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_SHORT"))
	{
		return ConvertArrayToNormalizedArray<int16, uint16>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_USHORT"))
	{
		return ConvertArrayToNormalizedArray<uint16, uint16>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_INT"))
	{
		return ConvertArrayToNormalizedArray<int32, uint16>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_UINT"))
	{
		return ConvertArrayToNormalizedArray<uint32, uint16>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else if (FormatName.Equals("MET_FLOAT"))
	{
		return ConvertArrayToNormalizedArray<float, uint16>(InArray, ByteSize, OutInMin, OutInMax);
	}
	else
	{
		ensure(false);
		return nullptr;
	}
}

float* UVolumeTextureToolkit::ConvertArrayToFloat(uint8* InArray, uint64 VoxelCount, const FString FormatName)
{
	// #TODO maybe figure out a nice way to get the format->C++ type instead of this if/else monstrosity
	if (FormatName.Equals("MET_CHAR"))
	{
		return ConvertArrayToFloatTemplated<int8>(InArray, VoxelCount);
	}
	else if (FormatName.Equals("MET_UCHAR"))
	{
		return ConvertArrayToFloatTemplated<uint8>(InArray, VoxelCount);
	}
	else if (FormatName.Equals("MET_SHORT"))
	{
		return ConvertArrayToFloatTemplated<int16>(InArray, VoxelCount);
	}
	else if (FormatName.Equals("MET_USHORT"))
	{
		return ConvertArrayToFloatTemplated<uint16>(InArray, VoxelCount);
	}
	else if (FormatName.Equals("MET_INT"))
	{
		return ConvertArrayToFloatTemplated<int32>(InArray, VoxelCount);
	}
	else if (FormatName.Equals("MET_UINT"))
	{
		return ConvertArrayToFloatTemplated<uint32>(InArray, VoxelCount);
	}
	else
	{
		ensure(false);
		return nullptr;
	}
}

void UVolumeTextureToolkit::LoadRawIntoNewVolumeTextureAsset(FString RawFileName, FString FolderName, FString TextureName,
	FIntVector Dimensions, uint32 BytexPerVoxel, EPixelFormat OutPixelFormat, bool Persistent, UVolumeTexture*& LoadedTexture)
{
	const int64 TotalSize = Dimensions.X * Dimensions.Y * Dimensions.Z * BytexPerVoxel;

	uint8* TempArray = UVolumeTextureToolkit::LoadRawFileIntoArray(RawFileName, TotalSize);
	if (!TempArray)
	{
		return;
	}

	// Actually create the asset.
	bool Success = UVolumeTextureToolkit::CreateVolumeTextureAsset(
		LoadedTexture, TextureName, FolderName, OutPixelFormat, Dimensions, TempArray, Persistent);

	// Ddelete temp data.
	delete[] TempArray;
}

void UVolumeTextureToolkit::LoadRawIntoVolumeTextureAsset(FString RawFileName, UVolumeTexture* inTexture, FIntVector Dimensions,
	uint32 BytexPerVoxel, EPixelFormat OutPixelFormat, bool Persistent)
{
	const int64 TotalSize = Dimensions.X * Dimensions.Y * Dimensions.Z * BytexPerVoxel;

	uint8* TempArray = UVolumeTextureToolkit::LoadRawFileIntoArray(RawFileName, TotalSize);
	if (!TempArray)
	{
		return;
	}

	// Actually update the asset.
	bool Success = UVolumeTextureToolkit::UpdateVolumeTextureAsset(inTexture, OutPixelFormat, Dimensions, TempArray, Persistent);

	// Delete temp data.
	delete[] TempArray;
}

ETextureSourceFormat UVolumeTextureToolkit::PixelFormatToSourceFormat(EPixelFormat PixelFormat)
{
	// THIS IS UNTESTED FOR FORMATS OTHER THAN G8, G16 AND R16G16B16A16_SNORM!
	switch (PixelFormat)
	{
		case PF_G8:
		case PF_R8_UINT:
			return TSF_G8;

		case PF_G16:
			return TSF_G16;

		case PF_B8G8R8A8:
			return TSF_BGRA8;

		case PF_R8G8B8A8:
			return TSF_RGBA8;

		case PF_R16G16B16A16_SINT:
		case PF_R16G16B16A16_UINT:
			return TSF_RGBA16;

		case PF_R16G16B16A16_SNORM:
		case PF_R16G16B16A16_UNORM:
			return TSF_RGBA16F;

		default:
			return TSF_Invalid;
	}
}

void UVolumeTextureToolkit::SetupVolumeTexture(
	UVolumeTexture*& OutVolumeTexture, EPixelFormat PixelFormat, FIntVector Dimensions, uint8* ConvertedArray, bool Persistent)
{
	SetVolumeTextureDetails(OutVolumeTexture, PixelFormat, Dimensions);
	// Actually create the texture MIP.
	CreateVolumeTextureMip(OutVolumeTexture, PixelFormat, Dimensions, ConvertedArray);
	CreateVolumeTextureEditorData(OutVolumeTexture, PixelFormat, Dimensions, ConvertedArray, Persistent);
}
