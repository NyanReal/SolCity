#include "UI/CityHUD.h"

#include "Citizens/CatCitizen.h"
#include "Traffic/CityVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

void ACityHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine)
    {
        return;
    }

    UFont* Font = GEngine->GetSmallFont();
    const FLinearColor Ink(0.06f, 0.12f, 0.18f, 1.0f);
    const FLinearColor Cream(0.98f, 0.95f, 0.82f, 0.92f);

    int32 CatCount = 0;
    int32 VehicleCount = 0;
    for (TActorIterator<ACatCitizen> It(GetWorld()); It; ++It) ++CatCount;
    for (TActorIterator<ACityVehicle> It(GetWorld()); It; ++It) ++VehicleCount;

    DrawRect(Cream, 22.0f, 22.0f, 248.0f, bShowHelp ? 136.0f : 64.0f);
    DrawText(TEXT("SOL CITY  |  peaceful mode"), Ink, 34.0f, 34.0f, Font, 1.0f, false);
    DrawText(FString::Printf(TEXT("Cats: %d     Autonomous cars: %d"), CatCount, VehicleCount),
        Ink, 34.0f, 56.0f, Font, 0.9f, false);

    if (bShowHelp)
    {
        DrawText(TEXT("WASD / screen edge : pan"), Ink, 34.0f, 82.0f, Font, 0.9f, false);
        DrawText(TEXT("Mouse wheel : zoom   Q/E : rotate"), Ink, 34.0f, 102.0f, Font, 0.9f, false);
        DrawText(TEXT("Home : reset   H : hide help"), Ink, 34.0f, 122.0f, Font, 0.9f, false);
    }
}
