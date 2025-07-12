#pragma once

#include "Biome/NoiseInfo.h"
#include "WorldGenerationConfig.generated.h"

UCLASS(BlueprintType)
class BLOXELS_API UWorldGenerationConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // Core terrain structure
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel|World")
    int32 VoxelSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel|World")
    int32 ChunkSize = 16;

    // Biomes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome|Data")
    UDataTable* BiomeDataTable;

    // Biome Noise Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome|Noise Settings")
    FNoiseInfo Temperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome|Noise Settings")
    FNoiseInfo Habitability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Biome|Noise Settings")
    FNoiseInfo Elevation;
};