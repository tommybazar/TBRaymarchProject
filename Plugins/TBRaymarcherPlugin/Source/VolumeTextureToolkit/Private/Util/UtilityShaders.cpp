#include "Util/UtilityShaders.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 16	  // This has to be the same as in the compute shader's spec [X, X, 1]

IMPLEMENT_GLOBAL_SHADER(
	FClearVolumeTextureShaderCS, "/VolumeTextureToolkit/Private/ClearVolumeTextureShader.usf", "MainComputeShader", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(
	FClearFloatRWTextureCS, "/VolumeTextureToolkit/Private/ClearTextureShader.usf", "MainComputeShader", SF_Compute);

// For making statistics about GPU use - Clearing Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("ClearingVolumeTextures"), STAT_GPU_ClearingVolumeTextures, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUClearingVolumeTextures, TEXT("ClearingVolumeTextures"));

void ClearVolumeTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture3D* VolumeResourceRef, float ClearValues)
{
	// For GPU profiling.
	SCOPED_DRAW_EVENTF(RHICmdList, ClearVolumeTexture_RenderThread, TEXT("Clearing volume texture"));
	SCOPED_GPU_STAT(RHICmdList, GPUClearingVolumeTextures);

	TShaderMapRef<FClearVolumeTextureShaderCS> ComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	// RHICmdList.TransitionResource(EResourceTransitionAccess::ERWNoBarrier,
	// LightVolumeResource);
	FUnorderedAccessViewRHIRef VolumeUAVRef = RHICreateUnorderedAccessView(VolumeResourceRef);

	// Don't need barriers on these - we only ever read/write to the same pixel from one thread ->
	// no race conditions But we definitely need to transition the resource to Compute-shader
	// accessible, otherwise the renderer might touch our textures while we're writing them.
	RHICmdList.Transition(FRHITransitionInfo(VolumeUAVRef, ERHIAccess::UAVGraphics, ERHIAccess::UAVCompute));

	ComputeShader->SetParameters(RHICmdList, VolumeUAVRef, ClearValues, VolumeResourceRef->GetSizeZ());

	uint32 GroupSizeX = FMath::DivideAndRoundUp((int32) VolumeResourceRef->GetSizeX(), NUM_THREADS_PER_GROUP_DIMENSION);
	uint32 GroupSizeY = FMath::DivideAndRoundUp((int32) VolumeResourceRef->GetSizeY(), NUM_THREADS_PER_GROUP_DIMENSION);

	RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
	ComputeShader->UnbindUAV(RHICmdList);
	RHICmdList.Transition(FRHITransitionInfo(VolumeUAVRef, ERHIAccess::UAVCompute, ERHIAccess::UAVGraphics));
}

/// Clears a FloatTexture accesible as a UAV.
void Clear2DTexture_RenderThread(
	FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* TextureUAVRef, FIntPoint TextureSize, float Value)
{
	TShaderMapRef<FClearFloatRWTextureCS> ShaderRef(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ComputeShader = ShaderRef.GetComputeShader();
	RHICmdList.SetComputeShader(ComputeShader);

	RHICmdList.Transition(FRHITransitionInfo(TextureUAVRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

	ShaderRef->SetParameters(RHICmdList, TextureUAVRef, Value);
	uint32 GroupSizeX = FMath::DivideAndRoundUp(TextureSize.X, NUM_THREADS_PER_GROUP_DIMENSION);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(TextureSize.Y, NUM_THREADS_PER_GROUP_DIMENSION);

	RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
	//  DispatchComputeShader(RHICmdList, ShaderRef, GroupSizeX, GroupSizeY, 1);
	ShaderRef->UnbindUAV(RHICmdList);

	RHICmdList.Transition(FRHITransitionInfo(TextureUAVRef, ERHIAccess::UAVCompute, ERHIAccess::UAVGraphics));
}