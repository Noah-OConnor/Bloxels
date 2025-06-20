// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/Core/VoxelTypes.h"
#include "VoxelChunkData.generated.h"

USTRUCT(BlueprintType)
struct FVoxelChunkData
{
	GENERATED_BODY()

	// The 3D position of the chunk in chunk space
	FIntVector ChunkCoords;

	// Array of voxel types stored linearly
	TArray<EVoxelType> Voxels;

	// Size info (cached from config for safety)
	int32 ChunkSize = 16;
	int32 ChunkHeight = 16;

	void Initialize(int32 InChunkSize, int32 InChunkHeight, FIntVector InCoords)
	{
		ChunkCoords = InCoords;
		ChunkSize = InChunkSize;
		ChunkHeight = InChunkHeight;

		Voxels.SetNum(ChunkSize * ChunkSize * ChunkHeight);
	}

	FORCEINLINE int32 GetIndex(int32 X, int32 Y, int32 Z) const
	{
		return (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
	}

	FORCEINLINE bool IsInBounds(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && X < ChunkSize &&
			   Y >= 0 && Y < ChunkSize &&
			   Z >= 0 && Z < ChunkHeight;
	}

	FORCEINLINE EVoxelType GetVoxel(int32 X, int32 Y, int32 Z) const
	{
		if (!IsInBounds(X, Y, Z)) return EVoxelType::Air;
		return Voxels[GetIndex(X, Y, Z)];
	}

	FORCEINLINE void SetVoxel(int32 X, int32 Y, int32 Z, EVoxelType Voxel)
	{
		if (!IsInBounds(X, Y, Z)) return;
		Voxels[GetIndex(X, Y, Z)] = Voxel;
	}
};