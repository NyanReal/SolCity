#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatCitizen.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

/**
 * A lightweight, self-steering cat citizen.
 *
 * The citizen deliberately has no navigation-system or city-actor dependency.
 * Initialize() supplies a small description of the city, which is enough for
 * the actor to remain inside pedestrian space and away from roads and water.
 */
UCLASS(Blueprintable)
class SOLCITY_API ACatCitizen : public AActor
{
	GENERATED_BODY()

public:
	ACatCitizen();

	virtual void Tick(float DeltaSeconds) override;

	/** Supplies the rules used by the procedural pedestrian steering. */
	void Initialize(
		const FVector& BoundsMin,
		const FVector& BoundsMax,
		float RoadSpacing,
		float RoadWidth,
		float RiverX,
		float RiverHalfWidth,
		const TArray<FBox>& BridgeAreas);

	/** Tests the supplied point with this citizen's current city rules. */
	bool IsWalkableLocation(const FVector& Location, float ExtraClearance = 0.0f) const;

	/** Shared rule test used by the spawner before an actor exists. */
	static bool IsLocationWalkableForCity(
		const FVector& Location,
		const FVector& BoundsMin,
		const FVector& BoundsMax,
		float RoadSpacing,
		float RoadWidth,
		float RiverX,
		float RiverHalfWidth,
		const TArray<FBox>& BridgeAreas,
		float Clearance = 0.0f);

protected:
	virtual void BeginPlay() override;

private:
	void ApplyRandomPalette();
	void ChooseNewTarget();
	void UpdateMovement(float DeltaSeconds);
	void UpdateWalkAnimation(float DeltaSeconds);
	bool IsPathWalkable(const FVector& Start, const FVector& End) const;
	bool IsPointClearOfStaticObstacles(const FVector& Location) const;
	float ResolveWalkSurfaceZ(const FVector& Location) const;
	bool TryFindRecoveryLocation(FVector& OutLocation);
	FVector GetAvoidanceDirection(const FVector& DesiredDirection) const;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> LeftEar;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> RightEar;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> LeftEye;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> RightEye;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> Nose;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> FrontLeftLeg;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> FrontRightLeg;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> BackLeftLeg;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> BackRightLeg;

	UPROPERTY(VisibleAnywhere, Category = "Cat|Geometry")
	TObjectPtr<UStaticMeshComponent> Tail;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "10.0"))
	float MinimumMoveSpeed = 72.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "10.0"))
	float MaximumMoveSpeed = 118.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "50.0"))
	float WanderRadius = 720.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "0.1"))
	float SteeringSharpness = 4.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "0.1"))
	float RotationSharpness = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat|Movement", meta = (ClampMin = "0.0"))
	float PedestrianClearance = 42.0f;

	FVector CityBoundsMin = FVector(-3000.0f, -3000.0f, 0.0f);
	FVector CityBoundsMax = FVector(3000.0f, 3000.0f, 0.0f);
	float CityRoadSpacing = 0.0f;
	float CityRoadWidth = 0.0f;
	float CityRiverX = 0.0f;
	float CityRiverHalfWidth = 0.0f;
	TArray<FBox> CityBridgeAreas;

	FVector WanderTarget = FVector::ZeroVector;
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector BaseBodyLocation = FVector::ZeroVector;
	FRotator BaseTailRotation = FRotator::ZeroRotator;
	float MoveSpeed = 90.0f;
	float TimeUntilRetarget = 0.0f;
	float AnimationTime = 0.0f;
	float GroundZ = 0.0f;
	bool bHasCityRules = false;
	FRandomStream RandomStream;
};
