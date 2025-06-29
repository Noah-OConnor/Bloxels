#include "PathfindingManager.h"
#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"

void APathfindingManager::BuildPathfindingNodes(AVoxelWorld* VoxelWorld)
{
	NodeMap.Empty();

	for (const TPair<FIntPoint, AVoxelChunk*>& ChunkPair : VoxelWorld->Chunks)
	{
		AVoxelChunk* Chunk = ChunkPair.Value;
		if (!Chunk || !Chunk->bHasData) continue;

		int ChunkSize = VoxelWorld->ChunkSize;
		int ChunkHeight = VoxelWorld->ChunkHeight;
		FIntPoint ChunkCoords = Chunk->ChunkCoords;

		for (int X = 0; X < ChunkSize; ++X)
			for (int Y = 0; Y < ChunkSize; ++Y)
				for (int Z = 0; Z < ChunkHeight; ++Z)
				{
					int Index = (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
					uint16 VoxelID = Chunk->VoxelData[Index];

					// World position
					int WorldX = ChunkCoords.X * ChunkSize + X;
					int WorldY = ChunkCoords.Y * ChunkSize + Y;
					FIntVector WorldCoord(WorldX, WorldY, Z);

					const FVoxelData& VoxelDef = AVoxelWorld::VoxelProperties[VoxelID];
					bool bIsWalkable = false;

					if (!VoxelDef.bIsSolid)
					{
						int BelowZ = Z - 1;
						if (BelowZ >= 0)
						{
							int BelowIndex = (BelowZ * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
							uint16 BelowVoxelID = Chunk->VoxelData[BelowIndex];
							const FVoxelData& BelowVoxelDef = AVoxelWorld::VoxelProperties[BelowVoxelID];

							bIsWalkable = BelowVoxelDef.bIsSolid;
						}
					}


					auto Node = MakeShared<FPathFindingNode>();
					Node->X = WorldX;
					Node->Y = WorldY;
					Node->Z = Z;
					Node->Position = FVector(WorldX * VoxelWorld->VoxelSize, WorldY * VoxelWorld->VoxelSize, Z * VoxelWorld->VoxelSize);
					Node->bIsWalkable = bIsWalkable;

					NodeMap.Add(WorldCoord, Node);
				}
	}

	UE_LOG(LogTemp, Log, TEXT("Pathfinding grid built with %d nodes."), NodeMap.Num());
}

TSharedPtr<FPathFindingNode> APathfindingManager::GetNodeAt(FIntVector WorldCoord)
{
	if (const TSharedPtr<FPathFindingNode>* Found = NodeMap.Find(WorldCoord))
		return *Found;

	return nullptr;
}

TArray<TSharedPtr<FPathFindingNode>> APathfindingManager::GetNeighbors(const TSharedPtr<FPathFindingNode>& Node)
{
	TArray<TSharedPtr<FPathFindingNode>> Neighbors;

	for (int32 DX = -1; DX <= 1; ++DX)
		for (int32 DY = -1; DY <= 1; ++DY)
			for (int32 DZ = -1; DZ <= 1; ++DZ)
			{
				if (DX == 0 && DY == 0 && DZ == 0) continue;

				FIntVector NeighborCoord(Node->X + DX, Node->Y + DY, Node->Z + DZ);
				if (TSharedPtr<FPathFindingNode> Neighbor = GetNodeAt(NeighborCoord))
				{
					if (Neighbor->bIsWalkable)
						Neighbors.Add(Neighbor);
				}
			}

	return Neighbors;
}

TArray<FVector> APathfindingManager::FindPath(FVector WorldStart, FVector WorldEnd)
{
	FIntVector StartCoord = FIntVector(
        FMath::FloorToInt(WorldStart.X / 100.f),
        FMath::FloorToInt(WorldStart.Y / 100.f),
        FMath::FloorToInt(WorldStart.Z / 100.f)
    );

    FIntVector EndCoord = FIntVector(
        FMath::FloorToInt(WorldEnd.X / 100.f),
        FMath::FloorToInt(WorldEnd.Y / 100.f),
        FMath::FloorToInt(WorldEnd.Z / 100.f)
    );

    TSharedPtr<FPathFindingNode> StartNode = GetNodeAt(StartCoord);
    TSharedPtr<FPathFindingNode> EndNode = GetNodeAt(EndCoord);

    if (!StartNode || !EndNode || !StartNode->bIsWalkable || !EndNode->bIsWalkable)
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or End node invalid or unwalkable."));
        return {};
    }

    TArray<TSharedPtr<FPathFindingNode>> OpenSet;
    TSet<TSharedPtr<FPathFindingNode>> ClosedSet;

    OpenSet.Add(StartNode);

    while (OpenSet.Num() > 0)
    {
        // Sort by lowest FCost
        OpenSet.Sort([](const TSharedPtr<FPathFindingNode>& A, const TSharedPtr<FPathFindingNode>& B)
        {
            return A->FCost() < B->FCost();
        });

        TSharedPtr<FPathFindingNode> Current = OpenSet[0];
        OpenSet.RemoveAt(0);
        ClosedSet.Add(Current);

        if (Current == EndNode)
        {
            // Path found, retrace
            TArray<FVector> Path;
            for (auto Node = Current; Node.IsValid(); Node = Node->Parent.Pin())
            {
                Path.Insert(Node->Position, 0);
            }
            return Path;
        }

        for (TSharedPtr<FPathFindingNode> Neighbor : GetNeighbors(Current))
        {
            if (ClosedSet.Contains(Neighbor)) continue;

            float NewGCost = Current->GCost + FVector::Dist(Current->Position, Neighbor->Position);

            if (!OpenSet.Contains(Neighbor) || NewGCost < Neighbor->GCost)
            {
                Neighbor->GCost = NewGCost;
                Neighbor->HCost = FVector::Dist(Neighbor->Position, EndNode->Position);
                Neighbor->Parent = Current;

                if (!OpenSet.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }
    }

    return {}; // No path found
}
