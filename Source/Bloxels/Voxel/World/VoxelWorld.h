// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
class UVoxelConfig;

UCLASS()
class BLOXELS_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorld();

protected:
	virtual void BeginPlay() override;

public:
	/** The config asset to drive world generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Voxel Settings")
	UVoxelConfig* VoxelConfig;

	/** The chunk blueprint or class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Voxel Settings")
	TSubclassOf<AVoxelChunk> ChunkClass;

	/** The number of chunks to load outward from the center (X/Y/Z) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
	int32 LoadRadius = 2;

	/** Map of world chunk coords to their actor instances */
	UPROPERTY()
	TMap<FIntVector, AVoxelChunk*> Chunks;

	/** Spawns all chunks in a cubic grid around the player */
	void GenerateInitialChunks();

	/** Helper to spawn a single chunk at ChunkCoords if not already loaded */
	void SpawnChunkAt(FIntVector ChunkCoords);
};
