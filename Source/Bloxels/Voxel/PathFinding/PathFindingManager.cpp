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

		const int ChunkSize = VoxelWorld->ChunkSize;
		const int ChunkHeight = VoxelWorld->ChunkHeight;
		const FIntPoint ChunkCoords = Chunk->ChunkCoords;

		for (int X = 0; X < ChunkSize; ++X)
			for (int Y = 0; Y < ChunkSize; ++Y)
				for (int Z = 0; Z < ChunkHeight; ++Z)
				{
					const int Index = (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
					const uint16 VoxelID = Chunk->VoxelData[Index];

					// World position
					const int WorldX = ChunkCoords.X * ChunkSize + X;
					const int WorldY = ChunkCoords.Y * ChunkSize + Y;
					FIntVector WorldCoord(WorldX, WorldY, Z);

					const FVoxelData& VoxelDef = AVoxelWorld::VoxelProperties[VoxelID];
					bool bIsWalkable = false;

					if (VoxelDef.bIsSolid)
					{
						// Solid voxel is walkable if the space above is empty
						if (Z + 1 < ChunkHeight)
						{
							int AboveIndex = Index + (ChunkSize * ChunkSize);
							const auto& AboveDef = Chunk->VoxelData[AboveIndex];
							const auto& AboveVoxelDef = VoxelWorld->VoxelProperties[AboveDef];
					
							bIsWalkable = !AboveVoxelDef.bIsSolid;
						}
					}
					if (!VoxelDef.bIsSolid)
					{
						// Air block is walkable if block below is solid (for fallback compatibility)
						if (Z - 1 >= 0)
						{
							int BelowIndex = Index - (ChunkSize * ChunkSize);
							const auto& BelowDef = Chunk->VoxelData[BelowIndex];
							const auto& BelowVoxelDef = VoxelWorld->VoxelProperties[BelowDef];

							bIsWalkable = BelowVoxelDef.bIsSolid;
						}
					}
					
					auto Node = MakeShared<FPathFindingNode>();
					Node->X = WorldX;
					Node->Y = WorldY;
					Node->Z = Z;
					Node->Position = FVector(WorldX * VoxelWorld->VoxelSize, WorldY * VoxelWorld->VoxelSize, Z * VoxelWorld->VoxelSize);
					Node->bIsWalkable = bIsWalkable;
					Node->bIsSolid = VoxelDef.bIsSolid;

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

	const FIntVector Center(Node->X, Node->Y, Node->Z);

	// All 8 horizontal directions
	const TArray<FIntVector> HorizontalOffsets = {
		{  1,  0,  0 },
		{ -1,  0,  0 },
		{  0,  1,  0 },
		{  0, -1,  0 },
		{  1,  1,  0 },
		{ -1,  1,  0 },
		{  1, -1,  0 },
		{ -1, -1,  0 }
	};

	// Vertical: only pure up/down
	const TArray<FIntVector> VerticalOffsets = {
		{ 0, 0,  1 },
		{ 0, 0, -1 }
	};


	TArray<FIntVector> AllOffsets = HorizontalOffsets;
	AllOffsets.Append(VerticalOffsets);


	for (const FIntVector& Offset : AllOffsets)
	{
		const FIntVector Coord = Center + Offset;
		TSharedPtr<FPathFindingNode> Neighbor = GetNodeAt(Coord);

		if (!Neighbor.IsValid())
			continue;

		if (Offset.Z == 0)
		{
			if (Neighbor->bIsWalkable)
			{
				Neighbors.Add(Neighbor);
			}
			else
			{
				const FIntVector UpCoord = Coord + FIntVector(0, 0, 1);
				TSharedPtr<FPathFindingNode> UpNode = GetNodeAt(UpCoord);

				if (Neighbor->bIsSolid && UpNode.IsValid() && UpNode->bIsWalkable)
				{
					Neighbors.Add(UpNode); // Step/climb up
				}
			}

			const FIntVector DownCoord = Coord + FIntVector(0, 0, -1);
			TSharedPtr<FPathFindingNode> DownNode = GetNodeAt(DownCoord);

			if (DownNode.IsValid() && DownNode->bIsWalkable)
			{
				Neighbors.Add(DownNode); // Fall
			}
		}

		if (Offset.X == 0 && Offset.Y == 0)
		{
			if (Neighbor->bIsWalkable)
			{
				Neighbors.Add(Neighbor);
			}
		}
	}




	return Neighbors;
}

TArray<FVector> APathfindingManager::FindPath(const FVector& WorldStart, const FVector& WorldEnd)
{
	const FIntVector StartCoord = FIntVector(
        FMath::FloorToInt(WorldStart.X / 100.f),
        FMath::FloorToInt(WorldStart.Y / 100.f),
        FMath::FloorToInt(WorldStart.Z / 100.f)
    );

    FIntVector EndCoord = FIntVector(
        FMath::FloorToInt(WorldEnd.X / 100.f),
        FMath::FloorToInt(WorldEnd.Y / 100.f),
        FMath::FloorToInt(WorldEnd.Z / 100.f)
    );

    const TSharedPtr<FPathFindingNode> StartNode = GetNodeAt(StartCoord);
    const TSharedPtr<FPathFindingNode> EndNode = GetNodeAt(EndCoord);

    if (!StartNode || !EndNode || !StartNode->bIsWalkable || !EndNode->bIsWalkable)
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or End node invalid or unwalkable."));
        return {};
    }

	// Reset traversal state
	for (auto& Elem : NodeMap)
	{
		const TSharedPtr<FPathFindingNode>& Node = Elem.Value;
		Node->GCost = 0.f;
		Node->HCost = 0.f;
		Node->Parent.Reset();
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

            if (const float NewGCost = Current->GCost + FVector::Dist(Current->Position, Neighbor->Position); !OpenSet.Contains(Neighbor) || NewGCost < Neighbor->GCost)
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
