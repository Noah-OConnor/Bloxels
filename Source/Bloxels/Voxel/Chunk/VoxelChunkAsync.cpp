#include "VoxelChunkAsync.h"

#include <functional>

#include "VoxelChunk.h"
#include "Bloxels/Voxel/World/Biome/BiomeProperties.h"
#include "Tasks/Task.h"
#include "Async/Async.h"

namespace VoxelChunkAsync
{
    void GenerateChunkDataAsync(TWeakObjectPtr<AVoxelChunk> Chunk, TWeakObjectPtr<AVoxelWorld> World, FIntVector ChunkCoords)
    {
        UE::Tasks::Launch(TEXT("VoxelGen"), [Chunk, World, ChunkCoords]()
        {
            if (!Chunk.IsValid() || !World.IsValid()) return;

            const int32 ChunkSize = World->ChunkSize;
            const int32 ChunkX = ChunkCoords.X;
            const int32 ChunkY = ChunkCoords.Y;

            TArray<uint16> VoxelData;
            VoxelData.SetNum(ChunkSize * ChunkSize * ChunkSize);

        	// INITIALIZE ALL VOXELS TO AIR
        	const uint16 AirID = World->GetVoxelRegistry()->GetIDFromName("Air");
			VoxelData.Init(AirID, ChunkSize * ChunkSize * ChunkSize);

            for (int x = 0; x < ChunkSize; ++x)
            {
                for (int y = 0; y < ChunkSize; ++y)
                {
                    int WorldX = ChunkX * ChunkSize + x;
                    int WorldY = ChunkY * ChunkSize + y;

                    EBiome Biome = World->GetBiome(WorldX, WorldY);
                    const FBiomeProperties* BiomeData = World->GetBiomeData(Biome);
                    int TerrainHeight = World->GetTerrainHeight(WorldX, WorldY, Biome);

                    for (int z = 0; z < TerrainHeight; ++z)
                    {
                        int Index = (z * ChunkSize * ChunkSize) + (y * ChunkSize) + x;
                        VoxelData[Index] = World->GetVoxelRegistry()->GetIDFromName(GetVoxelTypeForPosition(z, TerrainHeight, BiomeData));
                    }
                }
            }

            AsyncTask(ENamedThreads::GameThread, [VoxelData = MoveTemp(VoxelData), Chunk]()
            {
                if (Chunk.IsValid())
                {
                    Chunk->OnChunkDataGenerated(VoxelData);
                }
            });
        });
    }
    
    FName GetVoxelTypeForPosition(const int Z, const int TerrainHeight, const FBiomeProperties* BiomeData)
    {
        const FName Stone = FName("Stone");
        const FName Obsidian = FName("Obsidian");
    	
        if (Z < 2)
        {
            return Obsidian;
        }

        if (BiomeData && BiomeData->SurfaceBlocks.Num() > 0)
        {
            for (const auto& [VoxelType, BlocksFromSurface, NumBlocks] : BiomeData->SurfaceBlocks)
            {
                const int Bottom = TerrainHeight - BlocksFromSurface - NumBlocks;
                if (const int Top = TerrainHeight - BlocksFromSurface; Z >= Bottom && Z <= Top)
                {
                    return VoxelType;
                }
            }
        }

        return Stone;
    }

	void GenerateChunkMeshAsync(
		TWeakObjectPtr<AVoxelChunk> Chunk,
		TWeakObjectPtr<AVoxelWorld> World,
		const TArray<uint16>& VoxelDataCopy,
		FIntVector ChunkCoords)
	{
		UE::Tasks::Launch(TEXT("VoxelMeshTask"), [=]()
		{
			if (!Chunk.IsValid() || !World.IsValid())
				return;
			
			const int ChunkSize = World->ChunkSize;

			TMap<FMeshSectionKey, FMeshData> MeshSections;

			// +Z (Top)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(0, 0, 1),
				[&](int x, int y, int z) { return GetIndex(x, y, z, ChunkSize); },
				[&](int x, int y, int z) { return CheckVoxel(Chunk, ChunkCoords, x, y, z + 1); },
				[](int x, int y, int z) { return FVector(x, y, z); }
			);

			// -Z (Bottom)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(0, 0, -1),
				[&](int x, int y, int z) { return GetIndex(x, y, z, ChunkSize); },
				[&](int x, int y, int z) { return CheckVoxel(Chunk, ChunkCoords, x, y, z - 1); },
				[](int x, int y, int z) { return FVector(x, y, z); }
			);

			// +Y (Front)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(0, 1, 0),
				[&](int x, int z, int y) { return GetIndex(x, y, z, ChunkSize); },
				[&](int x, int z, int y) { return CheckVoxel(Chunk, ChunkCoords, x, y + 1, z); },
				[](int x, int z, int y) { return FVector(x, y, z); }
			);

