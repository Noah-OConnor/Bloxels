// Copyright 2025 Bloxels. All rights reserved.

#include "VoxelWorld.h"
#include "DrawDebugHelpers.h"
#include "WorldGenerationConfig.h"
#include "WorldGenerationSubsystem.h"
#include "Bloxels/Voxel/Chunk/VoxelChunkAsync.h"
#include "Bloxels/Voxel/Chunk/VoxelChunkData.h"
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

    ProcessChunkCreationQueue();
    ProcessPendingMeshChunks();
    ProcessChunkMeshGenQueue();
    ProcessChunkMeshDisplayQueue();
    //ProcessChunkUnloadQueue();
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
                if (!ActiveChunks.Contains(FIntVector(X, Y, Z)))
                {
                    QueueChunk(FIntVector(X, Y, Z));
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

void AVoxelWorld::UpdateChunks()  
{  
     UE_LOG(LogTemp, Warning, TEXT("UPDATE CHUNKS"));

     //ActiveChunksLock.ReadLock();
     for (auto& Pair : ActiveChunks)  
     {
         FVoxelChunkData Chunk = Pair.Value;
         if (FMath::Abs(Chunk.Coords.X - CurrentChunk.X) > WorldSize + 1 ||  
             FMath::Abs(Chunk.Coords.Y - CurrentChunk.Y) > WorldSize + 1 || 
             FMath::Abs(Chunk.Coords.Z - CurrentChunk.Z) > WorldSize + 1) 
         {
             UnloadChunk(Chunk.Coords);
             //ChunkUnloadQueue.Enqueue(Chunk.Coords);
         }
     }
     //ActiveChunksLock.ReadUnlock();
     
     GenerateInitialWorld();
}

void AVoxelWorld::QueueChunk(const FIntVector ChunkCoord)
{
    if (!ActiveChunks.Contains(ChunkCoord))
    {
        ChunkCreationQueue.Enqueue(ChunkCoord);
    }
}

void AVoxelWorld::ProcessChunkCreationQueue()
{
    for (int32 i = 0; i < ChunkCreationPerFrame && !ChunkCreationQueue.IsEmpty(); i++)
    {
        FIntVector Coord;
        ChunkCreationQueue.Dequeue(Coord);

        FVoxelChunkData NewChunk;
        NewChunk.Coords = Coord;
        
        //ActiveChunksLock.WriteLock();
        ActiveChunks.Add(Coord, NewChunk);
        //ActiveChunksLock.WriteUnlock();

        GenerateChunkDataAsync(Coord);
    }
}

void AVoxelWorld::GenerateChunkDataAsync(FIntVector ChunkCoord)
{
    TWeakObjectPtr<AVoxelWorld> WeakWorld(this);
    VoxelChunkAsync::GenerateChunkDataAsync(WeakWorld, ChunkCoord);
}

void AVoxelWorld::OnChunkDataGenerated(const FIntVector Coord, TArray<uint16> VoxelData)
{
    FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
    if (!Chunk) return;

    Chunk->VoxelData = MoveTemp(VoxelData);
    Chunk->bHasData = true;

    if (!Chunk->bHasMesh)
    {
        PendingMeshChunks.Add(Coord);
    }
}

void AVoxelWorld::ProcessPendingMeshChunks()
{
    for (auto It = PendingMeshChunks.CreateIterator(); It; ++It)
    {
        const FIntVector& Coord = *It;
        FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
        if (!Chunk || Chunk->bHasMesh) {
            It.RemoveCurrent();
            continue;
        }

        bool bPlayerNearby = IsChunkNearPlayer(Coord);
        bool bNeighborsReady = Chunk->AreNeighborsReady(ActiveChunks);

        if (bPlayerNearby && bNeighborsReady)
        {
            ChunkMeshGenQueue.Enqueue(Coord);
            It.RemoveCurrent();
        }
    }
}

void AVoxelWorld::ProcessChunkMeshGenQueue()
{
    for (int32 i = 0; i < ChunkMeshGenPerFrame && !ChunkMeshGenQueue.IsEmpty(); i++)
    {
        FIntVector Coord;
        if (ChunkMeshGenQueue.Dequeue(Coord))
        {
            FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
            if (!Chunk || !Chunk->AreNeighborsReady(ActiveChunks))
            {
                ChunkMeshGenQueue.Enqueue(Coord);
                continue;
            }

            GenerateChunkMeshAsync(Coord, *Chunk);
        }
    }
}

void AVoxelWorld::GenerateChunkMeshAsync(FIntVector ChunkCoords, const FVoxelChunkData& ChunkData)
{
    if (!ChunkData.bHasData && !ChunkData.bHasMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid mesh generation call"));
        return;
    }

    TWeakObjectPtr<AVoxelWorld> WeakWorld(this);
    TArray<uint16> VoxelDataCopy = ChunkData.VoxelData;
    const FIntVector ChunkCoordsCopy = ChunkCoords;

    VoxelChunkAsync::GenerateChunkMeshAsync(WeakWorld, VoxelDataCopy, ChunkCoordsCopy);
}

void AVoxelWorld::OnMeshGenerated(FIntVector Coord, TMap<FMeshSectionKey, FMeshData> MeshSections)
{
    FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
    if (!Chunk) return;

    Chunk->MeshSections = MoveTemp(MeshSections);
    Chunk->bHasMesh = true;

    ChunkMeshDisplayQueue.Enqueue(Coord);
}

void AVoxelWorld::ProcessChunkMeshDisplayQueue()
{
    for (int32 i = 0; i < ChunkMeshDisplayPerFrame && !ChunkMeshDisplayQueue.IsEmpty(); i++)
    {
        FIntVector Coord;
        if (ChunkMeshDisplayQueue.Dequeue(Coord))
        {
            FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
            if (Chunk)
            {
                DisplayChunkMesh(*Chunk);
            }
        }
    }
}

void AVoxelWorld::DisplayChunkMesh(FVoxelChunkData& Chunk)
{
    if (!Chunk.Mesh)
    {
        Chunk.Mesh = GetOrCreateMeshComponent();
        Chunk.Mesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    }

    Chunk.Mesh->SetRelativeLocation(CoordToWorldPosition(Chunk.Coords));
    Chunk.Mesh->ClearAllMeshSections();

   int SectionIndex = 0;  
   int TotalTris = 0;  
   int TotalVerts = 0;  
	
   for (const auto& Entry : Chunk.MeshSections)  
   {
       const FMeshSectionKey SectionKey = Entry.Key;

       if (const FMeshData& MeshData = Entry.Value; MeshData.Vertices.Num() > 0)  
       {  
           Chunk.Mesh->CreateMeshSection(  
               SectionIndex, MeshData.Vertices, MeshData.Triangles, MeshData.Normals,  
               MeshData.UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);

           UMaterialInterface* BaseMaterial = GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->Material;

           if (UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this))
           {  
               if (SectionKey.Normal.Z == 0)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->SideTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->SideTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == 1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->TopTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->TopTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == -1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->BottomTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->BottomTileOffset.Y);
               }

               Chunk.Mesh->SetMaterial(SectionIndex, MaterialInstance);
           }

           TotalTris += MeshData.Triangles.Num() / 3;  
           TotalVerts += MeshData.Vertices.Num();  
           SectionIndex++;  
       }  
   }  
}

