#pragma once

#include "CoreMinimal.h"
#include "CityTrafficTypes.generated.h"

/** Cardinal travel direction on the city grid. */
UENUM(BlueprintType)
enum class ECityRoadDirection : uint8
{
	North UMETA(DisplayName = "North"),
	East UMETA(DisplayName = "East"),
	South UMETA(DisplayName = "South"),
	West UMETA(DisplayName = "West")
};

/** The traffic lights intentionally use a simple two-phase cycle. */
UENUM(BlueprintType)
enum class ECityTrafficPhase : uint8
{
	NorthSouthGreen UMETA(DisplayName = "North / South Green"),
	EastWestGreen UMETA(DisplayName = "East / West Green")
};

UENUM(BlueprintType)
enum class ECityVehicleDriveState : uint8
{
	ApproachingIntersection,
	CrossingIntersection
};

/** A node in the directed grid road graph. */
USTRUCT(BlueprintType)
struct SOLCITY_API FCityRoadNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traffic")
	FIntPoint GridCoordinate = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Traffic")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traffic")
	TArray<FIntPoint> Connections;
};

