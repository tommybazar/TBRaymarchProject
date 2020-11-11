// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CoreMinimal.h"
#include "Widget/SliderAndValueBox.h"
#include "Components/ComboBoxString.h"
#include "Actor/RaymarchVolume.h"

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

	/// The volume this menu is affecting.
	/// #TODO do not touch the volume directly and expose delegates instead? 	
	UPROPERTY(EditAnywhere)
	ARaymarchVolume* AssociatedVolume;

	/// Sets a new volume to be affected by this menu.
	UFUNCTION(BlueprintCallable)
	void SetVolume(ARaymarchVolume* NewRaymarchVolume);
};
