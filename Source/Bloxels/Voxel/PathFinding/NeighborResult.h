#pragma once

struct FPathfindingNode;

struct FNeighborResult
{
    TSharedPtr<FPathfindingNode> Node;
    float MoveCost;

    FNeighborResult(TSharedPtr<FPathfindingNode> InNode, float InCost)
        : Node(InNode), MoveCost(InCost) {}
};
