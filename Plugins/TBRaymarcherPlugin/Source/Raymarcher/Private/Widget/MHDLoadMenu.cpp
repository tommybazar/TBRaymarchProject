#include "Widget/MHDLoadMenu.h"

#include <DesktopPlatform/Public/DesktopPlatformModule.h>
#include <DesktopPlatform/Public/IDesktopPlatform.h>

DEFINE_LOG_CATEGORY(MHDLoadMenu)

bool UMHDLoadMenu::Initialize()
{
	Super::Initialize();
	if (LoadG16Button)
	{
		LoadG16Button->OnClicked.Clear();
		LoadG16Button->OnClicked.AddDynamic(this, &UMHDLoadMenu::OnLoadNormalizedClicked);
	}

	if (LoadF32Button)
	{
		LoadF32Button->OnClicked.Clear();
		LoadF32Button->OnClicked.AddDynamic(this, &UMHDLoadMenu::OnLoadF32Clicked);
	}

	if (AssetSelectionComboBox)
	{
		// Add existing MHD files into box.
		AssetSelectionComboBox->ClearOptions();
		for (UMHDAsset* MHDAsset : AssetArray)
		{
			AssetSelectionComboBox->AddOption(GetNameSafe(MHDAsset));
		}

		AssetSelectionComboBox->OnSelectionChanged.Clear();
		AssetSelectionComboBox->OnSelectionChanged.AddDynamic(this, &UMHDLoadMenu::OnAssetSelected);
	}

	return true;
}

void UMHDLoadMenu::OnLoadNormalizedClicked()
{
	if (AssociatedVolume)
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
			UMHDAsset* OutAsset;
			UVolumeTexture* OutTexture;
			UMHDAsset::CreateAssetFromMhdFileNormalized(Filename, OutAsset, OutTexture, false);

			if (OutAsset)
			{
				UE_LOG(MHDLoadMenu, Display,
					TEXT("Creating MHD asset from filename %s succeeded, seting MHD asset into volume %s."), *Filename,
					*AssociatedVolume->GetName());

				// Add the asset to list of already loaded assets and select it through the combobox.
				AssetArray.Add(OutAsset);
				AssetSelectionComboBox->AddOption(GetNameSafe(OutAsset));
				AssetSelectionComboBox->SetSelectedOption(GetNameSafe(OutAsset));
			}
			else
			{
				UE_LOG(MHDLoadMenu, Warning, TEXT("Creating MHD asset from filename %s failed."), *Filename);
			}
		}
		else
		{
			UE_LOG(MHDLoadMenu, Warning, TEXT("Loading of MHD file cancelled. Dialog creation failed or no file was selected."));
		}
	}
	else
	{
		UE_LOG(MHDLoadMenu, Warning, TEXT("Attempted to load MHD file with no Raymarched Volume associated with menu, exiting."));
	}
}

void UMHDLoadMenu::OnLoadF32Clicked()
{
	if (AssociatedVolume)
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
			UMHDAsset* OutAsset;
			UVolumeTexture* OutTexture;
			UMHDAsset::CreateAssetFromMhdFileR32F(Filename, OutAsset, OutTexture);

			if (OutAsset)
			{
				UE_LOG(MHDLoadMenu, Display,
					TEXT("Creating MHD asset from filename %s succeeded, seting MHD asset into volume %s."),
					*Filename, *AssociatedVolume->GetName());

				// Add the asset to list of already loaded assets and select it through the combobox.
				AssetArray.Add(OutAsset);
				AssetSelectionComboBox->AddOption(GetNameSafe(OutAsset));
				AssetSelectionComboBox->SetSelectedOption(GetNameSafe(OutAsset));
			}
			else
			{
				UE_LOG(MHDLoadMenu, Warning, TEXT("Creating MHD asset from filename %s failed."), *Filename);
			}
		}
		else
		{
			UE_LOG(MHDLoadMenu, Warning, TEXT("Loading of MHD file cancelled. Dialog creation failed or no file was selected."));
		}
	}
	else
	{
		UE_LOG(MHDLoadMenu, Warning, TEXT("Attempted to load MHD file with no Raymarched Volume associated with menu, exiting."));
	}
}

void UMHDLoadMenu::OnAssetSelected(FString AssetName, ESelectInfo::Type SelectType)
{
	UMHDAsset* SelectedAsset = nullptr;
	for (UMHDAsset* Asset : AssetArray)
	{
		if (AssetName.Equals(GetNameSafe(Asset)))
		{
			SelectedAsset = Asset;
			break;
		}
	}

	// Set Curve in Raymarch volume
	if (AssociatedVolume && SelectedAsset)
	{
		AssociatedVolume->SetMHDAsset(SelectedAsset);
	}
}

void UMHDLoadMenu::SetVolume(ARaymarchVolume* NewRaymarchVolume)
{
	AssociatedVolume = NewRaymarchVolume;
}
