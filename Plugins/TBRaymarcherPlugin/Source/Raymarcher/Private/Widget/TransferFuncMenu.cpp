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
	if (AssociatedVolume)
	{
		AssociatedVolume->SaveCurrentParamsToMHDAsset();
	}
}

void UTransferFuncMenu::OnCenterChanged(float Value)
{
	// Set Center in Raymarch volume
	if (AssociatedVolume)
	{
		AssociatedVolume->SetWindowCenter(AssociatedVolume->MHDAsset->ImageInfo.NormalizeValue(Value));
	}
}

void UTransferFuncMenu::OnWidthChanged(float Value)
{
	if (AssociatedVolume)
	{
		AssociatedVolume->SetWindowWidth(AssociatedVolume->MHDAsset->ImageInfo.NormalizeRange(Value));
	}
}

void UTransferFuncMenu::OnLowCutoffToggled(bool bToggledOn)
{
	if (AssociatedVolume)
	{
		AssociatedVolume->SetLowCutoff(bToggledOn);
	}
}

void UTransferFuncMenu::OnHighCutoffToggled(bool bToggledOn)
{
	if (AssociatedVolume)
	{
		AssociatedVolume->SetHighCutoff(bToggledOn);
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
	if (AssociatedVolume && SelectedCurve)
	{
		if (AssociatedVolume->CurrentTFCurve != SelectedCurve)
		{
			AssociatedVolume->SetTFCurve(SelectedCurve);
		}
	}
}

void UTransferFuncMenu::OnNewVolumeLoaded()
{
	if (AssociatedVolume)
	{
		FWindowingParameters DefaultParameters = AssociatedVolume->MHDAsset->ImageInfo.DefaultWindowingParameters;
		if (WindowCenterBox)
		{
			WindowCenterBox->MinMax =
				FVector2D(AssociatedVolume->MHDAsset->ImageInfo.MinValue, AssociatedVolume->MHDAsset->ImageInfo.MaxValue);
			WindowCenterBox->SetValue(AssociatedVolume->MHDAsset->ImageInfo.DenormalizeValue(DefaultParameters.Center));

			WindowCenterBox->SetAllLabelsFromSlider();
		}

		if (WindowWidthBox)
		{
			WindowWidthBox->MinMax = FVector2D(0, 4000);
			WindowWidthBox->SetValue(AssociatedVolume->MHDAsset->ImageInfo.DenormalizeRange(DefaultParameters.Width));
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
			int32 OptionIndex = TFSelectionComboBox->FindOptionIndex(GetNameSafe(AssociatedVolume->CurrentTFCurve));
			if (OptionIndex == -1)
			{
				TFArray.Add(AssociatedVolume->CurrentTFCurve);
				TFSelectionComboBox->AddOption(GetNameSafe(AssociatedVolume->CurrentTFCurve));
			}

			TFSelectionComboBox->SetSelectedOption(GetNameSafe(AssociatedVolume->CurrentTFCurve));
		}
	}
}

void UTransferFuncMenu::SetVolume(ARaymarchVolume* NewRaymarchVolume)
{
	if (NewRaymarchVolume)
	{
		AssociatedVolume = NewRaymarchVolume;
		// Bind to a delegate to get notified when the Raymarch Volume loads a new volume.
		AssociatedVolume->OnVolumeLoaded.BindDynamic(this, &UTransferFuncMenu::OnNewVolumeLoaded);
		OnNewVolumeLoaded();
	}
}

#if WITH_EDITOR
void UTransferFuncMenu::OnDesignerChanged(const FDesignerChangedEventArgs& EventArgs)
{
	Initialize();
	Super::OnDesignerChanged(EventArgs);
}

void UTransferFuncMenu::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Initialize();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif