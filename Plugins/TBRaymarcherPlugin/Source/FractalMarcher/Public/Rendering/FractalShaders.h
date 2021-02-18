// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "Logging/MessageLog.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameters.h"
#include "RHIResources.h"
#include "Engine/VolumeTexture.h"

#include "FractalShaders.generated.h"

USTRUCT()
struct FMandelbulbSDFResources
{
	GENERATED_BODY()

	/// Mandelbulb volume texture.
	UPROPERTY();
	UVolumeTexture* MandelbulbVolume;

	/// The compute shader UAV. Used for all the calculations.
	FUnorderedAccessViewRHIRef MandelbulbVolumeUAVRef;

	UPROPERTY();
	FIntVector MandelbulbVolumeDimensions = FIntVector(128, 128, 128);

	/// Center of the volume texture will correspond this world coordinate when calculating SDF.
	UPROPERTY();
	FVector Center = FVector(0, 0, 0);

	/// The extent the whole volume covers. -> The whole volume will cover a box of Center +- FVector(Extent/2)
	UPROPERTY();
	float Extent = 2;

	/// Power used for mandelbulb calculation.
	UPROPERTY();
	float Power = 8;

	UPROPERTY();
	bool bIsInitialized = false;
};

// A shader calculating a Mandelbulb SDF function in a volume texture.
class FCalculateMandelbulbSDFCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FCalculateMandelbulbSDFCS, Global, FRACTALMARCHER_API);

public:
	FCalculateMandelbulbSDFCS() : FGlobalShader()
	{
	}

	FCalculateMandelbulbSDFCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		// Volume texture + Transfer function uniforms
		MandelbulbVolumeUAV.Bind(Initializer.ParameterMap, TEXT("MandelbulbVolumeUAV"), SPF_Mandatory);
		Center.Bind(Initializer.ParameterMap, TEXT("Center"), SPF_Mandatory);
		Extent.Bind(Initializer.ParameterMap, TEXT("Extent"), SPF_Mandatory);
		Power.Bind(Initializer.ParameterMap, TEXT("Power"), SPF_Mandatory);
		MandelbulbVolumeDimensions.Bind(Initializer.ParameterMap, TEXT("MandelbulbVolumeDimensions"), SPF_Mandatory);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void SetMandelbulbSDFParameters(
		FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FMandelbulbSDFResources Parameters)
	{
		// Set the multiplier to -1 if we're removing the light. Set to 1 if adding it.
		SetUAVParameter(RHICmdList, ShaderRHI, MandelbulbVolumeUAV, Parameters.MandelbulbVolumeUAVRef);
		SetShaderValue(RHICmdList, ShaderRHI, MandelbulbVolumeDimensions, FVector(Parameters.MandelbulbVolumeDimensions));

		SetShaderValue(RHICmdList, ShaderRHI, Center, Parameters.Center);
		SetShaderValue(RHICmdList, ShaderRHI, Extent, Parameters.Extent);
		SetShaderValue(RHICmdList, ShaderRHI, Power, Parameters.Power);
	}

protected:
	// Parameter for the added/removed multiplier.
	LAYOUT_FIELD(FShaderResourceParameter, MandelbulbVolumeUAV);
	LAYOUT_FIELD(FShaderParameter, MandelbulbVolumeDimensions);
	LAYOUT_FIELD(FShaderParameter, Center);
	LAYOUT_FIELD(FShaderParameter, Extent);
	LAYOUT_FIELD(FShaderParameter, Power);
	LAYOUT_FIELD(FShaderParameter, Power2);
};

void EnqueueRenderCommand_CalculateMandelbulbSDF(FMandelbulbSDFResources Resources);

void CalculateMandelbulbSDF_RenderThread(FRHICommandListImmediate& RHICmdList, FMandelbulbSDFResources Resources);