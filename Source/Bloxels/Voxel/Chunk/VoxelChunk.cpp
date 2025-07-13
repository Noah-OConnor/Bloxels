// Copyright 2025 Noah O'Connor. All rights reserved.


#include "VoxelChunk.h"

#include "VoxelChunkAsync.h"
#include "Bloxels/Voxel/World/WorldGenerationConfig.h"
#include "Tasks/Task.h"


AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void AVoxelChunk::InitializeChunk(AVoxelWorld* InVoxelWorld, int32 ChunkX, int32 ChunkY, int32 ChunkZ, bool bShouldGenMesh)
{
	VoxelWorld = InVoxelWorld;
	ChunkCoords = FIntVector(ChunkX, ChunkY, ChunkZ);
	bGenerateMesh = bShouldGenMesh;
    
    const int ChunkSize = VoxelWorld->GetWorldGenerationConfig()->ChunkSize;

	// Initialize Voxel Data Size ***THIS SHOULD NOT CHANGE ANYWHERE AFTER ITS SET***
	VoxelData.SetNum(ChunkSize * ChunkSize * ChunkSize);

	GenerateChunkDataAsync();
}

void AVoxelChunk::GenerateChunkDataAsync()
{
	TWeakObjectPtr<AVoxelChunk> WeakChunk(this);
	TWeakObjectPtr<AVoxelWorld> WeakWorld(VoxelWorld);
	VoxelChunkAsync::GenerateChunkDataAsync(WeakChunk, WeakWorld, ChunkCoords);
}

void AVoxelChunk::OnChunkDataGenerated(TArray<uint16> InVoxelData)
{
	TWeakObjectPtr<AVoxelChunk> WeakThis(this);
	AsyncTask(ENamedThreads::GameThread, [InVoxelData = MoveTemp(InVoxelData), WeakThis]()
	{
		if (!WeakThis.IsValid())  // Check if AVoxelWorld is still valid before proceeding
		{
			UE_LOG(LogTemp, Error, TEXT("VoxelChunk is no longer valid!"));
			return;
		}

		WeakThis->VoxelData = InVoxelData;
		WeakThis->bHasData = true;
		if (WeakThis->bGenerateMesh)
		{
			WeakThis->TryGenerateChunkMesh();
		}
		WeakThis->ChunkDataGeneratedEvent.Broadcast();
		// REMOVE ALL LISTENERS
		WeakThis->ChunkDataGeneratedEvent.Clear();
	});
}

void AVoxelChunk::TryGenerateChunkMesh()
{
	if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("VoxelChunk is invalid"));
        return;
    }

    if (!VoxelWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("VoxelWorld is null"));
        return;
    }

    if (!VoxelWorld->GetVoxelRegistry())
    {
        UE_LOG(LogTemp, Error, TEXT("Voxel Registry is null"));
        return;
    }

    if (!bHasData)
    {
        UE_LOG(LogTemp, Error, TEXT("No Data!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Generating chunk mesh (%d, %d)"), ChunkCoords.X, ChunkCoords.Y);

    static const FIntVector Offsets[] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };
    bool bAllGenerated = true;

    for (const FIntVector& Offset : Offsets)
    {
        FIntVector NeighborCoord(ChunkCoords.X + Offset.X, ChunkCoords.Y + Offset.Y, ChunkCoords.Z + Offset.Z);
        AVoxelChunk* NeighborChunk = nullptr;// = *VoxelWorld->Chunks.Find(NeighborCoord);
        if (VoxelWorld->Chunks.Contains(NeighborCoord))
        {
            NeighborChunk = *VoxelWorld->Chunks.Find(NeighborCoord);
        }
        else
        {
            bAllGenerated = false;
        }

        // If NeighborChunk doesn't already exist, then we need to create it
        if (NeighborChunk == nullptr)
        {
            bAllGenerated = false;
            // We need to create the chunk & subscribe to on data generated
            VoxelWorld->TryCreateNewChunk(NeighborCoord.X, NeighborCoord.Y, NeighborCoord.Z, false);

            NeighborChunk = *VoxelWorld->Chunks.Find(NeighborCoord);
        }

        if (NeighborChunk != nullptr)
        {
            // If neighbor chunk doesn't already have data, subscribe to get notified when it finishes
            if (!NeighborChunk->bHasData)
            {
                bAllGenerated = false;

                // Check if already subscribed before adding
                if (!NeighborChunk->OnChunkDataGenerated().IsBoundToObject(this))
                {
                    NeighborChunk->OnChunkDataGenerated().AddUObject(this, &AVoxelChunk::TryGenerateChunkMesh);
                }
            }
        }
    }

    if (bAllGenerated)
    {
        //UE_LOG(LogTemp, Display, TEXT("ALL NEIGHBORING CHUNKS ARE GENERATED FOR (%d, %d)"), ChunkCoords.X, ChunkCoords.Y);

        GenerateChunkMeshAsync();
    }
}

