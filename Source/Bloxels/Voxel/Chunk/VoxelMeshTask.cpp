#include "VoxelMeshTask.h"
#include "VoxelChunk.h"
#include "Bloxels/Voxel/Core/VoxelInfo.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "Bloxels/Voxel/Core/MeshData.h"
#include "Bloxels/Voxel/Core/MeshSectionKey.h"

static const FVector Normal_Top    = FVector(0, 0, 1);
static const FVector Normal_Bottom = FVector(0, 0, -1);
static const FVector Normal_Front  = FVector(0, 1, 0);
static const FVector Normal_Back   = FVector(0, -1, 0);
static const FVector Normal_Right  = FVector(1, 0, 0);
static const FVector Normal_Left   = FVector(-1, 0, 0);

FVoxelMeshTask::FVoxelMeshTask(const TArray<uint16>& InVoxelData, const FIntPoint& InChunkCoords,
    const TWeakObjectPtr<AVoxelWorld> InVoxelWorld, const TWeakObjectPtr<AVoxelChunk> InVoxelChunk)
    : VoxelData(InVoxelData), ChunkCoords(InChunkCoords), 
    VoxelWorld(InVoxelWorld), VoxelChunk(InVoxelChunk) {
}

TStatId FVoxelMeshTask::GetStatId()
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FVoxelMeshTask, STATGROUP_ThreadPoolAsyncTasks);
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
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;

    const int ChunkHeight = VoxelWorld->ChunkHeight;
    const int ChunkSize = VoxelWorld->ChunkSize;

    // PROCESS THE XY+ FACE | Top
    ProcessFace(
    ChunkHeight, ChunkSize, ChunkSize, Normal_Top,
    [=, this](const int X, const int Y, const int Z) { return GetIndex(X, Y, Z); },
    [=, this](const int X, const int Y, const int Z) { return VoxelChunk->CheckVoxel(X, Y, Z + 1, ChunkCoords); },
    [=](const int X, const int Y, const int Z) { return FVector(X, Y, Z); },
    MeshSections);

    // PROCESS THE XY- FACE | Bottom
    ProcessFace(
    ChunkHeight, ChunkSize, ChunkSize, Normal_Bottom,
    [=, this](const int X, const int Y, const int Z) { return GetIndex(X, Y, Z); },
    [=, this](const int X, const int Y, const int Z) { return VoxelChunk->CheckVoxel(X, Y, Z - 1, ChunkCoords); },
    [=](const int X, const int Y, const int Z) { return FVector(X, Y, Z); },
    MeshSections);

    // PROCESS THE XZ+ FACE | Front
    ProcessFace(
    ChunkSize, ChunkSize, ChunkHeight, Normal_Front,
    [=, this](const int Y, const int Z, const int X) { return GetIndex(X, Y, Z); },
    [=, this](const int Y, const int Z, const int X) { return VoxelChunk->CheckVoxel(X, Y + 1, Z, ChunkCoords); },
    [=](const int Y, const int Z, const int X) { return FVector(X, Y, Z); },
    MeshSections);

    // PROCESS THE XZ- FACE | Back
    ProcessFace(
    ChunkSize, ChunkSize, ChunkHeight, Normal_Back,
    [=, this](const int Y, const int Z, const int X) { return GetIndex(X, Y, Z); },
    [=, this](const int Y, const int Z, const int X) { return VoxelChunk->CheckVoxel(X, Y - 1, Z, ChunkCoords); },
    [=](const int Y, const int Z, const int X) { return FVector(X, Y, Z); },
    MeshSections);

    // PROCESS THE YZ+ FACE | Right
    ProcessFace(
    ChunkSize, ChunkSize, ChunkHeight, Normal_Right,
    [=, this](const int Y, const int Z, const int X) { return GetIndex(X, Y, Z); },
    [=, this](const int Y, const int Z, const int X) { return VoxelChunk->CheckVoxel(X + 1, Y, Z, ChunkCoords); },
    [=](const int Y, const int Z, const int X) { return FVector(X + 1, Y, Z); },
    MeshSections);

    // PROCESS THE YZ- FACE | Left
    ProcessFace(
    ChunkSize, ChunkSize, ChunkHeight, Normal_Left,
    [=, this](const int Y, const int Z, const int X) { return GetIndex(X, Y, Z); },
    [=, this](const int Y, const int Z, const int X) { return VoxelChunk->CheckVoxel(X - 1, Y, Z, ChunkCoords); },
    [=](const int Y, const int Z, const int X) { return FVector(X, Y, Z); },
    MeshSections);

}

