// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Actor/RaymarchVolume.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Widget/SliderAndValueBox.h"
#include <Components/ComboBoxString.h>

#include "MHDLoadMenu.generated.h"

class ARaymarchVolume;

DECLARE_LOG_CATEGORY_EXTERN(MHDLoadMenu, All, All)

/**
 * A menu that lets a user load new MHD files into a RaymarchVolume
 */
UCLASS()
class RAYMARCHER_API UMHDLoadMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Override initialize to bind button functions.
	virtual bool Initialize() override;

	///  Button that will let user load a new MHD file in normalized G16.
	UPROPERTY(meta = (BindWidget))
	UButton* LoadG16Button;

	///  Button that will let user load a new MHD file in F32 format
	UPROPERTY(meta = (BindWidget))
	UButton* LoadF32Button;

	/// Combobox for selecting loaded MHD Assets.
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* AssetSelectionComboBox;

	/// Array of existing MHD Assets that can be set immediately. Will populate the AssetSelection combo box.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UMHDAsset*> AssetArray;
	
	/// Called when LoadG16Button is clicked.
	UFUNCTION()
	void OnLoadNormalizedClicked();

	/// Called when LoadF32Button is clicked.
	UFUNCTION()
	void OnLoadF32Clicked();

	/// Called when AssetSelectionComboBox has a new value selected.
	UFUNCTION()
	void OnAssetSelected(FString AssetName, ESelectInfo::Type SelectType);

	/// The volume this menu is affecting.
	/// #TODO do not touch the volume directly and expose delegates instead?
	UPROPERTY(EditAnywhere)
	ARaymarchVolume* AssociatedVolume;

	/// Sets a new volume to be affected by this menu.
	UFUNCTION(BlueprintCallable)
	void SetVolume(ARaymarchVolume* NewRaymarchVolume);
};
