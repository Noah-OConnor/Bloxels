// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "GameFramework/Actor.h"
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
	FIntVector ChunkCoords = FIntVector(-MAX_int32, -MAX_int32, -MAX_int32);

	
	void InitializeChunk(AVoxelWorld* InVoxelWorld, int32 ChunkX, int32 ChunkY, int32 ChunkZ, bool bShouldGenMesh);

	void OnChunkDataGenerated(TArray<uint16> InVoxelData);

	void TryGenerateChunkMesh();

	void OnMeshGenerated(const TMap<FMeshSectionKey, FMeshData>& InMeshSections);

	void UnloadChunk();

	bool IsVoxelInChunk(int X, int Y, int Z) const;

	bool CheckVoxel(int X, int Y, int Z, FIntVector ChunkCoord);

	void SetChunkCoords(FIntVector InCoords);
	
	void GenerateChunkDataAsync();
	
	void GenerateChunkMeshAsync();
	
	void DisplayMesh();


	// BOOLS
	bool bGenerateMesh = false;
	bool bHasData = false;
	bool bHasMeshSections = false;

	TArray<uint16> VoxelData;

protected:
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* MeshComponent;
	
	UPROPERTY(VisibleAnywhere)
	AVoxelWorld* VoxelWorld = nullptr;
	
private:
	FOnChunkDataGenerated& OnChunkDataGenerated() { return ChunkDataGeneratedEvent; }
	FOnChunkDataGenerated ChunkDataGeneratedEvent;

	
	TMap<FMeshSectionKey, FMeshData> MeshSections;

};