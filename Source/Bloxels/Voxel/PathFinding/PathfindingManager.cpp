#include "PathfindingManager.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"


bool UPathfindingManager::IsWalkable(const FIntVector& Coord) const
{
    FIntVector Below = Coord - FIntVector(0, 0, 1);
    return IsAir(Coord) && IsAir(Coord + FIntVector(0, 0, 1)) && IsSolid(Below);
}

bool UPathfindingManager::IsAir(const FIntVector& Coord) const
{
    if (!VoxelWorld) return false;
    
    uint16 Voxel = VoxelWorld->GetVoxelAtWorldCoordinates(Coord.X, Coord.Y, Coord.Z);
    return VoxelWorld->VoxelProperties[Voxel].VoxelType == EVoxelType::Air;
}

void UPathfindingManager::SetVoxelWorld(AVoxelWorld* InWorld)
{
    VoxelWorld = InWorld;
}

bool UPathfindingManager::IsSolid(const FIntVector& Coord) const
{
    if (!VoxelWorld) return false;
    
    uint16 Voxel = VoxelWorld->GetVoxelAtWorldCoordinates(Coord.X, Coord.Y, Coord.Z);
    return VoxelWorld->VoxelProperties[Voxel].bIsSolid;
}