void AVoxelChunk::GenerateChunkMeshAsync()
{
	if (!bGenerateMesh || !bHasData)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid mesh generation call"));
		return;
	}

	TWeakObjectPtr<AVoxelChunk> WeakChunk(this);
	TWeakObjectPtr<AVoxelWorld> WeakWorld(VoxelWorld);
    TArray<uint16> VoxelDataCopy = VoxelData;
	const FIntVector ChunkCoordsCopy = ChunkCoords;

	VoxelChunkAsync::GenerateChunkMeshAsync(WeakChunk, WeakWorld, VoxelDataCopy, ChunkCoordsCopy);
}


void AVoxelChunk::OnMeshGenerated(const TMap<FMeshSectionKey, FMeshData>& InMeshSections)
{
    //UE_LOG(LogTemp, Error, TEXT("ON MESH GENERATED"));
    TWeakObjectPtr<AVoxelChunk> WeakThis(this);

    AsyncTask(ENamedThreads::GameThread, [InMeshSections, WeakThis]()
    {
        if (!WeakThis.IsValid())  // Check if AVoxelChunk is still valid before proceeding
        {
            UE_LOG(LogTemp, Error, TEXT("VoxelChunk was deleted before chunk could be loaded."));
            return;
        }
        WeakThis->MeshSections = InMeshSections;
        WeakThis->bHasMeshSections = true;
        WeakThis->DisplayMesh();
    });
}

void AVoxelChunk::DisplayMesh()  
{  
   MeshComponent->ClearAllMeshSections(); // Clear existing mesh sections before displaying new ones;  

   // Apply mesh sections for each voxel type   
   int SectionIndex = 0;  
   int TotalTris = 0;  
   int TotalVerts = 0;  

	//UE_LOG(LogTemp, Error, TEXT("AVoxelChunk::DisplayMesh"));
	
   for (const auto& Entry : MeshSections)  
   {
       const FMeshSectionKey SectionKey = Entry.Key;

       if (const FMeshData& MeshData = Entry.Value; MeshData.Vertices.Num() > 0)  
       {  
           MeshComponent->CreateMeshSection(  
               SectionIndex, MeshData.Vertices, MeshData.Triangles, MeshData.Normals,  
               MeshData.UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);  

           UMaterialInterface* BaseMaterial = VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->Material;
           // Check if the material is a dynamic material instance and set the TileCountX parameter  

           if (UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this))
           {  
               if (SectionKey.Normal.Z == 0)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->SideTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->SideTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == 1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->TopTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->TopTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == -1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->BottomTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->GetVoxelRegistry()->GetVoxelByID(SectionKey.VoxelType)->BottomTileOffset.Y);
               }

               // Set the material for the section  
               MeshComponent->SetMaterial(SectionIndex, MaterialInstance);
           }

           TotalTris += MeshData.Triangles.Num() / 3;  
           TotalVerts += MeshData.Vertices.Num();  
           SectionIndex++;  
       }  
   }  

   VoxelWorld->ActiveChunksLock.WriteLock();  
   VoxelWorld->ActiveChunks.Add(ChunkCoords, this);  
   VoxelWorld->ActiveChunksLock.WriteUnlock();
}

void AVoxelChunk::UnloadChunk()
{
    VoxelWorld->ActiveChunksLock.WriteLock();
    VoxelWorld->ActiveChunks.Remove(ChunkCoords);
    VoxelWorld->ActiveChunksLock.WriteUnlock();

    VoxelWorld->ChunksLock.WriteLock();
    VoxelWorld->Chunks.Remove(ChunkCoords);
    VoxelWorld->ChunksLock.WriteUnlock();

    MeshComponent->ClearAllMeshSections();
    this->Destroy();
}

bool AVoxelChunk::IsVoxelInChunk(int X, int Y, int Z) const
{
    const int ChunkSize = VoxelWorld->GetWorldGenerationConfig()->ChunkSize;
    
    if (!IsValid(VoxelWorld)) return false;
	// returns true if the voxel is within the chunk bounds
    return (X >= 0 && X < ChunkSize &&
        Y >= 0 && Y < ChunkSize &&
        Z >= 0 && Z < ChunkSize);
}

/// <returns>Returns true when voxel is transparent or outside the chunk</returns>
bool AVoxelChunk::CheckVoxel(int X, int Y, int Z, FIntVector ChunkCoord)
{
    const int ChunkSize = VoxelWorld->GetWorldGenerationConfig()->ChunkSize;
    
    if (!IsValid(VoxelWorld)) return false;
    if (IsVoxelInChunk(X, Y, Z))
    {
        if (!IsValid(VoxelWorld)) return false;
        int16 NeighborType = VoxelData[(Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X];
        return VoxelWorld->GetVoxelRegistry()->GetVoxelByID(NeighborType)->bIsTransparent;
    }
    else
    {
        if (!IsValid(VoxelWorld)) return false;
        int WorldX = ChunkCoord.X * ChunkSize + X;
        int WorldY = ChunkCoord.Y * ChunkSize + Y;
        int16 NeighborType = VoxelWorld->GetVoxelAtWorldCoordinates(WorldX, WorldY, Z);
        return VoxelWorld->GetVoxelRegistry()->GetVoxelByID(NeighborType)->bIsTransparent;
    }
}

void AVoxelChunk::SetChunkCoords(FIntVector InCoords)
{
    ChunkCoords = InCoords;
}