void FVoxelMeshTask::ProcessFace(
    const int PrimaryCount, const int ACount, const int BCount,
    const FVector& Normal,
    const std::function<int(int, int, int)>& GetVoxelIndex,
    const std::function<bool(int, int, int)>& IsNeighborVisible,
    const std::function<FVector(int, int, int)>& GetVoxelPosition,
    TMap<FMeshSectionKey, FMeshData>& MeshSections)
{
    if (!VoxelWorld.IsValid() || !VoxelChunk.IsValid()) return;  // Check validity before using it

    // Mask for face visibility and voxel types
    TArray<TArray<FVoxelFace>> Mask;
    Mask.SetNum(ACount);
    for (int A = 0; A < ACount; A++)
    {
        Mask[A].SetNum(BCount);
    }

    for (int P = 0; P < PrimaryCount; P++)
    {
        // Reset mask
        for (int A = 0; A < ACount; A++)
            for (int B = 0; B < BCount; B++)
                Mask[A][B] = {};

        // Fill mask
        for (int A = 0; A < ACount; A++)
        {
            for (int B = 0; B < BCount; B++)
            {
                const int Index = GetVoxelIndex(A, B, P);
                if (Index < 0 || Index >= VoxelData.Num())
                {
                    UE_LOG(LogTemp, Error, TEXT("Index out of bounds: %d"), Index);
                    continue;
                }

                if (const int16 VoxelType = VoxelData[Index]; VoxelWorld->VoxelProperties[VoxelType].bIsSolid && IsNeighborVisible(A, B, P))
                {
                    Mask[A][B] = { VoxelType, true };
                }
            }
        }

        // Greedy meshing
        for (int A = 0; A < ACount; A++)
        {
            for (int B = 0; B < BCount; B++)
            {
                if (!Mask[A][B].bVisible) continue;

                int Width = 1;
                while (A + Width < ACount &&
                       Mask[A + Width][B].bVisible &&
                       Mask[A + Width][B].VoxelType == Mask[A][B].VoxelType)
                {
                    ++Width;
                }

                int Height = 1;
                bool Expand = true;
                while (B + Height < BCount && Expand)
                {
                    for (int i = 0; i < Width; ++i)
                    {
                        if (!Mask[A + i][B + Height].bVisible ||
                            Mask[A + i][B + Height].VoxelType != Mask[A][B].VoxelType)
                        {
                            Expand = false;
                            break;
                        }
                    }
                    if (Expand) ++Height;
                }

                // Add merged face
                FMeshSectionKey Key(Mask[A][B].VoxelType, Normal);
                FMeshData& MeshData = MeshSections.FindOrAdd(Key);
                AddMergedFace(GetVoxelPosition(A, B, P), MeshData.Vertices, MeshData.Triangles,
                              MeshData.Normals, MeshData.UVs, Normal, Width, Height);

                // Mark merged mask area as processed
                for (int i = 0; i < Width; ++i)
                    for (int j = 0; j < Height; ++j)
                        Mask[A + i][B + j].bVisible = false;
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
    int32 Height) const
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

int FVoxelMeshTask::GetIndex(const int X, const int Y, const int Z) const
{
    const int ChunkSize = VoxelWorld->ChunkSize;
    return (Z * ChunkSize * ChunkSize) + (Y * ChunkSize) + X;
}