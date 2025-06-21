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
				EVoxelType VoxelType = EVoxelType::Stone; //(Z < Config->ChunkSize / 2) ? EVoxelType::Stone : EVoxelType::Air;
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
	const int32 Size = Config->ChunkSize;
	const float VoxelSize = Config->VoxelSize;

	MeshComponent->ClearAllMeshSections();
	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UVs.Reset();
	VertexColors.Reset();
	Tangents.Reset();

	auto GetVoxel = [&](int32 X, int32 Y, int32 Z) -> EVoxelType {
		return ChunkData.GetVoxel(X, Y, Z);
	};

	// Greedy sweep along each of the 3 axes
	GreedyMeshDirection(0, 1, 2, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), GetVoxel); // X+
	GreedyMeshDirection(0, 1, 2, FVector(-1,0,0), FVector(0,1,0), FVector(0,0,1), GetVoxel); // X+
	GreedyMeshDirection(1, 0, 2, FVector(0,1,0), FVector(1,0,0), FVector(0,0,1), GetVoxel); // Y+
	GreedyMeshDirection(1, 0, 2, FVector(0,-1,0), FVector(1,0,0), FVector(0,0,1), GetVoxel); // Y+
	GreedyMeshDirection(2, 0, 1, FVector(0,0,1), FVector(1,0,0), FVector(0,1,0), GetVoxel); // Z+
	GreedyMeshDirection(2, 0, 1, FVector(0,0,-1), FVector(1,0,0), FVector(0,1,0), GetVoxel); // Z+

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

void AVoxelChunk::GreedyMeshDirection(
	int D, int U, int V, 
	FVector Normal, FVector Right, FVector Up, 
	const TFunction<EVoxelType(int32, int32, int32)>& GetVoxel)
{
	const int Size = Config->ChunkSize;
	const float VoxelSize = Config->VoxelSize;

	TArray<bool> Mask;
	Mask.SetNum(Size * Size);

	for (int X = 0; X < Size; ++X)
	{
		for (int Y = 0; Y < Size; ++Y)
		{
			for (int Z = 0; Z < Size; ++Z)
			{
				int I[3] = { X, Y, Z };
				int D1 = I[D];
				if (D1 >= Size) continue;

				// Build mask
				for (int U1 = 0; U1 < Size; ++U1)
				{
					for (int V1 = 0; V1 < Size; ++V1)
					{
						I[U] = U1;
						I[V] = V1;

						EVoxelType Current = GetVoxel(I[0], I[1], I[2]);

						int Dir = FMath::RoundToInt(Normal[D]);
						I[D] += Dir;
						EVoxelType Neighbor = (!ChunkData.IsInBounds(I[0], I[1], I[2])) ? EVoxelType::Air : GetVoxel(I[0], I[1], I[2]);
						I[D] -= Dir;


						bool Visible = Current != EVoxelType::Air && Neighbor == EVoxelType::Air;
						Mask[U1 + V1 * Size] = Visible;
					}
				}

				// Perform greedy merge in 2D (U, V)
				for (int V1 = 0; V1 < Size; ++V1)
				{
					for (int U1 = 0; U1 < Size; )
					{
						if (!Mask[U1 + V1 * Size]) { ++U1; continue; }

						int Width = 1;
						while (U1 + Width < Size && Mask[U1 + Width + V1 * Size]) ++Width;

						int Height = 1;
						while (V1 + Height < Size)
						{
							bool RowGood = true;
							for (int K = 0; K < Width; ++K)
							{
								if (!Mask[U1 + K + (V1 + Height) * Size]) { RowGood = false; break; }
							}
							if (!RowGood) break;
							++Height;
						}

						// Clear used
						for (int V2 = 0; V2 < Height; ++V2)
						{
							for (int U2 = 0; U2 < Width; ++U2)
							{
								Mask[U1 + U2 + (V1 + V2) * Size] = false;
							}
						}

						// Create quad
						FVector Origin = FVector::ZeroVector;
						Origin[U] = U1;
						Origin[V] = V1;
						Origin[D] = (Normal[D] >= 0) ? D1 + 1 : D1;


						FVector BasePos = Origin * VoxelSize;
						FVector QuadRight = Right * Width * VoxelSize;
						FVector QuadUp = Up * Height * VoxelSize;

						int32 StartIndex = Vertices.Num();

						Vertices.Add(BasePos);
						Vertices.Add(BasePos + QuadRight);
						Vertices.Add(BasePos + QuadRight + QuadUp);
						Vertices.Add(BasePos + QuadUp);

						if (Normal.X > 0 || Normal.Y < 0 || Normal.Z > 0)
						{
							Triangles.Append({StartIndex, StartIndex + 2, StartIndex + 1, StartIndex, StartIndex + 3, StartIndex + 2});
						}
						else
						{
							Triangles.Append({StartIndex, StartIndex + 1, StartIndex + 2, StartIndex, StartIndex + 2, StartIndex + 3});
						}

						Normals.Append({Normal, Normal, Normal, Normal});
						
						FVector2D UV0(0, 0);
						FVector2D UV1(Width, 0);
						FVector2D UV2(Width, Height);
						FVector2D UV3(0, Height);

						// Determine if UVs need to be flipped horizontally or vertically
						if (Normal.X < 0 || Normal.Y > 0 || Normal.Z < 0)
						{
							std::swap(UV0.X, UV1.X);
							std::swap(UV3.X, UV2.X);
						}
						UVs.Append({UV0, UV1, UV2, UV3});
						//UVs.Append({FVector2D(0, 0), FVector2D(5, 0), FVector2D(5, 5), FVector2D(0, 5)});
						
						FLinearColor Color = FLinearColor::White;
						if (Normal.X > 0) Color = FLinearColor::Blue;
						if (Normal.X < 0) Color = FLinearColor::Red;
						if (Normal.Y > 0) Color = FLinearColor::Green;
						if (Normal.Y < 0) Color = FLinearColor::Yellow;
						if (Normal.Z > 0) Color = FLinearColor::Gray;
						if (Normal.Z < 0) Color = FLinearColor::Black;

						VertexColors.Append({Color, Color, Color, Color});

						FVector TangentVector = QuadRight.GetSafeNormal();
						if (FVector::DotProduct(TangentVector, Normal) < 0)
						{
							TangentVector *= -1;
						}

						FProcMeshTangent Tangent = FProcMeshTangent(TangentVector, false);
						Tangents.Append({Tangent, Tangent, Tangent, Tangent});

					}
				}
			}
		}
	}
}
