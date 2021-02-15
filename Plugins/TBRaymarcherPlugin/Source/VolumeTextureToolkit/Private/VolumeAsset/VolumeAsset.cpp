// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "VolumeAsset/VolumeAsset.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TextureUtilities.h"

#include <AssetRegistryModule.h>

int32 FVolumeInfo::VoxelFormatByteSize(EVolumeVoxelFormat InFormat)
{
	switch (InFormat)
	{
		case EVolumeVoxelFormat::UnsignedChar:	  // fall through
		case EVolumeVoxelFormat::SignedChar:
			return 1;
		case EVolumeVoxelFormat::UnsignedShort:	   // fall through
		case EVolumeVoxelFormat::SignedShort:
			return 2;
		case EVolumeVoxelFormat::UnsignedInt:	 // fall through
		case EVolumeVoxelFormat::SignedInt:		 // fall through
		case EVolumeVoxelFormat::Float:
			return 4;
		default:
			ensure(false);
			return 0;
	}
}

bool FVolumeInfo::IsVoxelFormatSigned(EVolumeVoxelFormat InFormat)
{
	switch (InFormat)
	{
		case EVolumeVoxelFormat::UnsignedChar:	   // fall through
		case EVolumeVoxelFormat::UnsignedShort:	   // fall through
		case EVolumeVoxelFormat::UnsignedInt:
			return false;
		case EVolumeVoxelFormat::SignedChar:	 // fall through
		case EVolumeVoxelFormat::SignedShort:	 // fall through
		case EVolumeVoxelFormat::SignedInt:		 // fall through
		case EVolumeVoxelFormat::Float:
			return true;
		default:
			ensure(false);
			return false;
	}
}

FVolumeInfo UVolumeAsset::ParseHeaderToImageInfo(FString FileName)
{
	FString FileContent;
	// First, try to read as absolute path
	if (!FFileHelper::LoadFileToString(/*out*/ FileContent, *FileName))
	{
		// Try it as a relative path
		FString RelativePath = FPaths::ProjectContentDir();
		FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*RelativePath) + FileName;
		FileName = FullPath;
		if (!FFileHelper::LoadFileToString(/*out*/ FileContent, *FullPath))
		{
			FVolumeInfo Info;
			Info.bParseWasSuccessful = false;
			return Info;
		}
	}
	return FVolumeInfo::ParseFromString(FileContent);
}

UVolumeAsset* UVolumeAsset::CreateAndLoadMHDAsset(
	const FString InFullMHDFileName, bool bIsPersistent, FString SaveFolder, const FString SaveName)
{
	UVolumeAsset* OutMHDAsset = nullptr;
	if (bIsPersistent)
	{
		// Add slash at the end if it's not already there.
		if (!SaveFolder.EndsWith("/"))
		{
			SaveFolder += "/";
		}
		// Create persistent package if we want the MHD info to be saveable.
		FString MHDPackageName = SaveFolder + "MHD_" + SaveName;
		UPackage* MHDPackage = CreatePackage(*MHDPackageName);
		MHDPackage->FullyLoad();

		OutMHDAsset =
			NewObject<UVolumeAsset>(MHDPackage, UVolumeAsset::StaticClass(), FName("MHD_" + SaveName), RF_Standalone | RF_Public);
		if (OutMHDAsset)
		{
			FAssetRegistryModule::AssetCreated(OutMHDAsset);
		}
	}
	else
	{
		OutMHDAsset = NewObject<UVolumeAsset>(
			GetTransientPackage(), UVolumeAsset::StaticClass(), FName("MHD_Transient_" + SaveName), RF_Standalone | RF_Public);
	}

	if (!OutMHDAsset)
	{
		// MHD info doesn't exist or parsing failed -> return null.
		return nullptr;
	}
	else
	{
		OutMHDAsset->ImageInfo = ParseHeaderToImageInfo(InFullMHDFileName);
	}
	return OutMHDAsset;
}

