// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SurfaceBlocks.generated.h"

USTRUCT(BlueprintType)
struct FSurfaceBlocks
{
    GENERATED_BODY()

    FSurfaceBlocks()
        : BlocksFromSurface()
        , NumBlocks()
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName VoxelID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int BlocksFromSurface; // 0 = Top Block
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int NumBlocks;
};
