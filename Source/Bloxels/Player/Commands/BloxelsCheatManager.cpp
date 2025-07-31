// Copyright 2025 Bloxels. All rights reserved.

#include "Bloxels/Player/Commands/BloxelsCheatManager.h"

#include "DebugSubsystem.h"
#include "Bloxels/Player/FreeCamera/FreeCameraPawn.h"
#include "Bloxels/Voxel/PathFinding/PathfindingSubsystem.h"
#include "Bloxels/Voxel/World/WorldGenerationConfig.h"
#include "Kismet/GameplayStatics.h"

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

void UBloxelsCheatManager::ExportSelection(const FString& FileName)
{
    UE_LOG(LogTemp, Log, TEXT("ExportSelection: Exporting to %s.blxl"), *FileName);
    
    UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>();
    if (!Debug) return;

    FVector Pos1 = Debug->Position1;
    FVector Pos2 = Debug->Position2;
    FVector Origin = Debug->Position3;

    const FVector MinVec = FVector::Min(Pos1, Pos2).GridSnap(100);
    const FVector MaxVec = FVector::Max(Pos1, Pos2).GridSnap(100);
    const FVector OriginVec = Origin.GridSnap(100);

    const FIntVector Min(
        FMath::RoundToInt(MinVec.X),
        FMath::RoundToInt(MinVec.Y),
        FMath::RoundToInt(MinVec.Z));

    const FIntVector Max(
        FMath::RoundToInt(MaxVec.X),
        FMath::RoundToInt(MaxVec.Y),
        FMath::RoundToInt(MaxVec.Z));

    const FIntVector OriginSnapped(
        FMath::RoundToInt(OriginVec.X),
        FMath::RoundToInt(OriginVec.Y),
        FMath::RoundToInt(OriginVec.Z));

    AVoxelWorld* World = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
    if (!World) return;

    FString Output = TEXT("{ \"Voxels\": [\n");
    int32 ExportedCount = 0;
    
    for (int32 X = Min.X; X <= Max.X; X += 100)
        for (int32 Y = Min.Y; Y <= Max.Y; Y += 100)
            for (int32 Z = Min.Z; Z <= Max.Z; Z += 100)
            {
                int VoxelID = World->GetVoxelAtWorldCoordinates((X - 100) / 100, (Y - 100) / 100, (Z - 100) / 100);

                if (VoxelID == World->GetVoxelRegistry()->GetIDFromName("Air")) continue; // Skip air

                FName Name = World->GetVoxelRegistry()->GetNameFromID(VoxelID);
                FIntVector Offset = FIntVector(X, Y, Z) - OriginSnapped;

                Output += FString::Printf(TEXT("{\"Offset\":[%d,%d,%d],\"ID\":\"%s\"},\n"),
                    Offset.X / 100, Offset.Y / 100, Offset.Z / 100, *Name.ToString());

                ExportedCount++;
            }

    Output.RemoveFromEnd(TEXT(",\n")); // remove trailing comma
    Output += TEXT("\n]}\n");

    FFileHelper::SaveStringToFile(Output, *(FPaths::ProjectSavedDir() + FileName + ".blxl"));

    FFileHelper::SaveStringToFile(Output, *(FPaths::ProjectSavedDir() + FileName + ".blxl"));
    UE_LOG(LogTemp, Log, TEXT("ExportSelection complete: %d voxels exported."), ExportedCount);
}


