// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Rendering/MyProceduralMesh.h"

AMyProceduralMesh::AMyProceduralMesh()
{
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	SetRootComponent(ProceduralMesh);

	Width = 160;
	Height = 90;
	Spacing = 1.0f;

	bShouldGenerateMesh = false;
}

void AMyProceduralMesh::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bShouldGenerateMesh)
	{
		bShouldGenerateMesh = false;

		ClearMeshData();

		GenerateVertices();
		GenerateTriangles();

		// Function that creates mesh section
		ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	}
}

void AMyProceduralMesh::GenerateVertices()//FIntVector Sections, FIntVector ThisSection)
{
	float uvSpacingX = 1.0f / Width;
	float uvSpacingY = 1.0f / Height;

	for (int32 y = 0; y < Height; y++)
	{
		for (int32 x = 0; x < Width; x++)
		{
			Vertices.Add(FVector(x * Spacing, y * Spacing, 0.0f));
			Normals.Add(FVector(0.0f, 0.0f, 1.0f));
			UVs.Add(FVector2D(x * uvSpacingX, y * uvSpacingY));
			VertexColors.Add(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	}
}

void AMyProceduralMesh::GenerateTriangles()//FIntVector Sections, FIntVector ThisSection)
{
	for (int32 y = 0; y < Height - 1; y++)
	{
		for (int32 x = 0; x < Width - 1; x++)
		{
			Triangles.Add(x + (y * Width));				   // current vertex
			Triangles.Add(x + (y * Width) + Width);		   // current vertex + row
			Triangles.Add(x + (y * Width) + Width + 1);	   // current vertex + row + one right

			Triangles.Add(x + (y * Width));				   // current vertex
			Triangles.Add(x + (y * Width) + Width + 1);	   // current vertex + row + one right
			Triangles.Add(x + (y * Width) + 1);			   // current vertex + one right
		}
	}
}

void AMyProceduralMesh::ClearMeshData()
{
	Vertices.Empty();
	Triangles.Empty();
	UVs.Empty();
	Normals.Empty();
	VertexColors.Empty();
	Tangents.Empty();
}