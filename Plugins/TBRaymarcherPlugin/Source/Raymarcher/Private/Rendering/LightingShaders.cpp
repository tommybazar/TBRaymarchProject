// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Rendering/LightingShaders.h"

#include "AssetRegistryModule.h"
#include "RenderCore/Public/RenderUtils.h"
#include "Renderer/Public/VolumeRendering.h"
#include "TextureUtilities.h"

#define LOCTEXT_NAMESPACE "RaymarchPlugin"

IMPLEMENT_UNREGISTERED_TEMPLATE_TYPE_LAYOUT(, FRaymarchVolumeShader);
IMPLEMENT_UNREGISTERED_TEMPLATE_TYPE_LAYOUT(, FLightPropagationShader);
IMPLEMENT_UNREGISTERED_TEMPLATE_TYPE_LAYOUT(, FDirLightPropagationShader);

IMPLEMENT_GLOBAL_SHADER(FClearVolumeTextureShaderCS, "/Raymarcher/Private/ClearVolumeTextureShader.usf",
	"MainComputeShader", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(
	FAddDirLightShaderCS, "/Raymarcher/Private/AddDirLightShader.usf", "MainComputeShader", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(
	FChangeDirLightShader, "/Raymarcher/Private/ChangeDirLightShader.usf", "MainComputeShader", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(
	FClearFloatRWTextureCS, "/Raymarcher/Private/ClearTextureShader.usf", "MainComputeShader", SF_Compute);

// For making statistics about GPU use - Adding Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("AddingLights"), STAT_GPU_AddingLights, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUAddingLights, TEXT("AddingLightsToVolume"));

// For making statistics about GPU use - Changing Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("ChangingLights"), STAT_GPU_ChangingLights, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUChangingLights, TEXT("ChangingLightsInVolume"));

// For making statistics about GPU use - Clearing Lights.
DECLARE_FLOAT_COUNTER_STAT(TEXT("ClearingLights"), STAT_GPU_ClearingLights, STATGROUP_GPU);
DECLARE_GPU_STAT_NAMED(GPUClearingLights, TEXT("ClearingLightsInVolume"));

#define NUM_THREADS_PER_GROUP_DIMENSION 16	  // This has to be the same as in the compute shader's spec [X, X, 1]
// #TODO profile with different dimensions.

/// Returns the dimensions of the plane cutting through the volume when going along an axis at the given indes.
FIntVector GetTransposedDimensions(const FMajorAxes& Axes, const FRHITexture3D* VolumeRef, const unsigned index)
{
	FCubeFace face = Axes.FaceWeight[index].first;
	unsigned axis = (uint8) face / 2;
	switch (axis)
	{
		case 0:	   // going along X -> Volume Y = x, volume Z = y
			return FIntVector(VolumeRef->GetSizeY(), VolumeRef->GetSizeZ(), VolumeRef->GetSizeX());
		case 1:	   // going along Y -> Volume X = x, volume Z = y
			return FIntVector(VolumeRef->GetSizeX(), VolumeRef->GetSizeZ(), VolumeRef->GetSizeY());
		case 2:	   // going along Z -> Volume X = x, volume Y = y
			return FIntVector(VolumeRef->GetSizeX(), VolumeRef->GetSizeY(), VolumeRef->GetSizeZ());
		default:
			check(false);
			return FIntVector(0, 0, 0);
	}
}

/// Returns +1 if going along the specified axis index means increasing the index.
/// Returns -1 if going along the axis decreases the index.
/// E.G. if we're going along +X axis, will return 1, going along -X will return -1.
int GetAxisDirection(const FMajorAxes& Axes, unsigned index)
{
	// All even axis number are going down on their respective axes.
	return ((uint8) Axes.FaceWeight[index].first % 2 ? 1 : -1);
}

/// Returns the ReadWriteBuffer resource appropriate for the axis index.
OneAxisReadWriteBufferResources& GetBuffers(const FMajorAxes Axes, const unsigned index, FBasicRaymarchRenderingResources& InParams)
{
	FCubeFace face = Axes.FaceWeight[index].first;
	unsigned axis = (uint8) face / 2;
	return InParams.XYZReadWriteBuffers[axis];
}

/// Clears a FloatTexture accesible as a UAV.
void ClearFloatTextureRW(
	FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* TextureRW, FIntPoint TextureSize, float Value)
{
	TShaderMapRef<FClearFloatRWTextureCS> ShaderRef(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ComputeShader = ShaderRef.GetComputeShader();
	RHICmdList.SetComputeShader(ComputeShader);

	RHICmdList.TransitionResource(
		EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EComputeToCompute, TextureRW);

	ShaderRef->SetParameters(RHICmdList, TextureRW, Value);
	uint32 GroupSizeX = FMath::DivideAndRoundUp(TextureSize.X, NUM_THREADS_PER_GROUP_DIMENSION);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(TextureSize.Y, NUM_THREADS_PER_GROUP_DIMENSION);

	RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
	//  DispatchComputeShader(RHICmdList, ShaderRef, GroupSizeX, GroupSizeY, 1);
	ShaderRef->UnbindUAV(RHICmdList);
}

///  Returns the UV offset to the previous layer. This is the position in the previous layer that is in the direction of the light.
FVector2D GetUVOffset(FCubeFace Axis, FVector LightPosition, FIntVector TransposedDimensions)
{
	FVector normLightPosition = LightPosition;
	// Normalize the light position to get the major axis to be one. The other 2 components are then
	// an offset to apply to current pos to read from our read buffer texture.
	FVector2D RetVal;
	switch (Axis)
	{
		case FCubeFace::XPositive:	  // +X
			normLightPosition /= normLightPosition.X;
			RetVal = FVector2D(normLightPosition.Y, normLightPosition.Z);
			break;
		case FCubeFace::XNegative:	  // -X
			normLightPosition /= -normLightPosition.X;
			RetVal = FVector2D(normLightPosition.Y, normLightPosition.Z);
			break;
		case FCubeFace::YPositive:	  // +Y
			normLightPosition /= normLightPosition.Y;
			RetVal = FVector2D(normLightPosition.X, normLightPosition.Z);
			break;
		case FCubeFace::YNegative:	  // -Y
			normLightPosition /= -normLightPosition.Y;
			RetVal = FVector2D(normLightPosition.X, normLightPosition.Z);
			break;
		case FCubeFace::ZPositive:	  // +Z
			normLightPosition /= normLightPosition.Z;
			RetVal = FVector2D(normLightPosition.X, normLightPosition.Y);
			break;
		case FCubeFace::ZNegative:	  // -Z
			normLightPosition /= -normLightPosition.Z;
			RetVal = FVector2D(normLightPosition.X, normLightPosition.Y);
			break;
		default:
		{
			check(false);
			return FVector2D(0, 0);
		};
	}

	// Now normalize for different voxel sizes. Because we have Transposed Dimensions as input, we
	// know that we're propagating along TD.Z, The X-size of the buffer is in TD.X and Y-size of the
	// buffer is in TD.Y

	//// Divide by length of step
	RetVal /= TransposedDimensions.Z;

	return RetVal;
}

/// For the given axis, light position and volume dimensions and world parameters, return the StepSize and UVW offset.
///
/// StepSize is the distance the light has to travel between 2 layers (in world units) and UVW offset is the offset between a voxel
/// and the place where it should read lighting information from from the previous layer (in UVW 0-1 space).
void GetStepSizeAndUVWOffset(FCubeFace Axis, FVector LightPosition, FIntVector TransposedDimensions,
	const FRaymarchWorldParameters WorldParameters, float& OutStepSize, FVector& OutUVWOffset)
{
	OutUVWOffset = LightPosition;
	// Since we don't care about the direction, just the size, ignore signs.
	switch (Axis)
	{
		case FCubeFace::XPositive:	  // +-X
		case FCubeFace::XNegative:
			OutUVWOffset /= abs(LightPosition.X) * TransposedDimensions.Z;
			break;
		case FCubeFace::YPositive:	  // +-Y
		case FCubeFace::YNegative:
			OutUVWOffset /= abs(LightPosition.Y) * TransposedDimensions.Z;
			break;
		case FCubeFace::ZPositive:	  // +-Z
		case FCubeFace::ZNegative:
			OutUVWOffset /= abs(LightPosition.Z) * TransposedDimensions.Z;
			break;
		default:
			check(false);
			;
	}

	// Transform local vector to world space.
	FVector WorldVec = WorldParameters.VolumeTransform.TransformVector(OutUVWOffset);
	OutStepSize = WorldVec.Size();
}

void GetLocalLightParamsAndAxes(const FDirLightParameters& LightParameters, const FTransform& VolumeTransform,
	FDirLightParameters& OutLocalLightParameters, FMajorAxes& OutLocalMajorAxes)
{
	// #TODO Why the fuck does light direction need NoScale and no multiplication by scale and clipping
	// plane needs to be multiplied?

	// Transform light directions into local space.
	OutLocalLightParameters.LightDirection = VolumeTransform.InverseTransformVector(LightParameters.LightDirection);
	// Normalize Light Direction to get unit length.
	OutLocalLightParameters.LightDirection.Normalize();

	// Intensity is the same in local space -> copy.
	OutLocalLightParameters.LightIntensity = LightParameters.LightIntensity;

	// Get Major Axes (notice inverting of light Direction - for a directional light, the position of
	// the light is the opposite of the direction) e.g. Directional light with direction (1, 0, 0) is
	// assumed to be shining from (-1, 0, 0)
	OutLocalMajorAxes = FMajorAxes::GetMajorAxes(-OutLocalLightParameters.LightDirection);

	// Prevent near-zero bugs in the shader (if first axis is very, very dominant, the secondary axis offsets are giant
	// and they just read out of bounds all the time.
	if (OutLocalMajorAxes.FaceWeight[0].second > 0.99f)
	{
		OutLocalMajorAxes.FaceWeight[0].second = 1.0f;
	}

	// Set second axis weight to (1 - (first axis weight))
	OutLocalMajorAxes.FaceWeight[1].second = 1 - OutLocalMajorAxes.FaceWeight[0].second;
}

FSamplerStateRHIRef GetBufferSamplerRef(uint32 BorderColorInt)
{
	// Return a sampler for RW buffers - bordered by specified color.
	return RHICreateSamplerState(
		FSamplerStateInitializerRHI(SF_Bilinear, AM_Border, AM_Border, AM_Border, 0, 0, 0, 1, BorderColorInt));
}

// Returns the color int required for the given light color and major axis (single channel)
uint32 GetBorderColorIntSingle(FDirLightParameters LightParams, FMajorAxes MajorAxes, unsigned index)
{
	// Set alpha channel to the texture's red channel (when reading single-channel, only red component
	// is read)
	FLinearColor LightColor = FLinearColor(LightParams.LightIntensity * MajorAxes.FaceWeight[index].second, 0.0, 0.0, 0.0);
	return LightColor.ToFColor(true).ToPackedARGB();
}

FClippingPlaneParameters RAYMARCHER_API GetLocalClippingParameters(const FRaymarchWorldParameters WorldParameters)
{
	FClippingPlaneParameters RetVal;
	// Get clipping center to (0-1) texture local space. (Invert transform, add 0.5 to get to (0-1)
	// space of a unit cube centered on 0,0,0)
	RetVal.Center = WorldParameters.VolumeTransform.InverseTransformPosition(WorldParameters.ClippingPlaneParameters.Center) + 0.5;
	// Get clipping direction in local space
	// TODO Why the hell does light direction work with regular InverseTransformVector
	// but clipping direction only works with NoScale and multiplying by scale afterwards?
	RetVal.Direction =
		WorldParameters.VolumeTransform.InverseTransformVectorNoScale(WorldParameters.ClippingPlaneParameters.Direction);
	RetVal.Direction *= WorldParameters.VolumeTransform.GetScale3D();
	RetVal.Direction.Normalize();

	return RetVal;
}

// Returns the light's alpha at this major axis and weight (single channel)
float GetLightAlpha(FDirLightParameters LightParams, FMajorAxes MajorAxes, unsigned index)
{
	return LightParams.LightIntensity * MajorAxes.FaceWeight[index].second;
}

// Returns a 3x3 permutation matrix depending on the current propagation axis.
// We use this matrix in the shaders to create offsets regardless of direction.
FMatrix GetPermutationMatrix(FMajorAxes MajorAxes, unsigned index)
{
	uint8 Axis = (uint8) MajorAxes.FaceWeight[index].first / 2;
	FMatrix retVal;
	retVal.SetIdentity();

	FVector xVec(1, 0, 0);
	FVector yVec(0, 1, 0);
	FVector zVec(0, 0, 1);
	switch (Axis)
	{
		case 0:	   // X Axis
			retVal.SetAxes(&yVec, &zVec, &xVec);
			break;
		case 1:	   // Y Axis
			retVal.SetAxes(&xVec, &zVec, &yVec);
			break;
		case 2:	   // We keep identity set...
		default:
			break;
	}
	return retVal;
}

// Returns the Loop Start index, end index and the way the loop is going along the axis.
void GetLoopStartStopIndexes(
	int& OutStart, int& OutStop, int& OutAxisDirection, const FMajorAxes& MajorAxes, const unsigned& index, const int zDimension)
{
	OutAxisDirection = GetAxisDirection(MajorAxes, index);
	if (OutAxisDirection == -1)
	{
		OutStart = zDimension - 1;
		OutStop = -1;	 // We want to go all the way to zero, so stop at -1
	}
	else
	{
		OutStart = 0;
		OutStop = zDimension;	 // want to go to Z - 1, so stop at Z
	}
}

// Used for swapping read/write buffers - transitions one to Readable and other to Writable.
void TransitionBufferResources(
	FRHICommandListImmediate& RHICmdList, FRHITexture* NewlyReadableTexture, FRHIUnorderedAccessView* NewlyWriteableUAV)
{
	RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, NewlyReadableTexture);
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EWritable, EResourceTransitionPipeline::EComputeToCompute, NewlyWriteableUAV);
}

void AddDirLightToSingleLightVolume_RenderThread(FRHICommandListImmediate& RHICmdList, FBasicRaymarchRenderingResources Resources,
	const FDirLightParameters LightParameters, const bool Added, const FRaymarchWorldParameters WorldParameters)
{
	check(IsInRenderingThread());

	// Can't have directional light without direction...
	if (LightParameters.LightDirection == FVector(0.0, 0.0, 0.0))
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 100.0f, FColor::Yellow, TEXT("Returning because the directional light doesn't have a direction."));
		return;
	}

	FDirLightParameters LocalLightParams;
	FMajorAxes LocalMajorAxes;
	// Calculate local Light parameters and corresponding axes.
	GetLocalLightParamsAndAxes(LightParameters, WorldParameters.VolumeTransform, LocalLightParams, LocalMajorAxes);

	// Transform clipping parameters into local space.
	FClippingPlaneParameters LocalClippingParameters = GetLocalClippingParameters(WorldParameters);

	// For GPU profiling.
	SCOPED_DRAW_EVENTF(RHICmdList, AddDirLightToSingleLightVolume_RenderThread, TEXT("Adding Lights"));
	SCOPED_GPU_STAT(RHICmdList, GPUAddingLights);

	// TODO create structure with 2 sets of buffers so we don't have to look for them again in the
	// actual shader loop! Clear buffers for the two axes we will be using.
	for (unsigned i = 0; i < 2; i++)
	{
		// Break if the axis weight == 0
		if (LocalMajorAxes.FaceWeight[i].second == 0)
		{
			break;
		}
		// Get the X, Y and Z transposed into the current axis orientation.
		FIntVector TransposedDimensions =
			GetTransposedDimensions(LocalMajorAxes, Resources.LightVolumeTextureRef->Resource->TextureRHI->GetTexture3D(), i);
		OneAxisReadWriteBufferResources& Buffers = GetBuffers(LocalMajorAxes, i, Resources);

		float LightAlpha = GetLightAlpha(LocalLightParams, LocalMajorAxes, i);

		ClearFloatTextureRW(RHICmdList, Buffers.UAVs[0], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), LightAlpha);
		ClearFloatTextureRW(RHICmdList, Buffers.UAVs[1], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), LightAlpha);
	}

	// Find and set compute shader
	TShaderMapRef<FAddDirLightShaderCS> ComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	// Don't need barriers on these - we only ever read/write to the same pixel from one thread ->
	// no race conditions But we definitely need to transition the resource to Compute-shader
	// accessible, otherwise the renderer might touch our textures while we're writing there.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, Resources.LightVolumeUAVRef);

	// Set parameters, resources, LightAdded and ALightVolume
	ComputeShader->SetRaymarchParameters(
		RHICmdList, ShaderRHI, LocalClippingParameters, Resources.WindowingParameters.ToLinearColor());
	ComputeShader->SetRaymarchResources(RHICmdList, ShaderRHI, Resources.DataVolumeTextureRef->Resource->TextureRHI->GetTexture3D(),
		Resources.TFTextureRef->Resource->TextureRHI->GetTexture2D(), Resources.WindowingParameters);
	ComputeShader->SetLightAdded(RHICmdList, ShaderRHI, Added);
	ComputeShader->SetALightVolume(RHICmdList, ShaderRHI, Resources.LightVolumeUAVRef);

	for (unsigned i = 0; i < 2; i++)
	{
		// Break if the main axis weight == 1
		if (LocalMajorAxes.FaceWeight[i].second == 0)
		{
			break;
		}
		OneAxisReadWriteBufferResources& Buffers = GetBuffers(LocalMajorAxes, i, Resources);

		uint32 ColorInt = GetBorderColorIntSingle(LocalLightParams, LocalMajorAxes, i);
		FSamplerStateRHIRef readBuffSampler = GetBufferSamplerRef(ColorInt);

		// Get the X, Y and Z transposed into the current axis orientation.
		FIntVector TransposedDimensions =
			GetTransposedDimensions(LocalMajorAxes, Resources.LightVolumeTextureRef->Resource->TextureRHI->GetTexture3D(), i);

		FVector2D UVOffset =
			GetUVOffset(LocalMajorAxes.FaceWeight[i].first, -LocalLightParams.LightDirection, TransposedDimensions);
		FMatrix PermutationMatrix = GetPermutationMatrix(LocalMajorAxes, i);

		FIntVector LightVolumeSize = FIntVector(Resources.LightVolumeTextureRef->GetSizeX(),
			Resources.LightVolumeTextureRef->GetSizeY(), Resources.LightVolumeTextureRef->GetSizeZ());

		FVector UVWOffset;
		float StepSize;
		GetStepSizeAndUVWOffset(LocalMajorAxes.FaceWeight[i].first, -LocalLightParams.LightDirection, TransposedDimensions,
			WorldParameters, StepSize, UVWOffset);

		// Normalize UVW offset to length of largest voxel size to get rid of artifacts. (Not correct,
		// but consistent!)
		int LowestVoxelCount = FMath::Min3(TransposedDimensions.X, TransposedDimensions.Y, TransposedDimensions.Z);
		float LongestVoxelSide = 1.0f / LowestVoxelCount;
		UVWOffset.Normalize();
		UVWOffset *= LongestVoxelSide;

		ComputeShader->SetStepSize(RHICmdList, ShaderRHI, StepSize);
		ComputeShader->SetPermutationMatrix(RHICmdList, ShaderRHI, PermutationMatrix);
		ComputeShader->SetUVOffset(RHICmdList, ShaderRHI, UVOffset);
		ComputeShader->SetUVWOffset(RHICmdList, ShaderRHI, UVWOffset);

		uint32 GroupSizeX = FMath::DivideAndRoundUp(TransposedDimensions.X, NUM_THREADS_PER_GROUP_DIMENSION);
		uint32 GroupSizeY = FMath::DivideAndRoundUp(TransposedDimensions.Y, NUM_THREADS_PER_GROUP_DIMENSION);

		int Start, Stop, AxisDirection;
		GetLoopStartStopIndexes(Start, Stop, AxisDirection, LocalMajorAxes, i, TransposedDimensions.Z);

		for (int j = Start; j != Stop; j += AxisDirection)
		{
			// Switch read and write buffers each row.
			if (j % 2 == 0)
			{
				ComputeShader->SetLoop(RHICmdList, ShaderRHI, j, Buffers.Buffers[0], readBuffSampler, Buffers.UAVs[1]);
			}
			else
			{
				ComputeShader->SetLoop(RHICmdList, ShaderRHI, j, Buffers.Buffers[1], readBuffSampler, Buffers.UAVs[0]);
			}
			RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
		}
	}

	// Unbind UAVs.
	ComputeShader->UnbindResources(RHICmdList, ShaderRHI);

	// Transition resources back to the renderer.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, Resources.LightVolumeUAVRef);
}