			// -Y (Back)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(0, -1, 0),
				[&](int x, int z, int y) { return GetIndex(x, y, z, ChunkSize); },
				[&](int x, int z, int y) { return CheckVoxel(Chunk, ChunkCoords, x, y - 1, z); },
				[](int x, int z, int y) { return FVector(x, y, z); }
			);

			// +X (Right)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(1, 0, 0),

				[&](int y, int z, int x) { return GetIndex(x, y, z, ChunkSize); },
				[&](int y, int z, int x) { return CheckVoxel(Chunk, ChunkCoords, x + 1, y, z); },
				[](int y, int z, int x) { return FVector(x + 1, y, z); }
			);

			// -X (Left)
			ProcessFace(
				Chunk, World, MeshSections, VoxelDataCopy,
				ChunkSize, ChunkSize, ChunkSize,
				FVector(-1, 0, 0),
				[&](int y, int z, int x) { return GetIndex(x, y, z, ChunkSize); },
				[&](int y, int z, int x) { return CheckVoxel(Chunk, ChunkCoords, x - 1, y, z); },
				[](int y, int z, int x) { return FVector(x, y, z); }
			);

			// Apply result on game thread
			AsyncTask(ENamedThreads::GameThread, [Chunk, ChunkCoords, MeshSections = MoveTemp(MeshSections)]()
			{
				if (Chunk.IsValid())
				{
					Chunk->SetChunkCoords(ChunkCoords);
					Chunk->OnMeshGenerated(MeshSections);
				}
			});
		});
	}


	int32 GetIndex(int X, int Y, int Z, int ChunkSize)
    {
    	return (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
    }

	bool CheckVoxel(const TWeakObjectPtr<AVoxelChunk>& Chunk, FIntVector ChunkCoords, int X, int Y, int Z)
    {
    	return Chunk.IsValid() ? Chunk->CheckVoxel(X, Y, Z, ChunkCoords) : false;
    }

	void AddMergedFace(
	const TWeakObjectPtr<AVoxelChunk>& Chunk,
	const TWeakObjectPtr<AVoxelWorld>& World,
	FVector Position, FVector Normal, int32 Width, int32 Height,
	TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs)
    {
    	if (!Chunk.IsValid() || !World.IsValid()) return;

    	const int VoxelSize = World->VoxelSize;
    	int32 VertexIndex = Vertices.Num();
    	FVector Right, Up;

    	if (Normal == FVector(0, 0, 1)) // Top
    	{
    		Right = FVector(Width, 0, 0) * VoxelSize;
    		Up = FVector(0, Height, 0) * VoxelSize;
    		Position += Normal;
    	}
    	else if (Normal == FVector(0, 0, -1)) // Bottom
    	{
    		Right = FVector(Width, 0, 0) * VoxelSize;
    		Up = FVector(0, Height, 0) * VoxelSize;
    	}
    	else if (Normal == FVector(0, 1, 0)) // Front
    	{
    		Right = FVector(Width, 0, 0) * VoxelSize;
    		Up = FVector(0, 0, Height) * VoxelSize;
    		Position += Normal;
    	}
    	else if (Normal == FVector(0, -1, 0)) // Back
    	{
    		Right = FVector(Width, 0, 0) * VoxelSize;
    		Up = FVector(0, 0, Height) * VoxelSize;
    	}
    	else if (Normal == FVector(1, 0, 0)) // Right
    	{
    		Right = FVector(0, Width, 0) * VoxelSize;
    		Up = FVector(0, 0, Height) * VoxelSize;
    	}
    	else if (Normal == FVector(-1, 0, 0)) // Left
    	{
    		Right = FVector(0, Width, 0) * VoxelSize;
    		Up = FVector(0, 0, Height) * VoxelSize;
    	}

    	FVector V0 = Position * VoxelSize;
    	FVector V1 = V0 + Right;
    	FVector V2 = V0 + Right + Up;
    	FVector V3 = V0 + Up;

    	Vertices.Append({ V0, V1, V2, V3 });

    	if (Normal == FVector(0, 0, 1) || Normal == FVector(0, -1, 0) || Normal == FVector(1, 0, 0))
    	{
    		// Reverse the order of vertices for bottom, back, and left faces
    		Triangles.Append({
				VertexIndex,
				VertexIndex + 2,
				VertexIndex + 1,
				VertexIndex,
				VertexIndex + 3,
				VertexIndex + 2
				});
    	}
    	else
    	{
    		Triangles.Append({
				VertexIndex,
				VertexIndex + 1,
				VertexIndex + 2,
				VertexIndex,
				VertexIndex + 2,
				VertexIndex + 3
				});
    	}

    	for (int i = 0; i < 4; i++)
    	{
    		Normals.Add(Normal);
    	}

    	UVs.Append({
			FVector2D(0.0f, static_cast<float>(Height)),
			FVector2D(static_cast<float>(Width), static_cast<float>(Height)),
			FVector2D(static_cast<float>(Width), 0.0f),
			FVector2D(0.0f, 0.0f)
			});
    }

	void ProcessFace(
		const TWeakObjectPtr<AVoxelChunk>& Chunk,
		const TWeakObjectPtr<AVoxelWorld>& World,
		TMap<FMeshSectionKey, FMeshData>& MeshSections,
		const TArray<uint16>& VoxelData,
		int PrimaryCount, int ACount, int BCount, const FVector& Normal,
		const std::function<int(int, int, int)>& GetVoxelIndex,
		const std::function<bool(int, int, int)>& IsNeighborVisible,
		const std::function<FVector(int, int, int)>& GetVoxelPosition)
    {
    	struct FVoxelFace { int16 VoxelType = 0; bool bVisible = false; };

    	TArray<TArray<FVoxelFace>> Mask;
    	Mask.SetNum(ACount);
    	for (int A = 0; A < ACount; A++) Mask[A].SetNum(BCount);

    	for (int P = 0; P < PrimaryCount; P++)
    	{
    		for (int A = 0; A < ACount; A++)
    			for (int B = 0; B < BCount; B++)
    				Mask[A][B] = {};

    		for (int A = 0; A < ACount; A++)
    		{
    			for (int B = 0; B < BCount; B++)
    			{
    				const int Index = GetVoxelIndex(A, B, P);
    				if (Index < 0 || Index >= VoxelData.Num()) continue;

    				const int16 VoxelType = VoxelData[Index];
    				const UVoxelData* Voxel = World->GetVoxelRegistry()->GetVoxelByID(VoxelType);

    				if (Voxel && !Voxel->bIsInvisible && IsNeighborVisible(A, B, P))
    				{
    					Mask[A][B] = { VoxelType, true };
    				}
    			}
    		}

    		for (int A = 0; A < ACount; A++)
    		{
    			for (int B = 0; B < BCount; B++)
    			{
    				if (!Mask[A][B].bVisible) continue;

    				int Width = 1;
    				while (A + Width < ACount &&
						   Mask[A + Width][B].bVisible &&
						   Mask[A + Width][B].VoxelType == Mask[A][B].VoxelType)
    					++Width;

    				int Height = 1;
    				bool Expand = true;
    				while (B + Height < BCount && Expand)
    				{
    					for (int i = 0; i < Width; ++i)
    					{
    						if (!Mask[A + i][B + Height].bVisible ||
								Mask[A + i][B + Height].VoxelType != Mask[A][B].VoxelType)
    						{
    							Expand = false;
    							break;
    						}
    					}
    					if (Expand) ++Height;
    				}

    				FMeshSectionKey Key(Mask[A][B].VoxelType, Normal);
    				FMeshData& MeshData = MeshSections.FindOrAdd(Key);
    				AddMergedFace(Chunk, World, GetVoxelPosition(A, B, P), Normal, Width, Height,
						MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UVs);


    				for (int i = 0; i < Width; ++i)
    					for (int j = 0; j < Height; ++j)
    						Mask[A + i][B + j].bVisible = false;
    			}
    		}
    	}
    }
}

