#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatCitizenSpawner.generated.h"

class ACatCitizen;
class USceneComponent;

/** Owns and places a population of ACatCitizen actors in pedestrian space. */
UCLASS(Blueprintable)
class SOLCITY_API ACatCitizenSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACatCitizenSpawner();

	/**
	 * Spawns up to Count citizens and returns the successfully created actors.
	 * Road grid lines are centred on world-space multiples of RoadSpacing.
	 */
	TArray<ACatCitizen*> SpawnCitizens(
		int32 Count,
		const FVector& BoundsMin,
		const FVector& BoundsMax,
		float RoadSpacing,
		float RoadWidth,
		float RiverX,
		float RiverHalfWidth,
		const TArray<FBox>& BridgeAreas);

	UFUNCTION(BlueprintCallable, Category = "SolCity|Citizens")
	void ClearCitizens();

	int32 GetSpawnedCitizenCount() const;

private:
	bool TryFindSpawnLocation(
		FVector& OutLocation,
		const FVector& BoundsMin,
		const FVector& BoundsMax,
		float RoadSpacing,
		float RoadWidth,
		float RiverX,
		float RiverHalfWidth,
		const TArray<FBox>& BridgeAreas);

	bool IsFarEnoughFromExistingCitizens(const FVector& Location) const;

	UPROPERTY(VisibleAnywhere, Category = "Citizens")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Citizens")
	TSubclassOf<ACatCitizen> CitizenClass;

	UPROPERTY(EditAnywhere, Category = "Citizens")
	int32 RandomSeed = 7319;

	UPROPERTY(EditAnywhere, Category = "Citizens", meta = (ClampMin = "0.0"))
	float SpawnClearance = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Citizens", meta = (ClampMin = "0.0"))
	float MinimumCitizenSeparation = 145.0f;

	UPROPERTY(EditAnywhere, Category = "Citizens")
	float SpawnHeightOffset = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACatCitizen>> SpawnedCitizens;

	FRandomStream RandomStream;
};
