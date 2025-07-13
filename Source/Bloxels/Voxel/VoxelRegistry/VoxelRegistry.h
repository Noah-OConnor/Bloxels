// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Bloxels/Voxel/Core/VoxelData.h"
#include "VoxelRegistry.generated.h"

UCLASS()
class BLOXELS_API UVoxelRegistry : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RegisterVoxel(UVoxelData* Voxel);

    uint16 GetIDFromName(FName Name) const;
    FName GetNameFromID(uint16 ID) const;
    UVoxelData* GetVoxelByID(uint16 ID) const;
    
    int32 GetVoxelCount() const { return VoxelAssets.Num(); }

    
    UPROPERTY(EditAnywhere)
    TArray<UVoxelData*> VoxelAssets;

private:
    UPROPERTY()
    TMap<FName, UVoxelData*> VoxelMap;

    UPROPERTY()
    TMap<uint16, FName> IDToName;

    UPROPERTY()
    TMap<FName, uint16> NameToID;

    UPROPERTY()
    TMap<uint16, UVoxelData*> IDToVoxel;
};