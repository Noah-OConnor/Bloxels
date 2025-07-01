#pragma once

#include "CoreMinimal.h"
#include "NeighborResult.h"
#include "PathfindingNode.h"
#include "PathfindingManager.h"
#include "Components/ActorComponent.h"
#include "PathfindingComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLOXELS_API UPathfindingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPathfindingComponent();

    void Initialize(UPathfindingManager* InManager);

    void FindPath(const FIntVector& StartCoord, const FIntVector& EndCoord);
    void DrawDebugPath(float Duration = 5.0f) const;

protected:
    TArray<FVector> Path;
    UPROPERTY()
    UPathfindingManager* PathManager;

    TSharedPtr<FPathfindingNode> CreateNode(const FIntVector& Coord);
    TArray<FNeighborResult> GetNeighbors(TSharedPtr<FPathfindingNode> Node);
};
