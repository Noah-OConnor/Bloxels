// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BiomeNoiseRanges.generated.h"

USTRUCT(BlueprintType)
struct FBiomeNoiseRanges
{
    GENERATED_BODY()

    FBiomeNoiseRanges()
        : MinTemperature()
        , MaxTemperature()
        , MinHabitability()
        , MaxHabitability()
        , MinElevation()
        , MaxElevation()
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinTemperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxTemperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinHabitability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxHabitability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxElevation;
};
