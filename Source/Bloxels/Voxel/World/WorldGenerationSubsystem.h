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
    const FBiomeProperties* GetBiomeData(EBiome Biome) const;

private:
    UPROPERTY()
    UWorldGenerationConfig* Config;

    UPROPERTY()
    UFastNoiseWrapper* TemperatureNoise;

    UPROPERTY()
    UFastNoiseWrapper* HabitabilityNoise;

    UPROPERTY()
    UFastNoiseWrapper* ElevationNoise;

    void InitNoiseGenerators();
};