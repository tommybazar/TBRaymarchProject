// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#pragma once

#include "CoreMinimal.h"

#include "Rendering/RaymarchTypes.h"

#include "Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/VolumeTexture.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "Logging/MessageLog.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneInterface.h"
#include "SceneUtils.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameters.h"
#include "ComputeVolumeTexture.h"
#include "MHD/WindowingParameters.h"

/// Creates a SamplerState RHI with "Border" handling of outside-of-UV reads.
/// The color read from outside the buffer is specified by the BorderColorInt.
FSamplerStateRHIRef GetBufferSamplerRef(uint32 BorderColorInt);

/// Returns the integer specifying the color needed for the border sampler.
uint32 GetBorderColorIntSingle(FDirLightParameters LightParams, FMajorAxes MajorAxes, unsigned index);

/// Returns clipping parameters from global world parameters.
FClippingPlaneParameters RAYMARCHER_API GetLocalClippingParameters(const FRaymarchWorldParameters WorldParameters);

void AddDirLightToSingleLightVolume_RenderThread(FRHICommandListImmediate& RHICmdList, FBasicRaymarchRenderingResources Resources,
	const FDirLightParameters LightParameters, const bool Added, const FRaymarchWorldParameters WorldParameters);

void ChangeDirLightInSingleLightVolume_RenderThread(FRHICommandListImmediate& RHICmdList,
	FBasicRaymarchRenderingResources Resources, const FDirLightParameters OldLightParameters,
	const FDirLightParameters NewLightParameters, const FRaymarchWorldParameters WorldParameters);

void ClearVolumeTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture3D* ALightVolumeResource, float ClearValue);

// Compute shader for clearing a single-channel 2D float RW texture
class FClearFloatRWTextureCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FClearFloatRWTextureCS, Global, RAYMARCHER_API);

public:
	FClearFloatRWTextureCS() : FGlobalShader()
	{
	}
	FClearFloatRWTextureCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		ClearValue.Bind(Initializer.ParameterMap, TEXT("ClearValue"), SPF_Mandatory);
		ClearTexture2DRW.Bind(Initializer.ParameterMap, TEXT("ClearTextureRW"), SPF_Mandatory);
	}


	void SetParameters(FRHICommandList& RHICmdList, FRHIUnorderedAccessView* TextureRW, float Value)
	{
		SetUAVParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), ClearTexture2DRW, TextureRW);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ClearValue, Value);
	}

	void UnbindUAV(FRHICommandList& RHICmdList)
	{
		SetUAVParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), ClearTexture2DRW, nullptr);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	const FShaderParameter& GetClearColorParameter()
	{
		return ClearValue;
	}

	const FShaderResourceParameter& GetClearTextureRWParameter()
	{
		return ClearTexture2DRW;
	}

protected:
	LAYOUT_FIELD(FShaderResourceParameter, ClearTexture2DRW);
	LAYOUT_FIELD(FShaderParameter, ClearValue);
};

//
// Shaders for illumination propagation follow.
//

// Parent shader to any shader working with a raymarched volume.
// Contains Volume, Transfer Function, Windowing parameters, Clipping Parameters and StepSize.
// StepSize is needed so that we know how far through the volume we went in each step
// so we can properly calculate the opacity.
class FRaymarchVolumeShader : public FGlobalShader
{
	DECLARE_EXPORTED_TYPE_LAYOUT(FRaymarchVolumeShader, RAYMARCHER_API, Virtual);

public:
	FRaymarchVolumeShader() : FGlobalShader()
	{
	}
	virtual ~FRaymarchVolumeShader();

	FRaymarchVolumeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		Volume.Bind(Initializer.ParameterMap, TEXT("Volume"), SPF_Mandatory);
		VolumeSampler.Bind(Initializer.ParameterMap, TEXT("VolumeSampler"), SPF_Mandatory);

		TransferFunc.Bind(Initializer.ParameterMap, TEXT("TransferFunc"), SPF_Mandatory);
		TransferFuncSampler.Bind(Initializer.ParameterMap, TEXT("TransferFuncSampler"), SPF_Mandatory);
		LocalClippingCenter.Bind(Initializer.ParameterMap, TEXT("LocalClippingCenter"), SPF_Mandatory);
		LocalClippingDirection.Bind(Initializer.ParameterMap, TEXT("LocalClippingDirection"), SPF_Mandatory);

