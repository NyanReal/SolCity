#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Traffic/CityTrafficTypes.h"
#include "CityRoadNetwork.generated.h"

class ACityTrafficLight;
class ACityVehicle;
class USceneComponent;

/**
 * Owns the logical grid used by traffic. Roads remain connected across the
 * river column; IsRiverBridgeIntersection is provided for world visuals.
 */
UCLASS(Blueprintable)
class SOLCITY_API ACityRoadNetwork : public AActor
{
	GENERATED_BODY()

public:
	ACityRoadNetwork();

	virtual void BeginPlay() override;

	/** Builds the graph, then optionally creates signals and the initial cars. */
	UFUNCTION(BlueprintCallable, Category = "Traffic")
	void Initialize(int32 InGridHalfExtent, float InRoadSpacing, float InCellSize, int32 InRiverColumn);

	UFUNCTION(BlueprintCallable, Category = "Traffic")
	void RebuildRoadGraph();

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	bool IsValidIntersection(FIntPoint GridCoordinate) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	bool GetRoadNode(FIntPoint GridCoordinate, FCityRoadNode& OutNode) const;

	const FCityRoadNode* FindRoadNode(FIntPoint GridCoordinate) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	FVector GridToWorldIntersection(FIntPoint GridCoordinate) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	FIntPoint WorldToNearestIntersection(FVector WorldLocation, bool bClampToNetwork = true) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	bool IsRoadLocation(FVector WorldLocation, float ExtraTolerance = 0.0f) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Road")
	bool IsRiverBridgeIntersection(FIntPoint GridCoordinate) const;

	/** Lane center at the middle of an intersection for a given travel direction. */
	UFUNCTION(BlueprintPure, Category = "Traffic|Lane")
	FVector GetLaneWaypoint(FIntPoint Intersection, ECityRoadDirection TravelDirection) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Lane")
	FVector GetStopLineLocation(FIntPoint Intersection, ECityRoadDirection IncomingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Lane")
	FVector GetLaneExitLocation(FIntPoint Intersection, ECityRoadDirection OutgoingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Lane")
	FVector GetWorldDirection(ECityRoadDirection Direction) const;

	UFUNCTION(BlueprintCallable, Category = "Traffic|Lane")
	void GetOutgoingDirections(FIntPoint Intersection, ECityRoadDirection IncomingDirection,
		TArray<ECityRoadDirection>& OutDirections) const;

	/** Generates a short, lane-correct quadratic path through an intersection. */
	void BuildIntersectionPath(FIntPoint Intersection, ECityRoadDirection IncomingDirection,
		ECityRoadDirection OutgoingDirection, TArray<FVector>& OutPath, int32 SampleCount = 7) const;

	UFUNCTION(BlueprintCallable, Category = "Traffic|Signals")
	int32 SpawnTrafficLights();

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	ACityTrafficLight* GetTrafficLightAt(FIntPoint Intersection) const;

	UFUNCTION(BlueprintPure, Category = "Traffic|Signals")
	bool CanProceedThrough(FIntPoint Intersection, ECityRoadDirection IncomingDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Traffic|Vehicles")
	int32 SpawnVehicles(int32 Count);

	/** Configure how many cars the next Initialize call creates. */
	UFUNCTION(BlueprintCallable, Category = "Traffic|Vehicles")
	void SetInitialVehicleCount(int32 Count) { InitialVehicleCount = FMath::Clamp(Count, 0, 200); }

	UFUNCTION(BlueprintCallable, Category = "Traffic|Vehicles")
	ACityVehicle* SpawnVehicleAt(FIntPoint FromIntersection, ECityRoadDirection Direction,
		float SegmentAlpha = 0.15f);

	void RegisterVehicle(ACityVehicle* Vehicle);
	void UnregisterVehicle(ACityVehicle* Vehicle);
	float GetDistanceToVehicleAhead(const ACityVehicle* Requestor, const FVector& Forward,
		float MaxDistance, float LaneTolerance) const;

	static FIntPoint DirectionToGridDelta(ECityRoadDirection Direction);
	static ECityRoadDirection OppositeDirection(ECityRoadDirection Direction);

	UFUNCTION(BlueprintPure, Category = "Traffic")
	int32 GetGridHalfExtent() const { return GridHalfExtent; }

	UFUNCTION(BlueprintPure, Category = "Traffic")
	float GetRoadSpacing() const { return RoadSpacing; }

	UFUNCTION(BlueprintPure, Category = "Traffic")
	float GetRoadHalfWidth() const { return RoadHalfWidth; }

	UFUNCTION(BlueprintPure, Category = "Traffic")
	float GetLaneOffset() const { return LaneOffset; }

	UFUNCTION(BlueprintPure, Category = "Traffic")
	int32 GetRiverColumn() const { return RiverColumn; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup", meta = (ClampMin = "1", ClampMax = "50"))
	int32 GridHalfExtent = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup", meta = (ClampMin = "500.0"))
	float RoadSpacing = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup", meta = (ClampMin = "100.0"))
	float CellSize = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	int32 RiverColumn = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	float RoadSurfaceZ = 8.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	float RoadHalfWidth = 220.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	float LaneOffset = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	float StopLineOffset = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Setup")
	bool bAutoInitializeOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Spawning")
	bool bSpawnSignalsOnInitialize = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Spawning")
	bool bSpawnSignalsAtBoundary = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Spawning", meta = (ClampMin = "0", ClampMax = "200"))
	int32 InitialVehicleCount = 18;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Spawning")
	TSubclassOf<ACityTrafficLight> TrafficLightClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic|Spawning")
	TSubclassOf<ACityVehicle> VehicleClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Traffic")
	bool bInitialized = false;

private:
	void ClearSpawnedTraffic();

	UPROPERTY(VisibleInstanceOnly, Category = "Traffic")
	TMap<FIntPoint, FCityRoadNode> RoadNodes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityTrafficLight>> SpawnedTrafficLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityVehicle>> SpawnedVehicles;

	TMap<FIntPoint, TWeakObjectPtr<ACityTrafficLight>> LightByIntersection;
	TArray<TWeakObjectPtr<ACityVehicle>> RegisteredVehicles;
};
