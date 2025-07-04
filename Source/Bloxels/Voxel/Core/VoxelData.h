// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"
#include "VoxelData.generated.h"

USTRUCT(BlueprintType)
struct FVoxelData
{
	GENERATED_BODY()

	FVoxelData()
		: VoxelType()
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVoxelType VoxelType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSolid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TopTileOffset = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint BottomTileOffset = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint SideTileOffset = FIntPoint(0, 0);
};