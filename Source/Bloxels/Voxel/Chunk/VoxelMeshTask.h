#pragma once

#include "functional"

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class AVoxelWorld;
class AVoxelChunk;
struct FMeshData;
struct FMeshSectionKey;

class BLOXELS_API FVoxelMeshTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<FVoxelMeshTask>;

public:
    FVoxelMeshTask(const TArray<uint16>& InVoxelData, const FIntPoint& InChunkCoords, 
        TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk);

    static TStatId GetStatId();
    
private:
    struct FVoxelFace
    {
        int16 VoxelType = 0;
        bool bVisible = false;
    };
    
    TArray<uint16> VoxelData;
    FIntPoint ChunkCoords;
    TWeakObjectPtr<AVoxelWorld> VoxelWorld;
    TWeakObjectPtr<AVoxelChunk> VoxelChunk;
    
    void DoWork();
    void GreedyMeshChunk(TMap<FMeshSectionKey, FMeshData>& MeshSections);

    void ProcessFace(
        int PrimaryCount,
        int ACount,
        int BCount,
        const FVector& Normal,
        const std::function<int(int, int, int)>& GetVoxelIndex,
        const std::function<bool(int, int, int)>& IsNeighborVisible,
        const std::function<FVector(int, int, int)>& GetVoxelPosition,
        TMap<FMeshSectionKey, FMeshData>& MeshSections);

    void AddMergedFace(FVector Position, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, FVector Normal, int32 Width, int32 Height) const;
    int GetIndex(int X, int Y, int Z) const;
};
