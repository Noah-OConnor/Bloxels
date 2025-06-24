// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelData.h"
#include "VoxelInfo.generated.h"

UCLASS(BlueprintType)
class UVoxelInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TArray<FVoxelData> VoxelData;
};