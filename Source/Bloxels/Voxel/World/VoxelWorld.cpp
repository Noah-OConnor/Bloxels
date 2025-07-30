// Copyright 2025 Bloxels. All rights reserved.

#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "WorldGenerationConfig.h"
#include "WorldGenerationSubsystem.h"
#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/VoxelRegistry/VoxelRegistrySubsystem.h"
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

    InitializePlayer();
    GenerateInitialWorld();
}

void AVoxelWorld::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerPawn) return;
    
    const int ChunkSize = VoxelWorldConfig->ChunkSize;
    const int VoxelSize = VoxelWorldConfig->VoxelSize;

    const FVector PlayerPosition = PlayerPawn->GetActorLocation();
    
    CurrentChunk = FIntVector(
        FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
        FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize)),
        FMath::FloorToInt(PlayerPosition.Z / (ChunkSize * VoxelSize))
    );

    if (CurrentChunk != PreviousChunk)
    {
        UpdateChunks();
    }

    PreviousChunk = CurrentChunk;

    int32 Count = 0;
    while (Count < ChunkUnloadPerFrame)
    {
        if (ChunkUnloadQueue.Num() > 0)
        {
            AVoxelChunk* NextChunk = ChunkUnloadQueue[0];
            
            ChunkDataGenQueue.Remove(NextChunk);
            ChunkMeshGenQueue.Remove(NextChunk);
            ChunkMeshDisplayQueue.Remove(NextChunk);
            ChunkUnloadQueue.Remove(NextChunk);
            
            NextChunk->UnloadChunk();
        }
        else break;

        Count++;
    }

    // Count = 0;
    // while (Count < ChunkCreationPerFrame)
    // {
    //     if (ChunkCreationQueue.Num() > 0)
    //     {
    //         FNewChunk NextChunk = ChunkCreationQueue[0];
    //         ChunkCreationQueue.RemoveAt(0);
    //         TryCreateNewChunk(NextChunk.ChunkCoords.X, NextChunk.ChunkCoords.Y, NextChunk.ChunkCoords.Z, NextChunk.bShouldGenMesh);
    //     }
    //     else break;
    //
    //     Count++;
    // }

    Count = 0;
    while (Count < ChunkDataGenPerFrame)
    {
        if (ChunkDataGenQueue.Num() > 0)
        {
            AVoxelChunk* NextChunk = ChunkDataGenQueue[0];
            ChunkDataGenQueue.Remove(NextChunk);
            NextChunk->GenerateChunkDataAsync();
        }
        else break;

        Count++;
    }

    Count = 0;
    while (Count < ChunkMeshGenPerFrame)
    {
        if (ChunkMeshGenQueue.Num() > 0)
        {
            AVoxelChunk* NextChunk = ChunkMeshGenQueue[0];

            ChunkMeshGenQueue.Remove(NextChunk);

            if (!NextChunk->bHasData) continue;
            
            NextChunk->GenerateChunkMeshAsync();
        }
        else break;

        Count++;
    }

    Count = 0;
    while (Count < ChunkMeshDisplayPerFrame)
    {
        if (ChunkMeshDisplayQueue.Num() > 0)
        {
            AVoxelChunk* NextChunk = ChunkMeshDisplayQueue[0];
            ChunkMeshDisplayQueue.Remove(NextChunk);
            NextChunk->DisplayMesh();
        }
        else break;

        Count++;
    }
}

void AVoxelWorld::GenerateInitialWorld()
{
    UE_LOG(LogTemp, Error, TEXT("CURRENT CHUNK: %d, %d, %d"), CurrentChunk.X, CurrentChunk.Y, CurrentChunk.Z);
    for (int X = CurrentChunk.X - WorldSize; X <= CurrentChunk.X + WorldSize; X++)
    {
        for (int Y = CurrentChunk.Y - WorldSize; Y <= CurrentChunk.Y + WorldSize; Y++)
        {
            for (int Z = CurrentChunk.Z - WorldSize; Z <= CurrentChunk.Z + WorldSize; Z++)
            {
                if (!Chunks.Contains(FIntVector(X, Y, Z)))
                {
                    //FNewChunk NewChunk = FNewChunk(FIntVector(X, Y, Z), true);
                    //ChunkCreationQueue.Add(NewChunk);
                    TryCreateNewChunk(X, Y, Z, true);
                }
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

        UWorldGenerationSubsystem* WorldGenSubsystem = GetWorldGenerationSubsystem();
        
        PlayerPawn->SetActorLocation(FVector(0, 0, WorldGenSubsystem->GetTerrainHeight(0,0, WorldGenSubsystem->GetBiome(0, 0)) * VoxelSize + 200));
    
        FVector PlayerPosition = PlayerPawn->GetActorLocation();
    
        CurrentChunk = FIntVector(
            FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Z / (ChunkSize * VoxelSize))
        );
    }
    PreviousChunk = CurrentChunk;
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
                Chunk->bGenerateMesh = bShouldGenMesh;

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
            
            AVoxelChunk* NewChunk;

            if (ChunkPool.Num() > 0)
            {
                NewChunk = ChunkPool[0];
                ChunkPool.RemoveAt(0);
                NewChunk->SetActorLocation(Location);
                //UE_LOG(LogTemp, Warning, TEXT("USING A POOLED CHUNK INSTEAD OF SPAWNING"));
            }
            else
            {
                NewChunk = GetWorld()->SpawnActor<AVoxelChunk>(AVoxelChunk::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
                //UE_LOG(LogTemp, Error, TEXT("SPAWNED A NEW CHUNK INSTEAD OF POOLING"));
            }

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

void AVoxelWorld::UpdateChunks()  
{  
    UE_LOG(LogTemp, Warning, TEXT("UPDATE CHUNKS"));

    TArray<AVoxelChunk*> ChunksToUnload;

    for (auto& Pair : Chunks)  
    {
        if (AVoxelChunk* Chunk = Pair.Value)  
        {  
            if (FMath::Abs(Chunk->ChunkCoords.X - CurrentChunk.X) > WorldSize + 1 ||  
                FMath::Abs(Chunk->ChunkCoords.Y - CurrentChunk.Y) > WorldSize + 1 || 
                FMath::Abs(Chunk->ChunkCoords.Z - CurrentChunk.Z) > WorldSize + 1) 
            {
                //UE_LOG(LogTemp, Warning, TEXT("MARKING FOR UNLOAD (%d, %d, %d)"), Chunk->ChunkCoords.X, Chunk->ChunkCoords.Y, Chunk->ChunkCoords.Z);
                ChunksToUnload.Add(Chunk);
            }  
        }  
    }

    for (AVoxelChunk* Chunk : ChunksToUnload)
    {
        //Chunk->UnloadChunk();
        //AddToChunkUnloadQueue(Chunk);
        ChunkUnloadQueue.Add(Chunk);
    }

    GenerateInitialWorld();
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

int16 AVoxelWorld::GetVoxelAtWorldCoordinates(int X, int Y, int Z)
{
    const int ChunkSize = VoxelWorldConfig->ChunkSize;

    const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / ChunkSize);
    const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / ChunkSize);
    const int ChunkZ = FMath::FloorToInt(static_cast<float>(Z) / ChunkSize);

    const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
    const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
    const int LocalZ = (Z % ChunkSize + ChunkSize) % ChunkSize;

    TWeakObjectPtr<AVoxelChunk> Chunk;
    
    ChunksLock.ReadLock(); 
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
