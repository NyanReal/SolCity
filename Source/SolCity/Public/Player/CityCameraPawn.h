#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CityCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class SOLCITY_API ACityCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ACityCameraPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;

private:
    void Zoom(float Value);
    void ZoomIn();
    void ZoomOut();
    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUp(float Value);
    void Rotate(float Value);
    void AdjustPitch(float Value);
    void ToggleFreeFly();
    void SetRotateDragPressed();
    void SetRotateDragReleased();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(EditAnywhere, Category="Camera")
    float MinZoom = 800.0f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float MaxZoom = 48000.0f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float ZoomStep = 2500.0f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.1", UIMin="0.1"))
    float ZoomInterpSpeed = 5.0f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float PanSpeed = 5000.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="100.0", UIMin="100.0"))
    float FreeFlySpeed = 6000.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="10.0", UIMin="10.0"))
    float FreeFlySpeedStep = 1000.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="100.0", UIMin="100.0"))
    float MinFreeFlySpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="100.0", UIMin="100.0"))
    float MaxFreeFlySpeed = 30000.0f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0", UIMin="0.0"))
    float PitchDragSpeed = 1.5f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0"))
    float MinCameraPitch = -80.0f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0"))
    float MaxCameraPitch = -25.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="-89.0", ClampMax="89.0", UIMin="-89.0", UIMax="89.0"))
    float MinFreeFlyPitch = -89.0f;

    UPROPERTY(EditAnywhere, Category="Camera|Free Fly", meta=(ClampMin="-89.0", ClampMax="89.0", UIMin="-89.0", UIMax="89.0"))
    float MaxFreeFlyPitch = 89.0f;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    float UpInput = 0.0f;
    float DesiredZoom = 11000.0f;
    bool bRotateDragging = false;
    bool bFreeFlyMode = false;
    FTransform SavedCityActorTransform = FTransform::Identity;
    FRotator SavedCityArmRotation = FRotator::ZeroRotator;
    float SavedCityArmLength = 11000.0f;
    float SavedCityDesiredZoom = 11000.0f;
};
