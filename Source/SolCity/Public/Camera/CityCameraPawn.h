#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CityCameraPawn.generated.h"

class UCameraComponent;
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

    /** Sets the wheel-zoom target; useful for Blueprint views and render validation. */
    UFUNCTION(BlueprintCallable, Category="Sol City|Camera")
    void SetZoomDistance(float NewZoomDistance, bool bImmediate = false);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    float MinZoom = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    float MaxZoom = 21600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    float BasePanSpeed = 1700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    float ZoomStep = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    float RotationSpeed = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sol City|Camera")
    bool bEnableEdgeScroll = true;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Sol City|Camera")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category="Sol City|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, Category="Sol City|Camera")
    TObjectPtr<UCameraComponent> Camera;

    FVector2D MoveInput = FVector2D::ZeroVector;
    float RotationInput = 0.0f;
    float TargetZoom = 6200.0f;
    float CameraYaw = -45.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void RotateCamera(float Value);
    void ZoomCamera(float Value);
    void ResetCamera();
    void ToggleHelp();
    FVector2D GetEdgeScrollInput() const;
};
