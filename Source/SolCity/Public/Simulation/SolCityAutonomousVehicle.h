#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolCityAutonomousVehicle.generated.h"

class ASolCityRoadNetwork;
class USceneComponent;
class UStaticMeshComponent;

/** Lightweight kinematic traffic agent constrained to a directed road waypoint graph. */
UCLASS(BlueprintType)
class SOLCITY_API ASolCityAutonomousVehicle : public AActor
{
	GENERATED_BODY()

public:
	ASolCityAutonomousVehicle();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	TObjectPtr<ASolCityRoadNetwork> RoadNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "10.0"))
	float MaxSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "10.0"))
	float Acceleration = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "10.0"))
	float BrakingDeceleration = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "50.0"))
	float FollowingDistance = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "10.0"))
	float StopLineDistance = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "1.0"))
	float WaypointAcceptanceRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (ClampMin = "0.1"))
	float RotationResponsiveness = 6.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	float CurrentSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	int32 CurrentNodeIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	int32 NextNodeIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	bool InitializeOnRoad(ASolCityRoadNetwork* InRoadNetwork, int32 StartNodeIndex = -1);

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	bool IsWaitingForTraffic() const { return bWaitingForTraffic; }

protected:
	bool SelectNextNode();
	bool HasVehicleAhead() const;
	bool MustStopAtSignal(float DistanceToTarget) const;

	/** Keeps the actor's +X navigation axis independent from authored mesh axes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<USceneComponent> VehicleRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UStaticMeshComponent> VehicleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UStaticMeshComponent> CabinMesh;

	UPROPERTY(Transient)
	bool bWaitingForTraffic = false;

	FRandomStream RandomStream;
};
