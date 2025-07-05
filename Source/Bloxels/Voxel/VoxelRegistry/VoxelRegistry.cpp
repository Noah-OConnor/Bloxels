// Copyright 2025 Noah O'Connor. All rights reserved.

#include "VoxelRegistry.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UVoxelRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const FString TargetPath = TEXT("/Game/Bloxels/VoxelDataAssets");

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByPath(FName(*TargetPath), AssetDataList, true);

    //UE_LOG(LogTemp, Warning, TEXT("Found %d voxel assets"), AssetDataList.Num());

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    for (const FAssetData& AssetData : AssetDataList)
    {
        // Build a soft object path
        FSoftObjectPath AssetPath = AssetData.ToSoftObjectPath();

        // Log the path
        //UE_LOG(LogTemp, Display, TEXT("Attempting to load: %s"), *AssetPath.ToString());

        // Load synchronously
        UObject* RawObject = Streamable.LoadSynchronous(AssetPath, false); // false = don't log warnings internally

        if (!RawObject)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load voxel asset: %s"), *AssetPath.ToString());
            continue;
        }

        UVoxelData* Voxel = Cast<UVoxelData>(RawObject);
        if (!Voxel)
        {
            UE_LOG(LogTemp, Error, TEXT("Loaded object is not a UVoxelData: %s"), *AssetPath.ToString());
            continue;
        }

        // Register it
        RegisterVoxel(Voxel);

    }
}


void UVoxelRegistry::RegisterVoxel(UVoxelData* Voxel)
{
    if (!Voxel || !Voxel->VoxelID.IsValid()) return;

    const FName VoxelName = Voxel->VoxelID;
    const uint16 ID = static_cast<uint16>(VoxelMap.Num());

    VoxelMap.Add(VoxelName, Voxel);
    VoxelAssets.Add(Voxel);

    NameToID.Add(VoxelName, ID);
    IDToName.Add(ID, VoxelName);
    IDToVoxel.Add(ID, Voxel);
}


uint16 UVoxelRegistry::GetIDFromName(FName Name) const
{
    if (const uint16* ID = NameToID.Find(Name))
        return *ID;
    return 0; // Default/fallback
}

FName UVoxelRegistry::GetNameFromID(uint16 ID) const
{
    if (const FName* Name = IDToName.Find(ID))
        return *Name;
    return NAME_None;
}

UVoxelData* UVoxelRegistry::GetVoxelByID(uint16 ID) const
{
    if (UVoxelData* const* Voxel = IDToVoxel.Find(ID))
        return *Voxel;
    return nullptr;
}

