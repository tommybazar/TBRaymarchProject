#pragma once

#include "EngineMinimal.h"

#include "WindowingParameters.generated.h"

/// Struct for raymarch windowing parameters. These work exactly the same as DICOM window.
USTRUCT(BlueprintType)
struct FWindowingParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Center = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Width = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool LowCutoff = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool HighCutoff = true;

	/** Transforms the 4 values into a FLinear color to be used in materials.**/
	FLinearColor ToLinearColor()
	{
		return FLinearColor(Center, Width, LowCutoff, HighCutoff);
	}
};