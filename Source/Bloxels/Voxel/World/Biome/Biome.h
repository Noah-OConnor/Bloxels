// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Biome.generated.h"

UENUM(BlueprintType)
enum class EBiome : uint8 {
    None,
    Ocean,
    Coast,
    Tundra,
    Plains,
    Desert,
    Jungle,
    RockyMountain
};
