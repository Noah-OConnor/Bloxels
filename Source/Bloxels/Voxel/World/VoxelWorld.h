// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastNoiseWrapper.h"
#include "WorldGenerationSubsystem.h"
#include "Bloxels/Voxel/VoxelRegistry/VoxelRegistrySubsystem.h"
#include "GameFramework/Actor.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
struct FBiomeProperties;
class UWorldGenerationConfig;

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

    
    UPROPERTY()
    TMap<FIntVector, AVoxelChunk*> Chunks;

    UPROPERTY()
    TMap<FIntVector, AVoxelChunk*> ActiveChunks;

    
    //mutable FRWLock ChunksLock;
    //mutable FRWLock ActiveChunksLock;
    
    int16 GetVoxelAtWorldCoordinates(int X, int Y, int Z);
    UVoxelRegistrySubsystem* GetVoxelRegistry() const;
    UWorldGenerationSubsystem* GetWorldGenerationSubsystem() const;
    UWorldGenerationConfig* GetWorldGenerationConfig() const;
    void TryCreateNewChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ, bool bShouldGenMesh);

    UPROPERTY()
    TArray<AVoxelChunk*> ChunkMeshUpdateQueue;
    int32 ChunkMeshesPerFrame = 10;

    void AddToChunkMeshQueue(AVoxelChunk* Chunk);

    UPROPERTY()
    TArray<AVoxelChunk*> ChunksToDeleteQueue;
    int32 ChunksToDeletePerFrame = 10;

    void AddToChunkToDeleteQueue(AVoxelChunk* Chunk);
    
    UPROPERTY()
    TArray<AVoxelChunk*> ChunkPool;
    
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
};
