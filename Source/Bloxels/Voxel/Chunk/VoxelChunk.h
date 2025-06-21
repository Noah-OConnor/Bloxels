// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelChunkData.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "VoxelChunk.generated.h"


class UVoxelConfig;

UCLASS()
class BLOXELS_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel|Mesh")
	UProceduralMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Mesh")
	UMaterial* Material;

	/** The chunk's grid location in world space (in chunk units, not voxels) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel|Chunk")
	FIntVector ChunkCoords;

	/** Raw voxel data for this chunk */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel|Chunk")
	FVoxelChunkData ChunkData;

	/** Reference to the config instance (injected by the world) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Chunk")
	UVoxelConfig* Config;

	/** Initializes the chunk with a location and voxel config */
	void Initialize(FIntVector InCoords, UVoxelConfig* InConfig);

	void GenerateMesh();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
