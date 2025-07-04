// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/Core/VoxelTypes.h"
#include "SurfaceBlocks.generated.h"

USTRUCT(BlueprintType)
struct FSurfaceBlocks
{
    GENERATED_BODY()

    FSurfaceBlocks()
        : VoxelType()
        , BlocksFromSurface()
        , NumBlocks()
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EVoxelType VoxelType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int BlocksFromSurface; // 0 = Top Block
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int NumBlocks;
};
