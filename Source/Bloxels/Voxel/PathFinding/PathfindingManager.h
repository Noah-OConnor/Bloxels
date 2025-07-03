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

	UFUNCTION()
	bool FindPath(const FIntVector& StartCoord, const FIntVector& EndCoord, TArray<FVector>& OutPathPoints);
	
	void SetVoxelWorld(AVoxelWorld* InWorld);
	bool HasVoxelWorld() const { return VoxelWorld != nullptr; }
	
private:
	UPROPERTY()
	AVoxelWorld* VoxelWorld;
	
	TArray<FNeighborResult> GetNeighbors(TSharedPtr<FPathfindingNode> Node);
	TSharedPtr<FPathfindingNode> CreateNode(const FIntVector& Coord);

	bool IsWalkable(const FIntVector& Coord) const;
	bool IsSolid(const FIntVector& Coord) const;
	bool IsAir(const FIntVector& Coord) const;
	bool IsDiagonalAllowed(const FIntVector& Coord, const FIntVector& Offset, int MaxFallDistance) const;
	void TryStepUp(TArray<FNeighborResult>& OutNeighbors, const FIntVector& Coord, float MoveCost, float StepCost, int MaxStep);
	void TryFallDown(TArray<FNeighborResult>& OutNeighbors,const FIntVector& NodeCoord,const FIntVector& Offset,float MoveCost,float FallCost,int MaxFall);
};
