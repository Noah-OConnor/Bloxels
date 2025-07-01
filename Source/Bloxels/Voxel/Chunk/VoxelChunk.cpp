// Copyright 2025 Noah O'Connor. All rights reserved.


#include "VoxelChunk.h"

#include "EngineUtils.h"
#include "VoxelGenerationTask.h"
#include "VoxelMeshTask.h"
#include "Bloxels/Voxel/PathFinding/PathfindingManager.h"


AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void AVoxelChunk::InitializeChunk(AVoxelWorld* InVoxelWorld, int32 ChunkX, int32 ChunkY, bool bShouldGenMesh)
{
	VoxelWorld = InVoxelWorld;
	ChunkCoords = FIntPoint(ChunkX, ChunkY);
	bGenerateMesh = bShouldGenMesh;

	// Initialize Voxel Data Size ***THIS SHOULD NOT CHANGE ANYWHERE AFTER ITS SET***
	VoxelData.SetNum(VoxelWorld->ChunkSize * VoxelWorld->ChunkSize * VoxelWorld->ChunkHeight);

	GenerateChunkDataAsync();
}

void AVoxelChunk::GenerateChunkDataAsync()
{
	TWeakObjectPtr<AVoxelChunk> WeakThis(this);
	TWeakObjectPtr<AVoxelWorld> WeakWorld(VoxelWorld);
	const auto AsyncTraceTask = new FAutoDeleteAsyncTask<FVoxelGenerationTask>(ChunkCoords.X, ChunkCoords.Y, WeakWorld, WeakThis);
	AsyncTraceTask->StartBackgroundTask();
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

    if (!VoxelWorld->VoxelProperties)
    {
        UE_LOG(LogTemp, Error, TEXT("VoxelProperties is null"));
        return;
    }

    if (!bHasData)
    {
        UE_LOG(LogTemp, Error, TEXT("No Data!"));
        return;
    }

    static const FIntPoint Offsets[] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1} // Only considering 2D neighbors
    };
    bool bAllGenerated = true;

    for (const FIntPoint& Offset : Offsets)
    {
        FIntPoint NeighborCoord(ChunkCoords.X + Offset.X, ChunkCoords.Y + Offset.Y);
        AVoxelChunk* NeighborChunk = nullptr;// = *VoxelWorld->Chunks.Find(NeighborCoord);
        if (VoxelWorld->Chunks.Contains(NeighborCoord))
        {
            NeighborChunk = *VoxelWorld->Chunks.Find(NeighborCoord);
        }
        else
        {
            bAllGenerated = false;
        }

        // If NeighborChunk doesnt already exist, then we need to create it
        if (NeighborChunk == nullptr)
        {
            bAllGenerated = false;
            // We need to create the chunk & subscribe to on data generated
            VoxelWorld->TryCreateNewChunk(NeighborCoord.X, NeighborCoord.Y, false);

            NeighborChunk = *VoxelWorld->Chunks.Find(NeighborCoord);
        }

        if (NeighborChunk != nullptr)
        {
            // If neighborchunk doesnt already have data, subscribe to get notified when it finishes
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
	if (!bGenerateMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("CALLED GENERATE CHUNK ASYNC BUT CHUNK IS NOT MARKED FOR GENERATION"));
		return;
	}

	if (!bHasData)
	{
		UE_LOG(LogTemp, Error, TEXT("CALLED GENERATE CHUNK ASYNC EVEN THOUGH WE DONT HAVE DATA YET"));
		return;
	}

	//UE_LOG(LogTemp, Warning, TEXT("CALLED GENERATE CHUNK ASYNC"));
	TWeakObjectPtr<AVoxelChunk> WeakThis(this);
	TWeakObjectPtr<AVoxelWorld> WeakVoxelWorld(VoxelWorld);
	const auto AsyncTraceTask =
		new FAutoDeleteAsyncTask<FVoxelMeshTask>(VoxelData, ChunkCoords, WeakVoxelWorld, WeakThis);
	AsyncTraceTask->StartBackgroundTask();
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

           UMaterialInterface* BaseMaterial = VoxelWorld->VoxelProperties[SectionKey.VoxelType].Material;
           // Check if the material is a dynamic material instance and set the TileCountX parameter  

           if (UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this))
           {  
               if (SectionKey.Normal.Z == 0)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].SideTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].SideTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == 1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].TopTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].TopTileOffset.Y);
               }
               else if (SectionKey.Normal.Z == -1)
               {
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetX"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].BottomTileOffset.X);
                   MaterialInstance->SetScalarParameterValue(TEXT("TileOffsetY"), VoxelWorld->VoxelProperties[SectionKey.VoxelType].BottomTileOffset.Y);
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
    if (!IsValid(VoxelWorld)) return false;
	// returns true if the voxel is within the chunk bounds
    return (X >= 0 && X < VoxelWorld->ChunkSize &&
        Y >= 0 && Y < VoxelWorld->ChunkSize &&
        Z >= 0 && Z < VoxelWorld->ChunkHeight);
}

/// <returns>Returns true when voxel is transparent or outside of the chunk</returns>
bool AVoxelChunk::CheckVoxel(int X, int Y, int Z, FIntPoint ChunkCoord)
{
    if (!IsValid(VoxelWorld)) return false;
    if (IsVoxelInChunk(X, Y, Z))
    {
        if (!IsValid(VoxelWorld)) return false;
        int16 NeighborType = VoxelData[(Z * VoxelWorld->ChunkSize * VoxelWorld->ChunkSize) + (Y * VoxelWorld->ChunkSize) + X];
        return AVoxelWorld::VoxelProperties[NeighborType].bIsTransparent;
    }
    else
    {
        if (!IsValid(VoxelWorld)) return false;
        int WorldX = ChunkCoord.X * VoxelWorld->ChunkSize + X;
        int WorldY = ChunkCoord.Y * VoxelWorld->ChunkSize + Y;
        int16 NeighborType = VoxelWorld->GetVoxelAtWorldCoordinates(WorldX, WorldY, Z);
        return AVoxelWorld::VoxelProperties[NeighborType].bIsTransparent;
    }
}

void AVoxelChunk::SetChunkCoords(FIntPoint InCoords)
{
    ChunkCoords = InCoords;
}
