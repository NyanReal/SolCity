#pragma once

#include "CoreMinimal.h"
#include "SolCitySimulationTypes.generated.h"

class ASolCityTrafficSignal;

UENUM(BlueprintType)
enum class ESolCityTrafficAxis : uint8
{
	AxisA,
	AxisB,
	Uncontrolled
};

UENUM(BlueprintType)
enum class ESolCityTrafficPhase : uint8
{
	AxisAGreen,
	AxisAYellow,
	AllRedToB,
	AxisBGreen,
	AxisBYellow,
	AllRedToA
};

/** One directed road-lane waypoint. OutgoingNodeIndices make arbitrary curves and loops possible. */
USTRUCT(BlueprintType)
struct SOLCITY_API FSolCityRoadWaypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road")
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road")
	TArray<int32> OutgoingNodeIndices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road", meta = (ClampMin = "50.0"))
	float SpeedLimit = 600.0f;

	/** Set on the stop-line node immediately before entering an intersection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
	TObjectPtr<ASolCityTrafficSignal> TrafficSignal = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
	ESolCityTrafficAxis MovementAxis = ESolCityTrafficAxis::Uncontrolled;
};

/** One pedestrian-only waypoint. Cat citizens never leave this graph. */
USTRUCT(BlueprintType)
struct SOLCITY_API FSolCityPedestrianWaypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pedestrian")
	TArray<int32> ConnectedNodeIndices;
};
