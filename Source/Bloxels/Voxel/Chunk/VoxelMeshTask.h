#pragma once

#include <functional>

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
    FVoxelMeshTask(TArray<uint16>& InVoxelData, FIntPoint& InChunkCoords, 
        TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk);

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FVoxelMeshTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();
    void GreedyMeshChunk(TMap<FMeshSectionKey, FMeshData>& MeshSections);
    void ProcessXYFace(int Axis1, int Axis2, int Axis3, FVector Normal, std::function<bool(int, int, int)> CheckNeighbor, std::function<FVector(int, int, int)> GetVoxelPosition, TMap<FMeshSectionKey, FMeshData>& MeshSections);
    void ProcessXZFace(int Axis1, int Axis2, int Axis3, FVector Normal, std::function<bool(int, int, int)> CheckNeighbor, std::function<FVector(int, int, int)> GetVoxelPosition, TMap<FMeshSectionKey, FMeshData>& MeshSections);
    void ProcessYZFace(int Direction, TMap<FMeshSectionKey, FMeshData>& MeshSections, FIntPoint ChunkCoord);
    void AddMergedFace(FVector Position, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, FVector Normal, int32 Width, int32 Height);

    TArray<uint16> VoxelData;
    FIntPoint ChunkCoords;
    TWeakObjectPtr<AVoxelWorld> VoxelWorld;
    TWeakObjectPtr<AVoxelChunk> VoxelChunk;
};
