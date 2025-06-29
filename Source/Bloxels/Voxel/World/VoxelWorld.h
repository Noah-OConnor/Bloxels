// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastNoiseWrapper.h"
#include "Biome/Biome.h"
#include "Biome/NoiseInfo.h"
#include "Bloxels/Voxel/Core/VoxelInfo.h"
#include "Engine/TriggerVolume.h"
#include "GameFramework/Actor.h"
#include "VoxelWorld.generated.h"

class AVoxelChunk;
class UVoxelConfig;
struct FBiomeProperties;

UCLASS()
class BLOXELS_API AVoxelWorld : public AActor
{
    GENERATED_BODY()

public:
    AVoxelWorld();

    virtual void BeginPlay() override;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Data")
    UVoxelInfo* VoxelInfoDataAsset;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Data")
    UDataTable* BiomeDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    int WorldSize = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    float VoxelSize = 100.0f; // 100 is 1 meter which is what minecraft uses

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    int32 ChunkSize = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    int32 ChunkHeight = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|World Generation")
    ATriggerVolume* ChunkTriggerVolume = nullptr;

    // Biome Noise Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome Noise Settings")
    FNoiseInfo Temperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome Noise Settings")
    FNoiseInfo Habitability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome Noise Settings")
    FNoiseInfo Elevation;

    UFUNCTION(BlueprintCallable, Category = "Voxel|Player")
    int PlaceBlock(int X, int Y, int Z, int BlockToPlace);

    
    UPROPERTY()
    TMap<FIntPoint, AVoxelChunk*> Chunks;

    UPROPERTY()
    TMap<FIntPoint, AVoxelChunk*> ActiveChunks;

    
    const FBiomeProperties* GetBiomeData(EBiome Biome) const;
    static FVoxelData VoxelProperties[UINT16_MAX];
    
    mutable FRWLock ChunksLock;
    mutable FRWLock ActiveChunksLock;
    
    EBiome GetBiome(int X, int Y) const;
    int GetTerrainHeight(int X, int Y, EBiome Biome) const;
    int16 GetVoxelAtWorldCoordinates(int X, int Y, int Z);

    
    void TryCreateNewChunk(int32 ChunkX, int32 ChunkY, bool bShouldGenMesh);


private:
    UPROPERTY()
    UFastNoiseWrapper* TemperatureNoise;
    UPROPERTY()
    UFastNoiseWrapper* HabitabilityNoise;
    UPROPERTY()
    UFastNoiseWrapper* ElevationNoise;

    UPROPERTY()
    APawn* PlayerPawn;

    UFUNCTION()
    void OnChunkExit(AActor* OverlappedActor, AActor* OtherActor);
    
    FIntPoint CurrentChunk = 0;
    FIntPoint PreviousChunk = 0;
    bool bIsShuttingDown = false;

    void InitializeTriggerVolume();
    void InitializeVoxelProperties() const;
    void InitializeNoiseLayers();
    void GenerateInitialWorld();
    void InitializePlayer();
    void UpdateTriggerVolume(FVector PlayerPosition) const;
    void UpdateChunks();
};
