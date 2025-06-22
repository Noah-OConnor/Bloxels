#include "VoxelGenerationTask.h"
#include "VoxelChunk.h"
#include "Bloxels/Voxel/World/Biome/BiomeProperties.h"


FVoxelGenerationTask::FVoxelGenerationTask(int32 InChunkX, int32 InChunkY, TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk)
    : ChunkX(InChunkX), ChunkY(InChunkY), VoxelWorld(InVoxelWorld), VoxelChunk(InVoxelChunk) {}

void FVoxelGenerationTask::DoWork()
{
    TArray<uint16> VoxelData;

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    int ChunkSize = VoxelWorld->ChunkSize;
    int ChunkHeight = VoxelWorld->ChunkHeight;

    VoxelData.SetNum(ChunkSize * ChunkSize * ChunkHeight);
    for (int x = 0; x < ChunkSize; x++)
    {
        for (int y = 0; y < VoxelWorld->ChunkSize; y++)
        {
            int WorldX = ChunkX * ChunkSize + x;
            int WorldY = ChunkY * ChunkSize + y;

            if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
            EBiome biome = VoxelWorld->GetBiome(WorldX, WorldY);
            if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
            const FBiomeProperties* BiomeData = VoxelWorld->GetBiomeData(biome);

            if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
            int TerrainHeight = VoxelWorld->GetTerrainHeight(WorldX, WorldY, biome);

            for (int z = 0; z < TerrainHeight; z++)
            {
                VoxelData[(z * ChunkSize * ChunkSize) + (y * ChunkSize) + x] = 1; // set as stone

                if (z < 2)
                {
                    VoxelData[(z * ChunkSize * ChunkSize) + (y * ChunkSize) + x] = 4; // set as obsidian
                }
                else if (BiomeData && BiomeData->SurfaceBlocks.Num() != 0)
                {
                    for (const FSurfaceBlocks& blocks : BiomeData->SurfaceBlocks)
                    {
                        if (z >= (TerrainHeight - blocks.blocksFromSurface) - blocks.numBlocks && z <= TerrainHeight - blocks.blocksFromSurface)
                        {
                            VoxelData[(z * ChunkSize * ChunkSize) + (y * ChunkSize) + x] = static_cast<uint16>(blocks.VoxelType);
                            break;
                        }
                    }
                }
            }
        }
    }

    AsyncTask(ENamedThreads::GameThread, [WeakChunk = VoxelChunk, VoxelData]()
    {
        if (!WeakChunk.IsValid()) return; // Ensure validity before calling the function
        WeakChunk->OnChunkDataGenerated(VoxelData);
    });
}