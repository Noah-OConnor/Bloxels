#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "PathfindingManager.generated.h"

UCLASS()
class BLOXELS_API UPathfindingManager : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Pathfinding")
	AVoxelWorld* VoxelWorld;
	
	bool IsWalkable(const FIntVector& Coord) const;
	bool IsSolid(const FIntVector& Coord) const;
	bool IsAir(const FIntVector& Coord) const;
	void SetVoxelWorld(AVoxelWorld* InWorld);
	bool HasVoxelWorld() const { return VoxelWorld != nullptr; }

};
