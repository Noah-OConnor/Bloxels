// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"

struct FMeshData;
struct FMeshSectionKey;
class AVoxelChunk;

namespace VoxelChunkAsync
{
    // Chunk Data Generation
    void GenerateChunkDataAsync(TWeakObjectPtr<AVoxelChunk> Chunk, TWeakObjectPtr<AVoxelWorld> World, FIntVector ChunkCoords);

    // Chunk Mesh Generation
    void GenerateChunkMeshAsync(
        TWeakObjectPtr<AVoxelChunk> Chunk,
        TWeakObjectPtr<AVoxelWorld> World,
        const TArray<uint16>& VoxelDataCopy,
        FIntVector ChunkCoords);
    
    int32 GetIndex(int X, int Y, int Z, int ChunkSize);
    
    bool CheckVoxel(const TWeakObjectPtr<AVoxelChunk>& Chunk, FIntVector ChunkCoords, int X, int Y, int Z);
    
    void AddMergedFace(
        const TWeakObjectPtr<AVoxelChunk>& Chunk,
        const TWeakObjectPtr<AVoxelWorld>& World,
        FVector Position, FVector Normal, int32 Width, int32 Height,
        TArray<FVector>& Vertices, TArray<int32>& Triangles,
        TArray<FVector>& Normals, TArray<FVector2D>& UVs);
    
    void ProcessFace(
        const TWeakObjectPtr<AVoxelChunk>& Chunk,
        const TWeakObjectPtr<AVoxelWorld>& World,
        TMap<FMeshSectionKey, FMeshData>& MeshSections,
        const TArray<uint16>& VoxelData,
        int PrimaryCount, int ACount, int BCount, const FVector& Normal,
        const std::function<int(int, int, int)>& GetVoxelIndex,
        const std::function<bool(int, int, int)>& IsNeighborVisible,
        const std::function<FVector(int, int, int)>& GetVoxelPosition);
}
