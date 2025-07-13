// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"

struct FPathfindingNode
{
	TWeakPtr<FPathfindingNode> Parent;
	
	FIntVector Coord;
	float GCost = 0.0f; // Cost of this path so far
	float HCost = 0.0f; // Guess at remaining cost / heuristic
	float FCost() const { return GCost + HCost; }

	FPathfindingNode(FIntVector InCoord)
		: Coord(InCoord)
	{}

	FVector WorldPosition() const
	{
		return FVector(Coord) * 100.f; // Assuming 100 units = 1 block
	}
};
