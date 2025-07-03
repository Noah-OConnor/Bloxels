#pragma once

#include "CoreMinimal.h"
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

protected:
    UPROPERTY()
    UPathfindingManager* PathManager;
};
