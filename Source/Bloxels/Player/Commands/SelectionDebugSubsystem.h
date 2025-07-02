#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "SelectionDebugSubsystem.generated.h"

UCLASS()
class BLOXELS_API USelectionDebugSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    FVector Position1;
    FVector Position2;
    FVector Position3;

    bool bPosition1Set = false;
    bool bPosition2Set = false;
    bool bPosition3Set = false;

    void SetPosition(int32 Index, const FVector& InPos);
    void ClearSelection(int32 Index);

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USelectionVisualizerSubsystem, STATGROUP_Tickables); }
};
