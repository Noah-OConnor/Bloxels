#include "VoxelMeshTask.h"
#include "VoxelChunk.h"
#include "Bloxels/Voxel/Core/VoxelInfo.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"

FVoxelMeshTask::FVoxelMeshTask(TArray<uint16>& InVoxelData, FIntPoint& InChunkCoords, 
    TWeakObjectPtr<AVoxelWorld> InVoxelWorld, TWeakObjectPtr<AVoxelChunk> InVoxelChunk)
    : VoxelData(InVoxelData), ChunkCoords(InChunkCoords), 
    VoxelWorld(InVoxelWorld), VoxelChunk(InVoxelChunk) {
}

void FVoxelMeshTask::DoWork()
{
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it

    // Perform greedy meshing algorithm
    TMap<FMeshSectionKey, FMeshData> MeshSections;
    GreedyMeshChunk(MeshSections);

    const FIntPoint ChunkCoordsCopy = ChunkCoords;

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
    // Send mesh data back to the main thread
    AsyncTask(ENamedThreads::GameThread, [WeakChunk = VoxelChunk, ChunkCoordsCopy, MeshSections]()
    {
        if (!WeakChunk.IsValid()) return; // Ensure validity before calling the function
        WeakChunk->SetChunkCoords(ChunkCoordsCopy);  // Ensure it has the correct coords
        WeakChunk->OnMeshGenerated(MeshSections);
    });
}

void FVoxelMeshTask::GreedyMeshChunk(TMap<FMeshSectionKey, FMeshData>& MeshSections)
{
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
    //// Process all faces, but store data separately for each voxel type
    ProcessXYFace(0, 1, 2, FVector(0, 0, 1), [&](int x, int y, int z) { return VoxelChunk->CheckVoxel(x, y, z + 1, ChunkCoords); },
        [&](int x, int y, int z) { return FVector(x, y, z); }, MeshSections); // Top

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
    ProcessXYFace(0, 1, 2, FVector(0, 0, -1), [&](int x, int y, int z) { return VoxelChunk->CheckVoxel(x, y, z - 1, ChunkCoords); },
        [&](int x, int y, int z) { return FVector(x, y, z); }, MeshSections); // Bot

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
    ProcessXZFace(0, 2, 1, FVector(0, 1, 0), [&](int x, int y, int z) { return VoxelChunk->CheckVoxel(x, y + 1, z, ChunkCoords); },
        [&](int x, int y, int z) { return FVector(x, y, z); }, MeshSections); // Front

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
    ProcessXZFace(0, 2, 1, FVector(0, -1, 0), [&](int x, int y, int z) { return VoxelChunk->CheckVoxel(x, y - 1, z, ChunkCoords); },
        [&](int x, int y, int z) { return FVector(x, y, z); }, MeshSections); // Back

    ProcessYZFace(1, MeshSections, ChunkCoords);  // Right Face
    ProcessYZFace(-1, MeshSections, ChunkCoords); // Left Face
}

