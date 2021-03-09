// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "CoreMinimal.h"

#include "SliderAndValueBox.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSliderValueChanged, float, Value);

/**
 *
 */
UCLASS()
class RAYMARCHER_API USliderAndValueBox : public UUserWidget
{
	GENERATED_BODY()

public:
	bool Initialize() override;

	// Label describing the current minimal value on the slider.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SliderMinLabel;

	// Label describing the current maximal value on the slider.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SliderMaxLabel;

	// Label describing the current value on the slider.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SliderValueLabel;

	// Slider for changing the value.
	UPROPERTY(meta = (BindWidget))
	USlider* ValueSlider;

	// Checkbox allowing us to fine-tune the slider (make the min/max of the slider close to current value).
	UPROPERTY(meta = (BindWidget))
	UCheckBox* FineTuneCheckBox;

	// Min-Max range of the slider.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D MinMax = FVector2D(-1000, 3000);

	// +- range of the slider when in fine-tuning mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FineTuneRange = 100;

	// Step Size of the slider when in regular mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FineTuneStepSize = 1;

	// Step Size of the slider when in fine-tuning mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepSize = 5;

	// Delegate to bind to when we want to be notified of value being changed.
	FOnSliderValueChanged OnValueChanged;

	/// Sets a value of the slider.
	void SetValue(float Value);

	/// Sets the min and max labels to the values matching the ones in the slider.
	void SetMinMaxLabelsFromSlider();

	/// Makes all the FText labels (value, min, max) consistent with the value contained in slider.
	void SetAllLabelsFromSlider();

	/// Makes the value label consistent with the value contained in sliders.
	void SetValueLabelFromSlider();

	/// Called when the fine tuning status changes (is/isn't fine-tuning)
	UFUNCTION()
	void OnFineTuningChanged(bool bFineTuning);

	/// Called when the slider value is changed.
	UFUNCTION()
	void OnSliderValueChanged(float Value);

#if WITH_EDITOR
	void OnDesignerChanged(const FDesignerChangedEventArgs& EventArgs);
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif
};
