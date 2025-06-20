// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelConfig.generated.h"

UCLASS(BlueprintType, Blueprintable)
class BLOXELS_API UVoxelConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Settings")
	int32 ChunkSize = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Settings")
	int32 ChunkHeight = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Settings")
	float VoxelSize = 100.0f; // 100 = 1m blocks
};