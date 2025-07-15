// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldGenerationConfig.h"
#include "Biome/BiomeProperties.h"
#include "WorldGenerationSubsystem.generated.h"

UCLASS()
class BLOXELS_API UWorldGenerationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void InitializeConfig(UWorldGenerationConfig* InConfig);

    EBiome GetBiome(int X, int Y) const;
    int GetTerrainHeight(int X, int Y, EBiome Biome) const;
    FName GetVoxelTypeForPosition(int Z, int TerrainHeight, const FBiomeProperties* BiomeData) const;
    FName GetVoxelAtPosition(int X, int Y, int Z) const;
    const FBiomeProperties* GetBiomeData(EBiome Biome) const;
    void LoadStructureAt(const FString& FileName, const FIntVector& OriginWorldCoords);


private:
    UPROPERTY()
    UWorldGenerationConfig* Config;

    UPROPERTY()
    UFastNoiseWrapper* TemperatureNoise;

    UPROPERTY()
    UFastNoiseWrapper* HabitabilityNoise;

    UPROPERTY()
    UFastNoiseWrapper* ElevationNoise;

    UPROPERTY()
    UFastNoiseWrapper* UndergroundNoise;

    UPROPERTY()
    UFastNoiseWrapper* UndergroundNoise2;

    UPROPERTY()
    UFastNoiseWrapper* WarpNoise;

    void InitNoiseGenerators();
};