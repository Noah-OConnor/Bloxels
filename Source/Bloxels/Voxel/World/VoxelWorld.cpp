// Copyright 2025 Noah O'Connor. All rights reserved.

#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "Biome/BiomeProperties.h"
#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/VoxelRegistry/VoxelRegistry.h"
#include "Components/BrushComponent.h"
#include "Kismet/GameplayStatics.h"

AVoxelWorld::AVoxelWorld()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AVoxelWorld::BeginPlay()
{
    Super::BeginPlay();

    InitializeNoiseLayers();

    FTimerHandle DelayWorldGenTimer;
    GetWorldTimerManager().SetTimer(DelayWorldGenTimer, this, &AVoxelWorld::DelayedGenerateWorld, 0.1f, false);

    InitializePlayer();
    InitializeTriggerVolume();
}

void AVoxelWorld::InitializeNoiseLayers()
{
    // Setup Temperature Noise
    TemperatureNoise = NewObject<UFastNoiseWrapper>();
    TemperatureNoise->SetupFastNoise(Temperature.NoiseType); // Set noise type
    TemperatureNoise->SetFrequency(Temperature.NoiseFrequency);
    TemperatureNoise->SetFractalType(Temperature.NoiseFractalType);
    TemperatureNoise->SetOctaves(Temperature.NoiseOctaves);
    TemperatureNoise->SetSeed(Temperature.NoiseSeed);

    // Setup Habitability Noise
    HabitabilityNoise = NewObject<UFastNoiseWrapper>();
    HabitabilityNoise->SetupFastNoise(Habitability.NoiseType); // Set noise type
    HabitabilityNoise->SetFrequency(Habitability.NoiseFrequency);
    HabitabilityNoise->SetFractalType(Habitability.NoiseFractalType);
    HabitabilityNoise->SetOctaves(Habitability.NoiseOctaves);
    HabitabilityNoise->SetSeed(Habitability.NoiseSeed);

    // Setup Elevation Noise
    ElevationNoise = NewObject<UFastNoiseWrapper>();
    ElevationNoise->SetupFastNoise(Elevation.NoiseType); // Set noise type
    ElevationNoise->SetFrequency(Elevation.NoiseFrequency);
    ElevationNoise->SetFractalType(Elevation.NoiseFractalType);
    ElevationNoise->SetOctaves(Elevation.NoiseOctaves);
    ElevationNoise->SetSeed(Elevation.NoiseSeed);
}

void AVoxelWorld::DelayedGenerateWorld()
{
    // Ensure voxel registry is ready
    UVoxelRegistry* Registry = GetVoxelRegistry();
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
    for (int X = CurrentChunk.X - WorldSize; X <= CurrentChunk.X + WorldSize; X++)
    {
        for (int Y = CurrentChunk.Y - WorldSize; Y <= CurrentChunk.Y + WorldSize; Y++)
        {
            TryCreateNewChunk(X, Y, true);
        }
    }
}

void AVoxelWorld::InitializePlayer()
{
    PlayerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn();
    if (PlayerPawn)
    {
        PlayerPawn->SetActorLocation(FVector((ChunkSize * VoxelSize) / 2, (ChunkSize * VoxelSize) / 2, (ChunkHeight * VoxelSize) / 4));

		//AGauntletCharacter* GauntletCharacter = Cast<AGauntletCharacter>(PlayerPawn);
		//GauntletCharacter->VoxelWorld = this; // Set the VoxelWorld reference in the character
    
        FVector PlayerPosition = PlayerPawn->GetActorLocation();
    
        CurrentChunk = FIntPoint(
            FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize))
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
            UBrushComponent* BrushComponent = ChunkTriggerVolume->GetBrushComponent();
            BrushComponent->SetMobility(EComponentMobility::Movable);
            ChunkTriggerVolume->SetActorScale3D(FVector((ChunkSize / 2) * VoxelSize / 100.0f, (ChunkSize / 2) * VoxelSize / 100.0f, ChunkHeight * VoxelSize / 100.0f));
            ChunkTriggerVolume->OnActorEndOverlap.AddDynamic(this, &AVoxelWorld::OnChunkExit);

            //UpdateChunks(PlayerPosition);
            UpdateTriggerVolume(PlayerPosition);
        }
    }
}

