// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "DebugSubsystem.generated.h"

UCLASS()
class BLOXELS_API UDebugSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // Area Selection
    FVector Position1;
    FVector Position2;
    FVector Position3;

    bool bPosition1Set = false;
    bool bPosition2Set = false;
    bool bPosition3Set = false;

    void SetPosition(int32 Index, const FVector& InPos);
    void ClearSelection(int32 Index);
    
    // Pathfinding Selection
    FIntVector PathStart;
    FIntVector PathEnd;
    TArray<FVector> DebugPath;
    
    bool bPathStartSet = false;
    bool bPathEndSet = false;
    bool bHasPath = false;

    void SetPathCoord(bool bIsStart, const FVector& WorldPos);
    void SetDebugPath(const TArray<FVector>& PathPoints);
    void ClearPathCoord(bool bIsStart);
    void ClearDebugPath();

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USelectionVisualizerSubsystem, STATGROUP_Tickables); }
};
