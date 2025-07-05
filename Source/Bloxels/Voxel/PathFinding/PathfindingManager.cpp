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
    const float DiagonalCost = 14.f;
    const float StepUpCost = 5.f;
    const float FallCost = 3.f;
    const int MaxStepUp = 1;
    const int MaxFallDistance = 3;

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

    for (const auto& DirPair : Directions)
    {
        const FIntVector Offset = DirPair.Key;
        float MoveCost = DirPair.Value;
        FIntVector BaseCoord = Node->Coord + Offset;

        if (Offset.X != 0 && Offset.Y != 0 && !IsDiagonalAllowed(Node->Coord, Offset, MaxFallDistance))
        {
            continue;
        }

        // Try same level
        if (auto N = CreateNode(BaseCoord))
        {
            Neighbors.Add(FNeighborResult(N, MoveCost));
            continue;
        }

        // Step up
        TryStepUp(Neighbors, BaseCoord, MoveCost, StepUpCost, MaxStepUp);

        // Drop down
        TryFallDown(Neighbors, Node->Coord, Offset, MoveCost, FallCost, MaxFallDistance);
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
    return VoxelWorld->GetVoxelRegistry()->GetVoxelByID(Voxel)->VoxelID == FName("Air");
}

void UPathfindingManager::SetVoxelWorld(AVoxelWorld* InWorld)
{
    VoxelWorld = InWorld;
}

bool UPathfindingManager::IsDiagonalAllowed(const FIntVector& Coord, const FIntVector& Offset, int MaxFallDistance) const
{
    FIntVector Side1 = Coord + FIntVector(Offset.X, 0, 0);
    FIntVector Side2 = Coord + FIntVector(0, Offset.Y, 0);
    FIntVector UpOffset(0, 0, 1);

    if (IsAir(Side1) && IsAir(Side1 + UpOffset) && IsAir(Side2) && IsAir(Side2 + UpOffset))
    {
        FIntVector Diagonal = Coord + Offset;

        for (int Drop = 1; Drop <= MaxFallDistance; ++Drop)
        {
            FIntVector DropCoord = Diagonal - FIntVector(0, 0, Drop);
            if (IsWalkable(DropCoord)) return true;
            if (IsSolid(DropCoord)) break;
        }
    }

    // If not dropping, enforce no corner cutting
    return IsWalkable(Side1) && IsWalkable(Side2);
}


void UPathfindingManager::TryStepUp(
    TArray<FNeighborResult>& OutNeighbors,
    const FIntVector& Coord,
    float MoveCost,
    float StepCost,
    int MaxStep
)
{
    for (int Step = 1; Step <= MaxStep; ++Step)
    {
        FIntVector StepCoord = Coord + FIntVector(0, 0, Step);
        if (auto N = CreateNode(StepCoord))
        {
            OutNeighbors.Add(FNeighborResult(N, MoveCost + StepCost));
            break;
        }
    }
}


void UPathfindingManager::TryFallDown(
    TArray<FNeighborResult>& OutNeighbors,
    const FIntVector& NodeCoord,
    const FIntVector& Offset,
    float MoveCost,
    float FallCost,
    int MaxFall
)
{
    FIntVector Side1 = NodeCoord + FIntVector(Offset.X, 0, 0);
    FIntVector Side2 = NodeCoord + FIntVector(0, Offset.Y, 0);
    FIntVector Up(0, 0, 1);

    if (!(IsAir(Side1) && IsAir(Side1 + Up) && IsAir(Side2) && IsAir(Side2 + Up)))
        return;

    FIntVector Start = NodeCoord + Offset;

    for (int Drop = 1; Drop <= MaxFall; ++Drop)
    {
        FIntVector FallCoord = Start - FIntVector(0, 0, Drop);
        if (auto N = CreateNode(FallCoord))
        {
            OutNeighbors.Add(FNeighborResult(N, MoveCost + FallCost * Drop));
            break;
        }
        if (IsSolid(FallCoord))
            break;
    }
}


bool UPathfindingManager::IsSolid(const FIntVector& Coord) const
{
    if (!VoxelWorld) return false;
    
    uint16 Voxel = VoxelWorld->GetVoxelAtWorldCoordinates(Coord.X, Coord.Y, Coord.Z);
    return VoxelWorld->GetVoxelRegistry()->GetVoxelByID(Voxel)->bIsSolid;
}
