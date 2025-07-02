// Copyright 2025 Noah O'Connor. All rights reserved.


#include "Bloxels/Player/Commands/BloxelsCheatManager.h"

#include "SelectionDebugSubsystem.h"
#include "Bloxels/Player/FreeCamera/FreeCameraPawn.h"

void UBloxelsCheatManager::SelectPositionCoordinates(int32 Index, float X, float Y, float Z)
{
    SetPosition(Index, FVector(X, Y, Z));
}

void UBloxelsCheatManager::SelectPositionLookAt(int32 Index, bool bOffset)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (!PC) return;

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
            if (bOffset)
            {
                Pos += Hit.ImpactNormal * 50.f; // adjust based on voxel size if needed
            }
            else
            {
                Pos -= Hit.ImpactNormal * 50.f; // adjust based on voxel size if needed
            }

            
            
            DrawDebugPoint(PC->GetWorld(), Pos, 10.f, FColor::Green, false, 2.f);
            SetPosition(Index, Pos);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No valid voxel hit detected."));
        }
    }
}

void UBloxelsCheatManager::SelectPositionHead(int32 Index)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (!PC) return;

    if (const AFreeCameraPawn* CameraPawn = Cast<AFreeCameraPawn>(PC->GetPawn()))
    {
        FVector Pos = CameraPawn->GetCamera()->GetComponentLocation();
        SetPosition(Index, Pos);

        // Index of 3 must be within the bounds of 1 and 2 to work
    }
}

void UBloxelsCheatManager::ClearSelection(int32 Index)
{
    if (USelectionDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<USelectionDebugSubsystem>())
    {
        SelectionDebug->ClearSelection(Index);
    }
}

void UBloxelsCheatManager::ClearAllSelections()
{
    if (USelectionDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<USelectionDebugSubsystem>())
    {
        SelectionDebug->ClearSelection(1);
        SelectionDebug->ClearSelection(2);
        SelectionDebug->ClearSelection(3);
    }
}

void UBloxelsCheatManager::SetPosition(int32 Index, const FVector& Pos)
{
    if (USelectionDebugSubsystem* SelectionDebug = GetWorld()->GetGameInstance()->GetSubsystem<USelectionDebugSubsystem>())
    {
        SelectionDebug->SetPosition(Index, Pos);
    }
}
