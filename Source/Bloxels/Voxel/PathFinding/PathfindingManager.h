#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "PathfindingManager.generated.h"

struct FPathfindingNode;
struct FNeighborResult;

UCLASS()
class BLOXELS_API UPathfindingManager : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Pathfinding")
	AVoxelWorld* VoxelWorld;

	UFUNCTION()
	bool FindPath(const FIntVector& StartCoord, const FIntVector& EndCoord, TArray<FVector>& OutPathPoints);
	TArray<FNeighborResult> GetNeighbors(TSharedPtr<FPathfindingNode> Node);
	TSharedPtr<FPathfindingNode> CreateNode(const FIntVector& Coord);


	bool IsWalkable(const FIntVector& Coord) const;
	bool IsSolid(const FIntVector& Coord) const;
	bool IsAir(const FIntVector& Coord) const;
	void SetVoxelWorld(AVoxelWorld* InWorld);
	bool HasVoxelWorld() const { return VoxelWorld != nullptr; }
};
