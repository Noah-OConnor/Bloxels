#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "Biome/BiomeProperties.h"
#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/Core/VoxelInfo.h"
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
    InitializeVoxelProperties();

    GenerateInitialWorld();
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

void AVoxelWorld::InitializeVoxelProperties()
{
    if (!VoxelDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Voxel DataTable is not assigned in the editor!"));
        return;
    }

    static const FString ContextString(TEXT("Voxel Properties Context"));
    TArray<FVoxelInfo*> AllRows;
    VoxelDataTable->GetAllRows(ContextString, AllRows);

    for (const FVoxelInfo* Row : AllRows)
    {
        if (!Row) continue;

        uint16 VoxelIndex = static_cast<uint16>(Row->VoxelType);

        if (VoxelIndex >= UINT16_MAX)
        {
            UE_LOG(LogTemp, Error, TEXT("Voxel ID %d exceeds maximum allowed index!"), VoxelIndex);
            continue;
        }

        AVoxelWorld::VoxelProperties[VoxelIndex] = *Row;
        UE_LOG(LogTemp, Log, TEXT("Loaded Voxel ID: %d - Solid: %s, Transparent: %s"),
            VoxelIndex,
            AVoxelWorld::VoxelProperties[VoxelIndex].bIsSolid ? TEXT("True") : TEXT("False"),
            AVoxelWorld::VoxelProperties[VoxelIndex].bIsTransparent ? TEXT("True") : TEXT("False"));
    }
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
        PlayerPawn->SetActorLocation(FVector((ChunkSize * VoxelSize) / 2, (ChunkSize * VoxelSize) / 2, (ChunkHeight * VoxelSize) / 2));

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
        FVector PlayerPosition = PlayerPawn->GetActorLocation();
        FIntPoint NewChunk = FIntPoint(
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
        AVoxelChunk* Chunk = Pair.Value;  
        if (Chunk)  
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

int AVoxelWorld::PlaceBlock(int X, int Y, int Z, int blockToPlace)
{
	// Find the chunk that contains this voxel
	int ChunkX = FMath::FloorToInt((float)X / (ChunkSize));
	int ChunkY = FMath::FloorToInt((float)Y / (ChunkSize));
	int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
	int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
	FIntPoint ChunkCoord(ChunkX, ChunkY);
	if (Chunks.Contains(ChunkCoord))
	{
		AVoxelChunk* Chunk = Chunks[ChunkCoord];
		if (Chunk && Z >= 0 && Z < ChunkHeight)
		{
			int originalBlock = Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
			Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX] = blockToPlace;

			// Optionally trigger mesh regeneration here
			Chunk->TryGenerateChunkMesh();

            // if block is on a block border, regenerate the adjacent chunk to that block
            if (LocalX == 0) 
            {
                AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX - 1, ChunkY)];
                if (AdjacentChunk)
                {
                    // regenerate the chunk to the left
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalX == ChunkSize - 1)
            {
                AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX + 1, ChunkY)];
                if (AdjacentChunk)
                {
                    // regenerate the chunk to the right
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }

            if (LocalY == 0)
            {
                AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX, ChunkY - 1)];
                if (AdjacentChunk)
                {
                    // regenerate the chunk to the left
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            else if (LocalY == ChunkSize - 1)
            {
                AVoxelChunk* AdjacentChunk = Chunks[FIntPoint(ChunkX, ChunkY + 1)];
                if (AdjacentChunk)
                {
                    // regenerate the chunk to the right
                    AdjacentChunk->TryGenerateChunkMesh();
                }
            }
            return originalBlock;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk (%d, %d) not found for voxel placement at (%d, %d, %d)!"), ChunkX, ChunkY, X, Y, Z);
	}
    return 0;
}

void AVoxelWorld::UpdateTriggerVolume(FVector PlayerPosition)
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

EBiome AVoxelWorld::GetBiome(int X, int Y)
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

int AVoxelWorld::GetTerrainHeight(int X, int Y, EBiome Biome)
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

FVoxelInfo AVoxelWorld::VoxelProperties[UINT16_MAX] = {};

int16 AVoxelWorld::GetVoxelAtWorldCoordinates(int X, int Y, int Z)
{
    // if the bottom of the world, don't generate the face
    if (Z < 0)
    {
        return 1;
    }

    int ChunkX = FMath::FloorToInt((float)X / ChunkSize);
    int ChunkY = FMath::FloorToInt((float)Y / ChunkSize);

    int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
    int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;

    FIntPoint ChunkCoord(ChunkX, ChunkY);
    if (Chunks.Contains(ChunkCoord))
    {
        AVoxelChunk* Chunk = Chunks[ChunkCoord];
        if (Chunk->IsVoxelInChunk(LocalX, LocalY, Z))
        {
            return Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
        }
    }

    // Return a default value (e.g., air) if the chunk or voxel is not found
    return 0;
}