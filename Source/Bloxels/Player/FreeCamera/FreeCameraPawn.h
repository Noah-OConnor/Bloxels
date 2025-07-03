// Copyright 2025 Noah O'Connor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "FreeCameraPawn.generated.h"

UCLASS()
class BLOXELS_API AFreeCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AFreeCameraPawn();

	UCameraComponent* GetCamera() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class UFloatingPawnMovement* Movement;

	UPROPERTY()
	AVoxelWorld* VoxelWorld;

	UFUNCTION()
	void OnLeftClick();

	UFUNCTION()
	void OnRightClick();

	// Movement input
	FVector2D MoveInput;
	float VerticalInput;
	FVector2D MouseInput;

	// Sensitivity
	float LookSensitivity = 1.5f;

	// Camera rotation state
	float Pitch = 0.0f;
	float Yaw = 0.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void Turn(float Value);
	void LookUp(float Value);
};
