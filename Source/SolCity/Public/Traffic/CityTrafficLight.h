#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Traffic/CityTrafficTypes.h"
#include "CityTrafficLight.generated.h"

class ACityRoadNetwork;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

/** A synchronized two-phase signal with lightweight engine-shape visuals. */
UCLASS(Blueprintable)
class SOLCITY_API ACityTrafficLight : public AActor
{
	GENERATED_BODY()

public:
	ACityTrafficLight();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Traffic|Signals")
	void InitializeLight(ACityRoadNetwork* InRoadNetwork, FIntPoint InIntersection, float InPhaseOffset = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	bool CanProceed(ECityRoadDirection IncomingDirection) const;

	/** Positive distance means the vehicle has not crossed the line yet. */
	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	float GetSignedDistanceToStopLine(ECityRoadDirection IncomingDirection, FVector VehicleLocation) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	FVector GetStopLineWorldLocation(ECityRoadDirection IncomingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	bool ShouldStop(ECityRoadDirection IncomingDirection, FVector VehicleLocation,
		float StopBuffer = 25.0f) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	ECityTrafficPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	FIntPoint GetIntersection() const { return Intersection; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> NorthSouthPole;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> NorthSouthSignal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> EastWestPole;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	TObjectPtr<UStaticMeshComponent> EastWestSignal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Signals", meta = (ClampMin = "2.0"))
	float PhaseDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Signals", meta = (ClampMin = "100.0"))
	float StopDetectionDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	FLinearColor GoColor = FLinearColor(0.20f, 1.00f, 0.62f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Visual")
	FLinearColor StopColor = FLinearColor(1.00f, 0.24f, 0.32f, 1.0f);

private:
	void UpdatePhaseAndVisuals();
	void ConfigureVisualPlacement();
	void EnsureDynamicMaterials();
	static void SetMaterialTint(UMaterialInstanceDynamic* Material, const FLinearColor& Color);

	UPROPERTY(Transient)
	TObjectPtr<ACityRoadNetwork> RoadNetwork;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> NorthSouthMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EastWestMaterial;

	UPROPERTY(VisibleInstanceOnly, Category = "Traffic|Signals")
	FIntPoint Intersection = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, Category = "Traffic|Signals")
	ECityTrafficPhase CurrentPhase = ECityTrafficPhase::NorthSouthGreen;

	float PhaseOffset = 0.0f;
	ECityTrafficPhase LastVisualPhase = ECityTrafficPhase::EastWestGreen;
	bool bVisualsInitialized = false;
};
