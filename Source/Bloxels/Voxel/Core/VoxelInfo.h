// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelInfo.generated.h"

USTRUCT(BlueprintType)
struct FVoxelInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSolid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material = nullptr;
};