void AVoxelWorld::ProcessChunkUnloadQueue()
{
    for (int32 i = 0; i < ChunkUnloadPerFrame && !ChunkUnloadQueue.IsEmpty(); i++)
    {
        FIntVector Coord;
        if (ChunkUnloadQueue.Dequeue(Coord))
        {
            FVoxelChunkData* Chunk = ActiveChunks.Find(Coord);
            if (Chunk)
            {
                UnloadChunk(Chunk->Coords);
            }
        }
    }
}

void AVoxelWorld::UnloadChunk(FIntVector ChunkCoords)
{
    if (!ActiveChunks.Contains(ChunkCoords)) return;
    
    FVoxelChunkData* Chunk = ActiveChunks.Find(ChunkCoords);
    
    if (Chunk->Mesh)
    {
        RecycleMesh(Chunk->Mesh);
        Chunk->Mesh = nullptr;
    }

    ChunkCreationQueue.Dequeue(ChunkCoords);
    ChunkMeshGenQueue.Dequeue(ChunkCoords);
    ChunkMeshDisplayQueue.Dequeue(ChunkCoords);
    PendingMeshChunks.Remove(ChunkCoords);

    Chunk->VoxelData.Empty();
    Chunk->MeshSections.Empty();

    //ActiveChunksLock.WriteLock();
    ActiveChunks.Remove(ChunkCoords);
    //ActiveChunksLock.WriteUnlock();
}

