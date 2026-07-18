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
    void Rotate(float Value);
    void AdjustPitch(float Value);
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

    UPROPERTY(EditAnywhere, Category="Camera")
    float PanSpeed = 5000.0f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0", UIMin="0.0"))
    float PitchDragSpeed = 1.5f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0"))
    float MinCameraPitch = -80.0f;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0"))
    float MaxCameraPitch = -25.0f;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    bool bRotateDragging = false;
};
