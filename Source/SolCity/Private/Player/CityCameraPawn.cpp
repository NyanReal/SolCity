#include "Player/CityCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ACityCameraPawn::ACityCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Root);
    SpringArm->TargetArmLength = 11000.0f;
    SpringArm->SetRelativeRotation(FRotator(-55.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = PanSpeed;
    Movement->Acceleration = 12000.0f;
    Movement->Deceleration = 16000.0f;

    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ACityCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->DefaultMouseCursor = EMouseCursor::Crosshairs;
    }
}

void ACityCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FRotator FlatRotation(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
    const FVector Forward = FlatRotation.RotateVector(FVector::ForwardVector);
    const FVector Right = FlatRotation.RotateVector(FVector::RightVector);
    AddMovementInput(Forward, ForwardInput * PanSpeed * DeltaSeconds);
    AddMovementInput(Right, RightInput * PanSpeed * DeltaSeconds);
}

void ACityCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ACityCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ACityCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("CameraRotate"), this, &ACityCameraPawn::Rotate);
    PlayerInputComponent->BindAction(TEXT("ZoomIn"), IE_Pressed, this, &ACityCameraPawn::ZoomIn);
    PlayerInputComponent->BindAction(TEXT("ZoomOut"), IE_Pressed, this, &ACityCameraPawn::ZoomOut);
    PlayerInputComponent->BindAction(TEXT("RotateDrag"), IE_Pressed, this, &ACityCameraPawn::SetRotateDragPressed);
    PlayerInputComponent->BindAction(TEXT("RotateDrag"), IE_Released, this, &ACityCameraPawn::SetRotateDragReleased);
}

void ACityCameraPawn::ZoomIn() { Zoom(1.0f); }
void ACityCameraPawn::ZoomOut() { Zoom(-1.0f); }

void ACityCameraPawn::Zoom(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - Value * ZoomStep, MinZoom, MaxZoom);
    }
}

void ACityCameraPawn::MoveForward(float Value) { ForwardInput = Value; }
void ACityCameraPawn::MoveRight(float Value) { RightInput = Value; }

void ACityCameraPawn::Rotate(float Value)
{
    if (bRotateDragging && !FMath::IsNearlyZero(Value))
    {
        SpringArm->AddWorldRotation(FRotator(0.0f, Value * 1.5f, 0.0f));
    }
}

void ACityCameraPawn::SetRotateDragPressed() { bRotateDragging = true; }
void ACityCameraPawn::SetRotateDragReleased() { bRotateDragging = false; }
