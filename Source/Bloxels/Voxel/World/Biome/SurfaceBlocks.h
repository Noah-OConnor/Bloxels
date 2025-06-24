// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/Core/VoxelTypes.h"
#include "SurfaceBlocks.generated.h"

USTRUCT(BlueprintType)
struct FSurfaceBlocks
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EVoxelType VoxelType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int blocksFromSurface; // 0 = Top Block
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int numBlocks;
};
