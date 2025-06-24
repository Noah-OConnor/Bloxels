// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FastNoiseWrapper.h"
#include "Biome/Biome.h"
#include "Biome/NoiseInfo.h"
#include "Bloxels/Voxel/Core/VoxelInfo.h"
#include "Engine/TriggerVolume.h"
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

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    TSoftObjectPtr<UVoxelInfo> VoxelInfoDataAsset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    UDataTable* BiomeDataTable;

    virtual void BeginPlay() override;

public:
    void TryCreateNewChunk(int32 ChunkX, int32 ChunkY, bool bShouldGenMesh);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
    int WorldSize = 2;  // How many chunks in X and Y directions

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
    float VoxelSize = 100.0f; // 100 is 1 meter which is what minecraft uses

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
    int32 ChunkSize = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
    int32 ChunkHeight = 32;

    // Biome Noise Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
    FNoiseInfo Temperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
    FNoiseInfo Habitability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
    FNoiseInfo Elevation;

    UPROPERTY()
    UFastNoiseWrapper* TemperatureNoise;
    UPROPERTY()
    UFastNoiseWrapper* HabitabilityNoise;
    UPROPERTY()
    UFastNoiseWrapper* ElevationNoise;

    APawn* PlayerPawn;
    FIntPoint CurrentChunk = 0;
    FIntPoint PreviousChunk = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ATriggerVolume* ChunkTriggerVolume = nullptr;

    mutable FRWLock ChunksLock;
    UPROPERTY(VisibleAnywhere)
    TMap<FIntPoint, AVoxelChunk*> Chunks; // Store All chunks in a map
    mutable FRWLock ActiveChunksLock;
    UPROPERTY(VisibleAnywhere)
    TMap<FIntPoint, AVoxelChunk*> ActiveChunks; // Store Active chunks in a map
    static FVoxelData VoxelProperties[UINT16_MAX];

    bool bIsShuttingDown = false;

    //UPROPERTY(EditDefaultsOnly, Category = "Materials")
    //UMaterialInterface* BaseBlockMaterial;

    int16 GetVoxelAtWorldCoordinates(int X, int Y, int Z);
    void InitializeTriggerVolume();
    void InitializeVoxelProperties();
    void InitializeNoiseLayers();
    void GenerateInitialWorld();
    void InitializePlayer();
    EBiome GetBiome(int X, int Y);
    int GetTerrainHeight(int X, int Y, EBiome Biome);
    const FBiomeProperties* GetBiomeData(EBiome Biome) const;
    void UpdateTriggerVolume(FVector PlayerPosition);
    UFUNCTION()
    void OnChunkExit(AActor* OverlappedActor, AActor* OtherActor);
    void UpdateChunks();

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    int PlaceBlock(int X, int Y, int Z, int blockToPlace);
};