void ChangeDirLightInSingleLightVolume_RenderThread(FRHICommandListImmediate& RHICmdList,
	FBasicRaymarchRenderingResources Resources, const FDirLightParameters RemovedLightParameters,
	const FDirLightParameters AddedLightParameters, const FRaymarchWorldParameters WorldParameters)
{
	// Can't have directional light without direction...
	if (AddedLightParameters.LightDirection == FVector(0.0, 0.0, 0.0) ||
		RemovedLightParameters.LightDirection == FVector(0.0, 0.0, 0.0))
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 100.0f, FColor::Yellow, TEXT("Returning because the directional light doesn't have a direction."));
		return;
	}

	// Create local copies of Light Params, so that if we have to fall back to 2x
	// AddOrRemoveLight, we can just pass the original parameters.
	FDirLightParameters RemovedLocalLightParams, AddedLocalLightParams;
	FMajorAxes RemovedLocalMajorAxes, AddedLocalMajorAxes;
	// Calculate local Light parameters and corresponding axes.
	GetLocalLightParamsAndAxes(
		RemovedLightParameters, WorldParameters.VolumeTransform, RemovedLocalLightParams, RemovedLocalMajorAxes);
	GetLocalLightParamsAndAxes(AddedLightParameters, WorldParameters.VolumeTransform, AddedLocalLightParams, AddedLocalMajorAxes);

	// If lights have different major axes, do a separate removal and addition.
	// (Change dir light only works if it runs on the same major axes).
	if (RemovedLocalMajorAxes.FaceWeight[0].first != AddedLocalMajorAxes.FaceWeight[0].first ||
		RemovedLocalMajorAxes.FaceWeight[1].first != AddedLocalMajorAxes.FaceWeight[1].first)
	{
		AddDirLightToSingleLightVolume_RenderThread(RHICmdList, Resources, RemovedLightParameters, false, WorldParameters);
		AddDirLightToSingleLightVolume_RenderThread(RHICmdList, Resources, AddedLightParameters, true, WorldParameters);
		return;
	}

	FClippingPlaneParameters LocalClippingParameters = GetLocalClippingParameters(WorldParameters);

	// Clear buffers for the two axes we will be using.
	for (unsigned i = 0; i < 2; i++)
	{
		// Get the X, Y and Z transposed into the current axis orientation.
		FIntVector TransposedDimensions = GetTransposedDimensions(
			RemovedLocalMajorAxes, Resources.LightVolumeTextureRef->Resource->TextureRHI->GetTexture3D(), i);
		OneAxisReadWriteBufferResources& Buffers = GetBuffers(RemovedLocalMajorAxes, i, Resources);

		float RemovedLightAlpha = GetLightAlpha(RemovedLocalLightParams, RemovedLocalMajorAxes, i);
		float AddedLightAlpha = GetLightAlpha(AddedLocalLightParams, AddedLocalMajorAxes, i);

		// Clear R/W buffers for Removed Light
		ClearFloatTextureRW(
			RHICmdList, Buffers.UAVs[0], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), RemovedLightAlpha);
		ClearFloatTextureRW(
			RHICmdList, Buffers.UAVs[1], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), RemovedLightAlpha);
		// Clear R/W buffers for Added Light
		ClearFloatTextureRW(
			RHICmdList, Buffers.UAVs[2], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), AddedLightAlpha);
		ClearFloatTextureRW(
			RHICmdList, Buffers.UAVs[3], FIntPoint(TransposedDimensions.X, TransposedDimensions.Y), AddedLightAlpha);
	}

	// For GPU profiling.
	SCOPED_DRAW_EVENTF(RHICmdList, ChangeDirLightInSingleLightVolume_RenderThread, TEXT("Changing Lights"));
	SCOPED_GPU_STAT(RHICmdList, GPUChangingLights);

	TShaderMapRef<FChangeDirLightShader> ComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	// Don't need barriers on these - we only ever read/write to the same pixel from one thread ->
	// no race conditions But we definitely need to transition the resource to Compute-shader
	// accessible, otherwise the renderer might touch our textures while we're writing them.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, Resources.LightVolumeUAVRef);

	ComputeShader->SetRaymarchParameters(
		RHICmdList, ShaderRHI, LocalClippingParameters, Resources.WindowingParameters.ToLinearColor());
	ComputeShader->SetRaymarchResources(RHICmdList, ShaderRHI, Resources.DataVolumeTextureRef->Resource->TextureRHI->GetTexture3D(),
		Resources.TFTextureRef->Resource->TextureRHI->GetTexture2D(), Resources.WindowingParameters);
	ComputeShader->SetALightVolume(RHICmdList, ShaderRHI, Resources.LightVolumeUAVRef);

	for (unsigned AxisIndex = 0; AxisIndex < 2; AxisIndex++)
	{
		// Get Color ints for texture borders.
		uint32 RemovedColorInt = GetBorderColorIntSingle(RemovedLocalLightParams, RemovedLocalMajorAxes, AxisIndex);
		uint32 AddedColorInt = GetBorderColorIntSingle(AddedLocalLightParams, AddedLocalMajorAxes, AxisIndex);
		// Get the sampler for read buffer to use border with the proper light color.
		FSamplerStateRHIRef RemovedReadBuffSampler = GetBufferSamplerRef(RemovedColorInt);
		FSamplerStateRHIRef AddedReadBuffSampler = GetBufferSamplerRef(AddedColorInt);

		OneAxisReadWriteBufferResources& Buffers = GetBuffers(RemovedLocalMajorAxes, AxisIndex, Resources);
		// TODO take these from buffers.
		FIntVector TransposedDimensions = GetTransposedDimensions(
			RemovedLocalMajorAxes, Resources.LightVolumeTextureRef->Resource->TextureRHI->GetTexture3D(), AxisIndex);

		FVector2D AddedPixOffset = GetUVOffset(
			AddedLocalMajorAxes.FaceWeight[AxisIndex].first, -AddedLocalLightParams.LightDirection, TransposedDimensions);
		FVector2D RemovedPixOffset = GetUVOffset(
			RemovedLocalMajorAxes.FaceWeight[AxisIndex].first, -RemovedLocalLightParams.LightDirection, TransposedDimensions);

		FVector2D AddedUVOffset = GetUVOffset(
			AddedLocalMajorAxes.FaceWeight[AxisIndex].first, -AddedLocalLightParams.LightDirection, TransposedDimensions);
		FVector2D RemovedUVOffset = GetUVOffset(
			RemovedLocalMajorAxes.FaceWeight[AxisIndex].first, -RemovedLocalLightParams.LightDirection, TransposedDimensions);

		FVector AddedUVWOffset, RemovedUVWOffset;
		float AddedStepSize, RemovedStepSize;

		GetStepSizeAndUVWOffset(AddedLocalMajorAxes.FaceWeight[AxisIndex].first, -AddedLocalLightParams.LightDirection,
			TransposedDimensions, WorldParameters, AddedStepSize, AddedUVWOffset);
		GetStepSizeAndUVWOffset(RemovedLocalMajorAxes.FaceWeight[AxisIndex].first, -RemovedLocalLightParams.LightDirection,
			TransposedDimensions, WorldParameters, RemovedStepSize, RemovedUVWOffset);

		// Normalize UVW offset to length of largest voxel size to get rid of artifacts. (Not correct,
		// but consistent!)
		int LowestVoxelCount = FMath::Min3(TransposedDimensions.X, TransposedDimensions.Y, TransposedDimensions.Z);
		float LongestVoxelSide = 1.0f / LowestVoxelCount;

		AddedUVWOffset.Normalize();
		AddedUVWOffset *= LongestVoxelSide;
		RemovedUVWOffset.Normalize();
		RemovedUVWOffset *= LongestVoxelSide;

		ComputeShader->SetStepSizes(RHICmdList, ShaderRHI, AddedStepSize, RemovedStepSize);

		ComputeShader->SetPixelOffsets(RHICmdList, ShaderRHI, AddedPixOffset, RemovedPixOffset);
		ComputeShader->SetUVWOffsets(RHICmdList, ShaderRHI, AddedUVWOffset, RemovedUVWOffset);

		FMatrix perm = GetPermutationMatrix(RemovedLocalMajorAxes, AxisIndex);
		ComputeShader->SetPermutationMatrix(RHICmdList, ShaderRHI, perm);

		// Get group sizes for compute shader
		uint32 GroupSizeX = FMath::DivideAndRoundUp(TransposedDimensions.X, NUM_THREADS_PER_GROUP_DIMENSION);
		uint32 GroupSizeY = FMath::DivideAndRoundUp(TransposedDimensions.Y, NUM_THREADS_PER_GROUP_DIMENSION);

		int Start, Stop, AxisDirection;
		GetLoopStartStopIndexes(Start, Stop, AxisDirection, RemovedLocalMajorAxes, AxisIndex, TransposedDimensions.Z);

		for (int LoopIndex = Start; LoopIndex != Stop; LoopIndex += AxisDirection)
		{	 // Switch read and write buffers each cycle.
			if (LoopIndex % 2 == 0)
			{
				TransitionBufferResources(RHICmdList, Buffers.Buffers[0], Buffers.UAVs[1]);
				TransitionBufferResources(RHICmdList, Buffers.Buffers[2], Buffers.UAVs[3]);
				ComputeShader->SetLoop(RHICmdList, ShaderRHI, LoopIndex, Buffers.Buffers[0], RemovedReadBuffSampler,
					Buffers.UAVs[1], Buffers.Buffers[2], AddedReadBuffSampler, Buffers.UAVs[3]);
			}
			else
			{
				TransitionBufferResources(RHICmdList, Buffers.Buffers[1], Buffers.UAVs[0]);
				TransitionBufferResources(RHICmdList, Buffers.Buffers[3], Buffers.UAVs[2]);
				ComputeShader->SetLoop(RHICmdList, ShaderRHI, LoopIndex, Buffers.Buffers[1], RemovedReadBuffSampler,
					Buffers.UAVs[0], Buffers.Buffers[3], AddedReadBuffSampler, Buffers.UAVs[2]);
			}
			RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
		}
	}

	// Unbind Resources.
	ComputeShader->UnbindResources(RHICmdList, ShaderRHI);

	// Transition resources back to the renderer.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, Resources.LightVolumeUAVRef);
}

void ClearVolumeTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture3D* VolumeResourceRef, float ClearValues)
{
	// For GPU profiling.
	SCOPED_DRAW_EVENTF(RHICmdList, ClearVolumeTexture_RenderThread, TEXT("Clearing lights"));
	SCOPED_GPU_STAT(RHICmdList, GPUClearingLights);

	TShaderMapRef<FClearVolumeTextureShaderCS> ComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	// RHICmdList.TransitionResource(EResourceTransitionAccess::ERWNoBarrier,
	// LightVolumeResource);
	FUnorderedAccessViewRHIRef VolumeUAVRef = RHICreateUnorderedAccessView(VolumeResourceRef);

	// Don't need barriers on these - we only ever read/write to the same pixel from one thread ->
	// no race conditions But we definitely need to transition the resource to Compute-shader
	// accessible, otherwise the renderer might touch our textures while we're writing them.
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, VolumeUAVRef);

	ComputeShader->SetParameters(RHICmdList, VolumeUAVRef, ClearValues, VolumeResourceRef->GetSizeZ());

	uint32 GroupSizeX = FMath::DivideAndRoundUp((int32) VolumeResourceRef->GetSizeX(), NUM_THREADS_PER_GROUP_DIMENSION);
	uint32 GroupSizeY = FMath::DivideAndRoundUp((int32) VolumeResourceRef->GetSizeY(), NUM_THREADS_PER_GROUP_DIMENSION);

	RHICmdList.DispatchComputeShader(GroupSizeX, GroupSizeY, 1);
	ComputeShader->UnbindUAV(RHICmdList);
	RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, VolumeUAVRef);
}

/*
  double end = FPlatformTime::Seconds();
  FString text = "Time elapsed before shader & copy creation = ";
  text += FString::SanitizeFloat(end - start, 6);
  GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, text);*/

// start = FPlatformTime::Seconds();

// text = "Time elapsed in shader & copy = ";
// text += FString::SanitizeFloat(start - end, 6);
// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, text);

FRaymarchVolumeShader::~FRaymarchVolumeShader()
{
}

#undef LOCTEXT_NAMESPACE