void AVoxelWorld::TryCreateNewChunk(int32 ChunkX, int32 ChunkY, bool bShouldGenMesh)
{
    FIntPoint ChunkCoords(ChunkX, ChunkY);
    
    // Does the chunk already exist with a generated mesh?
    if (!ActiveChunks.Contains(ChunkCoords))
    {
        if (Chunks.Contains(ChunkCoords))
        {
            //UE_LOG(LogTemp, Warning, TEXT("\nChunks Already Contains (%d, %d)"), ChunkX, ChunkY);
            AVoxelChunk* chunk = *Chunks.Find(ChunkCoords);
            if (chunk && bShouldGenMesh)
            {
                chunk->bGenerateMesh = true;

                if (chunk->bHasData)
                {
                    // Call Generate chunk Mesh Async
                    chunk->TryGenerateChunkMesh();
                }
            }
        }
        else
        {
            FVector Location(ChunkX * ChunkSize * VoxelSize, ChunkY * ChunkSize * VoxelSize, 0);
            //UE_LOG(LogTemp, Warning, TEXT("Spawning new chunk at World Location: (%f, %f, %f)"), Location.X, Location.Y, Location.Z);
            FActorSpawnParameters SpawnParams;

            // Ensure chunk doesn't already exist before spawning
            if (ActiveChunks.Contains(FIntPoint(ChunkX, ChunkY)))
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
                        NewChunk->InitializeChunk(this, ChunkX, ChunkY, bShouldGenMesh);

            ChunksLock.WriteLock();
            Chunks.Add(FIntPoint(ChunkX, ChunkY), NewChunk);
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
        const FVector PlayerPosition = PlayerPawn->GetActorLocation();
        const FIntPoint NewChunk = FIntPoint(
            FMath::FloorToInt(PlayerPosition.X / (ChunkSize * VoxelSize)),
            FMath::FloorToInt(PlayerPosition.Y / (ChunkSize * VoxelSize))
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
	// Find the chunk that contains this voxel
	const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / (ChunkSize));
	const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / (ChunkSize));
	const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
	const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
    if (const FIntPoint ChunkCoord(ChunkX, ChunkY); Chunks.Contains(ChunkCoord))
	{
        if (AVoxelChunk* Chunk = Chunks[ChunkCoord]; Chunk && Z >= 0 && Z < ChunkHeight)
		{
			const int OriginalBlock = Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
			Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX] = BlockToPlace;

			// Optionally trigger mesh regeneration here
			Chunk->TryGenerateChunkMesh();

            // if block is on a block border, regenerate the adjacent chunk to that block
            if (LocalX == 0) 
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX - 1, ChunkY)])
                {
                    // regenerate the chunk to the left
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalX == ChunkSize - 1)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX + 1, ChunkY)])
                {
                    // regenerate the chunk to the right
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }

            if (LocalY == 0)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX, ChunkY - 1)])
                {
                    // regenerate the chunk to the left
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalY == ChunkSize - 1)
            {
                if (AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX, ChunkY + 1)])
                {
                    // regenerate the chunk to the right
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
    return 0;
}

void AVoxelWorld::UpdateTriggerVolume(FVector PlayerPosition) const
{
    if (ChunkTriggerVolume)
    {
        //UE_LOG(LogTemp, Log, TEXT("Updating Trigger Volume at Position: %s"), *PlayerPosition.ToString());
        
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

EBiome AVoxelWorld::GetBiome(int X, int Y) const
{
    float temperature = 0;
    float habitability = 0;
    float elevation = 0;
    if (Temperature.UseThisNoise)
    {
        temperature = (TemperatureNoise->GetNoise2D(X, Y) + 1) / 2;
    }
    if (Habitability.UseThisNoise)
    {
        habitability = (HabitabilityNoise->GetNoise2D(X, Y) + 1) / 2;
    }
    if (Elevation.UseThisNoise)
    {
        elevation = (ElevationNoise->GetNoise2D(X, Y) + 1) / 2;
    }

    static const FString ContextString(TEXT("Biome Properties Context"));
    TArray<FBiomeProperties*> AllRows;
    BiomeDataTable->GetAllRows(ContextString, AllRows);

    for (const FBiomeProperties* Row : AllRows)
    {
        if (!Row) continue;

        for (const FBiomeNoiseRanges& row : Row->BiomeNoiseRanges)
        {
            if (temperature >= row.MinTemperature && temperature <= row.MaxTemperature &&
                habitability >= row.MinHabitability && habitability <= row.MaxHabitability &&
                elevation >= row.MinElevation && elevation <= row.MaxElevation)
            {
                return Row->BiomeType;
            }
        }
    }
    return EBiome::None; // Default biome if no match is found
}

int AVoxelWorld::GetTerrainHeight(int X, int Y, EBiome Biome) const
{
    // default baseheight in case we're not using any noise layers
    float BaseHeight = ChunkHeight / 2;
    
    // STEP 1: set up base noise
    if (Elevation.UseThisNoise)
    {
        float elevation = (ElevationNoise->GetNoise2D(X, Y) + 1) / 2;
        BaseHeight = Elevation.NoiseCurve->GetFloatValue(elevation) / 2;
    }

    // STEP 2: Add biome specific noise



    return BaseHeight * ChunkHeight; //FMath::Clamp(BaseHeight, 20.0f, ChunkHeight * 0.8f);
}

const FBiomeProperties* AVoxelWorld::GetBiomeData(EBiome Biome) const
{
    if (BiomeDataTable)
    {
        FString ContextString;
        TArray<FBiomeProperties*> AllRows;
        BiomeDataTable->GetAllRows(ContextString, AllRows);

        for (const FBiomeProperties* Row : AllRows)
        {
            if (Row && Row->BiomeType == Biome)
            {
                return Row;
            }
        }
    }
    return nullptr;
}

int16 AVoxelWorld::GetVoxelAtWorldCoordinates(int X, int Y, int Z)
{
    if (Z < 0)
    {
        return 1; // Bedrock
    }

    const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / ChunkSize);
    const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / ChunkSize);
    const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
    const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;

    ChunksLock.ReadLock(); 
    TWeakObjectPtr<AVoxelChunk> Chunk;
    if (Chunks.Contains(FIntPoint(ChunkX, ChunkY)))
    {
        Chunk = Chunks[FIntPoint(ChunkX, ChunkY)];
    }
    ChunksLock.ReadUnlock();

    if (Chunk.IsValid() && Chunk->IsVoxelInChunk(LocalX, LocalY, Z))
    {
        return Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
    }

    return GetVoxelRegistry()->GetIDFromName(FName("Air")); // Air
}

UVoxelRegistry* AVoxelWorld::GetVoxelRegistry() const
{
    return GetGameInstance()->GetSubsystem<UVoxelRegistry>();
}