void UVolumeAsset::CreateAssetFromMhdFileNormalized(const FString Filename, UVolumeAsset*& OutMHDAsset,
	UVolumeTexture*& OutVolumeTexture, bool bIsPersistent, const FString OutFolder)
{
	FString FilePath;
	FString FileNamePart;
	FString ExtensionPart;

	FPaths::Split(Filename, FilePath, FileNamePart, ExtensionPart);
	FileNamePart = FPaths::MakeValidFileName(FileNamePart);
	// Periods are not cool for package names -> get rid of them.
	FileNamePart.ReplaceCharInline('.', '_');

	// Allow for creating non-persistent G8/G16 MHDs by passing bIsPersistent.
	OutMHDAsset = CreateAndLoadMHDAsset(Filename, bIsPersistent, OutFolder, FileNamePart);
	if (!OutMHDAsset)
	{
		return;
	}
	OutMHDAsset->ImageInfo.bIsNormalized = true;

	int64 TotalBytes = OutMHDAsset->ImageInfo.GetTotalBytes();
	uint8* LoadedArray;

	if (OutMHDAsset->ImageInfo.bIsCompressed)
	{
		LoadedArray = UVolumeTextureToolkit::LoadZLibCompressedRawFileIntoArray(
			FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes);
	}

	// We want to normalize and cap at G16, perform that normalization.
	uint8* ConvertedArray = UVolumeTextureToolkit::NormalizeArrayByFormat(OutMHDAsset->ImageInfo.VoxelFormat, LoadedArray,
		TotalBytes, OutMHDAsset->ImageInfo.MinValue, OutMHDAsset->ImageInfo.MaxValue);
	delete[] LoadedArray;

	EPixelFormat PixelFormat = PF_G8;
	if (OutMHDAsset->ImageInfo.BytesPerVoxel > 1)
	{
		OutMHDAsset->ImageInfo.BytesPerVoxel = 2;
		PixelFormat = PF_G16;
	}
	// If not persistent, add the "Transient_" part to the asset name.
	FString TransientString = bIsPersistent ? "" : "Transient_";
	FileNamePart = TransientString + FileNamePart;

	UPackage* DataPackage = GetTransientPackage();
	if (bIsPersistent)
	{
		FString DataPackageName = OutFolder + "Data_" + FileNamePart;
		DataPackage = CreatePackage(*DataPackageName);
		DataPackage->FullyLoad();
	}

	// Create the Volume texture.
	OutVolumeTexture = NewObject<UVolumeTexture>(
		DataPackage, UVolumeTexture::StaticClass(), FName("Data_" + FileNamePart), RF_Standalone | RF_Public);
	// Initialize it with the details taken from MHD.
	UVolumeTextureToolkit::SetupVolumeTexture(
		OutVolumeTexture, PixelFormat, OutMHDAsset->ImageInfo.Dimensions, ConvertedArray, bIsPersistent);
	// Update resource, notify asset registry.
	OutVolumeTexture->UpdateResource();
	FAssetRegistryModule::AssetCreated(OutVolumeTexture);

	OutMHDAsset->AssociatedTexture = OutVolumeTexture;

	delete[] ConvertedArray;
}

void UVolumeAsset::CreateAssetFromMhdFileR32F(const FString Filename, UVolumeAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture)
{
	FString FilePath;
	FString FileNamePart;
	FString ExtensionPart;

	FPaths::Split(Filename, FilePath, FileNamePart, ExtensionPart);
	FileNamePart = FPaths::MakeValidFileName(FileNamePart);
	// Periods are not cool for package names -> get rid of them.
	FileNamePart.ReplaceCharInline('.', '_');

	OutMHDAsset = CreateAndLoadMHDAsset(Filename, false, "", FileNamePart);
	if (!OutMHDAsset)
	{
		return;
	}
	OutMHDAsset->ImageInfo.bIsNormalized = false;

	int64 TotalBytes = OutMHDAsset->ImageInfo.GetTotalBytes();
	uint8* LoadedArray;

	if (OutMHDAsset->ImageInfo.bIsCompressed)
	{
		LoadedArray = UVolumeTextureToolkit::LoadZLibCompressedRawFileIntoArray(
			FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes);
	}

	float* ConvertedArray = nullptr;
	if (OutMHDAsset->ImageInfo.VoxelFormat != EVolumeVoxelFormat::Float)
	{
		// We want to convert the whole array to R32_Float. Notice we need to provide the voxel count, not byte count!
		ConvertedArray = UVolumeTextureToolkit::ConvertArrayToFloat(
			OutMHDAsset->ImageInfo.VoxelFormat, LoadedArray, TotalBytes / OutMHDAsset->ImageInfo.BytesPerVoxel);
		delete[] LoadedArray;
	}
	else
	{
		// Just use the loaded array if the format is float already.
		ConvertedArray = reinterpret_cast<float*>(LoadedArray);
	}

	EPixelFormat PixelFormat = PF_R32_FLOAT;
	OutMHDAsset->ImageInfo.BytesPerVoxel = 4;

	// Create the Volume texture in the transient package.
	OutVolumeTexture = NewObject<UVolumeTexture>(
		GetTransientPackage(), UVolumeTexture::StaticClass(), FName("Data_Transient_" + FileNamePart), RF_Standalone | RF_Public);
	// Initialize it with the details taken from MHD.
	UVolumeTextureToolkit::SetupVolumeTexture(
		OutVolumeTexture, PixelFormat, OutMHDAsset->ImageInfo.Dimensions, reinterpret_cast<uint8*>(ConvertedArray), false);
	// Update resource, notify asset registry.
	OutVolumeTexture->UpdateResource();
	FAssetRegistryModule::AssetCreated(OutVolumeTexture);
	OutMHDAsset->AssociatedTexture = OutVolumeTexture;

	delete[] ConvertedArray;
}

