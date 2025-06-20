// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.generated.h"

UENUM(BlueprintType)
enum class EVoxelType : uint8
{
    Air     UMETA(DisplayName = "Air"),
    Dirt    UMETA(DisplayName = "Dirt"),
    Stone   UMETA(DisplayName = "Stone"),
    Obsidian UMETA(DisplayName = "Obsidian")
	// More will be added here as needed
};
