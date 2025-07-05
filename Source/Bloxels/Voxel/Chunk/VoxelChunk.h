// Copyright 2025 Noah O'Connor. All rights reserved.

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
	FIntPoint ChunkCoords = FIntPoint(-MAX_int32, -MAX_int32);

	
	void InitializeChunk(AVoxelWorld* InVoxelWorld, int32 ChunkX, int32 ChunkY, bool bShouldGenMesh);

	void OnChunkDataGenerated(TArray<uint16> VoxelData);

	void TryGenerateChunkMesh();

	void OnMeshGenerated(const TMap<FMeshSectionKey, FMeshData>& InMeshSections);

	void UnloadChunk();

	bool IsVoxelInChunk(int X, int Y, int Z) const;

	bool CheckVoxel(int X, int Y, int Z, FIntPoint ChunkCoord);

	void SetChunkCoords(FIntPoint InCoords);


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

	
	void GenerateChunkDataAsync();
	
	void GenerateChunkMeshAsync();
	
	void DisplayMesh();
	
	static uint16 GetVoxelTypeForPosition(int Z, int TerrainHeight, const FBiomeProperties* BiomeData);
};