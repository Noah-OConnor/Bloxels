// Copyright 2025 Bloxels. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "BloxelsCheatManager.generated.h"

class UDebugSubsystem;
/**
 * 
 */
UCLASS()
class BLOXELS_API UBloxelsCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// Block Selection Commands
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

	// Structure I/O
	UFUNCTION(Exec)
	void ExportSelection(const FString& FileName);

	UFUNCTION(Exec)
	void ImportStructure(const FString& FileName);

	// Pathfinding Commands
	UFUNCTION(Exec)
	void SetPathStartLookAt(bool bOffset = false);

	UFUNCTION(Exec)
	void SetPathEndLookAt(bool bOffset = false);

	UFUNCTION(Exec)
	void ClearPathStart();

	UFUNCTION(Exec)
	void ClearPathEnd();

private:
	// Block Selection Helpers
	void SetPosition(int32 Index, const FVector& Pos);

	// Pathfinding Helpers
	FVector GetLookAt(bool bReturnNormal);

	void GeneratePathDebug(UDebugSubsystem* Debug);
};
