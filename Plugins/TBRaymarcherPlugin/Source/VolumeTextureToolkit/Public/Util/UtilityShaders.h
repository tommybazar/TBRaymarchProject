// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "Engine/VolumeTexture.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "SceneUtils.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameters.h"

void VOLUMETEXTURETOOLKIT_API ClearVolumeTexture_RenderThread(
	FRHICommandListImmediate& RHICmdList, FRHITexture3D* ALightVolumeResource, float ClearValue);

void VOLUMETEXTURETOOLKIT_API Clear2DTexture_RenderThread(
	FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* TextureRW, FIntPoint TextureSize, float Value);
// void ClearVolumeTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture2D* ALightVolumeResource, float
// ClearValue);

// Compute shader for clearing a single-channel 2D float RW texture
class FClearFloatRWTextureCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FClearFloatRWTextureCS, Global, VOLUMETEXTURETOOLKIT_API);

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

// Compute Shader used for fast clearing of RW volume textures.
class FClearVolumeTextureShaderCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FClearVolumeTextureShaderCS, Global, VOLUMETEXTURETOOLKIT_API);

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

	void SetParameters(FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* VolumeRef, float clearColor, int ZSizeParam)
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
