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

    float BaseHeight = Config->ChunkSize / 2.f; // default base height if there are no noise layers
    
    // STEP 1: Set up base noise from elevation
    if (Config->Elevation.UseThisNoise && Config->Elevation.NoiseCurve)
    {
        const float Elevation = (ElevationNoise->GetNoise2D(X, Y) + 1) / 2;
        BaseHeight = Config->Elevation.NoiseCurve->GetFloatValue(Elevation);
    }

    // STEP 2: Add Biome Specific noise

    return FMath::FloorToInt(Config->SurfaceMinHeight + (BaseHeight * (Config->SurfaceMaxHeight - Config->SurfaceMinHeight)));
}

FName UWorldGenerationSubsystem::GetVoxelTypeForPosition(const int Z, const int TerrainHeight,
    const FBiomeProperties* BiomeData) const
{
    const FName Stone = TEXT("Stone");
    const FName Obsidian = TEXT("Obsidian");

    //if (Z < 2) return Obsidian;

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

FName UWorldGenerationSubsystem::GetVoxelAtPosition(int X, int Y, int Z) const
{
    if (!Config) return TEXT("Air");

    // Always return air for anything above generation height
    if (Z > Config->ChunkSize * 20) return TEXT("Air");
    if (Z < 0) return TEXT("Stone");

    const EBiome Biome = GetBiome(X, Y);
    const int TerrainHeight = GetTerrainHeight(X, Y, Biome);
    const FBiomeProperties* BiomeData = GetBiomeData(Biome);

    if (Z > TerrainHeight)
    {
        return TEXT("Air");
    }
    else
    {
        if (UndergroundNoise && UndergroundNoise2)
        {
            float WarpStrength = 5;
            
            float WarpValX = WarpNoise->GetNoise3D(X, Y, Z) * WarpStrength;
            float WarpValY = WarpNoise->GetNoise3D(X + 1337, Y + 1337, Z + 1337) * WarpStrength;
            float WarpValZ = WarpNoise->GetNoise3D(X + 9999, Y + 9999, Z + 9999) * WarpStrength;
            
            float NoiseVal = UndergroundNoise->GetNoise3D(X + WarpValX, Y + WarpValY, Z + WarpValZ); // Expected range: [-1, 1]

            float NoiseVal2 = UndergroundNoise2->GetNoise3D(X + WarpValX, Y + WarpValY, Z + WarpValZ);

            NoiseVal = (NoiseVal + NoiseVal2) / 2;
            if (NoiseVal > 0.95f) return TEXT("Air"); // threshold tunable
        }
        
        if (Z >= TerrainHeight - 10)
        {
            return GetVoxelTypeForPosition(Z, TerrainHeight, BiomeData);
        }
        
        return TEXT("Stone");
    }

    
    
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

    // Underground Noise
    if (Config->Underground.UseThisNoise)
    {
        UndergroundNoise = NewObject<UFastNoiseWrapper>(this);
        UndergroundNoise->SetupFastNoise(Config->Underground.NoiseType);
        UndergroundNoise->SetFrequency(Config->Underground.NoiseFrequency);
        UndergroundNoise->SetFractalType(Config->Underground.NoiseFractalType);
        UndergroundNoise->SetOctaves(Config->Underground.NoiseOctaves);
        UndergroundNoise->SetSeed(Config->Underground.NoiseSeed);
    }

    // Underground Noise
    if (Config->Underground.UseThisNoise)
    {
        UndergroundNoise2 = NewObject<UFastNoiseWrapper>(this);
        UndergroundNoise2->SetupFastNoise(Config->Underground.NoiseType);
        UndergroundNoise2->SetFrequency(Config->Underground.NoiseFrequency);
        UndergroundNoise2->SetFractalType(Config->Underground.NoiseFractalType);
        UndergroundNoise2->SetOctaves(Config->Underground.NoiseOctaves);
        UndergroundNoise2->SetSeed(Config->Underground.NoiseSeed + 9999);
    }

    WarpNoise = NewObject<UFastNoiseWrapper>(this);
    WarpNoise->SetupFastNoise(EFastNoise_NoiseType::Simplex);
    WarpNoise->SetFrequency(0.05f);
    WarpNoise->SetFractalType(EFastNoise_FractalType::FBM);
    WarpNoise->SetOctaves(2);
}
