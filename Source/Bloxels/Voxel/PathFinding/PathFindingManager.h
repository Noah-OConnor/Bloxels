#pragma once

#include "CoreMinimal.h"
#include "PathFindingNode.h"
#include "GameFramework/Actor.h"
#include "PathFindingManager.generated.h"

class AVoxelWorld;

UCLASS()
class BLOXELS_API APathfindingManager : public AActor
{
	GENERATED_BODY()

public:
	void BuildPathfindingNodes(AVoxelWorld* VoxelWorld);
	TSharedPtr<FPathFindingNode> GetNodeAt(FIntVector WorldCoord);
	TArray<TSharedPtr<FPathFindingNode>> GetNeighbors(const TSharedPtr<FPathFindingNode>& Node);
	
	TArray<FVector> FindPath(FVector WorldStart, FVector WorldEnd);

private:
	TMap<FIntVector, TSharedPtr<FPathFindingNode>> NodeMap;
};
