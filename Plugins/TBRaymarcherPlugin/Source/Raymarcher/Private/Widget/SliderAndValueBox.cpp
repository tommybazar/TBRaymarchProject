// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Widget/SliderAndValueBox.h"

bool USliderAndValueBox::Initialize()
{
	Super::Initialize();

	if (FineTuneCheckBox)
	{
		FineTuneCheckBox->OnCheckStateChanged.Clear();
		FineTuneCheckBox->OnCheckStateChanged.AddDynamic(this, &USliderAndValueBox::OnFineTuningChanged);
	}

	if (ValueSlider)
	{
		ValueSlider->SetMinValue(MinMax.X);
		ValueSlider->SetMaxValue(MinMax.Y);
		ValueSlider->SetStepSize(StepSize);
		ValueSlider->OnValueChanged.Clear();
		ValueSlider->OnValueChanged.AddDynamic(this, &USliderAndValueBox::OnSliderValueChanged);
	}

	SetAllLabelsFromSlider();

	return true;
}

void USliderAndValueBox::SetValue(float Value)
{
	if (ValueSlider)
	{
		if (Value >= ValueSlider->MinValue && Value <= ValueSlider->MaxValue) // Value within current range
		{
			ValueSlider->SetValue(Value);
		}
		else if (Value >= MinMax.X && Value <= MinMax.Y) // Value within MinMax, but not current range.
		{
			OnFineTuningChanged(false);
			ValueSlider->SetValue(Value);
		}
		else // Value isn't within range at all -> extend MinMax slider range
		{
			if (Value > MinMax.Y)
			{
				MinMax.Y = Value;
			}
			else
			{
				MinMax.X = Value;
			}
			OnFineTuningChanged(false);
			ValueSlider->SetValue(Value);
		}
	}

	SetValueLabelFromSlider();
}

void USliderAndValueBox::SetMinMaxLabelsFromSlider()
{
	if (ValueSlider)
	{
		FNumberFormattingOptions NumberFormatOptions;
		NumberFormatOptions.MaximumFractionalDigits = 0;

		if (SliderMinLabel)
		{
			SliderMinLabel->SetText(FText::AsNumber(ValueSlider->MinValue, &NumberFormatOptions));
		}

		if (SliderMaxLabel)
		{
			SliderMaxLabel->SetText(FText::AsNumber(ValueSlider->MaxValue, &NumberFormatOptions));
		}
	}
}

void USliderAndValueBox::SetAllLabelsFromSlider()
{
	SetMinMaxLabelsFromSlider();
	SetValueLabelFromSlider();
}

void USliderAndValueBox::SetValueLabelFromSlider()
{
	if (ValueSlider)
	{
		FNumberFormattingOptions NumberFormatOptions;
		NumberFormatOptions.MaximumFractionalDigits = 0;

		if (SliderValueLabel)
		{
			SliderValueLabel->SetText(FText::AsNumber(ValueSlider->Value, &NumberFormatOptions));
		}
	}
}

void USliderAndValueBox::OnFineTuningChanged(bool bFineTuning)
{
	// When fine-tuning, we shall change the slider's range to a narrow band around current value.
	// FMath::Min and Max are used to avoid the range to get out of the MinMax bounds.
	if (ValueSlider)
	{
		if (bFineTuning)
		{
			ValueSlider->SetMinValue(FMath::Max(MinMax.X, ValueSlider->Value - FineTuneRange));
			ValueSlider->SetMaxValue(FMath::Min(MinMax.Y, ValueSlider->Value + FineTuneRange));
			ValueSlider->SetStepSize(FineTuneStepSize);
		}
		else
		{
			ValueSlider->SetMinValue(MinMax.X);
			ValueSlider->SetMaxValue(MinMax.Y);
			ValueSlider->SetStepSize(StepSize);
		}

		// Set the value to force slider to redraw the bar.
		//ValueSlider->SetValue(ValueSlider->Value);
		SetMinMaxLabelsFromSlider();
	}
}

void USliderAndValueBox::OnSliderValueChanged(float Value)
{
	ValueSlider->Value = Value;
	SetValueLabelFromSlider();

	// Notify owner that value changed.
	OnValueChanged.ExecuteIfBound(Value);
}

#if WITH_EDITOR
void USliderAndValueBox::OnDesignerChanged(const FDesignerChangedEventArgs& EventArgs)
{
	Super::OnDesignerChanged(EventArgs);
	this->Initialize();
}

void USliderAndValueBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	this->Initialize();
}
#endif