void FVoxelMeshTask::ProcessXYFace(
    int Axis1, int Axis2, int Axis3,
    FVector Normal,
    std::function<bool(int, int, int)> CheckNeighbor,
    std::function<FVector(int, int, int)> GetVoxelPosition,
    TMap<FMeshSectionKey, FMeshData>& MeshSections)
{
    // Implementation of ProcessXYFace
    struct VoxelFace
    {
        int16 VoxelType;
        bool bVisible;
    };

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    int ChunkSize = VoxelWorld->ChunkSize;
    int ChunkHeight = VoxelWorld->ChunkHeight;

    TArray<TArray<VoxelFace>> Mask;
    Mask.SetNum(ChunkSize);
    for (int i = 0; i < ChunkSize; i++)
    {
        Mask[i].SetNum(ChunkSize);
    }

    for (int a3 = 0; a3 < ChunkHeight; a3++) // Iterate along primary axis
    {
        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkSize; a2++)
            {
                Mask[a1][a2] = { 0, false }; // Initialize the mask
            }
        }

        // Populate the mask
        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkSize; a2++)
            {
                int Index = (a3 * ChunkSize * ChunkSize) + (a2 * ChunkSize) + a1;
                if (Index < 0 || Index >= VoxelData.Num())
                {
                    UE_LOG(LogTemp, Error, TEXT("Index out of bounds: %d"), Index);
                    continue;
                }

                int16 VoxelType = VoxelData[Index];

                if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
                if (AVoxelWorld::VoxelProperties[VoxelType].bIsSolid && CheckNeighbor(a1, a2, a3))
                {
                    Mask[a1][a2].VoxelType = VoxelType;
                    Mask[a1][a2].bVisible = true;
                }
            }
        }

        // Apply Greedy Meshing
        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkSize; a2++)
            {
                if (!Mask[a1][a2].bVisible) continue;

                int Width = 1;
                while (a1 + Width < ChunkSize &&
                    Mask[a1 + Width][a2].bVisible &&
                    Mask[a1 + Width][a2].VoxelType == Mask[a1][a2].VoxelType)
                {
                    Width++;
                }

                int Height = 1;
                bool Expand = true;
                while (a2 + Height < ChunkSize && Expand)
                {
                    for (int i = 0; i < Width; i++)
                    {
                        if (!Mask[a1 + i][a2 + Height].bVisible ||
                            Mask[a1 + i][a2 + Height].VoxelType != Mask[a1][a2].VoxelType)
                        {
                            Expand = false;
                            break;
                        }
                    }
                    if (Expand)
                        Height++;
                }

				FMeshSectionKey MeshKey = FMeshSectionKey(Mask[a1][a2].VoxelType, Normal);

                // Generate the merged face
                //int16 VoxelType = Mask[a1][a2].VoxelType;
                FMeshData& MeshData = MeshSections.FindOrAdd(MeshKey);
                AddMergedFace(GetVoxelPosition(a1, a2, a3), MeshData.Vertices, MeshData.Triangles,
                    MeshData.Normals, MeshData.UVs, Normal, Width, Height);

                // Clear processed mask area
                for (int i = 0; i < Width; i++)
                    for (int j = 0; j < Height; j++)
                        Mask[a1 + i][a2 + j].bVisible = false;
            }
        }
    }
}

void FVoxelMeshTask::ProcessXZFace(
    int Axis1, int Axis2, int Axis3,
    FVector Normal,
    std::function<bool(int, int, int)> CheckNeighbor,
    std::function<FVector(int, int, int)> GetVoxelPosition,
    TMap<FMeshSectionKey, FMeshData>& MeshSections)
{
    // Implementation of ProcessXZFace
    struct VoxelFace
    {
        int16 VoxelType;
        bool bVisible;
    };
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    int ChunkSize = VoxelWorld->ChunkSize;
    int ChunkHeight = VoxelWorld->ChunkHeight;

    TArray<TArray<VoxelFace>> Mask;
    Mask.SetNum(ChunkSize);
    for (int i = 0; i < ChunkSize; i++)
    {
        Mask[i].SetNum(ChunkHeight);
    }

    for (int a3 = 0; a3 < ChunkSize; a3++) // Iterate along primary axis
    {

        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkHeight; a2++)
            {
                Mask[a1][a2] = { 0, false }; // Initialize the mask
            }
        }
        // Populate the mask
        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkHeight; a2++)
            {
                int16 VoxelType = VoxelData[(a2 * ChunkSize * ChunkSize) + (a3 * ChunkSize) + a1];//[a1][a3][a2];

                if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
                if (AVoxelWorld::VoxelProperties[VoxelType].bIsSolid && CheckNeighbor(a1, a3, a2))
                {
                    Mask[a1][a2].VoxelType = VoxelType;
                    Mask[a1][a2].bVisible = true;
                }
            }
        }

        // Apply Greedy Meshing
        for (int a1 = 0; a1 < ChunkSize; a1++)
        {
            for (int a2 = 0; a2 < ChunkHeight; a2++)
            {
                if (!Mask[a1][a2].bVisible) continue;

                int Width = 1;
                while (a1 + Width < ChunkSize &&
                    Mask[a1 + Width][a2].bVisible &&
                    Mask[a1 + Width][a2].VoxelType == Mask[a1][a2].VoxelType)
                {
                    Width++;
                }

                int Height = 1;
                bool Expand = true;
                while (a2 + Height < ChunkHeight && Expand)
                {
                    for (int i = 0; i < Width; i++)
                    {
                        if (!Mask[a1 + i][a2 + Height].bVisible ||
                            Mask[a1 + i][a2 + Height].VoxelType != Mask[a1][a2].VoxelType)
                        {
                            Expand = false;
                            break;
                        }
                    }
                    if (Expand)
                        Height++;
                }

                FMeshSectionKey MeshKey = FMeshSectionKey(Mask[a1][a2].VoxelType, Normal);

                // Generate the merged face
                //int16 VoxelType = Mask[a1][a2].VoxelType;
                FMeshData& MeshData = MeshSections.FindOrAdd(MeshKey);
                AddMergedFace(GetVoxelPosition(a1, a3, a2), MeshData.Vertices, MeshData.Triangles,
                    MeshData.Normals, MeshData.UVs, Normal, Width, Height);

                // Clear processed mask area
                for (int i = 0; i < Width; i++)
                    for (int j = 0; j < Height; j++)
                        Mask[a1 + i][a2 + j].bVisible = false;
            }
        }
    }
}