void UBloxelsCheatManager::ImportStructure(const FString& FileName)
{
    FString Input;
    const FString FullPath = FPaths::ProjectSavedDir() + FileName + ".blxl";
    UE_LOG(LogTemp, Log, TEXT("ImportStructure: Loading from %s"), *FullPath);
    
    if (!FFileHelper::LoadFileToString(Input, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportStructure: Failed to load file to string"));
        return;
    }
    
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Input);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("ImportStructure: Failed to deserialize JSON structure."));
        return;
    }

    UDebugSubsystem* Debug = GetWorld()->GetGameInstance()->GetSubsystem<UDebugSubsystem>();
    if (!Debug) return;

    AVoxelWorld* World = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
    if (!World) return;

    FVector SnappedOrigin = Debug->Position3.GridSnap(100) / 100.0f;
    FIntVector Origin = FIntVector(FMath::RoundToInt(SnappedOrigin.X), FMath::RoundToInt(SnappedOrigin.Y), FMath::RoundToInt(SnappedOrigin.Z));


    const TArray<TSharedPtr<FJsonValue>> Voxels = Root->GetArrayField(TEXT("Voxels"));
    UE_LOG(LogTemp, Log, TEXT("ImportStructure: Found %d voxel entries"), Voxels.Num());

    TSet<FIntVector> AffectedChunks;
    
    int32 ImportedCount = 0;

    for (const TSharedPtr<FJsonValue>& VoxelVal : Voxels)
    {
        TSharedPtr<FJsonObject> VoxelObj = VoxelVal->AsObject();
        const TArray<TSharedPtr<FJsonValue>>& OffsetArr = VoxelObj->GetArrayField(TEXT("Offset"));

        if (OffsetArr.Num() != 3)
        {
            UE_LOG(LogTemp, Warning, TEXT("ImportStructure: Skipping malformed Offset array."));
            continue;
        }

        FIntVector Offset(
            OffsetArr[0]->AsNumber(),
            OffsetArr[1]->AsNumber(),
            OffsetArr[2]->AsNumber()
        );

        FIntVector WorldVoxelPos = Origin + Offset;
        FName IDName = FName(VoxelObj->GetStringField(TEXT("ID")));

        if (IDName == TEXT("AirForced"))
        {
            IDName = TEXT("Air");
        }
        
        uint16 BlockID = World->GetVoxelRegistry()->GetIDFromName(IDName);

        UE_LOG(LogTemp, Verbose, TEXT("Importing voxel: Offset=(%d,%d,%d) -> World=(%d,%d,%d), ID=%s"),
            Offset.X, Offset.Y, Offset.Z,
            WorldVoxelPos.X, WorldVoxelPos.Y, WorldVoxelPos.Z,
            *IDName.ToString());

        
        World->PlaceBlock(WorldVoxelPos.X - 1, WorldVoxelPos.Y - 1, WorldVoxelPos.Z - 1, BlockID);

        const int ChunkSize = World->GetWorldGenerationConfig()->ChunkSize;
        
        // Track affected chunk
        FIntVector ChunkCoord(
            FMath::FloorToInt(static_cast<float>(WorldVoxelPos.X) / ChunkSize),
            FMath::FloorToInt(static_cast<float>(WorldVoxelPos.Y) / ChunkSize),
            FMath::FloorToInt(static_cast<float>(WorldVoxelPos.Z) / ChunkSize)
        );
        AffectedChunks.Add(ChunkCoord);
        ImportedCount++;
    }

    // // Regenerate all affected chunks
    // for (const FIntVector& Coord : AffectedChunks)
    // {
    //     if (World->Chunks.Contains(Coord))
    //     {
    //         if (AVoxelChunk* Chunk = World->Chunks[Coord])
    //         {
    //             Chunk->TryGenerateChunkMesh();
    //         }
    //     }
    // }
    
    UE_LOG(LogTemp, Log, TEXT("ImportStructure complete: %d voxels imported."), ImportedCount);
    UE_LOG(LogTemp, Log, TEXT("Chunks regenerated: %d"), AffectedChunks.Num());
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

void UBloxelsCheatManager::GiveBlock(const FString& BlockName) const
{
    if (const APlayerController* PC = GetOuterAPlayerController())
    {
        if (AFreeCameraPawn* CameraPawn = Cast<AFreeCameraPawn>(PC->GetPawn()))
        {
            CameraPawn->SetCurrentBlock(FName(BlockName));
        }
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
