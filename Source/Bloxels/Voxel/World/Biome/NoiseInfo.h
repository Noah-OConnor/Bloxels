// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastNoiseWrapper.h"
#include "NoiseInfo.generated.h"

USTRUCT(BlueprintType)
struct FNoiseInfo
{
	GENERATED_BODY()

	// Noise Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	bool UseThisNoise = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	EFastNoise_NoiseType NoiseType = EFastNoise_NoiseType::SimplexFractal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	EFastNoise_FractalType NoiseFractalType = EFastNoise_FractalType::FBM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	UCurveFloat* NoiseCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	float NoiseOctaves = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	float NoiseFrequency = 0.0008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Noise Settings")
	int NoiseSeed = 0;
};