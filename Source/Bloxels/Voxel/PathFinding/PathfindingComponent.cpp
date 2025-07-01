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
    TQueue<TSharedPtr<FPathfindingNode>> OpenSet;

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
    OpenSet.Enqueue(StartNode);

    while (!OpenSet.IsEmpty())
    {
        TSharedPtr<FPathfindingNode> Current;
        OpenSet.Dequeue(Current);

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

        for (const FNeighborResult& Neighbor : GetNeighbors(Current))        {
            float NewGCost = Current->GCost + Neighbor.MoveCost;
            if (!NodeMap.Contains(Neighbor.Node->Coord) || NewGCost < Neighbor.Node->GCost)
            {
                Neighbor.Node->GCost = NewGCost;
                Neighbor.Node->HCost = FVector::Dist(
                    FVector(Neighbor.Node->Coord),
                    FVector(EndCoord)
                );
                Neighbor.Node->Parent = Current;

                NodeMap.Add(Neighbor.Node->Coord, Neighbor.Node);
                OpenSet.Enqueue(Neighbor.Node);
            }
        }
    }

}

void UPathfindingComponent::DrawDebugPath(float Duration) const
{
    for (int i = 0; i < Path.Num() - 1; ++i)
    {
        DrawDebugLine(GetWorld(), Path[i] + FVector(50), Path[i + 1] + FVector(50), FColor::Green, false, Duration, 0, 3.f);
    }
}

TSharedPtr<FPathfindingNode> UPathfindingComponent::CreateNode(const FIntVector& Coord)
{
    if (!PathManager || !PathManager->IsWalkable(Coord)) return nullptr;
    return MakeShared<FPathfindingNode>(Coord);
}

TArray<FNeighborResult> UPathfindingComponent::GetNeighbors(TSharedPtr<FPathfindingNode> Node)
{
    const float StraightCost = 1.0f;
    const float DiagonalCost = 1.41f; // Approx √2

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
        const float MoveCost = DirPair.Value;

        FIntVector BaseCoord = Node->Coord + Offset;

        // 1. Try flat
        if (auto N = CreateNode(BaseCoord))
        {
            Neighbors.Add(FNeighborResult(N, MoveCost));
            continue;
        }

        // 2. Step up
        for (int Step = 1; Step <= MaxStepUp; ++Step)
        {
            FIntVector UpCoord = BaseCoord + FIntVector(0, 0, Step);
            if (auto N = CreateNode(UpCoord))
            {
                Neighbors.Add(FNeighborResult(N, MoveCost));
                break;
            }
        }

        // 3. Drop down
        for (int Drop = 1; Drop <= MaxFallDistance; ++Drop)
        {
            FIntVector DownCoord = BaseCoord - FIntVector(0, 0, Drop);
            if (auto N = CreateNode(DownCoord))
            {
                Neighbors.Add(FNeighborResult(N, MoveCost));
                break;
            }

            if (PathManager->IsSolid(DownCoord))
                break;
        }
    }

    return Neighbors;
}
