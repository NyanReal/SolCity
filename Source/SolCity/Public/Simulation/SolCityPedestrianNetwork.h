#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/SolCitySimulationTypes.h"
#include "SolCityPedestrianNetwork.generated.h"

/** Separate graph for sidewalks, plazas, parks and crosswalks. */
UCLASS(BlueprintType)
class SOLCITY_API ASolCityPedestrianNetwork : public AActor
{
	GENERATED_BODY()

public:
	ASolCityPedestrianNetwork();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian Network")
	TArray<FSolCityPedestrianWaypoint> Nodes;

	UFUNCTION(BlueprintCallable, Category = "Pedestrian Network")
	void SetNodes(const TArray<FSolCityPedestrianWaypoint>& InNodes);

	UFUNCTION(BlueprintPure, Category = "Pedestrian Network")
	bool IsValidNode(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "Pedestrian Network")
	FVector GetNodeWorldPosition(int32 NodeIndex) const;

	UFUNCTION(BlueprintPure, Category = "Pedestrian Network")
	int32 FindNearestNode(const FVector& WorldPosition) const;

	int32 ChooseConnectedNode(int32 NodeIndex, int32 PreviousNodeIndex, FRandomStream& RandomStream) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian Network")
	TObjectPtr<USceneComponent> SceneRoot;
};