		WindowingParameters.Bind(Initializer.ParameterMap, TEXT("WindowingParameters"), SPF_Mandatory);
		StepSize.Bind(Initializer.ParameterMap, TEXT("StepSize"), SPF_Mandatory);
	}

	void SetRaymarchResources(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI,
		const FTexture3DRHIRef pVolume, const FTexture2DRHIRef pTransferFunc, FWindowingParameters WindowingParams)
	{
		// Set the zero color to fit the zero point of the windowing parameters (Center - Width/2)
		// so that after sampling out of bounds, it gets changed to 0 on the Transfer Function in
		// GetTransferFuncPosition() hlsl function.
		float ZeroTFValue = WindowingParams.Center - 0.5 * WindowingParams.Width;

		FLinearColor VolumeClearColor = FLinearColor(ZeroTFValue, 0.0, 0.0, 0.0);
		const uint32 BorderColorInt = VolumeClearColor.ToFColor(false).ToPackedARGB();

		// Create a static sampler reference and bind it together with the volume texture.
		FSamplerStateRHIRef DataVolumeSamplerRef = RHICreateSamplerState(
			FSamplerStateInitializerRHI(SF_Trilinear, AM_Border, AM_Border, AM_Border, 0, 1, 0, 0, BorderColorInt));

		FSamplerStateRHIRef TFSamplerRef =
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, ShaderRHI, Volume, VolumeSampler, DataVolumeSamplerRef, pVolume);
		SetTextureParameter(RHICmdList, ShaderRHI, TransferFunc, TransferFuncSampler, TFSamplerRef, pTransferFunc);
	}

	virtual void UnbindResources(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI)
	{
		SetTextureParameter(RHICmdList, ShaderRHI, Volume, nullptr);
		SetTextureParameter(RHICmdList, ShaderRHI, TransferFunc, nullptr);
	}

	// Sets the shader uniforms in the pipeline.
	void SetRaymarchParameters(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI,
		FClippingPlaneParameters LocalClippingParams, FLinearColor pWindowingParameters)
	{
		SetShaderValue(RHICmdList, ShaderRHI, LocalClippingCenter, LocalClippingParams.Center);
		SetShaderValue(RHICmdList, ShaderRHI, LocalClippingDirection, LocalClippingParams.Direction);
		SetShaderValue(RHICmdList, ShaderRHI, WindowingParameters, pWindowingParameters);
	}

	// Sets the step-size. This is a crucial parameter, because when raymarching, we need to know how long our step was,
	// so that we can calculate how large an effect the volume's density has.
	void SetStepSize(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, float pStepSize)
	{
		SetShaderValue(RHICmdList, ShaderRHI, StepSize, pStepSize);
	}

protected:
	// Volume texture + transfer function resource parameters
	LAYOUT_FIELD(FShaderResourceParameter, Volume);
	LAYOUT_FIELD(FShaderResourceParameter, VolumeSampler);
	LAYOUT_FIELD(FShaderResourceParameter, TransferFunc);
	LAYOUT_FIELD(FShaderResourceParameter, TransferFuncSampler);
	// Clipping uniforms
	LAYOUT_FIELD(FShaderParameter, LocalClippingCenter);
	LAYOUT_FIELD(FShaderParameter, LocalClippingDirection);
	// TF intensity Domain
	LAYOUT_FIELD(FShaderParameter, WindowingParameters);
	// Step size taken each iteration
	LAYOUT_FIELD(FShaderParameter, StepSize);
};

// Parent Shader to shaders for propagating light as described by Sundén and Ropinski.
// @cite - https://ieeexplore.ieee.org/abstract/document/7156382
//
// The shader only works on one layer of the Volume Texture at once. That's why we need
// the Loop variable and Permutation Matrix.
// See AddDirLightShader.usf and AddDirLightToSingleLightVolume_RenderThread
class FLightPropagationShader : public FRaymarchVolumeShader
{
	// INTERNAL_DECLARE_SHADER_TYPE_COMMON(FLightPropagationShader, Global, RAYMARCHER_API);
	DECLARE_EXPORTED_TYPE_LAYOUT(FLightPropagationShader, RAYMARCHER_API, Virtual);

public:
	FLightPropagationShader() : FRaymarchVolumeShader()
	{
	}

