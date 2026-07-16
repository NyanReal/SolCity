#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CityPlayerController.generated.h"

UCLASS()
class SOLCITY_API ACityPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
};
