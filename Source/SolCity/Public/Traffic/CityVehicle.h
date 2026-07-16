#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Traffic/CityTrafficTypes.h"
#include "CityVehicle.generated.h"

class ACityRoadNetwork;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

/** A small autonomous grid vehicle built entirely from engine basic shapes. */
UCLASS(Blueprintable)
class SOLCITY_API ACityVehicle : public AActor
{
	GENERATED_BODY()

public:
	ACityVehicle();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Traffic|Vehicle")
	bool InitializeVehicle(ACityRoadNetwork* InRoadNetwork, FIntPoint FromIntersection,
		ECityRoadDirection InitialDirection, float SegmentAlpha = 0.15f);

	UFUNCTION(BlueprintPure, Category = "Traffic|Vehicle")
	ECityVehicleDriveState GetDriveState() const { return DriveState; }

	UFUNCTION(BlueprintPure, Category = "Traffic|Vehicle")
	ECityRoadDirection GetTravelDirection() const { return TravelDirection; }

	UFUNCTION(BlueprintPure, Category = "Traffic|Vehicle")
	FIntPoint GetTargetIntersection() const { return TargetIntersection; }

	UFUNCTION(BlueprintPure, Category = "Traffic|Vehicle")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Traffic|Vehicle")
	void SetVehicleColor(FLinearColor NewColor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Chassis;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Hood;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Trunk;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Cabin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> Roof;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> FrontBumper;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> RearBumper;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelFrontLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelFrontRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelRearLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelRearRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelCapFrontLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelCapFrontRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelCapRearLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> WheelCapRearRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> HeadlightLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> HeadlightRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> TailLightLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> TailLightRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "100.0"))
	float CruiseSpeed = 620.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "100.0"))
	float TurnSpeed = 390.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "50.0"))
	float Acceleration = 440.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "100.0"))
	float BrakeDeceleration = 1050.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "100.0"))
	float MinimumFollowingDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "100.0"))
	float VehicleLookAheadDistance = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StraightChoiceWeight = 0.58f;

private:
	void TickApproach(float DeltaSeconds);
	void TickCrossing(float DeltaSeconds);
	bool BeginCrossing();
	bool AdvanceToward(const FVector& Target, float DeltaSeconds, float DesiredSpeed);
	float ApplyFollowingLimit(float DesiredSpeed) const;
	ECityRoadDirection ChooseOutgoingDirection(const TArray<ECityRoadDirection>& Choices) const;
	void EnsureDynamicMaterials();
	void ApplyInitialPastelColor();

	UPROPERTY(Transient)
	TObjectPtr<ACityRoadNetwork> RoadNetwork;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CabinMaterial;

	UPROPERTY(VisibleInstanceOnly, Category = "Traffic|Vehicle")
	ECityVehicleDriveState DriveState = ECityVehicleDriveState::ApproachingIntersection;

	UPROPERTY(VisibleInstanceOnly, Category = "Traffic|Vehicle")
	ECityRoadDirection TravelDirection = ECityRoadDirection::East;

	ECityRoadDirection PendingDirection = ECityRoadDirection::East;
	FIntPoint PreviousIntersection = FIntPoint::ZeroValue;
	FIntPoint TargetIntersection = FIntPoint::ZeroValue;
	TArray<FVector> CrossingPath;
	int32 CrossingPathIndex = 0;
	float CurrentSpeed = 0.0f;
	bool bVehicleInitialized = false;

	static constexpr float VehicleHalfLength = 170.0f;
	static constexpr float LaneTolerance = 135.0f;
};
