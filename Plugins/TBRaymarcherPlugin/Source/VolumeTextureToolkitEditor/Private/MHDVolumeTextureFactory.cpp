// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "MHDVolumeTextureFactory.h"

#include "Containers/UnrealString.h"
#include "Engine/VolumeTexture.h"
#include "MHD/MHDAsset.h"
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

	UMHDAsset* MHDAsset = NewObject<UMHDAsset>(InParent, UMHDAsset::StaticClass(), FName("MHD_" + FileNamePart), Flags);

	if (!MHDAsset->LoadAndParseMhdFile(Filename))
	{
		// MHD parsing failed -> return null.
		return nullptr;
	}

	int64 TotalBytes = MHDAsset->ImageInfo.GetTotalBytes();

	uint8* LoadedArray = UVolumeTextureToolkit::LoadRawFileIntoArray(FilePath + "/" + MHDAsset->DataFileName, TotalBytes);
	EPixelFormat PixelFormat = PF_G8;

	if (MHDAsset->ParseSuccessful)
	{
		EAppReturnType::Type DialogAnswer = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::Yes,
			NSLOCTEXT("Volumetrics", "Normalize?",
				"Would you like your volume converted to G8 or G16 and normalized to the whole type range? This will allow it to "
				"be saved persistently as an asset and make inspecting it with Texture editor easier. Also, rendering with the "
				"default raymarching material and transfer function will be easier.\nIf your volume already is MET_(U)CHAR or "
				"MET_(U)SHORT, your volume will be persistent even without conversion."));

		if (DialogAnswer == EAppReturnType::Yes)
		{
			// We want to normalize and cap at G16 -> convert
			uint8* ConvertedArray = UVolumeTextureToolkit::NormalizeArrayByFormat(
				MHDAsset->ElementType, LoadedArray, TotalBytes, MHDAsset->ImageInfo.MinValue, MHDAsset->ImageInfo.MaxValue);
			delete[] LoadedArray;
			LoadedArray = ConvertedArray;
			if (MHDAsset->ImageInfo.BytesPerVoxel > 1)
			{
				MHDAsset->ImageInfo.BytesPerVoxel = 2;
				PixelFormat = PF_G16;
			}
		}
		else
		{
			if (!MHDAsset->ElementType.Equals("MET_FLOAT"))
			{
				EAppReturnType::Type DialogAnswer2 = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::Yes,
					NSLOCTEXT("Volumetrics", "Normalize to R32?",
						"Should we convert it to R32_FLOAT? This will make sure the materials can read it, but will make the "
						"texture un-saveable."));

				if (DialogAnswer2 == EAppReturnType::Yes)
				{
					const int64 TotalVoxels = MHDAsset->ImageInfo.GetTotalVoxels();
					float* ConvertedArray =
						UVolumeTextureToolkit::ConvertArrayToFloat(LoadedArray, TotalVoxels, MHDAsset->ElementType);
					delete[] LoadedArray;
					LoadedArray = reinterpret_cast<uint8*>(ConvertedArray);
					TotalBytes = TotalVoxels * 4;
					MHDAsset->ImageInfo.BytesPerVoxel = 4;
					MHDAsset->ElementType = "MET_FLOAT";
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
			}
		}
	}
	else
	{
		return nullptr;
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