	~FLightPropagationShader(){};

	FLightPropagationShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FRaymarchVolumeShader(Initializer)
	{
		Loop.Bind(Initializer.ParameterMap, TEXT("Loop"), SPF_Mandatory);
		PermutationMatrix.Bind(Initializer.ParameterMap, TEXT("PermutationMatrix"), SPF_Mandatory);

		// Read buffer and sampler.
		ReadBuffer.Bind(Initializer.ParameterMap, TEXT("ReadBuffer"), SPF_Mandatory);
		ReadBufferSampler.Bind(Initializer.ParameterMap, TEXT("ReadBufferSampler"), SPF_Mandatory);
		// Write buffer.
		WriteBuffer.Bind(Initializer.ParameterMap, TEXT("WriteBuffer"), SPF_Mandatory);
		// Actual light volume
		ALightVolume.Bind(Initializer.ParameterMap, TEXT("ALightVolume"), SPF_Mandatory);
	}

	// Sets loop-dependent uniforms in the pipeline.
	void SetLoop(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, const unsigned loopIndex,
		const FTexture2DRHIRef pReadBuffer, const FSamplerStateRHIRef pReadBuffSampler,
		const FUnorderedAccessViewRHIRef pWriteBuffer)
	{
		// Update the Loop index.
		SetShaderValue(RHICmdList, ShaderRHI, Loop, loopIndex);
		// Set read/write buffers.
		SetUAVParameter(RHICmdList, ShaderRHI, WriteBuffer, pWriteBuffer);
		SetTextureParameter(RHICmdList, ShaderRHI, ReadBuffer, ReadBufferSampler, pReadBuffSampler, pReadBuffer);
	}

	void SetALightVolume(
		FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FUnorderedAccessViewRHIRef pALightVolume)
	{
		// Set the multiplier to -1 if we're removing the light. Set to 1 if adding it.
		SetUAVParameter(RHICmdList, ShaderRHI, ALightVolume, pALightVolume);
	}

	void SetPermutationMatrix(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FMatrix PermMatrix)
	{
		SetShaderValue(RHICmdList, ShaderRHI, PermutationMatrix, PermMatrix);
	}

	void UnbindResources(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI) override
	{
		// Unbind volume buffer.
		FRaymarchVolumeShader::UnbindResources(RHICmdList, ShaderRHI);
		SetUAVParameter(RHICmdList, ShaderRHI, ALightVolume, nullptr);
		SetUAVParameter(RHICmdList, ShaderRHI, WriteBuffer, nullptr);
		SetTextureParameter(RHICmdList, ShaderRHI, ReadBuffer, nullptr);
	}

protected:
	// The current loop index of this shader run.
	LAYOUT_FIELD(FShaderParameter, Loop);
	// Permutation matrix - used to get position in the volume from axis-aligned X,Y and loop index.
	LAYOUT_FIELD(FShaderParameter, PermutationMatrix);
	// Read buffer texture and sampler.
	LAYOUT_FIELD(FShaderResourceParameter, ReadBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, ReadBufferSampler);
	// Write buffer UAV.
	LAYOUT_FIELD(FShaderResourceParameter, WriteBuffer);
	// Light volume to modify.
	LAYOUT_FIELD(FShaderResourceParameter, ALightVolume);
};

// A shader implementing directional light propagation.
// Originally, spot and cone lights were supposed to be also supported, but other things were more
// important.
class FDirLightPropagationShader : public FLightPropagationShader
{
	DECLARE_EXPORTED_TYPE_LAYOUT(FDirLightPropagationShader, RAYMARCHER_API, Virtual);

public:
	FDirLightPropagationShader() : FLightPropagationShader()
	{
	}

	FDirLightPropagationShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLightPropagationShader(Initializer)
	{
		// Volume texture + Transfer function uniforms
		PrevPixelOffset.Bind(Initializer.ParameterMap, TEXT("PrevPixelOffset"), SPF_Mandatory);
		UVWOffset.Bind(Initializer.ParameterMap, TEXT("UVWOffset"), SPF_Mandatory);
	}

