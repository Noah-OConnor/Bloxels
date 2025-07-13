// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelData.generated.h"

UCLASS(BlueprintType)
class BLOXELS_API UVoxelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "The internal name of this block. Written using PascalCase. Example: StoneBrick"))
	FName VoxelID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Whether this block is solid. Used for physics and collisions."))
	bool bIsSolid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Whether this block is Transparent. Should block faces behind it be rendered?"))
	bool bIsTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Whether this block is Invisible. Should this block be rendered at all?"))
	bool bIsInvisible = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TopTileOffset = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint BottomTileOffset = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint SideTileOffset = FIntPoint(0, 0);
};