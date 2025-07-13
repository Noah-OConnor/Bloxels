// Copyright 2025 Bloxels. All rights reserved.


#include "PathfindingSubsystem.h"

#include "EngineUtils.h"
#include "PathfindingManager.h"

void UPathfindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create and keep a reference to the PathfindingManager
    PathfindingManager = NewObject<UPathfindingManager>(this);
}

UPathfindingManager* UPathfindingSubsystem::GetPathfindingManager()
{
    if (PathfindingManager && !PathfindingManager->HasVoxelWorld())
    {
        for (TActorIterator<AVoxelWorld> It(GetWorld()); It; ++It)
        {
            PathfindingManager->SetVoxelWorld(*It);
            break;
        }
    }
    return PathfindingManager;
}

