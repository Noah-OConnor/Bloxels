#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"

struct FVoxelChunkData
{
    FIntVector Coords;
    TArray<uint16> VoxelData;
    bool bHasData = false;
    bool bHasMesh = false;

    TMap<FMeshSectionKey, FMeshData> MeshSections;
    UProceduralMeshComponent* Mesh = nullptr;

    bool AreNeighborsReady(const TMap<FIntVector, FVoxelChunkData>& AllChunks) const
    {
        static const TArray<FIntVector> NeighborOffsets = {
            FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
            FIntVector(0, 1, 0), FIntVector(0, -1, 0),
            FIntVector(0, 0, 1), FIntVector(0, 0, -1)
        };
        
        for (const FIntVector& Offset : NeighborOffsets)
        {
            FIntVector NeighborCoord = Coords + Offset;

            const FVoxelChunkData* Neighbor = AllChunks.Find(NeighborCoord);
            if (!Neighbor || !Neighbor->bHasData)
            {
                return false;
            }
        }
        return true;
    }
};
