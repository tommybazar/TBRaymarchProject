// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Rendering/FractalShaders.h"

#include "AssetRegistryModule.h"
#include "TextureUtilities.h"

IMPLEMENT_GLOBAL_SHADER(
	FCalculateMandelbulbSDFCS, "/FractalMarcher/Private/CalculateMandelbulbSDF.usf", "MainComputeShader", SF_Compute);

// IMPLEMENT_GLOBAL_SHADER(FQuickLight_CS, "/FractalMarcher/Private/CalculateMandelbulbSDF.usf", "MainComputeShader", SF_Compute);

// For making statistics about GPU use - Adding Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("Mandelbulb SDF Calculation"), STAT_GPU_MandelbulbSDF, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUMandelbulbSDF, TEXT("MandelbulbSDF"));

// For making statistics about GPU use - Changing Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("Mandelbulb Illumination Calculation"), STAT_GPU_MandelbulbLight, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUMandelbulbLight, TEXT("MandelbulbLight"));

#define GROUPSIZE_X 16	// This has to be the same as in the compute shader's spec [16, 16, 4]
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 4

void EnqueueRenderCommand_CalculateMandelbulbSDF(FMandelbulbSDFResources Resources)
{
	if (!Resources.MandelbulbVolumeUAVRef || Resources.Extent <= 0)
	{
		return;
	}

	// Call the actual rendering code on RenderThread.
	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	([=](FRHICommandListImmediate& RHICmdList) { CalculateMandelbulbSDF_RenderThread(RHICmdList, Resources); });
}

// #TODO profile with different dimensions.

void CalculateMandelbulbSDF_RenderThread(FRHICommandListImmediate& RHICmdList, FMandelbulbSDFResources Parameters)
{
	check(IsInRenderingThread());

	// For GPU profiling.
	SCOPED_DRAW_EVENTF(RHICmdList, CalculateMandelbulbSDF_RenderThread, TEXT("Calculate Mandelbulb SDF"));
	SCOPED_GPU_STAT(RHICmdList, GPUMandelbulbSDF);

	// Find and set compute shader
	TShaderMapRef<FCalculateMandelbulbSDFCS> ComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	// Don't need barriers on these - we only ever read/write to the same pixel from one thread ->
	// no race conditions But we definitely need to transition the resource to Compute-shader
	// accessible, otherwise the renderer might touch our textures while we're writing there.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, Parameters.MandelbulbVolumeUAVRef);

	// Set parameters, resources, LightAdded and ALightVolume
	ComputeShader->SetMandelbulbSDFParameters(RHICmdList, ShaderRHI, Parameters);

	uint32 GroupSizeX = FMath::DivideAndRoundUp(Parameters.MandelbulbVolume->GetSizeX(), GROUPSIZE_X);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(Parameters.MandelbulbVolume->GetSizeY(), GROUPSIZE_Y);
	uint32 GroupSizeZ = FMath::DivideAndRoundUp(Parameters.MandelbulbVolume->GetSizeZ(), GROUPSIZE_Z);

	RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, GroupSizeZ);

	// Transition resources back to the renderer.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, Parameters.MandelbulbVolumeUAVRef);
}

#undef LOCTEXT_NAMESPACE
