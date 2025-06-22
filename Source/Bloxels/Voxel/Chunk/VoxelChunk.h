// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "VoxelChunk.generated.h"


class UVoxelConfig;

DECLARE_EVENT(AVoxelChunk, FOnChunkDataGenerated)

UCLASS()
class BLOXELS_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();
	
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* MeshComponent;
	UPROPERTY(VisibleAnywhere)
	AVoxelWorld* VoxelWorld = nullptr;

	void InitializeChunk(AVoxelWorld* InVoxelWorld, int32 ChunkX, int32 ChunkY, bool bShouldGenMesh);

	void GenerateChunkDataAsync();

	void OnChunkDataGenerated(TArray<uint16> VoxelData);

	void TryGenerateChunkMesh();

	void GenerateChunkMeshAsync();

	void OnMeshGenerated(const TMap<FMeshSectionKey, FMeshData> InMeshSections);
	
	void DisplayMesh();

	void UnloadChunk();

	bool IsVoxelInChunk(int X, int Y, int Z);

	bool CheckVoxel(int X, int Y, int Z, FIntPoint ChunkCoord);

	void SetChunkCoords(FIntPoint InCoords);

	//UPROPERTY(VisibleAnywhere)
	TArray<uint16> VoxelData; // Voxel Storage
	TMap<FMeshSectionKey, FMeshData> MeshSections;
	UPROPERTY(VisibleAnywhere)
	FIntPoint ChunkCoords = FIntPoint(-MAX_int32, -MAX_int32);

	// BOOLS
	bool bGenerateMesh = false;
	bool bHasData = false;
	bool bHasMeshSections = false;

	FOnChunkDataGenerated& OnChunkDataGenerated() { return ChunkDataGeneratedEvent; }
	
private:
	FOnChunkDataGenerated ChunkDataGeneratedEvent;
};