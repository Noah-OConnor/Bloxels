// Copyright 2025 Noah O'Connor. All rights reserved.

#include "VoxelGenerationTask.h"
#include "VoxelChunk.h"
#include "Bloxels/Voxel/World/Biome/BiomeProperties.h"


FVoxelGenerationTask::FVoxelGenerationTask(const int32 InChunkX, const int32 InChunkY, const TWeakObjectPtr<AVoxelWorld> InVoxelWorld, const TWeakObjectPtr<AVoxelChunk> InVoxelChunk)
    : ChunkX(InChunkX), ChunkY(InChunkY), VoxelWorld(InVoxelWorld), VoxelChunk(InVoxelChunk) {}

TStatId FVoxelGenerationTask::GetStatId()
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FVoxelGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
}

void FVoxelGenerationTask::DoWork()
{
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    

    const int ChunkSize = VoxelWorld->ChunkSize;
    const int ChunkHeight = VoxelWorld->ChunkHeight;
    
    TArray<uint16> VoxelData;
    VoxelData.SetNum(ChunkSize * ChunkSize * ChunkHeight);
    
    for (int x = 0; x < ChunkSize; ++x)
    {
        for (int y = 0; y < ChunkSize; ++y)
        {
            const int WorldX = ChunkX * ChunkSize + x;
            const int WorldY = ChunkY * ChunkSize + y;

            const EBiome Biome = VoxelWorld->GetBiome(WorldX, WorldY);
            const FBiomeProperties* BiomeData = VoxelWorld->GetBiomeData(Biome);
            const int TerrainHeight = VoxelWorld->GetTerrainHeight(WorldX, WorldY, Biome);

            for (int z = 0; z < TerrainHeight; ++z)
            {
                const int Index = (z * ChunkSize * ChunkSize) + (y * ChunkSize) + x;
                VoxelData[Index] = GetVoxelTypeForPosition(z, TerrainHeight, BiomeData);
            }
        }
    }

    AsyncTask(ENamedThreads::GameThread, [WeakChunk = VoxelChunk, VoxelData]()
    {
        if (!WeakChunk.IsValid()) return; // Ensure validity before calling the function
        WeakChunk->OnChunkDataGenerated(VoxelData);
    });
}

uint16 FVoxelGenerationTask::GetVoxelTypeForPosition(const int Z, const int TerrainHeight, const FBiomeProperties* BiomeData)
{
    constexpr uint16 Stone = 1;

    if (Z < 2)
    {
        constexpr uint16 Obsidian = 4;
        return Obsidian;
    }

    if (BiomeData && BiomeData->SurfaceBlocks.Num() > 0)
    {
        for (const auto& [VoxelType, BlocksFromSurface, NumBlocks] : BiomeData->SurfaceBlocks)
        {
            const int Bottom = TerrainHeight - BlocksFromSurface - NumBlocks;
            if (const int Top = TerrainHeight - BlocksFromSurface; Z >= Bottom && Z <= Top)
            {
                return static_cast<uint16>(VoxelType);
            }
        }
    }

    return Stone;
}
