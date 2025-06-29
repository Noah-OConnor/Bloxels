// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"

struct FPathFindingNode
{
	FVector Position;
	int32 X, Y, Z;
	float GCost = 0.0f;
	float HCost = 0.0f;
	float FCost() const { return GCost + HCost; }

	bool bIsWalkable = true;

	TWeakPtr<FPathFindingNode> Parent;
};