void FVoxelMeshTask::ProcessYZFace(
    int Direction, // 1 for Right Face, -1 for Left Face
    TMap<FMeshSectionKey, FMeshData>& MeshSections, FIntPoint ChunkCoord)
{
    // Implementation of ProcessYZFace
    struct VoxelFace
    {
        int16 VoxelType;
        bool bVisible;
    };

    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    int ChunkSize = VoxelWorld->ChunkSize;
    int ChunkHeight = VoxelWorld->ChunkHeight;

    TArray<TArray<VoxelFace>> Mask;
    Mask.SetNum(ChunkSize);
    for (int i = 0; i < ChunkSize; i++)
    {
        Mask[i].SetNum(ChunkHeight);
    }

    for (int x = 0; x < ChunkSize; x++)
    {
        for (int y = 0; y < ChunkSize; y++)
        {
            for (int z = 0; z < ChunkHeight; z++)
            {
                Mask[y][z] = { 0, false }; // Initialize the mask
            }
        }

        // Fill the mask by checking visibility of each voxel face
        for (int y = 0; y < ChunkSize; y++)
        {
            for (int z = 0; z < ChunkHeight; z++)
            {
                int16 VoxelType = VoxelData[(z * ChunkSize * ChunkSize) + (y * ChunkSize) + x];//[x][y][z];

                if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it
                if (VoxelWorld->VoxelProperties[VoxelType].bIsSolid && VoxelChunk->CheckVoxel(x + Direction, y, z, ChunkCoord))
                {
                    Mask[y][z] = { VoxelType, true };
                }
            }
        }

        // Process the mask to merge quads
        for (int y = 0; y < ChunkSize; y++)
        {
            for (int z = 0; z < ChunkHeight; z++)
            {
                if (!Mask[y][z].bVisible) continue;

                int Width = 1;
                while (y + Width < ChunkSize && Mask[y + Width][z].bVisible && Mask[y + Width][z].VoxelType == Mask[y][z].VoxelType)
                {
                    Width++;
                }

                int Height = 1;
                bool Expand = true;
                while (z + Height < ChunkHeight && Expand)
                {
                    for (int i = 0; i < Width; i++)
                    {
                        if (!Mask[y + i][z + Height].bVisible || Mask[y + i][z + Height].VoxelType != Mask[y][z].VoxelType)
                        {
                            Expand = false;
                            break;
                        }
                    }
                    if (Expand) Height++;
                }

                // Compute face normal (YZ plane)
                FVector Normal = FVector(Direction, 0, 0); // Right Face: (1,0,0), Left Face: (-1,0,0)

                FMeshSectionKey MeshKey = FMeshSectionKey(Mask[y][z].VoxelType, Normal);

                // Get Voxel Type and the corresponding Mesh Data
                //int16 VoxelType = Mask[y][z].VoxelType;
                FMeshData& MeshData = MeshSections.FindOrAdd(MeshKey);

                // Compute voxel position based on face direction
                auto GetVoxelPosition = [&](int x, int y, int z) -> FVector
                    {
                        return FVector(x + (Direction == 1 ? 1 : 0), y, z);
                    };

                // Add merged quad face
                AddMergedFace(GetVoxelPosition(x, y, z), MeshData.Vertices, MeshData.Triangles,
                    MeshData.Normals, MeshData.UVs, Normal, Width, Height);

                // Mark merged faces as processed
                for (int i = 0; i < Width; i++)
                    for (int j = 0; j < Height; j++)
                        Mask[y + i][z + j].bVisible = false;
            }
        }
    }
}

