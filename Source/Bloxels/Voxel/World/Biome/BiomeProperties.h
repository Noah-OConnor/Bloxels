#pragma once

#include "CoreMinimal.h"
#include "Biome.h"
#include "SurfaceBlocks.h"
#include "BiomeNoiseRanges.h"
#include "NoiseInfo.h"
#include "BiomeProperties.generated.h"

USTRUCT(BlueprintType)
struct FBiomeProperties : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBiome BiomeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNoiseInfo> Noise;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FBiomeNoiseRanges> BiomeNoiseRanges;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FSurfaceBlocks> SurfaceBlocks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInterface* Material;
};
