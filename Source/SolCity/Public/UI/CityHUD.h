#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CityHUD.generated.h"

UCLASS()
class SOLCITY_API ACityHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UFUNCTION(BlueprintCallable, Category="Sol City|UI")
    void ToggleHelp() { bShowHelp = !bShowHelp; }

private:
    bool bShowHelp = true;
};
