// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Widget/TransferFuncMenu.h"

bool UTransferFuncMenu::Initialize()
{
	Super::Initialize();
	// Bind all delegates.
	if (SaveButton)
	{
		SaveButton->OnClicked.Clear();
		SaveButton->OnClicked.AddDynamic(this, &UTransferFuncMenu::OnSaveClicked);
	}

	if (WindowCenterBox)
	{
		WindowCenterBox->OnValueChanged.BindDynamic(this, &UTransferFuncMenu::OnCenterChanged);
	}

	if (WindowWidthBox)
	{
		WindowWidthBox->OnValueChanged.BindDynamic(this, &UTransferFuncMenu::OnWidthChanged);
	}

	if (LowCutOffCheckBox)
	{
		LowCutOffCheckBox->OnCheckStateChanged.AddDynamic(this, &UTransferFuncMenu::OnLowCutoffToggled);
	}

	if (HighCutOffCheckBox)
	{
		HighCutOffCheckBox->OnCheckStateChanged.AddDynamic(this, &UTransferFuncMenu::OnHighCutoffToggled);
	}

	if (TFSelectionComboBox)
	{
		// Add TF names into box.
		TFSelectionComboBox->ClearOptions();
		for (UCurveLinearColor* TFCurve : TFArray)
		{
			TFSelectionComboBox->AddOption(GetNameSafe(TFCurve));
		}

		TFSelectionComboBox->SetSelectedIndex(0);

		TFSelectionComboBox->OnSelectionChanged.Clear();
		TFSelectionComboBox->OnSelectionChanged.AddDynamic(this, &UTransferFuncMenu::OnTFCurveChanged);
	}
	return true;
}

void UTransferFuncMenu::OnSaveClicked()
{
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SaveCurrentParamsToMHDAsset();
	}
}

void UTransferFuncMenu::OnCenterChanged(float Value)
{
	// Set Center in Raymarch volume
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SetWindowCenter(ListenerVolume->MHDAsset->ImageInfo.NormalizeValue(Value));
	}
}

void UTransferFuncMenu::OnWidthChanged(float Value)
{
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SetWindowWidth(ListenerVolume->MHDAsset->ImageInfo.NormalizeRange(Value));
	}
}

void UTransferFuncMenu::OnLowCutoffToggled(bool bToggledOn)
{
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SetLowCutoff(bToggledOn);
	}
}

void UTransferFuncMenu::OnHighCutoffToggled(bool bToggledOn)
{
	for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
	{
		ListenerVolume->SetHighCutoff(bToggledOn);
	}
}

void UTransferFuncMenu::OnTFCurveChanged(FString CurveName, ESelectInfo::Type SelectType)
{
	UCurveLinearColor* SelectedCurve = nullptr;
	for (UCurveLinearColor* TFCurve : TFArray)
	{
		if (CurveName.Equals(GetNameSafe(TFCurve)))
		{
			SelectedCurve = TFCurve;
			break;
		}
	}

	// Set Curve in Raymarch volume
	if (ListenerVolumes.Num() > 0 && SelectedCurve)
	{
		for (ARaymarchVolume* ListenerVolume : ListenerVolumes)
		{
			if (ListenerVolume->CurrentTFCurve != SelectedCurve)
			{
				ListenerVolume->SetTFCurve(SelectedCurve);
			}
		}
	}
}

void UTransferFuncMenu::OnNewVolumeLoaded()
{
	if (RangeProviderVolume)
	{
		FWindowingParameters DefaultParameters = RangeProviderVolume->MHDAsset->ImageInfo.DefaultWindowingParameters;
		if (WindowCenterBox)
		{
			WindowCenterBox->MinMax =
				FVector2D(RangeProviderVolume->MHDAsset->ImageInfo.MinValue, RangeProviderVolume->MHDAsset->ImageInfo.MaxValue);
			WindowCenterBox->SetValue(RangeProviderVolume->MHDAsset->ImageInfo.DenormalizeValue(DefaultParameters.Center));

			WindowCenterBox->SetAllLabelsFromSlider();
		}

		if (WindowWidthBox)
		{
			WindowWidthBox->MinMax = FVector2D(0, 4000);
			WindowWidthBox->SetValue(RangeProviderVolume->MHDAsset->ImageInfo.DenormalizeRange(DefaultParameters.Width));
			WindowWidthBox->SetAllLabelsFromSlider();
		}

		if (LowCutOffCheckBox)
		{
			LowCutOffCheckBox->SetCheckedState(DefaultParameters.LowCutoff ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		}

		if (HighCutOffCheckBox)
		{
			HighCutOffCheckBox->SetCheckedState(DefaultParameters.HighCutoff ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		}

		if (TFSelectionComboBox)
		{
			int32 OptionIndex = TFSelectionComboBox->FindOptionIndex(GetNameSafe(RangeProviderVolume->CurrentTFCurve));
			if (OptionIndex == -1)
			{
				TFArray.Add(RangeProviderVolume->CurrentTFCurve);
				TFSelectionComboBox->AddOption(GetNameSafe(RangeProviderVolume->CurrentTFCurve));
			}

			TFSelectionComboBox->SetSelectedOption(GetNameSafe(RangeProviderVolume->CurrentTFCurve));
		}
	}
}

void UTransferFuncMenu::SetRangeProviderVolume(ARaymarchVolume* NewRangeProviderVolume)
{
	if (NewRangeProviderVolume)
	{
		if (RangeProviderVolume)
		{
			// Clear OnVolumeLoaded delegate from old provider volume.
			RangeProviderVolume->OnVolumeLoaded.Clear();
		}

		RangeProviderVolume = NewRangeProviderVolume;
		// Bind to a delegate to get notified when the Raymarch Volume loads a new volume.
		RangeProviderVolume->OnVolumeLoaded.BindDynamic(this, &UTransferFuncMenu::OnNewVolumeLoaded);
		OnNewVolumeLoaded();
	}
}

void UTransferFuncMenu::AddListenerVolume(ARaymarchVolume* NewListenerVolume)
{
	if (ListenerVolumes.Contains(NewListenerVolume))
	{
		return;
	}
	ListenerVolumes.Add(NewListenerVolume);
}

void UTransferFuncMenu::RemoveListenerVolume(ARaymarchVolume* RemovedListenerVolume)
{
	ListenerVolumes.Remove(RemovedListenerVolume);
}
