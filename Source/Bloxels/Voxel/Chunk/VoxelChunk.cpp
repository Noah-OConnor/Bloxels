// Copyright 2025 Noah O'Connor. All rights reserved.


#include "VoxelChunk.h"
#include "Bloxels/Voxel/Core/VoxelConfig.h"
#include "Bloxels/Voxel/Core/VoxelUtilities.h"


AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Chunk Mesh"));
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AVoxelChunk::Initialize(FIntVector InCoords, UVoxelConfig* InConfig)
{
	ChunkCoords = InCoords;
	Config = InConfig;

	ChunkData.Initialize(Config->ChunkSize, InCoords);

	// TEMP: Fill the lower half with stone, upper half with air
	for (int32 X = 0; X < Config->ChunkSize; X++)
	{
		for (int32 Y = 0; Y < Config->ChunkSize; Y++)
		{
			for (int32 Z = 0; Z < Config->ChunkSize; Z++)
			{
				EVoxelType VoxelType = (Z < Config->ChunkSize / 2) ? EVoxelType::Stone : EVoxelType::Air;
				ChunkData.SetVoxel(X, Y, Z, VoxelType);
				GenerateMesh();
			}
		}
	}
	
	FVector WorldPos = VoxelUtils::ChunkToWorldLocation(ChunkCoords, Config->VoxelSize, Config->ChunkSize);
	SetActorLocation(WorldPos);
}

void AVoxelChunk::GenerateMesh()
{
	MeshComponent->ClearAllMeshSections();

	const int32 Size = Config->ChunkSize;
	const float VoxelSize = Config->VoxelSize;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	auto AddFace = [&](FVector BasePos, FVector Normal, FVector Right, FVector Up)
	{
		int32 StartIndex = Vertices.Num();

		Vertices.Add(BasePos);
		Vertices.Add(BasePos + Right * VoxelSize);
		Vertices.Add(BasePos + Right * VoxelSize + Up * VoxelSize);
		Vertices.Add(BasePos + Up * VoxelSize);

		// If the face is flipped (negative-facing), reverse winding order
		if (FVector::CrossProduct(Right, Up).Dot(Normal) >= 0)
		{
			Triangles.Append({StartIndex, StartIndex + 2, StartIndex + 1, StartIndex, StartIndex + 3, StartIndex + 2});
		}
		else
		{
			Triangles.Append({StartIndex, StartIndex + 1, StartIndex + 2, StartIndex, StartIndex + 2, StartIndex + 3});
		}

		Normals.Append({Normal, Normal, Normal, Normal});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
		
		FLinearColor DebugColor = FLinearColor::White;

		if (Normal.Z > 0) DebugColor = FLinearColor::Green;     // +Z
		if (Normal.Z < 0) DebugColor = FLinearColor::Red;       // -Z
		if (Normal.X > 0) DebugColor = FLinearColor::Blue;      // +X
		if (Normal.X < 0) DebugColor = FLinearColor::Yellow;    // -X
		if (Normal.Y > 0) DebugColor = FLinearColor::Gray;      // +Y
		if (Normal.Y < 0) DebugColor = FLinearColor::Black;		// -Y

		VertexColors.Append({DebugColor, DebugColor, DebugColor, DebugColor});

		Tangents.Append({FProcMeshTangent(1, 0, 0), FProcMeshTangent(1, 0, 0), FProcMeshTangent(1, 0, 0), FProcMeshTangent(1, 0, 0)});
	};

	for (int X = 0; X < Size; X++)
	for (int Y = 0; Y < Size; Y++)
	for (int Z = 0; Z < Size; Z++)
	{
		if (ChunkData.GetVoxel(X, Y, Z) == EVoxelType::Air)
			continue;

		FVector Pos = FVector(X, Y, Z) * VoxelSize;

		auto IsAir = [&](int32 NX, int32 NY, int32 NZ)
		{
			return !ChunkData.IsInBounds(NX, NY, NZ) || ChunkData.GetVoxel(NX, NY, NZ) == EVoxelType::Air;
		};

		if (IsAir(X + 1, Y, Z)) AddFace(Pos + FVector(VoxelSize, 0, 0), FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0)); // +X
		if (IsAir(X - 1, Y, Z)) AddFace(Pos, FVector(-1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0)); // -X
		if (IsAir(X, Y + 1, Z)) AddFace(Pos + FVector(0, VoxelSize, 0), FVector(0, 1, 0), FVector(1, 0, 0), FVector(0, 0, 1)); // +Y
		if (IsAir(X, Y - 1, Z)) AddFace(Pos, FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1)); // -Y
		if (IsAir(X, Y, Z + 1)) AddFace(Pos + FVector(0, 0, VoxelSize), FVector(0, 0, 1), FVector(1, 0, 0), FVector(0, 1, 0));

		if (IsAir(X, Y, Z - 1)) AddFace(Pos, FVector(0, 0, -1), FVector(1, 0, 0), FVector(0, 1, 0));





	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
	}
}


void AVoxelChunk::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}
