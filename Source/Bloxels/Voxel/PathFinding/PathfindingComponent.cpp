#include "PathfindingComponent.h"
#include "DrawDebugHelpers.h"
#include "NeighborResult.h"
#include "Containers/Queue.h"

UPathfindingComponent::UPathfindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPathfindingComponent::Initialize(UPathfindingManager* InManager)
{
    PathManager = InManager;
}

void UPathfindingComponent::FindPath(const FIntVector& StartCoord, const FIntVector& EndCoord)
{
    Path.Empty();

    TMap<FIntVector, TSharedPtr<FPathfindingNode>> NodeMap;
    TArray<TSharedPtr<FPathfindingNode>> OpenSet;

    auto StartNode = CreateNode(StartCoord);
    auto EndNode = CreateNode(EndCoord);

    if (!StartNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartNode is null at %s"), *StartCoord.ToString());
    }
    if (!EndNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("EndNode is null at %s"), *EndCoord.ToString());
    }
    
    if (!StartNode || !EndNode) return;

    NodeMap.Add(StartCoord, StartNode);
    OpenSet.Add(StartNode);

    while (!OpenSet.IsEmpty())
    {
        // Sort by FCost before each iteration (O(n log n), but fine for small sets)
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
                Path.Insert(Current->WorldPosition(), 0);
                Current = Current->Parent.Pin();
            }
            UE_LOG(LogTemp, Warning, TEXT("Path built with %d nodes."), Path.Num());
            return;
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
}

void UPathfindingComponent::DrawDebugPath(float Duration) const
{
    for (int i = 0; i < Path.Num() - 1; ++i)
    {
        DrawDebugLine(GetWorld(), Path[i] + FVector(50), Path[i + 1] + FVector(50), FColor::Red, false, Duration, 0, 5.f);
    }
}

TSharedPtr<FPathfindingNode> UPathfindingComponent::CreateNode(const FIntVector& Coord)
{
    if (!PathManager || !PathManager->IsWalkable(Coord)) return nullptr;
    return MakeShared<FPathfindingNode>(Coord);
}

TArray<FNeighborResult> UPathfindingComponent::GetNeighbors(TSharedPtr<FPathfindingNode> Node)
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
            if (!PathManager->IsWalkable(Side1) || !PathManager->IsWalkable(Side2))
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

            if (PathManager->IsSolid(DownCoord))
                break;
        }
    }

    return Neighbors;
}
