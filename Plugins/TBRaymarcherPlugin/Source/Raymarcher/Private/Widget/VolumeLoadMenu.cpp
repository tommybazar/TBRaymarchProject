#include "Widget/VolumeLoadMenu.h"

#include <DesktopPlatform/Public/DesktopPlatformModule.h>
#include <DesktopPlatform/Public/IDesktopPlatform.h>

DEFINE_LOG_CATEGORY(VolumeLoadMenu)

bool UVolumeLoadMenu::Initialize()
{
	Super::Initialize();
	if (LoadG16Button)
	{
		LoadG16Button->OnClicked.Clear();
		LoadG16Button->OnClicked.AddDynamic(this, &UVolumeLoadMenu::OnLoadNormalizedClicked);
	}

	if (LoadF32Button)
	{
		LoadF32Button->OnClicked.Clear();
		LoadF32Button->OnClicked.AddDynamic(this, &UVolumeLoadMenu::OnLoadF32Clicked);
	}

	if (AssetSelectionComboBox)
	{
		// Add existing MHD files into box.
		AssetSelectionComboBox->ClearOptions();
		for (UVolumeAsset* MHDAsset : AssetArray)
		{
			AssetSelectionComboBox->AddOption(GetNameSafe(MHDAsset));
		}

		AssetSelectionComboBox->OnSelectionChanged.Clear();
		AssetSelectionComboBox->OnSelectionChanged.AddDynamic(this, &UVolumeLoadMenu::OnAssetSelected);
	}

	return true;
}

void UVolumeLoadMenu::OnLoadNormalizedClicked()
{
	if (ListenerVolumes.Num() > 0)
	{
		// Get best window for file picker dialog.
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(TSharedPtr<SWindow>());
		const void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
											 ? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
											 : nullptr;

		TArray<FString> FileNames;
		// Open the file picker for mhd files.
		bool Success =
			FDesktopPlatformModule::Get()->OpenFileDialog(ParentWindowHandle, "Select MHD file", "", "", ".mhd", 0, FileNames);
		if (FileNames.Num() > 0)
		{
			FString Filename = FileNames[0];
			UVolumeAsset* OutAsset;
			UVolumeTexture* OutTexture;
			UVolumeAsset::CreateAssetFromMhdFileNormalized(Filename, OutAsset, OutTexture, false);

			if (OutAsset)
			{
				UE_LOG(VolumeLoadMenu, Display,
					TEXT("Creating MHD asset from filename %s succeeded, seting MHD asset into associated listener volumes."),
					*Filename);


				// Add the asset to list of already loaded assets and select it through the combobox. This will call
				// OnAssetSelected().
				AssetArray.Add(OutAsset);
				AssetSelectionComboBox->AddOption(GetNameSafe(OutAsset));
				AssetSelectionComboBox->SetSelectedOption(GetNameSafe(OutAsset));
			}
			else
			{
				UE_LOG(VolumeLoadMenu, Warning, TEXT("Creating MHD asset from filename %s failed."), *Filename);
			}
		}
		else
		{
			UE_LOG(VolumeLoadMenu, Warning, TEXT("Loading of MHD file cancelled. Dialog creation failed or no file was selected."));
		}
	}
	else
	{
		UE_LOG(VolumeLoadMenu, Warning, TEXT("Attempted to load MHD file with no Raymarched Volume associated with menu, exiting."));
	}
}

void UVolumeLoadMenu::OnLoadF32Clicked()
{
	if (ListenerVolumes.Num() > 0)
	{
		// Get best window for file picker dialog.
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(TSharedPtr<SWindow>());
		const void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
											 ? ParentWindow->GetNativeWindow()->GetOSWindowHandle()
											 : nullptr;

		TArray<FString> FileNames;
		// Open the file picker for mhd files.
		bool Success =
			FDesktopPlatformModule::Get()->OpenFileDialog(ParentWindowHandle, "Select MHD file", "", "", ".mhd", 0, FileNames);
		if (FileNames.Num() > 0)
		{
			FString Filename = FileNames[0];
			UVolumeAsset* OutAsset;
			UVolumeTexture* OutTexture;
			UVolumeAsset::CreateAssetFromMhdFileR32F(Filename, OutAsset, OutTexture);

			if (OutAsset)
			{
				UE_LOG(VolumeLoadMenu, Display,
					TEXT("Creating MHD asset from filename %s succeeded, seting MHD asset into associated listener volumes."),
					*Filename);

				// Add the asset to list of already loaded assets and select it through the combobox. This will call OnAssetSelected().
				AssetArray.Add(OutAsset);
				AssetSelectionComboBox->AddOption(GetNameSafe(OutAsset));
				AssetSelectionComboBox->SetSelectedOption(GetNameSafe(OutAsset));
			}
			else
			{
				UE_LOG(VolumeLoadMenu, Warning, TEXT("Creating MHD asset from filename %s failed."), *Filename);
			}
		}
		else
		{
			UE_LOG(VolumeLoadMenu, Warning, TEXT("Loading of MHD file cancelled. Dialog creation failed or no file was selected."));
		}
	}
	else
	{
		UE_LOG(VolumeLoadMenu, Warning, TEXT("Attempted to load MHD file with no Raymarched Volume associated with menu, exiting."));
	}
}

void UVolumeLoadMenu::OnAssetSelected(FString AssetName, ESelectInfo::Type SelectType)
{
	UVolumeAsset* SelectedAsset = nullptr;
	for (UVolumeAsset* Asset : AssetArray)
	{
		if (AssetName.Equals(GetNameSafe(Asset)))
		{
			SelectedAsset = Asset;
			break;
		}
	}
	if (!SelectedAsset)
	{
		return;
	}

	// Set Volume Asset to all listeners.
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SetMHDAsset(SelectedAsset);
	}
}

void UVolumeLoadMenu::RemoveListenerVolume(ARaymarchVolume* RemovedRaymarchVolume)
{
	ListenerVolumes.Remove(RemovedRaymarchVolume);
}

void UVolumeLoadMenu::AddListenerVolume(ARaymarchVolume* NewRaymarchVolume)
{
	if (ListenerVolumes.Contains(NewRaymarchVolume))
	{
		return;
	}
	ListenerVolumes.Add(NewRaymarchVolume);
}
