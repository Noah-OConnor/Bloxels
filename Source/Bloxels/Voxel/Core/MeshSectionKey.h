// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"

struct FMeshSectionKey
{
    uint16 VoxelType;
    FVector Normal;

    FMeshSectionKey()
        : VoxelType(0), Normal(FVector::ZeroVector)
    {
    }

    FMeshSectionKey(int16 InVoxelType, const FVector& InNormal)
        : VoxelType(InVoxelType), Normal(InNormal)
    {
    }

    bool operator==(const FMeshSectionKey& Other) const
    {
        return VoxelType == Other.VoxelType && Normal.Equals(Other.Normal, 0.01f); // Tolerance!
    }

    friend int16 GetTypeHash(const FMeshSectionKey& Key)
    {
        return HashCombine(::GetTypeHash(Key.VoxelType), 
            HashCombine(HashCombine(::GetTypeHash(Key.Normal.X), ::GetTypeHash(Key.Normal.Y)), ::GetTypeHash(Key.Normal.Z)));
    }

};
