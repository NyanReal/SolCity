#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SolCityLandscapeLibrary.generated.h"

/** Editor automation for rebuilding the persistent Sol City terrain. */
UCLASS()
class SOLCITY_API USolCityLandscapeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sol City|Landscape")
	static bool RebuildSolCityLandscape(
		float CityDiameter = 144000.0f,
		float RiverWidth = 6000.0f,
		float RiverSurfaceZ = -90.0f);
};
