// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastNoiseWrapper.h"
#include "ProceduralMeshComponent.h"
#include "WorldGenerationSubsystem.h"
#include "Bloxels/Voxel/Chunk/VoxelChunkData.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"
#include "Bloxels/Voxel/VoxelRegistry/VoxelRegistrySubsystem.h"
#include "GameFramework/Actor.h"
#include "VoxelWorld.generated.h"

//class AVoxelChunk;
struct FBiomeProperties;
class UWorldGenerationConfig;
//struct FVoxelChunkData;

UCLASS()
class BLOXELS_API AVoxelWorld : public AActor
{
    GENERATED_BODY()

public:
    AVoxelWorld();

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Config")
    UWorldGenerationConfig* VoxelWorldConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    int WorldSize = 2;

    UFUNCTION(BlueprintCallable, Category = "Voxel|Player")
    int PlaceBlock(int X, int Y, int Z, int BlockToPlace);

    
    //UPROPERTY()
    //TMap<FIntVector, FVoxelChunkData> Chunks;
    
    //UPROPERTY()
    TMap<FIntVector, FVoxelChunkData> ActiveChunks;

    
    mutable FRWLock ActiveChunksLock;
    
    int16 GetVoxelAtWorldCoordinates(int X, int Y, int Z);
    bool CheckVoxel(int X, int Y, int Z, FIntVector ChunkCoord);
    bool IsVoxelInChunk(int X, int Y, int Z) const;
    UVoxelRegistrySubsystem* GetVoxelRegistry() const;
    UWorldGenerationSubsystem* GetWorldGenerationSubsystem() const;
    UWorldGenerationConfig* GetWorldGenerationConfig() const;
    //void TryCreateNewChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ, bool bShouldGenMesh);

    // UPROPERTY()
    // TArray<AVoxelChunk*> ChunkUnloadQueue;
    // int32 ChunkUnloadPerFrame = 10;
    
    //UPROPERTY()
    TQueue<FIntVector> ChunkCreationQueue;
    
    UPROPERTY(EditAnywhere)
    int32 ChunkCreationPerFrame = 10;

    // UPROPERTY()
    // TArray<AVoxelChunk*> ChunkDataGenQueue;
    // int32 ChunkDataGenPerFrame = 10;

    //UPROPERTY()
    TQueue<FIntVector> ChunkMeshGenQueue;
    
    UPROPERTY(EditAnywhere)
    int32 ChunkMeshGenPerFrame = 10;

    //PROPERTY()
    TQueue<FIntVector> ChunkMeshDisplayQueue;
    
    UPROPERTY(EditAnywhere)
    int32 ChunkMeshDisplayPerFrame = 10;
    
    // UPROPERTY()
    // TArray<AVoxelChunk*> ChunkPool;

    TQueue<UProceduralMeshComponent*> MeshPool;

    TSet<FIntVector> PendingMeshChunks;

    void OnChunkDataGenerated(FIntVector Coord, TArray<uint16> VoxelData);

    void OnMeshGenerated(FIntVector Coord, TMap<FMeshSectionKey, FMeshData> MeshSections);
    
    void ProcessChunkMeshDisplayQueue();

private:
    UPROPERTY()
    UFastNoiseWrapper* TemperatureNoise;
    UPROPERTY()
    UFastNoiseWrapper* HabitabilityNoise;
    UPROPERTY()
    UFastNoiseWrapper* ElevationNoise;

    UPROPERTY()
    APawn* PlayerPawn;
    
    FIntVector CurrentChunk = FIntVector(0, 0, 0);
    FIntVector PreviousChunk = FIntVector(0, 0, 0);
    bool bIsShuttingDown = false;

    void GenerateInitialWorld();
    void InitializePlayer();
    void UpdateChunks();

    void QueueChunk(FIntVector ChunkCoord);

    void ProcessChunkCreationQueue();
    void GenerateChunkDataAsync(FIntVector ChunkCoord);
    void ProcessPendingMeshChunks();
    bool IsChunkNearPlayer(const FIntVector& Coord) const;

    void ProcessChunkMeshGenQueue();
    
    void GenerateChunkMeshAsync(FIntVector ChunkCoord, const FVoxelChunkData& ChunkData);

    void DisplayChunkMesh(FVoxelChunkData& Chunk);
    void UnloadChunk(FIntVector ChunkCoords);

    UProceduralMeshComponent* GetOrCreateMeshComponent();
    
    void RecycleMesh(UProceduralMeshComponent* Mesh);
    
    FVector CoordToWorldPosition(FIntVector Coord) const;
};
