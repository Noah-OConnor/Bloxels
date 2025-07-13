// Copyright 2025 Bloxels. All rights reserved.

#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "WorldGenerationConfig.h"
#include "WorldGenerationSubsystem.h"
#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/VoxelRegistry/VoxelRegistrySubsystem.h"
#include "Components/BrushComponent.h"
#include "Kismet/GameplayStatics.h"

AVoxelWorld::AVoxelWorld(): VoxelWorldConfig(nullptr),
                            TemperatureNoise(nullptr),
                            HabitabilityNoise(nullptr),
                            ElevationNoise(nullptr),
                            PlayerPawn(nullptr)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AVoxelWorld::BeginPlay()
{
    Super::BeginPlay();

    if (!VoxelWorldConfig)
    {
        UE_LOG(LogTemp, Error, TEXT("VoxelWorldConfig is NULL"));
        return;
    }

    if (UWorldGenerationSubsystem* WorldGenSubsystem = GetGameInstance()->GetSubsystem<UWorldGenerationSubsystem>())
    {
        WorldGenSubsystem->InitializeConfig(VoxelWorldConfig);
    }

    FTimerHandle DelayWorldGenTimer;
    GetWorldTimerManager().SetTimer(DelayWorldGenTimer, this, &AVoxelWorld::DelayedGenerateWorld, 0.1f, false);

    InitializePlayer();
    InitializeTriggerVolume();
}

void AVoxelWorld::DelayedGenerateWorld()
{
    // Ensure voxel registry is ready
    UVoxelRegistrySubsystem* Registry = GetVoxelRegistry();
    if (!Registry || Registry->VoxelAssets.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelRegistry not ready, delaying again..."));
        FTimerHandle RetryTimer;
        GetWorldTimerManager().SetTimer(RetryTimer, this, &AVoxelWorld::DelayedGenerateWorld, 0.1f, false);
        return;
    }

    //UE_LOG(LogTemp, Warning, TEXT("VoxelRegistry ready. Generating initial world."));
    GenerateInitialWorld();
}

void AVoxelWorld::GenerateInitialWorld()
{
    //UE_LOG(LogTemp, Error, TEXT("GENERATE INITIAL WORLD"));
    CurrentChunk = FIntVector(0, 0, 10);
    UE_LOG(LogTemp, Error, TEXT("CURRENT CHUNK: %d, %d, %d"), CurrentChunk.X, CurrentChunk.Y, CurrentChunk.Z);
    for (int X = CurrentChunk.X - WorldSize; X <= CurrentChunk.X + WorldSize; X++)
    {
        for (int Y = CurrentChunk.Y - WorldSize; Y <= CurrentChunk.Y + WorldSize; Y++)
        {
            for (int Z = CurrentChunk.Z - WorldSize; Z <= CurrentChunk.Z + WorldSize; Z++)
            {
                //UE_LOG(LogTemp, Warning, TEXT("Creating chunk at: %d %d %d"), X, Y, Z);
                TryCreateNewChunk(X, Y, Z, true);
            }
        }
    }
}

void AVoxelWorld::InitializePlayer()
{
    PlayerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn();
    if (PlayerPawn)
    {
        const int ChunkSize = VoxelWorldConfig->ChunkSize;
        const int VoxelSize = VoxelWorldConfig->VoxelSize;
        
        PlayerPawn->SetActorLocation(FVector((ChunkSize * VoxelSize) / 2, (ChunkSize * VoxelSize) / 2, (ChunkSize * VoxelSize) / 4));

		//AGauntletCharacter* GauntletCharacter = Cast<AGauntletCharacter>(PlayerPawn);
		//GauntletCharacter->VoxelWorld = this; // Set the VoxelWorld reference in the character
    
        FVector PlayerPosition = PlayerPawn->GetActorLocation();
    
        CurrentChunk = FIntVector(
            FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Z / (ChunkSize * VoxelSize))
            
        );
    }
    PreviousChunk = CurrentChunk;
}

void AVoxelWorld::InitializeTriggerVolume()
{
    if (PlayerPawn)
    {
        FVector PlayerPosition = PlayerPawn->GetActorLocation();

        if (ChunkTriggerVolume)
        {
            const int ChunkSize = VoxelWorldConfig->ChunkSize;
            const int VoxelSize = VoxelWorldConfig->VoxelSize;
            
            UBrushComponent* BrushComponent = ChunkTriggerVolume->GetBrushComponent();
            BrushComponent->SetMobility(EComponentMobility::Movable);
            ChunkTriggerVolume->SetActorScale3D(FVector((ChunkSize / 2) * VoxelSize / 100.0f, (ChunkSize / 2) * VoxelSize / 100.0f, ChunkSize * VoxelSize / 100.0f));
            ChunkTriggerVolume->OnActorEndOverlap.AddDynamic(this, &AVoxelWorld::OnChunkExit);

            //UpdateChunks(PlayerPosition);
            UpdateTriggerVolume(PlayerPosition);
        }
    }
}

