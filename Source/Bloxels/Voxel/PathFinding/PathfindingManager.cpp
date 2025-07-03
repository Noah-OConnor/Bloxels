#include "PathfindingManager.h"

#include "NeighborResult.h"
#include "PathfindingNode.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"


bool UPathfindingManager::FindPath(const FIntVector& StartCoord, const FIntVector& EndCoord, TArray<FVector>& OutPathPoints)
{
    OutPathPoints.Empty();

    if (!IsWalkable(StartCoord) || !IsWalkable(EndCoord)) return false;

    TMap<FIntVector, TSharedPtr<FPathfindingNode>> NodeMap;
    TArray<TSharedPtr<FPathfindingNode>> OpenSet;

    TSharedPtr<FPathfindingNode> StartNode = MakeShared<FPathfindingNode>(StartCoord);
    TSharedPtr<FPathfindingNode> EndNode = MakeShared<FPathfindingNode>(EndCoord);

    NodeMap.Add(StartCoord, StartNode);
    OpenSet.Add(StartNode);

    while (!OpenSet.IsEmpty())
    {
        OpenSet.Sort([](const TSharedPtr<FPathfindingNode>& A, const TSharedPtr<FPathfindingNode>& B)
        {
            return A->FCost() < B->FCost();
        });

        TSharedPtr<FPathfindingNode> Current = OpenSet[0];
        OpenSet.RemoveAt(0);

        if (Current->Coord == EndCoord)
        {
            while (Current)
            {
                OutPathPoints.Insert(Current->WorldPosition(), 0);
                Current = Current->Parent.Pin();
            }
            UE_LOG(LogTemp, Warning, TEXT("Path built with %d nodes."), OutPathPoints.Num());
            return true;
        }

        for (const FNeighborResult& Neighbor : GetNeighbors(Current))
        {
            float NewGCost = Current->GCost + Neighbor.MoveCost;

            if (!NodeMap.Contains(Neighbor.Node->Coord) || NewGCost < Neighbor.Node->GCost)
            {
                Neighbor.Node->GCost = NewGCost;
                Neighbor.Node->HCost = FVector::Dist(FVector(Neighbor.Node->Coord), FVector(EndCoord));
                Neighbor.Node->Parent = Current;

                NodeMap.Add(Neighbor.Node->Coord, Neighbor.Node);

                if (!OpenSet.Contains(Neighbor.Node))
                {
                    OpenSet.Add(Neighbor.Node);
                }
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Pathfinding failed."));
    return false;
}

TArray<FNeighborResult> UPathfindingManager::GetNeighbors(TSharedPtr<FPathfindingNode> Node)
{
    const float StraightCost = 10.f;
    const float DiagonalCost = 14.f; // Approx √2
    const float StepUpCost = 5.f;
    const float FallCost = 3.f;

    const TArray<TPair<FIntVector, float>> Directions = {
        {FIntVector( 1,  0, 0), StraightCost},
        {FIntVector(-1,  0, 0), StraightCost},
        {FIntVector( 0,  1, 0), StraightCost},
        {FIntVector( 0, -1, 0), StraightCost},
        {FIntVector( 1,  1, 0), DiagonalCost},
        {FIntVector( 1, -1, 0), DiagonalCost},
        {FIntVector(-1,  1, 0), DiagonalCost},
        {FIntVector(-1, -1, 0), DiagonalCost},
    };

    TArray<FNeighborResult> Neighbors;
    const int MaxStepUp = 1;
    const int MaxFallDistance = 3;

    for (const auto& DirPair : Directions)
    {
        const FIntVector Offset = DirPair.Key;
        float MoveCost = DirPair.Value;

        FIntVector BaseCoord = Node->Coord + Offset;
        
        bool bIsDiagonal = Offset.X != 0 && Offset.Y != 0;
        if (bIsDiagonal)
        {
            FIntVector Side1 = Node->Coord + FIntVector(Offset.X, 0, 0);
            FIntVector Side2 = Node->Coord + FIntVector(0, Offset.Y, 0);

            // If either side is not walkable, skip the diagonal
            if (!IsWalkable(Side1) || !IsWalkable(Side2))
            {
                continue;
            }
        }
        
        // Try flat
        if (auto N = CreateNode(BaseCoord))
        {
            Neighbors.Add(FNeighborResult(N, MoveCost));
            continue;
        }

        // Step up
        for (int Step = 1; Step <= MaxStepUp; ++Step)
        {
            FIntVector UpCoord = BaseCoord + FIntVector(0, 0, Step);
            if (auto N = CreateNode(UpCoord))
            {
                Neighbors.Add(FNeighborResult(N, MoveCost + StepUpCost));
                break;
            }
        }

        // Drop down
        for (int Drop = 1; Drop <= MaxFallDistance; ++Drop)
        {
            FIntVector DownCoord = BaseCoord - FIntVector(0, 0, Drop);
            if (auto N = CreateNode(DownCoord))
            {
                Neighbors.Add(FNeighborResult(N, MoveCost + (FallCost * Drop)));
                break;
            }

            if (IsSolid(DownCoord))
                break;
        }
    }

    return Neighbors;
}

TSharedPtr<FPathfindingNode> UPathfindingManager::CreateNode(const FIntVector& Coord)
{
    if (!IsWalkable(Coord)) return nullptr;
    return MakeShared<FPathfindingNode>(Coord);
}

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
