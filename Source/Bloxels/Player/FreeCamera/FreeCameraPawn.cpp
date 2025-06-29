// Copyright 2025 Noah O'Connor. All rights reserved.

#include "FreeCameraPawn.h"

#include "EngineUtils.h"
#include "Bloxels/Voxel/PathFinding/PathFindingNode.h"
#include "Bloxels/Voxel/World/VoxelWorld.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/InputComponent.h"

AFreeCameraPawn::AFreeCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 2000.f;
}

void AFreeCameraPawn::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    if (!PathManager)
    {
        for (TActorIterator<APathfindingManager> It(GetWorld()); It; ++It)
        {
            PathManager = *It;
            break;
        }
    }
}

void AFreeCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update rotation from mouse
    Yaw += MouseInput.X * LookSensitivity;
    Pitch = FMath::Clamp(Pitch - MouseInput.Y * LookSensitivity, -89.f, 89.f);

    FRotator NewRotation = FRotator(Pitch, Yaw, 0.f);
    Camera->SetWorldRotation(NewRotation);

    // Apply movement
    FVector Forward = Camera->GetForwardVector();
    FVector Right = Camera->GetRightVector();
    FVector Up = FVector::UpVector;

    FVector MoveDirection = Forward * MoveInput.Y + Right * MoveInput.X + Up * VerticalInput;
    if (!MoveDirection.IsNearlyZero())
    {
        AddMovementInput(MoveDirection.GetSafeNormal());
    }

    // Clear mouse input after applying
    MouseInput = FVector2D::ZeroVector;
}

void AFreeCameraPawn::OnLeftClick()
{
    UE_LOG(LogTemp, Log, TEXT("OnLeftClick"));
    if (!PathManager) return;

    FVector Start = Camera->GetComponentLocation();
    FVector Dir = Camera->GetForwardVector();
    FVector End = Start + Dir * 10000.f;

    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f, 0, 1.f);

    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
    {
        const FVector HitLocation = Hit.ImpactPoint;
        const FVector Normal = Hit.ImpactNormal;
        const FVector Adjusted = HitLocation + Normal * 100.f;

        StartCoord = FIntVector(
            FMath::FloorToInt(Adjusted.X / 100.f) ,
            FMath::FloorToInt(Adjusted.Y / 100.f) ,
            FMath::FloorToInt(Adjusted.Z / 100.f)
        );

        UE_LOG(LogTemp, Log, TEXT("Set START to %s"), *StartCoord.ToString());
    }

    if (TSharedPtr<FPathFindingNode> Node = PathManager->GetNodeAt(StartCoord))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start Node exists. Walkable? %s"), Node->bIsWalkable ? TEXT("YES") : TEXT("NO"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Start Node not found in NodeMap!"));
    }

}

void AFreeCameraPawn::OnRightClick()
{
    UE_LOG(LogTemp, Log, TEXT("OnRightClick"));
    
    if (!PathManager) return;

    FVector Start = Camera->GetComponentLocation();
    FVector Dir = Camera->GetForwardVector();
    FVector End = Start + Dir * 10000.f;

    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f, 0, 1.f);

    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
    {
        const FVector HitLocation = Hit.ImpactPoint;
        const FVector Normal = Hit.ImpactNormal;
        const FVector Adjusted = HitLocation + Normal * 100.f;

        EndCoord = FIntVector(
            FMath::FloorToInt(Adjusted.X / 100.f),
            FMath::FloorToInt(Adjusted.Y / 100.f),
            FMath::FloorToInt(Adjusted.Z / 100.f)
        );

        UE_LOG(LogTemp, Log, TEXT("Set END to %s"), *EndCoord.ToString());

        TSharedPtr<FPathFindingNode> StartNode = PathManager->GetNodeAt(StartCoord);
        TSharedPtr<FPathFindingNode> EndNode = PathManager->GetNodeAt(EndCoord);
        
        FVector Offset( 50.f, 50.f, -50.f );
        if (StartNode && EndNode)
        {
            TArray<FVector> Path = PathManager->FindPath(StartNode->Position, EndNode->Position);
            for (int i = 0; i < Path.Num() - 1; ++i)
            {
                DrawDebugLine(GetWorld(), Path[i] + Offset, Path[i + 1] + Offset, FColor::Red, false, 10.f, 0, 3.f);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid path nodes"));
        }
    }
    
    if (TSharedPtr<FPathFindingNode> Node = PathManager->GetNodeAt(StartCoord))
    {
        UE_LOG(LogTemp, Warning, TEXT("End Node exists. Walkable? %s"), Node->bIsWalkable ? TEXT("YES") : TEXT("NO"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("End Node not found in NodeMap!"));
    }
}

void AFreeCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AFreeCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AFreeCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis("MoveUp", this, &AFreeCameraPawn::MoveUp);

    PlayerInputComponent->BindAxis("Turn", this, &AFreeCameraPawn::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AFreeCameraPawn::LookUp);

    PlayerInputComponent->BindAction("LeftClick", IE_Pressed, this, &AFreeCameraPawn::OnLeftClick);
    PlayerInputComponent->BindAction("RightClick", IE_Pressed, this, &AFreeCameraPawn::OnRightClick);

}

void AFreeCameraPawn::MoveForward(float Value)
{
    MoveInput.Y = Value;
}

void AFreeCameraPawn::MoveRight(float Value)
{
    MoveInput.X = Value;
}

void AFreeCameraPawn::MoveUp(float Value)
{
    VerticalInput = Value;
}

void AFreeCameraPawn::Turn(float Value)
{
    MouseInput.X += Value;
}

void AFreeCameraPawn::LookUp(float Value)
{
    MouseInput.Y -= Value;
}