void UVolumeAsset::CreateAssetFromMhdFileNoConversion(
	const FString Filename, const FString OutFolder, UVolumeAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture)
{
	FString FilePath;
	FString FileNamePart;
	FString ExtensionPart;

	FPaths::Split(Filename, FilePath, FileNamePart, ExtensionPart);
	FileNamePart = FPaths::MakeValidFileName(FileNamePart);
	// Periods are not cool for package names -> get rid of them.
	FileNamePart.ReplaceCharInline('.', '_');

	// If we have 2 bytes per voxel or less, make the asset persistent.
	if (OutMHDAsset->ImageInfo.BytesPerVoxel <= 2)
	{
		OutMHDAsset = CreateAndLoadMHDAsset(Filename, true, OutFolder, FileNamePart);
	}
	else
	{
		OutMHDAsset = CreateAndLoadMHDAsset(Filename, false);
	}

	if (!OutMHDAsset)
	{
		return;
	}

	int64 TotalBytes = OutMHDAsset->ImageInfo.GetTotalBytes();
	uint8* LoadedArray;
	if (OutMHDAsset->ImageInfo.bIsCompressed)
	{
		LoadedArray = UVolumeTextureToolkit::LoadZLibCompressedRawFileIntoArray(
			FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->ImageInfo.DataFileName, TotalBytes);
	}
	// We want to convert the whole array to R32_Float. Notice we need to provide the Voxel count, not bytes!

	EPixelFormat PixelFormat = PF_G8;
	if (OutMHDAsset->ImageInfo.BytesPerVoxel == 2)
	{
		PixelFormat = PF_G16;
	}
	else if (OutMHDAsset->ImageInfo.BytesPerVoxel == 4)
	{
		PixelFormat = PF_R32_FLOAT;
	}

	if (OutMHDAsset->ImageInfo.BytesPerVoxel == 4)
	{
		// Create the Volume texture in the transient package.
		OutVolumeTexture = NewObject<UVolumeTexture>(GetTransientPackage(), UVolumeTexture::StaticClass(),
			FName("Data_Transient" + FileNamePart), RF_Standalone | RF_Public);
	}
	else
	{
		// Create a package for the Volume texture.
		FString DataPackageName = OutFolder + "Data_" + FileNamePart;
		UPackage* DataPackage = CreatePackage(*DataPackageName);
		DataPackage->FullyLoad();

		// Create the Volume texture.
		OutVolumeTexture = NewObject<UVolumeTexture>(
			DataPackage, UVolumeTexture::StaticClass(), FName("Data_" + FileNamePart), RF_Standalone | RF_Public);
	}

	// Initialize it with the details taken from MHD.
	UVolumeTextureToolkit::SetupVolumeTexture(
		OutVolumeTexture, PixelFormat, OutMHDAsset->ImageInfo.Dimensions, LoadedArray, OutMHDAsset->ImageInfo.BytesPerVoxel != 4);
	// Update resource, notify asset registry.
	OutVolumeTexture->UpdateResource();
	FAssetRegistryModule::AssetCreated(OutVolumeTexture);
	OutMHDAsset->AssociatedTexture = OutVolumeTexture;

	delete[] LoadedArray;
}

#if WITH_EDITOR
void UVolumeAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	const FName MemberPropertyName =
		(PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// If the curve property changed, broadcast the delegate.
	if (MemberPropertyName != GET_MEMBER_NAME_CHECKED(UVolumeAsset, TransferFuncCurve))
	{
		OnImageInfoChanged.Broadcast();
	}
}

void UVolumeAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName MemberPropertyName =
		(PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// If the curve property changed, broadcast the delegate.
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UVolumeAsset, TransferFuncCurve))
	{
		OnCurveChanged.Broadcast(TransferFuncCurve);
	}
}
#endif
