// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "VolumeTextureToolkitBPLibrary.h"
#include "TextureUtilities.h"

UVolumeTextureToolkitBPLibrary::UVolumeTextureToolkitBPLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UVolumeTextureToolkitBPLibrary::CreateVolumeTextureAsset(UVolumeTexture*& OutTexture, FString AssetName, FString FolderName,
	EPixelFormat PixelFormat, FIntVector Dimensions, bool bUAVTargettable /*= false*/)
{
	return UVolumeTextureToolkit::CreateVolumeTextureAsset(OutTexture, AssetName, FolderName, PixelFormat, Dimensions, nullptr, true, true);
}

