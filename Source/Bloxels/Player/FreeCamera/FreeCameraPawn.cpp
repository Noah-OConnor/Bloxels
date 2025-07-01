// Copyright 2025 Noah O'Connor. All rights reserved.

#include "FreeCameraPawn.h"

#include "EngineUtils.h"
#include "Bloxels/Voxel/PathFinding/PathfindingSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"

AFreeCameraPawn::AFreeCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 2000.f;

    PathfindingComponent = CreateDefaultSubobject<UPathfindingComponent>(TEXT("PathfindingComponent"));
}

void AFreeCameraPawn::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    
    UPathfindingSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UPathfindingSubsystem>();
    PathManager = Subsystem ? Subsystem->GetPathfindingManager() : nullptr;

    if (PathManager)
    {
        PathfindingComponent->Initialize(PathManager);
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
    
    const FVector Start = Camera->GetComponentLocation();
    const FVector Dir = Camera->GetForwardVector();
    const FVector End = Start + Dir * 10000.f;
    
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f, 0, 1.f);
    
    if (FHitResult Hit; GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
    {
        const FVector HitLocation = Hit.ImpactPoint;
        const FVector Normal = Hit.ImpactNormal;
        FVector Adjusted = HitLocation;
        
        Adjusted += Normal * 50.f;  
        
    
        StartCoord = FIntVector(
            FMath::FloorToInt(Adjusted.X / 100.f) ,
            FMath::FloorToInt(Adjusted.Y / 100.f) ,
            FMath::FloorToInt(Adjusted.Z / 100.f)
        );
    
        UE_LOG(LogTemp, Log, TEXT("Set START to %s"), *StartCoord.ToString());
    }
}

void AFreeCameraPawn::OnRightClick()
{
    UE_LOG(LogTemp, Log, TEXT("OnRightClick"));
    
    if (!PathManager || !PathfindingComponent) return;
    
    const FVector Start = Camera->GetComponentLocation();
    const FVector Dir = Camera->GetForwardVector();
    const FVector End = Start + Dir * 10000.f;
    
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f, 0, 1.f);
    
    if (FHitResult Hit; GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
    {
        const FVector HitLocation = Hit.ImpactPoint;
        const FVector Normal = Hit.ImpactNormal;
        FVector Adjusted = HitLocation;
    
        if (Normal.Equals(FVector::UpVector, 0.1f))
        {
            Adjusted += FVector(0, 0, 50.f);
        }
        else
        {
            Adjusted += Normal * 50.f;
        }
    
        EndCoord = FIntVector(
            FMath::FloorToInt(Adjusted.X / 100.f),
            FMath::FloorToInt(Adjusted.Y / 100.f),
            FMath::FloorToInt(Adjusted.Z / 100.f)
        );
    
        UE_LOG(LogTemp, Log, TEXT("Set END to %s"), *EndCoord.ToString());
    
        if (!StartCoord.IsZero())
        {
            PathfindingComponent->FindPath(StartCoord, EndCoord);
            PathfindingComponent->DrawDebugPath(10.f);
        }
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