bool AVoxelWorld::IsChunkNearPlayer(const FIntVector& Coord) const
{
    FVector PlayerPos = PlayerPawn->GetActorLocation();

    const int ChunkSize = VoxelWorldConfig->ChunkSize;
    const int VoxelSize = VoxelWorldConfig->VoxelSize;
    
    FIntVector PlayerChunk = FIntVector(PlayerPos.X / (ChunkSize * VoxelSize), PlayerPos.Y / (ChunkSize * VoxelSize), PlayerPos.Z / (ChunkSize * VoxelSize));
    
    return ((Coord.X - PlayerChunk.X) * (Coord.X - PlayerChunk.X))
    + ((Coord.Y - PlayerChunk.Y) * (Coord.Y - PlayerChunk.Y))
    + ((Coord.Z - PlayerChunk.Z) * (Coord.Z - PlayerChunk.Z)) <= WorldSize * WorldSize; // Change this for render distance?
    
}

UProceduralMeshComponent* AVoxelWorld::GetOrCreateMeshComponent()
{
    if (!MeshPool.IsEmpty())
    {
        UProceduralMeshComponent* Mesh;
        MeshPool.Dequeue(Mesh);
        //Mesh->RegisterComponent();
        return Mesh;
    }

    UProceduralMeshComponent* NewMesh = NewObject<UProceduralMeshComponent>(this);
    NewMesh->RegisterComponent();
    return NewMesh;
}

void AVoxelWorld::RecycleMesh(UProceduralMeshComponent* Mesh)
{
    if (!Mesh) return;
    //Mesh->ClearAllMeshSections();
    //Mesh->ClearCollisionConvexMeshes();
    //Mesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    //Mesh->UnregisterComponent();
    MeshPool.Enqueue(Mesh);
}

FVector AVoxelWorld::CoordToWorldPosition(const FIntVector Coord) const
{
    const int ChunkSize = VoxelWorldConfig->ChunkSize;
    const int VoxelSize = VoxelWorldConfig->VoxelSize;
    
    return  FVector(Coord.X * ChunkSize * VoxelSize, Coord.Y * ChunkSize * VoxelSize, Coord.Z * ChunkSize * VoxelSize);
}