	void SetUVOffset(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FVector2D PixelOffset)
	{
		SetShaderValue(RHICmdList, ShaderRHI, PrevPixelOffset, PixelOffset);
	}

	void SetUVWOffset(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FVector pUVWOffset)
	{
		SetShaderValue(RHICmdList, ShaderRHI, UVWOffset, pUVWOffset);
	}

protected:
	// Tells the shader the pixel offset for reading from the previous loop's buffer
	LAYOUT_FIELD(FShaderParameter, PrevPixelOffset);
	// And the offset in the volume from the previous volume sample.
	LAYOUT_FIELD(FShaderParameter, UVWOffset);
};

// A shader implementing adding or removing a single directional light.
// (As opposed to changing [e.g. add and remove at the same time] a directional light)
// Only adds the bAdded boolean for toggling adding/removing a light.
// Notice the UE macro DECLARE_SHADER_TYPE, unlike the shaders above (which are abstract)
// this one actually gets implemented.
class FAddDirLightShaderCS : public FDirLightPropagationShader
{
	INTERNAL_DECLARE_SHADER_TYPE_COMMON(FAddDirLightShaderCS, Global, RAYMARCHER_API);
	DECLARE_EXPORTED_TYPE_LAYOUT(FAddDirLightShaderCS, RAYMARCHER_API, Virtual);

public:
	FAddDirLightShaderCS() : FDirLightPropagationShader()
	{
	}

	FAddDirLightShaderCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FDirLightPropagationShader(Initializer)
	{
		// Volume texture + Transfer function uniforms
		bAdded.Bind(Initializer.ParameterMap, TEXT("bAdded"), SPF_Mandatory);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void SetLightAdded(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, bool bLightAdded)
	{
		// Set the multiplier to -1 if we're removing the light. Set to 1 if adding it.
		SetShaderValue(RHICmdList, ShaderRHI, bAdded, bLightAdded ? 1 : -1);
	}

protected:
	// Parameter for the added/removed multiplier.
	LAYOUT_FIELD(FShaderParameter, bAdded);
};

// A shader implementing changing a light in one pass.
// Works by subtracting the old light and adding the new one.
// Notice the UE macro DECLARE_SHADER_TYPE, unlike the shaders above (which are abstract)
// this one actually gets implemented.
class FChangeDirLightShader : public FDirLightPropagationShader
{
	INTERNAL_DECLARE_SHADER_TYPE_COMMON(FChangeDirLightShader, Global, RAYMARCHER_API);
	DECLARE_EXPORTED_TYPE_LAYOUT(FChangeDirLightShader, RAYMARCHER_API, Virtual);

public:
	FChangeDirLightShader() : FDirLightPropagationShader()
	{
	}

	FChangeDirLightShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FDirLightPropagationShader(Initializer)
	{
		// Volume texture + Transfer function uniforms
		RemovedPrevPixelOffset.Bind(Initializer.ParameterMap, TEXT("RemovedPrevPixelOffset"), SPF_Mandatory);
		RemovedReadBuffer.Bind(Initializer.ParameterMap, TEXT("RemovedReadBuffer"), SPF_Mandatory);
		RemovedReadBufferSampler.Bind(Initializer.ParameterMap, TEXT("RemovedReadBufferSampler"), SPF_Mandatory);
		RemovedWriteBuffer.Bind(Initializer.ParameterMap, TEXT("RemovedWriteBuffer"), SPF_Mandatory);
		RemovedUVWOffset.Bind(Initializer.ParameterMap, TEXT("RemovedUVWOffset"), SPF_Mandatory);
		RemovedStepSize.Bind(Initializer.ParameterMap, TEXT("RemovedStepSize"), SPF_Mandatory);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	// Sets loop-dependent uniforms in the pipeline.
	void SetLoop(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, const unsigned loopIndex,
		const FTexture2DRHIRef pRemovedReadBuffer, const FSamplerStateRHIRef pRemovedReadBuffSampler,
		const FUnorderedAccessViewRHIRef pRemovedWriteBuffer, const FTexture2DRHIRef pAddedReadBuffer,
		const FSamplerStateRHIRef pAddedReadBuffSampler, const FUnorderedAccessViewRHIRef pAddedWriteBuffer)
	{
		// Actually sets the shader uniforms in the pipeline.
		FLightPropagationShader::SetLoop(
			RHICmdList, ShaderRHI, loopIndex, pAddedReadBuffer, pAddedReadBuffSampler, pAddedWriteBuffer);
		// Set read/write buffers for removed light.
		SetUAVParameter(RHICmdList, ShaderRHI, RemovedWriteBuffer, pRemovedWriteBuffer);
		SetTextureParameter(
			RHICmdList, ShaderRHI, RemovedReadBuffer, RemovedReadBufferSampler, pRemovedReadBuffSampler, pRemovedReadBuffer);
	}

	void SetPixelOffsets(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FVector2D AddedPixelOffset,
		FVector2D RemovedPixelOffset)
	{
		SetShaderValue(RHICmdList, ShaderRHI, PrevPixelOffset, AddedPixelOffset);
		SetShaderValue(RHICmdList, ShaderRHI, RemovedPrevPixelOffset, RemovedPixelOffset);
	}

	void SetUVWOffsets(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, FVector pAddedUVWOffset,
		FVector pRemovedUVWOffset)
	{
		SetShaderValue(RHICmdList, ShaderRHI, UVWOffset, pAddedUVWOffset);
		SetShaderValue(RHICmdList, ShaderRHI, RemovedUVWOffset, pRemovedUVWOffset);
	}

	void SetStepSizes(
		FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI, float pAddedStepSize, float pRemovedStepSize)
	{
		SetShaderValue(RHICmdList, ShaderRHI, StepSize, pAddedStepSize);
		SetShaderValue(RHICmdList, ShaderRHI, RemovedStepSize, pRemovedStepSize);
	}

	void UnbindResources(FRHICommandListImmediate& RHICmdList, FRHIComputeShader* ShaderRHI) override
	{
		// Unbind parent and also our added parameters.
		FDirLightPropagationShader::UnbindResources(RHICmdList, ShaderRHI);
		SetUAVParameter(RHICmdList, ShaderRHI, RemovedWriteBuffer, nullptr);
		SetTextureParameter(RHICmdList, ShaderRHI, RemovedReadBuffer, nullptr);
	}

protected:
	// Same collection of parameters as for a "add dir light" shader, but these ones are the ones of
	// the removed light.

	// Tells the shader the pixel offset for reading from the previous loop's buffer
	LAYOUT_FIELD(FShaderParameter, RemovedPrevPixelOffset);

	LAYOUT_FIELD(FShaderResourceParameter, RemovedReadBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, RemovedReadBufferSampler);
	// Write buffer UAV.
	LAYOUT_FIELD(FShaderResourceParameter, RemovedWriteBuffer);
	// Removed light step size (is different than added one's)
	LAYOUT_FIELD(FShaderParameter, RemovedStepSize);
	// Removed light UVW offset
	LAYOUT_FIELD(FShaderParameter, RemovedUVWOffset);
};

// Compute Shader used for fast clearing of RW volume textures.
class FClearVolumeTextureShaderCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FClearVolumeTextureShaderCS, Global, RAYMARCHER_API);

public:
	FClearVolumeTextureShaderCS() : FGlobalShader()
	{
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	FClearVolumeTextureShaderCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		Volume.Bind(Initializer.ParameterMap, TEXT("Volume"), SPF_Mandatory);
		ClearValue.Bind(Initializer.ParameterMap, TEXT("ClearValue"), SPF_Mandatory);
		ZSize.Bind(Initializer.ParameterMap, TEXT("ZSize"), SPF_Mandatory);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* VolumeRef, float clearColor, int ZSizeParam)
	{
		FRHIComputeShader* ShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ShaderRHI, Volume, VolumeRef);
		SetShaderValue(RHICmdList, ShaderRHI, ClearValue, clearColor);
		SetShaderValue(RHICmdList, ShaderRHI, ZSize, ZSizeParam);
	}

	void UnbindUAV(FRHICommandList& RHICmdList)
	{
		SetUAVParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), Volume, nullptr);
	}

protected:
	// Float values to be set to the alpha volume.
	LAYOUT_FIELD(FShaderResourceParameter, Volume);
	LAYOUT_FIELD(FShaderParameter, ClearValue);
	LAYOUT_FIELD(FShaderParameter, ZSize);
};
