// Copyright 2025 Noah O'Connor. All rights reserved.

#include "FreeCameraPawn.h"

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
}

UCameraComponent* AFreeCameraPawn::GetCamera() const
{
    return Camera;
}

void AFreeCameraPawn::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    VoxelWorld = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
    if (!VoxelWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("VoxelWorld not found in scene!"));
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
    if (!VoxelWorld) return;

    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + Camera->GetForwardVector() * 10000.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        FVector NormalOffset = Hit.ImpactNormal * -50.f; // One voxel outward
        FVector HitPoint = Hit.ImpactPoint + NormalOffset;
        FIntVector BlockCoord = FIntVector(FMath::FloorToInt(HitPoint.X / 100.f),
                                           FMath::FloorToInt(HitPoint.Y / 100.f),
                                           FMath::FloorToInt(HitPoint.Z / 100.f));

        VoxelWorld->PlaceBlock(BlockCoord.X, BlockCoord.Y, BlockCoord.Z, VoxelWorld->GetVoxelRegistry()->GetIDFromName(FName("Air"))); // Air
        UE_LOG(LogTemp, Log, TEXT("Placed air at %s"), *BlockCoord.ToString());
    }
}


void AFreeCameraPawn::OnRightClick()
{
    if (!VoxelWorld) return;

    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + Camera->GetForwardVector() * 10000.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        FVector NormalOffset = Hit.ImpactNormal * 50.f; // One voxel outward
        FVector PlacePoint = Hit.ImpactPoint + NormalOffset;

        FIntVector BlockCoord = FIntVector(FMath::FloorToInt(PlacePoint.X / 100.f),
                                           FMath::FloorToInt(PlacePoint.Y / 100.f),
                                           FMath::FloorToInt(PlacePoint.Z / 100.f));

        VoxelWorld->PlaceBlock(BlockCoord.X, BlockCoord.Y, BlockCoord.Z, 1); // Block 0
        UE_LOG(LogTemp, Log, TEXT("Placed block 1 at %s"), *BlockCoord.ToString());
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
