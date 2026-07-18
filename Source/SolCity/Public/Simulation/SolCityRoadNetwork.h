#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/SolCitySimulationTypes.h"
#include "SolCityRoadNetwork.generated.h"

/** Runtime road graph consumed by autonomous vehicles. Node positions are actor-local. */
UCLASS(BlueprintType)
class SOLCITY_API ASolCityRoadNetwork : public AActor
{
	GENERATED_BODY()

public:
	ASolCityRoadNetwork();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Road Network")
	TArray<FSolCityRoadWaypoint> Nodes;

	UFUNCTION(BlueprintCallable, Category = "Road Network")
	void SetNodes(const TArray<FSolCityRoadWaypoint>& InNodes);

	UFUNCTION(BlueprintPure, Category = "Road Network")
	bool IsValidNode(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "Road Network")
	FVector GetNodeWorldPosition(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "Road Network")
	int32 FindNearestNode(const FVector& WorldPosition) const;

	UFUNCTION(BlueprintPure, Category = "Road Network")
	bool IsNodeOpenForTraffic(int32 NodeIndex) const;

	int32 ChooseNextNode(int32 NodeIndex, FRandomStream& RandomStream) const;
	const FSolCityRoadWaypoint* GetNode(int32 NodeIndex) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network")
	TObjectPtr<USceneComponent> SceneRoot;
};