void FVoxelMeshTask::AddMergedFace(
    FVector Position,
    TArray<FVector>& Vertices,
    TArray<int32>& Triangles,
    TArray<FVector>& Normals,
    TArray<FVector2D>& UVs,
    FVector Normal,
    int32 Width,
    int32 Height)
{
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;
    int VoxelSize = VoxelWorld->VoxelSize;

    int32 VertexIndex = Vertices.Num();

    FVector Right, Up;

    if (Normal == FVector(0, 0, 1)) // Top
    {
        Right = FVector(Width, 0, 0) * VoxelSize;
        Up = FVector(0, Height, 0) * VoxelSize;
        Position += Normal;
    }
    else if (Normal == FVector(0, 0, -1)) // Bottom
    {
        Right = FVector(Width, 0, 0) * VoxelSize;
        Up = FVector(0, Height, 0) * VoxelSize;
    }
    else if (Normal == FVector(0, 1, 0)) // Front
    {
        Right = FVector(Width, 0, 0) * VoxelSize;
        Up = FVector(0, 0, Height) * VoxelSize;
        Position += Normal;
    }
    else if (Normal == FVector(0, -1, 0)) // Back
    {
        Right = FVector(Width, 0, 0) * VoxelSize;
        Up = FVector(0, 0, Height) * VoxelSize;
    }
    else if (Normal == FVector(1, 0, 0)) // Right
    {
        Right = FVector(0, Width, 0) * VoxelSize;
        Up = FVector(0, 0, Height) * VoxelSize;
        //Position += Normal;
    }
    else if (Normal == FVector(-1, 0, 0)) // Left
    {
        Right = FVector(0, Width, 0) * VoxelSize;
        Up = FVector(0, 0, Height) * VoxelSize;
    }

    FVector V0 = Position * VoxelSize;
    FVector V1 = V0 + Right;
    FVector V2 = V0 + Right + Up;
    FVector V3 = V0 + Up;

    Vertices.Append({ V0, V1, V2, V3 });

    if (Normal == FVector(0, 0, 1) || Normal == FVector(0, -1, 0) || Normal == FVector(1, 0, 0))
    {
        // Reverse the order of vertices for bottom, back, and left faces
        Triangles.Append({
            VertexIndex,
            VertexIndex + 2,
            VertexIndex + 1,
            VertexIndex,
            VertexIndex + 3,
            VertexIndex + 2
            });
    }
    else
    {
        Triangles.Append({
            VertexIndex,
            VertexIndex + 1,
            VertexIndex + 2,
            VertexIndex,
            VertexIndex + 2,
            VertexIndex + 3
            });
    }

    for (int i = 0; i < 4; i++)
    {
        Normals.Add(Normal);
    }

    UVs.Append({
        FVector2D(0.0f, static_cast<float>(Height)),
        FVector2D(static_cast<float>(Width), static_cast<float>(Height)),
        FVector2D(static_cast<float>(Width), 0.0f),
        FVector2D(0.0f, 0.0f)
        });
}