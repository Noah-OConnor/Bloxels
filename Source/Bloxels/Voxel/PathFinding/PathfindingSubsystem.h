#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PathfindingManager.h"
#include "PathfindingSubsystem.generated.h"

UCLASS()
class BLOXELS_API UPathfindingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    UPathfindingManager* GetPathfindingManager();

private:
    UPROPERTY()
    UPathfindingManager* PathfindingManager;
};
