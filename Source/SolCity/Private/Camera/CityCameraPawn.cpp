#include "Camera/CityCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UI/CityHUD.h"

ACityCameraPawn::ACityCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = TargetZoom;
    SpringArm->SetRelativeRotation(FRotator(-58.0f, CameraYaw, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 9.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->FieldOfView = 45.0f;
    Camera->PostProcessBlendWeight = 1.0f;
    FPostProcessSettings& Look = Camera->PostProcessSettings;
    Look.bOverride_AutoExposureMethod = true;
    Look.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Look.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Look.AutoExposureApplyPhysicalCameraExposure = true;
    Look.bOverride_CameraISO = true;
    Look.CameraISO = 100.0f;
    Look.bOverride_CameraShutterSpeed = true;
    Look.CameraShutterSpeed = 250.0f;
    Look.bOverride_DepthOfFieldFstop = true;
    Look.DepthOfFieldFstop = 11.0f;
    Look.bOverride_ColorSaturation = true;
    Look.ColorSaturation = FVector4(1.08f, 1.08f, 1.08f, 1.0f);
    Look.bOverride_ColorContrast = true;
    Look.ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.0f);

    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ACityCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    TargetZoom = FMath::Clamp(SpringArm->TargetArmLength, MinZoom, MaxZoom);
    SetActorLocation(FVector(-1500.0f, 0.0f, 0.0f));
}

void ACityCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FVector2D TotalInput = (MoveInput + GetEdgeScrollInput()).GetClampedToMaxSize(1.0f);
    const FRotator FlatRotation(0.0f, CameraYaw, 0.0f);
    const FVector Forward = FRotationMatrix(FlatRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(FlatRotation).GetUnitAxis(EAxis::Y);
    const float SmoothedZoom = FMath::Clamp(SpringArm->TargetArmLength, MinZoom, MaxZoom);
    const float ZoomRatio = SmoothedZoom / FMath::Max(MinZoom, 1.0f);
    const float PanSpeedScale = FMath::Clamp(
        0.65f * FMath::Pow(ZoomRatio, 0.55f), 0.65f, 3.0f);
    const FVector Delta = (Forward * TotalInput.Y + Right * TotalInput.X)
        * BasePanSpeed * PanSpeedScale * DeltaSeconds;

    FVector NextLocation = GetActorLocation() + Delta;
    NextLocation.X = FMath::Clamp(NextLocation.X, -14000.0f, 14000.0f);
    NextLocation.Y = FMath::Clamp(NextLocation.Y, -14000.0f, 14000.0f);
    NextLocation.Z = 0.0f;
    SetActorLocation(NextLocation);

    CameraYaw = FMath::Fmod(CameraYaw + RotationInput * RotationSpeed * DeltaSeconds + 360.0f, 360.0f);
    SpringArm->SetRelativeRotation(FRotator(-58.0f, CameraYaw, 0.0f));
    const float ZoomRangeAlpha = FMath::GetMappedRangeValueClamped(
        FVector2D(MinZoom, MaxZoom), FVector2D(0.0f, 1.0f), SmoothedZoom);
    const float ZoomInterpSpeed = FMath::Lerp(11.0f, 6.5f, ZoomRangeAlpha);
    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength, TargetZoom, DeltaSeconds, ZoomInterpSpeed);
}

void ACityCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ACityCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ACityCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("RotateCamera"), this, &ACityCameraPawn::RotateCamera);
    PlayerInputComponent->BindAxis(TEXT("ZoomCamera"), this, &ACityCameraPawn::ZoomCamera);
    PlayerInputComponent->BindAction(TEXT("ResetCamera"), IE_Pressed, this, &ACityCameraPawn::ResetCamera);
    PlayerInputComponent->BindAction(TEXT("ToggleHelp"), IE_Pressed, this, &ACityCameraPawn::ToggleHelp);
}

void ACityCameraPawn::MoveForward(const float Value)
{
    MoveInput.Y = Value;
}

void ACityCameraPawn::MoveRight(const float Value)
{
    MoveInput.X = Value;
}

void ACityCameraPawn::RotateCamera(const float Value)
{
    RotationInput = Value;
}

void ACityCameraPawn::ZoomCamera(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        constexpr float ReferenceZoom = 6200.0f;
        const float ZoomMultiplier = FMath::Exp(-Value * FMath::Max(ZoomStep, 0.0f) / ReferenceZoom);
        TargetZoom = FMath::Clamp(TargetZoom * ZoomMultiplier, MinZoom, MaxZoom);
    }
}

void ACityCameraPawn::ResetCamera()
{
    SetActorLocation(FVector(-1500.0f, 0.0f, 0.0f));
    CameraYaw = -45.0f;
    TargetZoom = 6200.0f;
}

void ACityCameraPawn::ToggleHelp()
{
    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ACityHUD* HUD = Cast<ACityHUD>(PC->GetHUD()))
        {
            HUD->ToggleHelp();
        }
    }
}

FVector2D ACityCameraPawn::GetEdgeScrollInput() const
{
    if (!bEnableEdgeScroll)
    {
        return FVector2D::ZeroVector;
    }

    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return FVector2D::ZeroVector;
    }

    int32 Width = 0;
    int32 Height = 0;
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY))
    {
        return FVector2D::ZeroVector;
    }

    constexpr float Edge = 14.0f;
    FVector2D Result = FVector2D::ZeroVector;
    if (MouseX <= Edge) Result.X = -1.0f;
    else if (MouseX >= static_cast<float>(Width) - Edge) Result.X = 1.0f;
    if (MouseY <= Edge) Result.Y = 1.0f;
    else if (MouseY >= static_cast<float>(Height) - Edge) Result.Y = -1.0f;
    return Result;
}
