#include "PathfindingComponent.h"
#include "DrawDebugHelpers.h"
#include "Containers/Queue.h"

UPathfindingComponent::UPathfindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPathfindingComponent::Initialize(UPathfindingManager* InManager)
{
    PathManager = InManager;
}