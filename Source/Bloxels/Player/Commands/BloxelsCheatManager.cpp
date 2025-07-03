// Copyright 2025 Noah O'Connor. All rights reserved.


#include "Bloxels/Player/Commands/BloxelsCheatManager.h"

#include "DebugSubsystem.h"
#include "Bloxels/Player/FreeCamera/FreeCameraPawn.h"
#include "Bloxels/Voxel/PathFinding/PathfindingSubsystem.h"

void UBloxelsCheatManager::SelectPositionCoordinates(int32 Index, float X, float Y, float Z)
{
    SetPosition(Index, FVector(X, Y, Z));
}

void UBloxelsCheatManager::SelectPositionLookAt(int32 Index, bool bOffset)
{      
    DrawDebugPoint(GetWorld(), GetLookAt(bOffset), 10.f, FColor::Green, false, 2.f);
    SetPosition(Index, GetLookAt(bOffset));
}

void UBloxelsCheatManager::SelectPositionHead(int32 Index)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (!PC) return;

    if (const AFreeCameraPawn* CameraPawn = Cast<AFreeCameraPawn>(PC->GetPawn()))
    {
        FVector Pos = CameraPawn->GetCamera()->GetComponentLocation();
        SetPosition(Index, Pos);
    }
}

void UBloxelsCheatManager::ClearSelection(int32 Index)
{
    if (UDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        SelectionDebug->ClearSelection(Index);
    }
}

void UBloxelsCheatManager::ClearAllSelections()
{
    if (UDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        SelectionDebug->ClearSelection(1);
        SelectionDebug->ClearSelection(2);
        SelectionDebug->ClearSelection(3);
    }
}

void UBloxelsCheatManager::SetPathStartLookAt(bool bOffset)
{
    if (UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        Debug->SetPathCoord(true, GetLookAt(true));
        GeneratePathDebug(Debug);
    }
}

void UBloxelsCheatManager::SetPathEndLookAt(bool bOffset)
{
    if (UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        Debug->SetPathCoord(false, GetLookAt(true));
        GeneratePathDebug(Debug);
    }
}

void UBloxelsCheatManager::ClearPathStart()
{
    if (UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        Debug->ClearPathCoord(true);
    }
}

void UBloxelsCheatManager::ClearPathEnd()
{
    if (UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        Debug->ClearPathCoord(false);
    }
}


void UBloxelsCheatManager::SetPosition(int32 Index, const FVector& Pos)
{
    if (UDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>())
    {
        SelectionDebug->SetPosition(Index, Pos);
    }
}

FVector UBloxelsCheatManager::GetLookAt(bool bReturnNormal)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("NO PLAYER CONTROLLER???!!!"));
        return FVector::Zero();
    }
    
    if (const AFreeCameraPawn* CameraPawn = Cast<AFreeCameraPawn>(PC->GetPawn()))
    {
        const FVector Start = CameraPawn->GetCamera()->GetComponentLocation();
        const FVector Dir = CameraPawn->GetCamera()->GetForwardVector();
        const FVector End = Start + Dir * 10000.f;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(PC->GetPawn());

        if (PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            FVector Pos = Hit.ImpactPoint;
            if (bReturnNormal)
            {
                Pos += Hit.ImpactNormal * 50.f; // adjust based on voxel size if needed
            }
            else
            {
                Pos -= Hit.ImpactNormal * 50.f; // adjust based on voxel size if needed
            }
            
            return Pos;
        }
    }
    
    return FVector::Zero();
}

void UBloxelsCheatManager::GeneratePathDebug(UDebugSubsystem* Debug)
{
    TArray<FVector> Path;
    if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UPathfindingSubsystem>())
    {
        if (PathfindingSubsystem->GetPathfindingManager()->FindPath(Debug->PathStart, Debug->PathEnd, Path))
        {
            Debug->SetDebugPath(Path);
        }
        else
        {
            Debug->ClearDebugPath();
        }
    }
}
