// Copyright 2025 Noah O'Connor. All rights reserved.


#include "VoxelWorld.h"

#include "Bloxels/Voxel/Chunk/VoxelChunk.h"
#include "Bloxels/Voxel/Core/VoxelConfig.h"
#include "Bloxels/Voxel/Core/VoxelUtilities.h"
#include "Kismet/GameplayStatics.h"


AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AVoxelWorld::BeginPlay()
{
	Super::BeginPlay();

	if (!VoxelConfig || !ChunkClass)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelWorld is missing VoxelConfig or ChunkClass!"));
		return;
	}

	GenerateInitialChunks();
}

void AVoxelWorld::GenerateInitialChunks()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	const FVector PlayerLoc = Player->GetActorLocation();
	const FIntVector PlayerChunk = VoxelUtils::WorldToChunkCoords(
		PlayerLoc,
		VoxelConfig->VoxelSize,
		VoxelConfig->ChunkSize
	);

	for (int32 X = -LoadRadius; X <= LoadRadius; X++)
		for (int32 Y = -LoadRadius; Y <= LoadRadius; Y++)
			for (int32 Z = -LoadRadius; Z <= LoadRadius; Z++)
			{
				FIntVector ChunkCoords = PlayerChunk + FIntVector(X, Y, Z);
				SpawnChunkAt(ChunkCoords);
			}
}

void AVoxelWorld::SpawnChunkAt(FIntVector ChunkCoords)
{
	if (Chunks.Contains(ChunkCoords)) return;

	FActorSpawnParameters Params;
	Params.Owner = this;

	AVoxelChunk* NewChunk = GetWorld()->SpawnActor<AVoxelChunk>(
		ChunkClass,
		FVector::ZeroVector, // Will be set during Initialize()
		FRotator::ZeroRotator,
		Params
	);

	if (!NewChunk) return;

	NewChunk->Initialize(ChunkCoords, VoxelConfig);
	Chunks.Add(ChunkCoords, NewChunk);
}