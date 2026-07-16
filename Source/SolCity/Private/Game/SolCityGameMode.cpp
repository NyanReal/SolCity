#include "Game/SolCityGameMode.h"

#include "SolCity.h"
#include "Camera/CityCameraPawn.h"
#include "Citizens/CatCitizenSpawner.h"
#include "Player/CityPlayerController.h"
#include "Traffic/CityRoadNetwork.h"
#include "UI/CityHUD.h"
#include "World/CityWorldActor.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ASolCityGameMode::ASolCityGameMode()
{
    DefaultPawnClass = ACityCameraPawn::StaticClass();
    PlayerControllerClass = ACityPlayerController::StaticClass();
    HUDClass = ACityHUD::StaticClass();

    CityWorldClass = ACityWorldActor::StaticClass();
    RoadNetworkClass = ACityRoadNetwork::StaticClass();
    CitizenSpawnerClass = ACatCitizenSpawner::StaticClass();
}

void ASolCityGameMode::StartPlay()
{
    Super::StartPlay();
    BootstrapCity();

    // Opt-in render harness used by Tools/visual verification. Waiting a few
    // seconds avoids capturing the default material while new shaders stream.
    if (FParse::Param(FCommandLine::Get(), TEXT("SolCityCapture")) && GetWorld())
    {
        float CaptureZoom = 0.0f;
        if (FParse::Value(FCommandLine::Get(), TEXT("SolCityCaptureZoom="), CaptureZoom))
        {
            if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
            {
                if (ACityCameraPawn* CameraPawn = Cast<ACityCameraPawn>(PC->GetPawn()))
                {
                    CameraPawn->SetZoomDistance(CaptureZoom, true);
                }
            }
        }

        GetWorldTimerManager().SetTimer(CaptureTimer, this,
            &ASolCityGameMode::CaptureValidationFrame, 5.0f, false);
        GetWorldTimerManager().SetTimer(ExitTimer, this,
            &ASolCityGameMode::FinishValidationCapture, 7.0f, false);
    }
}

void ASolCityGameMode::CaptureValidationFrame()
{
    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        UE_LOG(LogSolCity, Display, TEXT("Capturing delayed Sol City validation frame."));
        PC->ConsoleCommand(TEXT("HighResShot 1"), true);
    }
}

void ASolCityGameMode::FinishValidationCapture()
{
    FPlatformMisc::RequestExit(false);
}

void ASolCityGameMode::BootstrapCity()
{
    UWorld* World = GetWorld();
    if (!World || CityWorld)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CityWorld = World->SpawnActor<ACityWorldActor>(CityWorldClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
    if (!CityWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Sol City could not spawn its procedural world."));
        return;
    }

    RoadNetwork = World->SpawnActor<ACityRoadNetwork>(RoadNetworkClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
    if (RoadNetwork)
    {
        RoadNetwork->SetInitialVehicleCount(CityWorld->VehicleCount);
        const int32 RiverColumn = FMath::RoundToInt(CityWorld->RiverX / FMath::Max(1.0f, CityWorld->RoadSpacing));
        RoadNetwork->Initialize(CityWorld->GridHalfExtent, CityWorld->RoadSpacing, CityWorld->CellSize, RiverColumn);
    }

    CitizenSpawner = World->SpawnActor<ACatCitizenSpawner>(CitizenSpawnerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
    if (CitizenSpawner)
    {
        TArray<FBox> BridgeBoxes;
        for (const FCityBridgeArea& Bridge : CityWorld->GetBridgeAreas())
        {
            FBox WorldBox(ForceInit);
            for (int32 XSign = -1; XSign <= 1; XSign += 2)
            {
                for (int32 YSign = -1; YSign <= 1; YSign += 2)
                {
                    for (int32 ZSign = -1; ZSign <= 1; ZSign += 2)
                    {
                        const FVector LocalCorner(
                            Bridge.Extent.X * static_cast<float>(XSign),
                            Bridge.Extent.Y * static_cast<float>(YSign),
                            Bridge.Extent.Z * static_cast<float>(ZSign));
                        WorldBox += Bridge.Transform.TransformPosition(LocalCorner);
                    }
                }
            }
            BridgeBoxes.Add(WorldBox);
        }

        const float HalfSize = CityWorld->GetCityHalfSize();
        CitizenSpawner->SpawnCitizens(
            CityWorld->CitizenCount,
            FVector(-HalfSize, -HalfSize, 0.0f),
            FVector(HalfSize, HalfSize, 0.0f),
            CityWorld->RoadSpacing,
            CityWorld->RoadWidth,
            CityWorld->GetActorLocation().X + CityWorld->RiverX,
            CityWorld->RiverHalfWidth,
            BridgeBoxes);
    }

    UE_LOG(LogSolCity, Display, TEXT("City bootstrap complete: %d vehicles requested, %d cat citizens spawned, %.0f cm half-size."),
        RoadNetwork ? CityWorld->VehicleCount : 0,
        CitizenSpawner ? CitizenSpawner->GetSpawnedCitizenCount() : 0,
        CityWorld->GetCityHalfSize());
}
