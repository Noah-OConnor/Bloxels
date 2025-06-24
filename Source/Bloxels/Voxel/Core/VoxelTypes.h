// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.generated.h"

UENUM(BlueprintType)
enum class EVoxelType : uint8
{
    Air     UMETA(DisplayName = "Air"),
	Grass	UMETA(DisplayName = "Grass"),
    Dirt    UMETA(DisplayName = "Dirt"),
    Stone   UMETA(DisplayName = "Stone"),
    Sand   UMETA(DisplayName = "Sand"),
    Obsidian UMETA(DisplayName = "Obsidian")
	// More will be added here as needed
};
