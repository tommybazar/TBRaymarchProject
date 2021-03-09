// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "MHDVolumeTextureFactory.h"

#include "Containers/UnrealString.h"
#include "Engine/VolumeTexture.h"
#include "VolumeAsset/VolumeAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "TextureUtilities.h"

/* UMHDVolumeTextureFactory structors
 *****************************************************************************/

UMHDVolumeTextureFactory::UMHDVolumeTextureFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("mhd;")) + NSLOCTEXT("UMHDVolumeTextureFactory", "FormatMhd", ".mhd File").ToString());

	SupportedClass = UVolumeTexture::StaticClass();
	bCreateNew = false;
	bEditorImport = true;

	// Set import priority to 1. In Raymarcher plugin we can create raymarchable assets from .mhd files
	// and that factory has priority 2, so it gets preference.
	ImportPriority = 1;
}

#pragma optimize("", off)
UObject* UMHDVolumeTextureFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UVolumeTexture* VolumeTexture = nullptr;
	
	FString FilePath;
	FString FileNamePart;
	FString ExtensionPart;

	FPaths::Split(Filename, FilePath, FileNamePart, ExtensionPart);
	FileNamePart = FPaths::MakeValidFileName(FileNamePart);
	// Periods are not cool for package names -> get rid of them.
	FileNamePart.ReplaceCharInline('.', '_');

	UVolumeAsset* MHDAsset =
		NewObject<UVolumeAsset>(InParent, UVolumeAsset::StaticClass(), FName("MHD_" + FileNamePart), Flags);

	MHDAsset->ImageInfo = UVolumeAsset::ParseHeaderToImageInfo(Filename);
	if (!MHDAsset->ImageInfo.bParseWasSuccessful)
	{
		// MHD parsing failed -> return null.
		return nullptr;
	}

	int64 TotalBytes = MHDAsset->ImageInfo.GetTotalBytes();

	uint8* LoadedArray;

	if (MHDAsset->ImageInfo.bIsCompressed)
	{
		LoadedArray = UVolumeTextureToolkit::LoadZLibCompressedRawFileIntoArray(
			FilePath + "/" + MHDAsset->ImageInfo.DataFileName, TotalBytes, MHDAsset->ImageInfo.CompressedBytes);
	}
	else
	{
		LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + MHDAsset->ImageInfo.DataFileName, TotalBytes);
	}

	EPixelFormat PixelFormat = PF_G8;

	if (!MHDAsset->ImageInfo.bParseWasSuccessful)
	{
		return nullptr;
	}

	EAppReturnType::Type DialogAnswer = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::Yes,
		NSLOCTEXT("Volumetrics", "Normalize?",
			"Would you like your volume converted to G8 or G16 and normalized to the whole type range? This will allow it to "
			"be saved persistently as an asset and make inspecting it with Texture editor easier. Also, rendering with the "
			"default raymarching material and transfer function will be easier.\nIf your volume already is MET_(U)CHAR or "
			"MET_(U)SHORT, your volume will be persistent even without conversion, but values might be all over the place."));

	if (DialogAnswer == EAppReturnType::Yes)
	{
		// We want to normalize and cap at G16 -> convert
		uint8* ConvertedArray = UVolumeTextureToolkit::NormalizeArrayByFormat(
			MHDAsset->ImageInfo.VoxelFormat, LoadedArray, TotalBytes, MHDAsset->ImageInfo.MinValue, MHDAsset->ImageInfo.MaxValue);
		delete[] LoadedArray;
		LoadedArray = ConvertedArray;
		if (MHDAsset->ImageInfo.BytesPerVoxel > 1)
		{
			MHDAsset->ImageInfo.BytesPerVoxel = 2;
			PixelFormat = PF_G16;
		}
		MHDAsset->ImageInfo.bIsNormalized = true;
	}
	else if (MHDAsset->ImageInfo.VoxelFormat != EVolumeVoxelFormat::Float)
	{
		EAppReturnType::Type DialogAnswer2 = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::Yes,
			NSLOCTEXT("Volumetrics", "Normalize to R32?",
				"Should we convert it to R32_FLOAT? This will make sure the materials can read it, but will make the "
				"texture un-saveable."));

		if (DialogAnswer2 == EAppReturnType::Yes)
		{
			const int64 TotalVoxels = MHDAsset->ImageInfo.GetTotalVoxels();
			float* ConvertedArray =
				UVolumeTextureToolkit::ConvertArrayToFloat(MHDAsset->ImageInfo.VoxelFormat, LoadedArray, TotalVoxels);
			delete[] LoadedArray;
			LoadedArray = reinterpret_cast<uint8*>(ConvertedArray);
			TotalBytes = TotalVoxels * 4;
			MHDAsset->ImageInfo.BytesPerVoxel = 4;
			MHDAsset->ImageInfo.VoxelFormat = EVolumeVoxelFormat::Float;
		}

		// Leave the texture in the original format and don't normalize.
		if (MHDAsset->ImageInfo.BytesPerVoxel == 2)
		{
			PixelFormat = PF_G16;
		}
		else if (MHDAsset->ImageInfo.BytesPerVoxel == 4)
		{
			// Cannot be saved natively (unreal only supports G8 and G16 as texture source).
			// #todo? Guess we could encode this into a RGBA 8bit color and then decode later.
			PixelFormat = PF_R32_FLOAT;
			FileNamePart = "Transient_" + FileNamePart;
		}
		MHDAsset->ImageInfo.bIsNormalized = false;
	}
	else
	{
		FileNamePart = "Transient_" + FileNamePart;
		PixelFormat = PF_R32_FLOAT;
		MHDAsset->ImageInfo.bIsNormalized = false;
	}

	// Create the Volume texture.
	VolumeTexture = NewObject<UVolumeTexture>(InParent, InClass, FName("Data_" + FileNamePart), Flags);
	// Initialize it with the details taken from MHD.
	UVolumeTextureToolkit::SetupVolumeTexture(
		VolumeTexture, PixelFormat, MHDAsset->ImageInfo.Dimensions, LoadedArray, (MHDAsset->ImageInfo.BytesPerVoxel != 4));

	VolumeTexture->UpdateResource();
	bOutOperationCanceled = false;

	delete[] LoadedArray;

	// Add created MHD file to AdditionalImportedObjects so it also gets saved in-editor.
	MHDAsset->AssociatedTexture = VolumeTexture;
	MHDAsset->MarkPackageDirty();
	AdditionalImportedObjects.Add(MHDAsset);

	return VolumeTexture;
}
#pragma optimize("", on)
