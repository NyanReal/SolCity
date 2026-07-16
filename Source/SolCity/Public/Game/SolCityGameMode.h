#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "SolCityGameMode.generated.h"

class ACatCitizenSpawner;
class ACityRoadNetwork;
class ACityWorldActor;

/**
 * Boots the complete procedural demo in any empty level, including Engine/Entry.
 * This keeps the repository text-only while generated texture assets are pending.
 */
UCLASS()
class SOLCITY_API ASolCityGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASolCityGameMode();
    virtual void StartPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="Sol City|Bootstrap")
    TSubclassOf<ACityWorldActor> CityWorldClass;

    UPROPERTY(EditDefaultsOnly, Category="Sol City|Bootstrap")
    TSubclassOf<ACityRoadNetwork> RoadNetworkClass;

    UPROPERTY(EditDefaultsOnly, Category="Sol City|Bootstrap")
    TSubclassOf<ACatCitizenSpawner> CitizenSpawnerClass;

private:
    UPROPERTY(Transient)
    TObjectPtr<ACityWorldActor> CityWorld;

    UPROPERTY(Transient)
    TObjectPtr<ACityRoadNetwork> RoadNetwork;

    UPROPERTY(Transient)
    TObjectPtr<ACatCitizenSpawner> CitizenSpawner;

    void BootstrapCity();
    void CaptureValidationFrame();
    void FinishValidationCapture();

    FTimerHandle CaptureTimer;
    FTimerHandle ExitTimer;
};
