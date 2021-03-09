// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "MyProceduralMesh.generated.h"

UCLASS()
class AMyProceduralMesh : public AActor
{
	GENERATED_BODY()

	UPROPERTY(Transient, DuplicateTransient, EditAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

public:
	AMyProceduralMesh();

	UPROPERTY()
	TArray<FVector> Vertices;
	UPROPERTY()
	TArray<FVector> Normals;
	UPROPERTY()
	TArray<int32> Triangles;
	UPROPERTY()
	TArray<FVector2D> UVs;
	UPROPERTY()
	TArray<FLinearColor> VertexColors;
	UPROPERTY()
	TArray<FProcMeshTangent> Tangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MuProceduralMesh")
	int32 Height;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MuProceduralMesh")
	int32 Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MuProceduralMesh")
	float Spacing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MuProceduralMesh")
	bool bShouldGenerateMesh;

	virtual void OnConstruction(const FTransform& Transform) override;

	void GenerateVertices();
	void GenerateTriangles();
	void ClearMeshData();
};