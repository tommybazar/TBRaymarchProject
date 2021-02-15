// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "VolumeAsset/VolumeAsset.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TextureUtilities.h"
#include "sstream"
#include "string"

#include <AssetRegistryModule.h>

bool UVolumeAsset::ParseFromString(const FString FileString)
{
	// #TODO UE probably has a nicer string parser than istringstream...
	// And the way I'm doing this is the ugliest you could imagine.
	// But hey, this is probably literally the first C++ code I ever wrote in Unreal, so I'm keeping it this way, so
	// I can look at it and shed a tear of remembering the sweet, sweet days of yesteryear.

	// #TODO stop being sentimental and use FConsole::Parse()
	{
		std::string MyStdString(TCHAR_TO_UTF8(*FileString));
		std::istringstream inStream = std::istringstream(MyStdString);

		std::string ReadWord;

		// Skip until we get to Dimensions.
		while (inStream.good() && ReadWord != "DimSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after DimSize now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;
			// Read the three values;
			inStream >> ImageInfo.Dimensions.X;
			inStream >> ImageInfo.Dimensions.Y;
			inStream >> ImageInfo.Dimensions.Z;
		}
		else
		{
			return false;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to spacing.
		while (inStream.good() && ReadWord != "ElementSpacing" && ReadWord != "ElementSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementSpacing/ElementSize now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;
			// Read the three values;
			inStream >> ImageInfo.Spacing.X;
			inStream >> ImageInfo.Spacing.Y;
			inStream >> ImageInfo.Spacing.Z;

			ImageInfo.WorldDimensions = ImageInfo.Spacing * FVector(ImageInfo.Dimensions);
		}
		else
		{
			return false;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "ElementType")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> ReadWord;
			if (ReadWord == "MET_UCHAR")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedChar;
			}
			else if (ReadWord == "MET_CHAR")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::SignedChar;
			}

			else if (ReadWord == "MET_USHORT")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedShort;
			}
			else if (ReadWord == "MET_SHORT")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::SignedShort;
			}
			else if (ReadWord == "MET_UINT")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedInt;
			}
			else if (ReadWord == "MET_INT")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::SignedInt;
			}
			else if (ReadWord == "MET_FLOAT")
			{
				ImageInfo.VoxelFormat = EVolumeVoxelFormat::Float;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		ImageInfo.BytesPerVoxel = FVolumeInfo::VoxelFormatByteSize(ImageInfo.VoxelFormat);
		ImageInfo.bIsSigned = FVolumeInfo::IsVoxelFormatSigned(ImageInfo.VoxelFormat);

		// Check for compressed data size tag.

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "CompressedDataSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			ImageInfo.bIsCompressed = true;

			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> ImageInfo.CompressedBytes;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "ElementDataFile")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> ReadWord;
		}
		else
		{
			return false;
		}
		DataFileName = FString(ReadWord.c_str());
		ParseSuccessful = true;
		// Return with constructor that sets success to true.
		return true;
	}
}

FString UVolumeAsset::ToString() const
{
	FVector WorldDimensions = GetWorldDimensions();

	FString text = "File name " + GetName() + " details:" + "\nDimensions = " + ImageInfo.Dimensions.ToString() +
				   "\nSpacing : " + ImageInfo.Spacing.ToString() + "\nWorld Size MM : " + ImageInfo.Dimensions.ToString() +
				   "\nDefault window center : " + FString::SanitizeFloat(ImageInfo.DefaultWindowingParameters.Center) +
				   "\nDefault window width : " + FString::SanitizeFloat(ImageInfo.DefaultWindowingParameters.Width) +
				   "\nOriginal Range : [" + FString::SanitizeFloat(ImageInfo.MinValue) + " - " +
				   FString::SanitizeFloat(ImageInfo.MaxValue) + "]";
	return text;
}

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

bool UVolumeAsset::ParseHeaderToImageInfo(FString FileName)
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
			return false;
		}
	}
	return ParseFromString(FileContent);
}

FVector UVolumeAsset::GetWorldDimensions() const
{
	return ImageInfo.WorldDimensions;
}

int64 FVolumeInfo::GetTotalBytes()
{
	return Dimensions.X * Dimensions.Y * Dimensions.Z * BytesPerVoxel;
}

int64 FVolumeInfo::GetTotalVoxels()
{
	return Dimensions.X * Dimensions.Y * Dimensions.Z;
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

	if (!OutMHDAsset || !OutMHDAsset->ParseHeaderToImageInfo(InFullMHDFileName))
	{
		// MHD info doesn't exist or parsing failed -> return null.
		return nullptr;
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
			FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);
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
			FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);
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
			FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes, OutMHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);
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
