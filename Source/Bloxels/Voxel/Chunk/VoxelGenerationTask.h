#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

struct FBiomeProperties;
class AVoxelWorld;
class AVoxelChunk;

class BLOXELS_API FVoxelGenerationTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<FVoxelGenerationTask>;

public:
    FVoxelGenerationTask(int32 InChunkX, int32 InChunkY, TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk);
    
    static TStatId GetStatId();

private:

    int32 ChunkX = 0;
    int32 ChunkY = 0;
    TWeakObjectPtr<AVoxelWorld> VoxelWorld;
    TWeakObjectPtr<AVoxelChunk> VoxelChunk;

    void DoWork();
    static uint16 GetVoxelTypeForPosition(int Z, int TerrainHeight, const FBiomeProperties* BiomeData);
};