int AVoxelWorld::PlaceBlock(const int X, const int Y, const int Z, const int BlockToPlace)
{
    return 0;
 //    const int ChunkSize = VoxelWorldConfig->ChunkSize;
 //    
	// // Find the chunk that contains this voxel
	// const int ChunkX = FMath::FloorToInt(static_cast<float>(X) / (ChunkSize));
	// const int ChunkY = FMath::FloorToInt(static_cast<float>(Y) / (ChunkSize));
	// const int ChunkZ = FMath::FloorToInt(static_cast<float>(Z) / (ChunkSize));
 //    
	// const int LocalX = (X % ChunkSize + ChunkSize) % ChunkSize;
	// const int LocalY = (Y % ChunkSize + ChunkSize) % ChunkSize;
	// const int LocalZ = (Z % ChunkSize + ChunkSize) % ChunkSize;
 //    
 //    if (const FIntVector ChunkCoord(ChunkX, ChunkY, ChunkZ); ActiveChunks.Contains(ChunkCoord))
	// {
 //        if (FVoxelChunkData* Chunk = ActiveChunks.Find(ChunkCoord))
	// 	{
	// 		const int OriginalBlock = Chunk->VoxelData[(LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX];
	// 		Chunk->VoxelData[(LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX] = BlockToPlace;
 //
	// 		// Optionally trigger mesh regeneration here
	// 		//Chunk->TryGenerateChunkMesh();
 //
 //            // if block is on a block border, regenerate the adjacent chunk to that block
 //            if (LocalX == 0) 
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX - 1, ChunkY, ChunkZ)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //            else if (LocalX == ChunkSize - 1)
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX + 1, ChunkY, ChunkZ)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //
 //            if (LocalY == 0)
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY - 1, ChunkZ)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //            else if (LocalY == ChunkSize - 1)
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY + 1, ChunkZ)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //
 //            if (LocalZ == 0)
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY, ChunkZ - 1)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //            else if (LocalZ == ChunkSize - 1)
 //            {
 //                if (AVoxelChunk* AdjacentChunk = Chunks[FIntVector(ChunkX, ChunkY, ChunkZ + 1)])
 //                {
 //                    AdjacentChunk->TryGenerateChunkMesh();
 //                }
 //            }
 //            return OriginalBlock;
	// 	}
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Chunk (%d, %d) not found for voxel placement at (%d, %d, %d)!"), ChunkX, ChunkY, X, Y, Z);
	// }
 //    return GetVoxelRegistry()->GetIDFromName("Air");
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

    FIntVector ChunkCoords (ChunkX, ChunkY, ChunkZ);
    FVoxelChunkData* Chunk = ActiveChunks.Find(ChunkCoords);
    
    //ActiveChunksLock.ReadLock(); 
    //Chunk = ActiveChunks.Find(ChunkCoords);
    //ActiveChunksLock.ReadUnlock();

    if (Chunk && Chunk->bHasData && IsVoxelInChunk(LocalX, LocalY, LocalZ))
    {
        const int Index = (LocalZ * ChunkSize * ChunkSize) + (LocalY * ChunkSize) + LocalX;
        if (Index >= 0 && Index < Chunk->VoxelData.Num())
        {
            return Chunk->VoxelData[Index];
        }
    }

    return GetVoxelRegistry()->GetIDFromName(FName("Air")); // Air
}

bool AVoxelWorld::CheckVoxel(int X, int Y, int Z, FIntVector ChunkCoord)
{
    const int ChunkSize = GetWorldGenerationConfig()->ChunkSize;
    
    if (!IsValid(this)) return false;
    FVoxelChunkData* Chunk = ActiveChunks.Find(ChunkCoord);
    if (Chunk && IsVoxelInChunk(X, Y, Z))
    {
        int16 NeighborType = Chunk->VoxelData[(Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X];
        return GetVoxelRegistry()->GetVoxelByID(NeighborType)->bIsTransparent;
    }
    else
    {
        int WorldX = ChunkCoord.X * ChunkSize + X;
        int WorldY = ChunkCoord.Y * ChunkSize + Y;
        int WorldZ = ChunkCoord.Z * ChunkSize + Z;
        int16 NeighborType = GetVoxelAtWorldCoordinates(WorldX, WorldY, WorldZ);
        return GetVoxelRegistry()->GetVoxelByID(NeighborType)->bIsTransparent;
    }
}

bool AVoxelWorld::IsVoxelInChunk(int X, int Y, int Z) const
{
    const int ChunkSize = GetWorldGenerationConfig()->ChunkSize;
    
    if (!IsValid(this)) return false;

    return (X >= 0 && X < ChunkSize &&
        Y >= 0 && Y < ChunkSize &&
        Z >= 0 && Z < ChunkSize);
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
