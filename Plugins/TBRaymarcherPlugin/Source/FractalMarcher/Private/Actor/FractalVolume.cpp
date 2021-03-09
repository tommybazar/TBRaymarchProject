// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/FractalVolume.h"

#include "TextureUtilities.h"

DEFINE_LOG_CATEGORY(LogFractalMarchVolume)

// Uncomment for easier debugging
// #pragma optimize("", off)

// Sets default values
AFractalVolume::AFractalVolume() : AActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SetActorEnableCollision(true);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Default Scene Root"));
	RootComponent->SetWorldScale3D(FVector(1.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> UnitCubeInsideOut(
		TEXT("/TBRaymarcherPlugin/Meshes/Unit_Cube_Inside_Out"));

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FractalMarch Cube Static Mesh"));
	/// Set basic unit cube properties.
	if (UnitCubeInsideOut.Succeeded())
	{
		StaticMeshComponent->SetStaticMesh(UnitCubeInsideOut.Object);
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
		StaticMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		StaticMeshComponent->SetRelativeScale3D(FVector(100.0f));
		StaticMeshComponent->SetupAttachment(RootComponent);
	}
	//
	// 	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeBorder(TEXT("/TBRaymarcherPlugin/Meshes/Unit_Cube"));
	//
	// 	if (CubeBorder.Succeeded())
	// 	{
	// 		// Find and assign cube material.
	// 		CubeBorderMeshComponent->SetStaticMesh(CubeBorder.Object);
	// 		static ConstructorHelpers::FObjectFinder<UMaterial>
	// BorderMaterial(TEXT("/TBFractalMarcherPlugin/Materials/M_CubeBorder")); 		if (BorderMaterial.Succeeded())
	// 		{
	// 			CubeBorderMeshComponent->SetMaterial(0, BorderMaterial.Object);
	// 		}
	// 	}
	//
	// 	// Find and assign default FractalMarch materials.
	// 	static ConstructorHelpers::FObjectFinder<UMaterial> LitMaterial(TEXT("/TBFractalMarcherPlugin/Materials/M_FractalMarch"));
	// 	static ConstructorHelpers::FObjectFinder<UMaterial> IntensityMaterial(
	// 		TEXT("/TBFractalMarcherPlugin/Materials/M_Intensity_FractalMarch"));
	//
	// 	if (LitMaterial.Succeeded())
	// 	{
	// 		LitFractalMarchMaterialBase = LitMaterial.Object;
	// 	}
	//
	// 	if (IntensityMaterial.Succeeded())
	// 	{
	// 		IntensityFractalMarchMaterialBase = IntensityMaterial.Object;
	// 	}

	// Set default values for steps and half-res.
	FractalMarchingSteps = 150;
}

// Called after registering all components. This is the last action performed before editor window is spawned and before BeginPlay.
void AFractalVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		// Do not perform this on default class objects or archetype objects
		return;
	}

	if (LitFractalMarchMaterialBase)
	{
		LitFractalMarchMaterial =
			UMaterialInstanceDynamic::Create(LitFractalMarchMaterialBase, this, "Lit FractalMarch Mat Dynamic Inst");
		// Set default values for the lit and intensity FractalMarchers.
		LitFractalMarchMaterial->SetScalarParameterValue("Steps", FractalMarchingSteps);
	}

	if (StaticMeshComponent && LitFractalMarchMaterial)
	{
		StaticMeshComponent->SetMaterial(0, LitFractalMarchMaterial);
	}
}

void AFractalVolume::CalculateMandelbulbSDF()
{
	EnqueueRenderCommand_CalculateMandelbulbSDF(MandelbulbResources);
}

#if WITH_EDITOR

void AFractalVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AFractalVolume, MandelbulbDimensions)) 
	{
		InitializeFractalMarchResources();
	}
	
	if (PropertyName != GET_MEMBER_NAME_CHECKED(AFractalVolume, MandelbulbVolume))
	{
		CalculateMandelbulbSDF();
		return;
	}


}

bool AFractalVolume::ShouldTickIfViewportsOnly() const
{
	return true;
}

#endif	  //#if WITH_EDITOR

// Called every frame
void AFractalVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFractalVolume::SetTFCurve(UCurveLinearColor*& InTFCurve)
{
	// 	if (InTFCurve)
	// 	{
	// 		CurrentTFCurve = InTFCurve;
	// 		UFractalMarchUtils::ColorCurveToTexture(CurrentTFCurve, FractalMarchResources.TFTextureRef);
	// 		FlushRenderingCommands();
	// 		// Set TF Texture in the lit material.
	// 		LitFractalMarchMaterial->SetTextureParameterValue(FractalMarchParams::TransferFunction,
	// FractalMarchResources.TFTextureRef); 		bRequestedRecompute = true;
	// 	}
}

void AFractalVolume::SetAllMaterialParameters()
{
	// 	SetMaterialVolumeParameters();
	// 	SetMaterialWindowingParameters();
	// 	SetMaterialClippingParameters();
}

void AFractalVolume::SetFractalMarchSteps(float InFractalMarchingSteps)
{
	FractalMarchingSteps = InFractalMarchingSteps;
	if (LitFractalMarchMaterial)
	{
		LitFractalMarchMaterial->SetScalarParameterValue("Steps", FractalMarchingSteps);
	}
}

void AFractalVolume::InitializeFractalMarchResources()
{
	EPixelFormat PixelFormat = PF_G16;



// 	UVolumeTextureToolkit::CreateVolumeTextureTransient(
// 		MandelbulbResources.MandelbulbVolume, PixelFormat, MandelbulbDimensions, nullptr, true, true);

	MandelbulbVolume = MandelbulbResources.MandelbulbVolume;

	MandelbulbResources.Center = Center;
	MandelbulbResources.Extent = Extent;
	MandelbulbResources.Power = Power;

	MandelbulbResources.MandelbulbVolumeDimensions = MandelbulbDimensions;
	// Flush rendering commands so that all textures are definitely initialized with resources.
	FlushRenderingCommands();

	if (!MandelbulbResources.MandelbulbVolume->Resource)
	{
		return;
	}

	// Create UAV for light volume to be targettable in Compute Shader.
	check(MandelbulbResources.MandelbulbVolume->Resource->TextureRHI);
	MandelbulbResources.MandelbulbVolumeUAVRef =
		RHICreateUnorderedAccessView(MandelbulbResources.MandelbulbVolume->Resource->TextureRHI);
	FlushRenderingCommands();
}

// Uncomment for easier debugging
// #pragma optimize("", on)
