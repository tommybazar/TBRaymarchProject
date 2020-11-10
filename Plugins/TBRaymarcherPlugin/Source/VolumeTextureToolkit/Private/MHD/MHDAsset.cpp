// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "MHDAsset.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TextureUtilities.h"

#include <AssetRegistryModule.h>

bool UMHDAsset::ParseFromString(const FString FileString)
{
	// #TODO UE probably has a nicer string parser than istringstream...
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
		while (inStream.good() && ReadWord != "ElementSpacing")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementSpacing now.
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
			if (ReadWord == "MET_UCHAR" || ReadWord == "MET_CHAR")
			{
				ImageInfo.BytesPerVoxel = 1;
			}
			else if (ReadWord == "MET_USHORT" || ReadWord == "MET_SHORT")
			{
				ImageInfo.BytesPerVoxel = 2;
			}
			else if (ReadWord == "MET_UINT" || ReadWord == "MET_INT" || ReadWord == "MET_FLOAT")
			{
				ImageInfo.BytesPerVoxel = 4;
			}
			else
			{
				return false;
			}

			ElementType = FString(ReadWord.c_str());

			FString ElementTypeSigned = ElementType.RightChop(4);
			if (ElementTypeSigned.StartsWith("U"))
			{
				ImageInfo.bIsSigned = false;
			}
			else
			{
				ImageInfo.bIsSigned = true;
			}
		}
		else
		{
			return false;
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

FString UMHDAsset::ToString() const
{
	FVector WorldDimensions = GetWorldDimensions();

	FString text = "MHD file name " + GetName() + " details:" + "\nDimensions = " + ImageInfo.Dimensions.ToString() +
				   "\nSpacing : " + ImageInfo.Spacing.ToString() + "\nWorld Size MM : " + ImageInfo.Dimensions.ToString() +
				   "\nDefault window center : " + FString::SanitizeFloat(ImageInfo.DefaultWindowingParameters.Center) +
				   "\nDefault window width : " + FString::SanitizeFloat(ImageInfo.DefaultWindowingParameters.Width) + "\nOriginal Range : [" +
				   FString::SanitizeFloat(ImageInfo.MinValue) + " - " + FString::SanitizeFloat(ImageInfo.MaxValue) + "]";
	return text;
}

bool UMHDAsset::LoadAndParseMhdFile(FString FileName)
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

FVector UMHDAsset::GetWorldDimensions() const
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

#if WITH_EDITOR
void UMHDAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	const FName MemberPropertyName =
		(PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// If the curve property changed, broadcast the delegate.
	if (MemberPropertyName != GET_MEMBER_NAME_CHECKED(UMHDAsset, TransferFuncCurve))
	{
		OnImageInfoChanged.Broadcast();
	}
}

void UMHDAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName MemberPropertyName =
		(PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// If the curve property changed, broadcast the delegate.
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMHDAsset, TransferFuncCurve))
	{
		OnCurveChanged.Broadcast(TransferFuncCurve);
	}
}

UMHDAsset* UMHDAsset::CreateAndLoadMHDAsset(
	const FString InFullMHDFileName, bool bIsPersistent, FString SaveFolder, const FString SaveName)
{
	UMHDAsset* OutMHDAsset = nullptr;
	if (bIsPersistent)
	{
		// Add slash at the end if it's not already there.
		if (!SaveFolder.EndsWith("/"))
		{
			SaveFolder += "/";
		}
		// Create persistent package if we want the MHD info to be saveable.
		FString MHDPackageName = SaveFolder + "MHD_" + SaveName;
		UPackage* MHDPackage = CreatePackage(nullptr, *MHDPackageName);
		MHDPackage->FullyLoad();

		OutMHDAsset = NewObject<UMHDAsset>(MHDPackage, UMHDAsset::StaticClass(), FName("MHD_" + SaveName), RF_Standalone | RF_Public);
		if (OutMHDAsset)
		{
			FAssetRegistryModule::AssetCreated(OutMHDAsset);
		}
	}
	else
	{
		OutMHDAsset = NewObject<UMHDAsset>(
			GetTransientPackage(), UMHDAsset::StaticClass(), FName("MHD_Transient_" + SaveName), RF_Standalone | RF_Public);
	}

	if (!OutMHDAsset || !OutMHDAsset->LoadAndParseMhdFile(InFullMHDFileName))
	{
		// MHD info doesn't exist or parsing failed -> return null.
		return nullptr;
	}
	return OutMHDAsset;
}

void UMHDAsset::CreateTextureFromMhdFileNormalized(const FString Filename, UMHDAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture,
	bool bIsPersistent, const FString OutFolder)
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
	uint8* LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);

	// We want to normalize and cap at G16, perform that normalization.
	uint8* ConvertedArray = UVolumeTextureToolkit::NormalizeArrayByFormat(
		OutMHDAsset->ElementType, LoadedArray, TotalBytes, OutMHDAsset->ImageInfo.MinValue, OutMHDAsset->ImageInfo.MaxValue);
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
		DataPackage = CreatePackage(nullptr, *DataPackageName);
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

void UMHDAsset::CreateTextureFromMhdFileR32F(const FString Filename, UMHDAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture)
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
	uint8* LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);

	float* ConvertedArray = nullptr;
	if (!OutMHDAsset->ElementType.Equals("MET_FLOAT"))
	{
		// We want to convert the whole array to R32_Float. Notice we need to provide the voxel count, not byte count!
		ConvertedArray = UVolumeTextureToolkit::ConvertArrayToFloat(
			LoadedArray, TotalBytes / OutMHDAsset->ImageInfo.BytesPerVoxel, OutMHDAsset->ElementType);
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

void UMHDAsset::CreateTextureFromMhdFileNoConversion(
	const FString Filename, const FString OutFolder, UMHDAsset*& OutMHDAsset, UVolumeTexture*& OutVolumeTexture)
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
	uint8* LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + OutMHDAsset->DataFileName, TotalBytes);

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
		UPackage* DataPackage = CreatePackage(nullptr, *DataPackageName);
		DataPackage->FullyLoad();

		// Create the Volume texture.
		OutVolumeTexture = NewObject<UVolumeTexture>(
			DataPackage, UVolumeTexture::StaticClass(), FName("Data_" + FileNamePart), RF_Standalone | RF_Public);
	}

	// Initialize it with the details taken from MHD.
	UVolumeTextureToolkit::SetupVolumeTexture(OutVolumeTexture, PixelFormat, OutMHDAsset->ImageInfo.Dimensions,
		LoadedArray, OutMHDAsset->ImageInfo.BytesPerVoxel != 4);
	// Update resource, notify asset registry.
	OutVolumeTexture->UpdateResource();
	FAssetRegistryModule::AssetCreated(OutVolumeTexture);
	OutMHDAsset->AssociatedTexture = OutVolumeTexture;

	delete[] LoadedArray;
}

#endif