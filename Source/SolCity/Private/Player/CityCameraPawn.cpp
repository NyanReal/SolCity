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
    const float LowerZoom = FMath::Min(MinZoom, MaxZoom);
    const float UpperZoom = FMath::Max(MinZoom, MaxZoom);
    DesiredZoom = FMath::Clamp(SpringArm->TargetArmLength, LowerZoom, UpperZoom);
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->DefaultMouseCursor = EMouseCursor::Crosshairs;
    }
}

void ACityCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bFreeFlyMode)
    {
        AddMovementInput(Camera->GetForwardVector(), ForwardInput);
        AddMovementInput(Camera->GetRightVector(), RightInput);
        AddMovementInput(FVector::UpVector, UpInput);
        return;
    }

    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength,
        DesiredZoom,
        DeltaSeconds,
        ZoomInterpSpeed);
    if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, DesiredZoom, 0.1f))
    {
        SpringArm->TargetArmLength = DesiredZoom;
    }

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
    PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &ACityCameraPawn::MoveUp);
    PlayerInputComponent->BindAxis(TEXT("CameraRotate"), this, &ACityCameraPawn::Rotate);
    PlayerInputComponent->BindAxis(TEXT("CameraPitch"), this, &ACityCameraPawn::AdjustPitch);
    PlayerInputComponent->BindAction(TEXT("ZoomIn"), IE_Pressed, this, &ACityCameraPawn::ZoomIn);
    PlayerInputComponent->BindAction(TEXT("ZoomOut"), IE_Pressed, this, &ACityCameraPawn::ZoomOut);
    PlayerInputComponent->BindAction(TEXT("RotateDrag"), IE_Pressed, this, &ACityCameraPawn::SetRotateDragPressed);
    PlayerInputComponent->BindAction(TEXT("RotateDrag"), IE_Released, this, &ACityCameraPawn::SetRotateDragReleased);
    PlayerInputComponent->BindAction(TEXT("ToggleFreeFly"), IE_Pressed, this, &ACityCameraPawn::ToggleFreeFly);
}

void ACityCameraPawn::ZoomIn() { Zoom(1.0f); }
void ACityCameraPawn::ZoomOut() { Zoom(-1.0f); }

void ACityCameraPawn::Zoom(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        if (bFreeFlyMode)
        {
            const float LowerSpeed = FMath::Min(MinFreeFlySpeed, MaxFreeFlySpeed);
            const float UpperSpeed = FMath::Max(MinFreeFlySpeed, MaxFreeFlySpeed);
            FreeFlySpeed = FMath::Clamp(FreeFlySpeed + Value * FreeFlySpeedStep, LowerSpeed, UpperSpeed);
            Movement->MaxSpeed = FreeFlySpeed;
            return;
        }
        const float LowerZoom = FMath::Min(MinZoom, MaxZoom);
        const float UpperZoom = FMath::Max(MinZoom, MaxZoom);
        DesiredZoom = FMath::Clamp(DesiredZoom - Value * ZoomStep, LowerZoom, UpperZoom);
    }
}

void ACityCameraPawn::MoveForward(float Value) { ForwardInput = Value; }
void ACityCameraPawn::MoveRight(float Value) { RightInput = Value; }
void ACityCameraPawn::MoveUp(float Value) { UpInput = Value; }

void ACityCameraPawn::Rotate(float Value)
{
    if (bRotateDragging && !FMath::IsNearlyZero(Value))
    {
        SpringArm->AddWorldRotation(FRotator(0.0f, Value * 1.5f, 0.0f));
    }
}

void ACityCameraPawn::AdjustPitch(float Value)
{
    if (bRotateDragging && !FMath::IsNearlyZero(Value))
    {
        FRotator ArmRotation = SpringArm->GetRelativeRotation();
        const float LowerPitch = bFreeFlyMode
            ? FMath::Min(MinFreeFlyPitch, MaxFreeFlyPitch)
            : FMath::Min(MinCameraPitch, MaxCameraPitch);
        const float UpperPitch = bFreeFlyMode
            ? FMath::Max(MinFreeFlyPitch, MaxFreeFlyPitch)
            : FMath::Max(MinCameraPitch, MaxCameraPitch);
        ArmRotation.Pitch = FMath::Clamp(ArmRotation.Pitch + Value * PitchDragSpeed, LowerPitch, UpperPitch);
        SpringArm->SetRelativeRotation(ArmRotation);
    }
}

void ACityCameraPawn::ToggleFreeFly()
{
    Movement->StopMovementImmediately();
    ForwardInput = 0.0f;
    RightInput = 0.0f;
    UpInput = 0.0f;

    if (!bFreeFlyMode)
    {
        SavedCityActorTransform = GetActorTransform();
        SavedCityArmRotation = SpringArm->GetRelativeRotation();
        SavedCityArmLength = SpringArm->TargetArmLength;
        SavedCityDesiredZoom = DesiredZoom;

        const FTransform CameraWorldTransform = Camera->GetComponentTransform();
        SetActorLocation(CameraWorldTransform.GetLocation());
        SpringArm->TargetArmLength = 0.0f;
        SpringArm->SetWorldRotation(CameraWorldTransform.Rotator());
        Movement->MaxSpeed = FreeFlySpeed;
        bFreeFlyMode = true;
        UE_LOG(LogTemp, Log, TEXT("SolCity camera mode: Free Fly"));
        return;
    }

    SetActorTransform(SavedCityActorTransform);
    SpringArm->TargetArmLength = SavedCityArmLength;
    DesiredZoom = SavedCityDesiredZoom;
    SpringArm->SetRelativeRotation(SavedCityArmRotation);
    Movement->MaxSpeed = PanSpeed;
    bFreeFlyMode = false;
    UE_LOG(LogTemp, Log, TEXT("SolCity camera mode: City"));
}

void ACityCameraPawn::SetRotateDragPressed() { bRotateDragging = true; }
void ACityCameraPawn::SetRotateDragReleased() { bRotateDragging = false; }
