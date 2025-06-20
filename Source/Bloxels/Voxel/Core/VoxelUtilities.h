// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"

namespace VoxelUtils
{
	FORCEINLINE int32 GetIndex(int32 X, int32 Y, int32 Z, int32 ChunkSize)
	{
		return (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
	}

	FORCEINLINE bool IsInBounds(int32 X, int32 Y, int32 Z, int32 ChunkSize, int32 ChunkHeight)
	{
		return X >= 0 && X < ChunkSize &&
			   Y >= 0 && Y < ChunkSize &&
			   Z >= 0 && Z < ChunkHeight;
	}

	FORCEINLINE FIntVector WorldToChunkCoords(const FVector& WorldPos, float VoxelSize, int32 ChunkSize, int32 ChunkHeight)
	{
		return FIntVector(
			FMath::FloorToInt(WorldPos.X / (ChunkSize * VoxelSize)),
			FMath::FloorToInt(WorldPos.Y / (ChunkSize * VoxelSize)),
			FMath::FloorToInt(WorldPos.Z / (ChunkHeight * VoxelSize))
		);
	}

	FORCEINLINE FVector ChunkToWorldLocation(const FIntVector& ChunkCoords, float VoxelSize, int32 ChunkSize, int32 ChunkHeight)
	{
		return FVector(
			ChunkCoords.X * ChunkSize * VoxelSize,
			ChunkCoords.Y * ChunkSize * VoxelSize,
			ChunkCoords.Z * ChunkHeight * VoxelSize
		);
	}
}