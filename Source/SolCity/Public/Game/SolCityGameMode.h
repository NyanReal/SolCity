#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolCityGameMode.generated.h"

UCLASS()
class SOLCITY_API ASolCityGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASolCityGameMode();

	UPROPERTY(EditDefaultsOnly, Category="Sol City|Population", meta=(ClampMin="0", ClampMax="100"))
	int32 VehicleCount = 18;

	UPROPERTY(EditDefaultsOnly, Category="Sol City|Population", meta=(ClampMin="0", ClampMax="200"))
	int32 CatCitizenCount = 36;

protected:
    virtual void BeginPlay() override;
};