void AVoxelWorld::TryCreateNewChunk(int32 ChunkX, int32 ChunkY, int32 ChunkZ, bool bShouldGenMesh)
{
    FIntVector ChunkCoords(ChunkX, ChunkY, ChunkZ);
    
    // Does the chunk already exist with a generated mesh?
    if (!ActiveChunks.Contains(ChunkCoords))
    {
        if (Chunks.Contains(ChunkCoords))
        {
            //UE_LOG(LogTemp, Warning, TEXT("\nChunks Already Contains (%d, %d)"), ChunkX, ChunkY);
            AVoxelChunk* Chunk = *Chunks.Find(ChunkCoords);
            if (Chunk && bShouldGenMesh)
            {
                Chunk->bGenerateMesh = true;

                if (Chunk->bHasData)
                {
                    // Call Generate chunk Mesh Async
                    Chunk->TryGenerateChunkMesh();
                }
            }
        }
        else
        {
            const int ChunkSize = VoxelWorldConfig->ChunkSize;
            const int VoxelSize = VoxelWorldConfig->VoxelSize;
            
            FVector Location(ChunkX * ChunkSize * VoxelSize, ChunkY * ChunkSize * VoxelSize, ChunkZ * ChunkSize * VoxelSize);
            //UE_LOG(LogTemp, Warning, TEXT("Spawning new chunk at World Location: (%f, %f, %f)"), Location.X, Location.Y, Location.Z);
            FActorSpawnParameters SpawnParams;

            // Ensure chunk doesn't already exist before spawning
            if (ActiveChunks.Contains(FIntVector(ChunkX, ChunkY, ChunkZ)))
            {
                //UE_LOG(LogTemp, Warning, TEXT("Chunk (%d, %d) already exists! Skipping."), ChunkX, ChunkY); 
                return;
            }

            AVoxelChunk* NewChunk = GetWorld()->SpawnActor<AVoxelChunk>(AVoxelChunk::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);

            if (!NewChunk)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn VoxelChunk!"));
                return;
            }

            #if WITH_EDITOR
                        NewChunk->SetActorLabel(*FString::Printf(TEXT("VoxelChunk(%d, %d)"), ChunkX, ChunkY));
            #endif
                        NewChunk->InitializeChunk(this, ChunkX, ChunkY, ChunkZ, bShouldGenMesh);

            ChunksLock.WriteLock();
            Chunks.Add(FIntVector(ChunkX, ChunkY, ChunkZ), NewChunk);
            ChunksLock.WriteUnlock();

            //UE_LOG(LogTemp, Warning, TEXT("Chunk (%d, %d) spawned and added to Chunks"), ChunkX, ChunkY);
        }
    }
    else
    {
        //UE_LOG(LogTemp, Error, TEXT("Chunk Already Exists at (%d, %d) skipping"), ChunkX, ChunkY);
    }
}

void AVoxelWorld::OnChunkExit(AActor* OverlappedActor, AActor* OtherActor)
{
    if (OtherActor == PlayerPawn)
    {
        const int ChunkSize = VoxelWorldConfig->ChunkSize;
        const int VoxelSize = VoxelWorldConfig->VoxelSize;
        
        const FVector PlayerPosition = PlayerPawn->GetActorLocation();
        const FIntVector NewChunk = FIntVector(
            FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Z / (ChunkSize * VoxelSize))
        );

        if (NewChunk != CurrentChunk)
        {
            PreviousChunk = CurrentChunk;
            CurrentChunk = NewChunk;
            GenerateInitialWorld();
            UpdateTriggerVolume(PlayerPosition);
            UpdateChunks();
        }
    }
}

void AVoxelWorld::UpdateChunks()  
{  
    UE_LOG(LogTemp, Warning, TEXT("UPDATE CHUNKS"));

    TArray<AVoxelChunk*> ChunksToUnload;

    for (auto& Pair : ActiveChunks)  
    {
        if (AVoxelChunk* Chunk = Pair.Value)  
        {  
            if (FMath::Abs(Chunk->ChunkCoords.X - CurrentChunk.X) > WorldSize + 1 ||  
                FMath::Abs(Chunk->ChunkCoords.Y - CurrentChunk.Y) > WorldSize + 1)  
            {
                UE_LOG(LogTemp, Warning, TEXT("MARKING FOR UNLOAD (%d, %d)"), Chunk->ChunkCoords.X, Chunk->ChunkCoords.Y);
                ChunksToUnload.Add(Chunk);
            }  
        }  
    }

    for (AVoxelChunk* Chunk : ChunksToUnload)
    {
        Chunk->UnloadChunk();
    }
}

