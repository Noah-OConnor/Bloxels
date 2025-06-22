#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class AVoxelWorld;
class AVoxelChunk;

class BLOXELS_API FVoxelGenerationTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<FVoxelGenerationTask>;

public:
    FVoxelGenerationTask(int32 InChunkX, int32 InChunkY, TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk);

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FVoxelGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

    int32 ChunkX = 0;
    int32 ChunkY = 0;
    TWeakObjectPtr<AVoxelWorld> VoxelWorld;
    TWeakObjectPtr<AVoxelChunk> VoxelChunk;
};
