// Copyright 2025 Bloxels. All rights reserved.

#include "DebugSubsystem.h"
#include "DrawDebugHelpers.h"

void UDebugSubsystem::SetPosition(int32 Index, const FVector& InPos)
{
    FVector Pos = InPos;

    Pos = FVector(FMath::FloorToInt(Pos.X / 100), FMath::FloorToInt(Pos.Y / 100), FMath::FloorToInt(Pos.Z / 100));

    Pos *= 100.f;
    Pos += FVector(50.f);
    
    UE_LOG(LogTemp, Log, TEXT("Position %d set to: %s"), Index, *Pos.ToString());
    
    switch (Index)
    {
        case 1: Position1 = Pos; bPosition1Set = true; break;
        case 2: Position2 = Pos; bPosition2Set = true; break;
        case 3: Position3 = Pos; bPosition3Set = true; break;
        default: break;
    }
}

void UDebugSubsystem::ClearSelection(int32 Index)
{
    switch (Index)
    {
    case 1: bPosition1Set = false; break;
    case 2: bPosition2Set = false; break;
    case 3: bPosition3Set = false; break;
    default: break; // should probably put a warning that the index is invalid or something
    }
}

void UDebugSubsystem::SetPathCoord(bool bIsStart, const FVector& WorldPos)
{
    FIntVector Coord = FIntVector(
        FMath::FloorToInt(WorldPos.X / 100.f),
        FMath::FloorToInt(WorldPos.Y / 100.f),
        FMath::FloorToInt(WorldPos.Z / 100.f)
    );

    if (bIsStart)
    {
        PathStart = Coord;
        bPathStartSet = true;
        UE_LOG(LogTemp, Log, TEXT("Path start set to %s"), *Coord.ToString());
    }
    else
    {
        PathEnd = Coord;
        bPathEndSet = true;
        UE_LOG(LogTemp, Log, TEXT("Path end set to %s"), *Coord.ToString());
    }
}

void UDebugSubsystem::SetDebugPath(const TArray<FVector>& PathPoints)
{
    DebugPath = PathPoints;
    bHasPath = DebugPath.Num() > 1;
}

void UDebugSubsystem::ClearPathCoord(bool bIsStart)
{
    if (bIsStart)
        bPathStartSet = false;
    else
        bPathEndSet = false;
}

void UDebugSubsystem::ClearDebugPath()
{
    DebugPath.Reset();
    bHasPath = false;
}

void UDebugSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const float BoxSize = 50.f;
    const float Thickness = 3.f;

    // Selection Debug
    if (bPosition1Set)
        DrawDebugBox(World, Position1, FVector(BoxSize), FColor::Red, false, 0.f, 0, Thickness);

    if (bPosition2Set)
        DrawDebugBox(World, Position2, FVector(BoxSize), FColor::Green, false, 0.f, 0, Thickness);

    if (bPosition3Set)
        DrawDebugBox(World, Position3, FVector(BoxSize), FColor::Blue, false, 0.f, 0, Thickness);

    if (bPosition1Set && bPosition2Set)
    {
        FVector Min = Position1.ComponentMin(Position2);
        FVector Max = Position1.ComponentMax(Position2);
        FVector Center = (Min + Max) * 0.5f;
        FVector Extent = (Max - Min) * 0.5f + 50.f;

        DrawDebugBox(World, Center, Extent, FColor::Yellow, false, 0.f, 0, Thickness);
    }

    // Pathfinding Debug
    if (bPathStartSet)
    {
        FVector Pos = FVector(PathStart) * 100.f + FVector(50.f);
        DrawDebugBox(World, Pos, FVector(BoxSize), FColor::Red, false, 0.f, 0, Thickness);
    }

    if (bPathEndSet)
    {
        FVector Pos = FVector(PathEnd) * 100.f + FVector(50.f);
        DrawDebugBox(World, Pos, FVector(BoxSize), FColor::Green, false, 0.f, 0, Thickness);
    }

    if (bHasPath && DebugPath.Num() >= 2)
    {
        for (int32 i = 1; i < DebugPath.Num(); ++i)
        {
            DrawDebugLine(World, DebugPath[i - 1] + FVector(50.f), DebugPath[i] + FVector(50.f), FColor::Cyan, false, 0.f, 0, 4.f);
        }
    }
}
