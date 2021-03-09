// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Actor/RaymarchVolume.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "CoreMinimal.h"
#include "Widget/SliderAndValueBox.h"

#include "TransferFuncMenu.generated.h"

/**
 * A menu that lets a user switch transfer functions and window width and center parameters.
 */
UCLASS()
class RAYMARCHER_API UTransferFuncMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	bool Initialize() override;

	///  Button that will save current values in the menu to the volume associated MHD asset.
	UPROPERTY(meta = (BindWidget))
	UButton* SaveButton;

	/// Slider and value box for changing Window Center.
	UPROPERTY(meta = (BindWidget))
	USliderAndValueBox* WindowCenterBox;

	/// Slider and value box for changing Window Width.
	UPROPERTY(meta = (BindWidget))
	USliderAndValueBox* WindowWidthBox;

	/// Combobox for selecting transfer functions.
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* TFSelectionComboBox;

	/// Checkbox to enable/disable low cutoff.
	UPROPERTY(meta = (BindWidget))
	UCheckBox* LowCutOffCheckBox;

	/// Checkbox to enable/disable high cutoff.
	UPROPERTY(meta = (BindWidget))
	UCheckBox* HighCutOffCheckBox;

	/// Array of Transfer Functions that will populate the selection combo box..
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UCurveLinearColor*> TFArray;

	/// Called when SaveButton is clicked.
	UFUNCTION()
	void OnSaveClicked();

	/// Called when Window Center is clicked.
	UFUNCTION()
	void OnCenterChanged(float Value);

	/// Called when Window Width is clicked.
	UFUNCTION()
	void OnWidthChanged(float Value);

	/// Called when the Low cutoff checkbox is toggled.
	UFUNCTION()
	void OnLowCutoffToggled(bool bToggledOn);

	/// Called when the Low cutoff checkbox is toggled.
	UFUNCTION()
	void OnHighCutoffToggled(bool bToggledOn);

	/// Called when a TF curve is selected in the combobox.
	UFUNCTION()
	void OnTFCurveChanged(FString CurveName, ESelectInfo::Type SelectType);

	/// Called when a new MHD file/volume is loaded into the associated raymarch volume.
	UFUNCTION()
	void OnNewVolumeLoaded();

	/// When this volume loads a new new VolumeAsset loaded, it will provide this menu with the new reasonable ranges to set sliders
	/// to.
	UPROPERTY(EditAnywhere)
	ARaymarchVolume* RangeProviderVolume;

	/// This array holds all volumes affected by the changes to the values in this menu. Allows multiple volumes to receive updates
	/// from one TF menu.
	UPROPERTY(EditAnywhere)
	TArray<ARaymarchVolume*> ListenerVolumes;

	/// Sets a new volume to be this menu's sliders range provider.
	UFUNCTION(BlueprintCallable)
	void SetRangeProviderVolume(ARaymarchVolume* NewRaymarchVolume);

	/// Adds a volume into the array of volumes affected by changes to this menu's sliders.
	UFUNCTION(BlueprintCallable)
	void AddListenerVolume(ARaymarchVolume* NewListenerVolume);

	/// Removes a volume from the array of volumes affected by changes to this menu's sliders.
	UFUNCTION(BlueprintCallable)
	void RemoveListenerVolume(ARaymarchVolume* RemovedListenerVolume);
};
