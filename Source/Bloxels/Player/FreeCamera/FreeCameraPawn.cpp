// Copyright 2025 Noah O'Connor. All rights reserved.

#include "FreeCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

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
}

void AFreeCameraPawn::OnRightClick()
{
    UE_LOG(LogTemp, Log, TEXT("OnRightClick"));
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
