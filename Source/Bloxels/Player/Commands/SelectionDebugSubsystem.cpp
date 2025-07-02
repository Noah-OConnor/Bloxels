#include "SelectionDebugSubsystem.h"
#include "DrawDebugHelpers.h"

void USelectionDebugSubsystem::SetPosition(int32 Index, const FVector& InPos)
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

void USelectionDebugSubsystem::ClearSelection(int32 Index)
{
    switch (Index)
    {
    case 1: bPosition1Set = false; break;
    case 2: bPosition2Set = false; break;
    case 3: bPosition3Set = false; break;
    default: break; // should probably put a warning that the index is invalid or something
    }
}

void USelectionDebugSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const float BoxSize = 50.f;
    const float Thickness = 3.f;

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
}
