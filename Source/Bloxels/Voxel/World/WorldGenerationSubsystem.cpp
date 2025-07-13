// Copyright 2025 Bloxels. All rights reserved.

#include "WorldGenerationSubsystem.h"

void UWorldGenerationSubsystem::InitializeConfig(UWorldGenerationConfig* InConfig)
{
    Config = InConfig;

    if (!Config)
    {
        UE_LOG(LogTemp, Error, TEXT("WorldGenerationSubsystem: Config is null!"));
        return;
    }

    InitNoiseGenerators();
}

EBiome UWorldGenerationSubsystem::GetBiome(const int X, const int Y) const
{
    if (!Config->BiomeDataTable) return EBiome::None;
    
    const float Temperature = Config->Temperature.UseThisNoise ? (TemperatureNoise->GetNoise2D(X, Y) + 1) / 2 : 0.f;
    const float Habitability = Config->Habitability.UseThisNoise ? (HabitabilityNoise->GetNoise2D(X, Y) + 1) / 2 : 0.f;
    const float Elevation = Config->Elevation.UseThisNoise ? (ElevationNoise->GetNoise2D(X, Y) + 1) / 2 : 0.f;
    
    TArray<FBiomeProperties*> AllRows;
    Config->BiomeDataTable->GetAllRows(TEXT("Biome Lookup"), AllRows);

    for (const FBiomeProperties* Row : AllRows)
    {
        if (!Row) continue;
        for (const FBiomeNoiseRanges& Range : Row->BiomeNoiseRanges)
        {
            if (Temperature >= Range.MinTemperature && Temperature <= Range.MaxTemperature &&
                Habitability >= Range.MinHabitability && Habitability <= Range.MaxHabitability &&
                Elevation >= Range.MinElevation && Elevation <= Range.MaxElevation)
            {
                return Row->BiomeType;
            }
        }
    }

    return EBiome::None;
}

int UWorldGenerationSubsystem::GetTerrainHeight(const int X, const int Y, EBiome Biome) const
{
    if (!Config) return 32;

    // STEP 1: Set up base noise from elevation
    float BaseHeight = Config->ChunkSize / 2.f;
    if (Config->Elevation.UseThisNoise && Config->Elevation.NoiseCurve)
    {
        const float Elevation = (ElevationNoise->GetNoise2D(X, Y) + 1) / 2;
        BaseHeight = Config->Elevation.NoiseCurve->GetFloatValue(Elevation) / 2.f;
    }

    // STEP 2: Add Biome Specific noise

    return FMath::FloorToInt(BaseHeight * Config->ChunkSize);
}

FName UWorldGenerationSubsystem::GetVoxelTypeForPosition(const int Z, const int TerrainHeight,
    const FBiomeProperties* BiomeData) const
{
    const FName Stone = TEXT("Stone");
    const FName Obsidian = TEXT("Obsidian");

    if (Z < 2) return Obsidian;

    if (BiomeData)
    {
        for (const auto& [VoxelType, BlocksFromSurface, NumBlocks] : BiomeData->SurfaceBlocks)
        {
            if (const int Top = TerrainHeight - BlocksFromSurface; Z >= Top - NumBlocks && Z <= Top)
            {
                return VoxelType;
            }
        }
    }

    return Stone;
}

const FBiomeProperties* UWorldGenerationSubsystem::GetBiomeData(const EBiome Biome) const
{
    if (!Config || !Config->BiomeDataTable) return nullptr;

    TArray<FBiomeProperties*> AllRows;
    Config->BiomeDataTable->GetAllRows(TEXT("Biome Fetch"), AllRows);

    for (const FBiomeProperties* Row : AllRows)
    {
        if (Row && Row->BiomeType == Biome)
        {
            return Row;
        }
    }

    return nullptr;
}

void UWorldGenerationSubsystem::InitNoiseGenerators()
{
    if (!Config) return;

    // Temperature Noise
    if (Config->Temperature.UseThisNoise)
    {
        TemperatureNoise = NewObject<UFastNoiseWrapper>(this);
        TemperatureNoise->SetupFastNoise(Config->Temperature.NoiseType);
        TemperatureNoise->SetFrequency(Config->Temperature.NoiseFrequency);
        TemperatureNoise->SetFractalType(Config->Temperature.NoiseFractalType);
        TemperatureNoise->SetOctaves(Config->Temperature.NoiseOctaves);
        TemperatureNoise->SetSeed(Config->Temperature.NoiseSeed);
    }

    // Habitability Noise
    if (Config->Habitability.UseThisNoise)
    {
        HabitabilityNoise = NewObject<UFastNoiseWrapper>(this);
        HabitabilityNoise->SetupFastNoise(Config->Habitability.NoiseType);
        HabitabilityNoise->SetFrequency(Config->Habitability.NoiseFrequency);
        HabitabilityNoise->SetFractalType(Config->Habitability.NoiseFractalType);
        HabitabilityNoise->SetOctaves(Config->Habitability.NoiseOctaves);
        HabitabilityNoise->SetSeed(Config->Habitability.NoiseSeed);
    }

    // Elevation Noise
    if (Config->Elevation.UseThisNoise)
    {
        ElevationNoise = NewObject<UFastNoiseWrapper>(this);
        ElevationNoise->SetupFastNoise(Config->Elevation.NoiseType);
        ElevationNoise->SetFrequency(Config->Elevation.NoiseFrequency);
        ElevationNoise->SetFractalType(Config->Elevation.NoiseFractalType);
        ElevationNoise->SetOctaves(Config->Elevation.NoiseOctaves);
        ElevationNoise->SetSeed(Config->Elevation.NoiseSeed);
    }
}