int AVoxelWorld::PlaceBlock(const int X, const int Y, const int Z, const int BlockToPlace)
{
    const int ChunkSize = VoxelWorldConfig->ChunkSize;
    
	// Find the chunk that contains this voxel
	const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / (ChunkSize));
	const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / (ChunkSize));
	const int ChunkZ = FMath::FloorToInt(static_cast<float>(Z) / (ChunkSize));
    
	const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
	const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
	const int LocalZ = (Z % ChunkSize + ChunkSize) % ChunkSize;
    
    if (const FIntVector ChunkCoord(ChunkX, ChunkY, ChunkZ); Chunks.Contains(ChunkCoord))
	{
        if (AVoxelChunk* Chunk = Chunks[ChunkCoord])
		{
			const int OriginalBlock = Chunk->VoxelData[(LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
			Chunk->VoxelData[(LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX] = BlockToPlace;
 
			// Optionally trigger mesh regeneration here
			Chunk->TryGenerateChunkMesh();
 
            // if block is on a block border, regenerate the adjacent chunk to that block
            if (LocalX == 0) 
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX - 1, ChunkY, ChunkZ)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalX == ChunkSize - 1)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX + 1, ChunkY, ChunkZ)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
 
            if (LocalY == 0)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY - 1, ChunkZ)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalY == ChunkSize - 1)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY + 1, ChunkZ)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
 
            if (LocalZ == 0)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY, ChunkZ - 1)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalZ == ChunkSize - 1)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY, ChunkZ + 1)])
                {
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            return OriginalBlock;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk (%d, %d) not found for voxel placement at (%d, %d, %d)!"), ChunkX, ChunkY, X, Y, Z);
	}
    return GetVoxelRegistry()->GetIDFromName("Air");
}

void AVoxelWorld::UpdateTriggerVolume(FVector PlayerPosition) const
{
    if (ChunkTriggerVolume)
    {
        //UE_LOG(LogTemp, Log, TEXT("Updating Trigger Volume at Position: %s"), *PlayerPosition.ToString());
        
        const int ChunkSize = VoxelWorldConfig->ChunkSize;
        const int VoxelSize = VoxelWorldConfig->VoxelSize;
        
        FVector ChunkCenter = FVector(
            (CurrentChunk.X + 0.5f) * ChunkSize * VoxelSize,
            (CurrentChunk.Y + 0.5f) * ChunkSize * VoxelSize,
            0.0f
        );
        ChunkTriggerVolume->SetActorLocation(ChunkCenter);
        /*DrawDebugBox(GetWorld(), ChunkTriggerVolume->GetActorLocation(), ChunkTriggerVolume->GetComponentsBoundingBox().GetExtent(),
            FColor::Orange, true, -1, 0, 5);*/
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ChunkTriggerVolume is null!"));
    }
}

int16 AVoxelWorld::GetVoxelAtWorldCoordinates(int X, int Y, int Z)
{
    const int ChunkSize = VoxelWorldConfig->ChunkSize;

    const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / ChunkSize);
    const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / ChunkSize);
    const int ChunkZ = FMath::FloorToInt(static_cast<float>(Z) / ChunkSize);

    const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
    const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
    const int LocalZ = (Z % ChunkSize + ChunkSize) % ChunkSize;

    ChunksLock.ReadLock(); 
    TWeakObjectPtr<AVoxelChunk> Chunk;
    if (Chunks.Contains(FIntVector(ChunkX, ChunkY, ChunkZ)))
    {
        Chunk = Chunks[FIntVector(ChunkX, ChunkY, ChunkZ)];
    }
    ChunksLock.ReadUnlock();

    if (Chunk.IsValid() && Chunk->IsVoxelInChunk(LocalX, LocalY, LocalZ))
    {
        return Chunk->VoxelData[(LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
    }

    return GetVoxelRegistry()->GetIDFromName(FName("Air")); // Air
}

UVoxelRegistrySubsystem* AVoxelWorld::GetVoxelRegistry() const
{
    return GetGameInstance()->GetSubsystem<UVoxelRegistrySubsystem>();
}

UWorldGenerationSubsystem* AVoxelWorld::GetWorldGenerationSubsystem() const
{
    return GetGameInstance()->GetSubsystem<UWorldGenerationSubsystem>();
}

UWorldGenerationConfig* AVoxelWorld::GetWorldGenerationConfig() const
{
    return VoxelWorldConfig;
}
