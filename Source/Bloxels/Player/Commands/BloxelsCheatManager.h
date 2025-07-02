// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "BloxelsCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class BLOXELS_API UBloxelsCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void SelectPositionCoordinates(int32 Index, float X, float Y, float Z);

	UFUNCTION(Exec)
	void SelectPositionLookAt(int32 Index, bool bOffset = false);

	UFUNCTION(Exec)
	void SelectPositionHead(int32 Index);

	UFUNCTION(Exec)
	void ClearSelection(int32 Index);

	UFUNCTION(Exec)
	void ClearAllSelections();

private:
	void SetPosition(int32 Index, const FVector& Pos);
};
