// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelData.generated.h"

UCLASS(BlueprintType)
class BLOXELS_API UVoxelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName VoxelID;

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