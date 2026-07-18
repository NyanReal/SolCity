#include "Game/SolCityGameMode.h"

#include "City/SolCityGenerator.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Materials/MaterialInterface.h"
#include "Player/CityCameraPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/SolCityAutonomousVehicle.h"
#include "Simulation/SolCityCatCitizen.h"
#include "Simulation/SolCityPedestrianNetwork.h"
#include "Simulation/SolCityRoadNetwork.h"
#include "Simulation/SolCityTrafficSignal.h"

namespace
{
	template<typename T>
	T* SpawnAlways(UWorld* World, const FVector& Location = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World ? World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params) : nullptr;
	}

	void AddUniqueConnection(TArray<int32>& Connections, int32 Candidate)
	{
		if (Candidate != INDEX_NONE && !Connections.Contains(Candidate))
		{
			Connections.Add(Candidate);
		}
	}

	UMaterialInterface* TryLoadMaterial(const TCHAR* AssetPath)
	{
		return LoadObject<UMaterialInterface>(nullptr, AssetPath);
	}
}

ASolCityGameMode::ASolCityGameMode()
{
    DefaultPawnClass = ACityCameraPawn::StaticClass();
}

void ASolCityGameMode::BeginPlay()
{
    Super::BeginPlay();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// A real atmosphere gives the movable skylight a blue environment to capture,
	// keeping shaded facades readable without flattening the sunny art direction.
	SpawnAlways<ASkyAtmosphere>(World);
	if (ADirectionalLight* Sun = SpawnAlways<ADirectionalLight>(World, FVector(0.0f, 0.0f, 8000.0f), FRotator(-52.0f, -35.0f, 0.0f)))
	{
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Sun->GetLightComponent()->SetIntensity(4.25f);
		Sun->GetLightComponent()->SetIndirectLightingIntensity(1.15f);
		Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.92f, 0.82f));
		Sun->GetComponent()->SetAtmosphereSunLight(true);
	}
	if (ASkyLight* Sky = SpawnAlways<ASkyLight>(World))
	{
		USkyLightComponent* SkyComponent = Sky->GetLightComponent();
		SkyComponent->SetMobility(EComponentMobility::Movable);
		SkyComponent->SourceType = SLS_CapturedScene;
		SkyComponent->SetLightColor(FLinearColor(0.68f, 0.82f, 1.0f));
		SkyComponent->SetIntensity(2.25f);
		SkyComponent->SetIndirectLightingIntensity(1.45f);
		SkyComponent->bLowerHemisphereIsBlack = false;
		SkyComponent->SetLowerHemisphereColor(FLinearColor(0.22f, 0.34f, 0.52f));
		SkyComponent->SetRealTimeCaptureEnabled(true);
		SkyComponent->RecaptureSky();
	}

	FTransform GeneratorTransform = FTransform::Identity;
	ASolCityGenerator* Generator = World->SpawnActorDeferred<ASolCityGenerator>(
		ASolCityGenerator::StaticClass(), GeneratorTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Generator)
	{
		return;
	}

	// These assets are produced by Content/Python/SetupSolCityAssets.py. The
	// generator keeps colorful engine-material fallbacks when setup was skipped.
	Generator->GroundMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeGrass.M_AnimeGrass"));
	Generator->RoadMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeAsphalt.M_AnimeAsphalt"));
	Generator->BuildingMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeFacade.M_AnimeFacade"));
	Generator->WaterMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeWater.M_AnimeWater"));
	UGameplayStatics::FinishSpawningActor(Generator, GeneratorTransform);

	const TArray<FVector> SignalLocations = Generator->GetTrafficSignalLocations();
	TArray<ASolCityTrafficSignal*> Signals;
	for (const FVector& Location : SignalLocations)
	{
		Signals.Add(SpawnAlways<ASolCityTrafficSignal>(World, Location));
	}

	ASolCityRoadNetwork* RoadNetwork = SpawnAlways<ASolCityRoadNetwork>(World);
	TArray<FSolCityRoadWaypoint> RoadNodes;
	struct FLaneLink { int32 Start = INDEX_NONE; int32 End = INDEX_NONE; int32 ReverseStart = INDEX_NONE; };
	TArray<FLaneLink> Links;

	for (const FSolCityRoadSegment& Segment : Generator->GetRoadSegments())
	{
		const FVector Direction = (Segment.End - Segment.Start).GetSafeNormal2D();
		if (Direction.IsNearlyZero())
		{
			continue;
		}
		const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
		const float LaneOffset = FMath::Min(180.0f, Segment.HalfWidth * 0.42f);
		const float LaneZ = Segment.bBridge ? 135.0f : 42.0f;
		const float Speed = Segment.RoadClass == ESolCityRoadClass::Arterial ? 760.0f :
			(Segment.RoadClass == ESolCityRoadClass::Local ? 430.0f : 600.0f);

		FLaneLink Forward;
		Forward.Start = RoadNodes.AddDefaulted();
		Forward.End = RoadNodes.AddDefaulted();
		FLaneLink Reverse;
		Reverse.Start = RoadNodes.AddDefaulted();
		Reverse.End = RoadNodes.AddDefaulted();

		RoadNodes[Forward.Start].LocalPosition = Segment.Start + Perpendicular * LaneOffset + FVector(0, 0, LaneZ);
		RoadNodes[Forward.End].LocalPosition = Segment.End + Perpendicular * LaneOffset + FVector(0, 0, LaneZ);
		RoadNodes[Reverse.Start].LocalPosition = Segment.End - Perpendicular * LaneOffset + FVector(0, 0, LaneZ);
		RoadNodes[Reverse.End].LocalPosition = Segment.Start - Perpendicular * LaneOffset + FVector(0, 0, LaneZ);
		RoadNodes[Forward.Start].SpeedLimit = RoadNodes[Forward.End].SpeedLimit = Speed;
		RoadNodes[Reverse.Start].SpeedLimit = RoadNodes[Reverse.End].SpeedLimit = Speed;
		AddUniqueConnection(RoadNodes[Forward.Start].OutgoingNodeIndices, Forward.End);
		AddUniqueConnection(RoadNodes[Reverse.Start].OutgoingNodeIndices, Reverse.End);
		Forward.ReverseStart = Reverse.Start;
		Reverse.ReverseStart = Forward.Start;
		Links.Add(Forward);
		Links.Add(Reverse);
	}

	// Join segment ends at shared/nearby junctions. A U-turn fallback prevents
	// isolated procedural branches from stranding a car.
	for (const FLaneLink& Link : Links)
	{
		if (!RoadNodes.IsValidIndex(Link.End))
		{
			continue;
		}
		for (const FLaneLink& Candidate : Links)
		{
			if (Candidate.Start != Link.Start && RoadNodes.IsValidIndex(Candidate.Start)
				&& FVector::DistSquared2D(RoadNodes[Link.End].LocalPosition, RoadNodes[Candidate.Start].LocalPosition) < FMath::Square(620.0f))
			{
				AddUniqueConnection(RoadNodes[Link.End].OutgoingNodeIndices, Candidate.Start);
			}
		}
		AddUniqueConnection(RoadNodes[Link.End].OutgoingNodeIndices, Link.ReverseStart);
	}

	for (FSolCityRoadWaypoint& Node : RoadNodes)
	{
		float BestSignalDistanceSq = FMath::Square(900.0f);
		for (int32 SignalIndex = 0; SignalIndex < SignalLocations.Num(); ++SignalIndex)
		{
			const float DistanceSq = FVector::DistSquared2D(Node.LocalPosition, SignalLocations[SignalIndex]);
			if (DistanceSq < BestSignalDistanceSq && Signals.IsValidIndex(SignalIndex))
			{
				BestSignalDistanceSq = DistanceSq;
				Node.TrafficSignal = Signals[SignalIndex];
			}
		}
		if (Node.OutgoingNodeIndices.Num() > 0 && RoadNodes.IsValidIndex(Node.OutgoingNodeIndices[0]))
		{
			const FVector Move = RoadNodes[Node.OutgoingNodeIndices[0]].LocalPosition - Node.LocalPosition;
			Node.MovementAxis = FMath::Abs(Move.X) >= FMath::Abs(Move.Y) ? ESolCityTrafficAxis::AxisA : ESolCityTrafficAxis::AxisB;
		}
	}
	RoadNetwork->SetNodes(RoadNodes);

	ASolCityPedestrianNetwork* PedestrianNetwork = SpawnAlways<ASolCityPedestrianNetwork>(World);
	TArray<FSolCityPedestrianWaypoint> PedestrianNodes;
	const TArray<FVector> PedestrianLocations = Generator->GetPedestrianWaypoints();
	PedestrianNodes.SetNum(PedestrianLocations.Num());
	for (int32 Index = 0; Index < PedestrianNodes.Num(); ++Index)
	{
		PedestrianNodes[Index].LocalPosition = PedestrianLocations[Index] + FVector(0, 0, 30.0f);
		const int32 PairIndex = (Index % 2 == 0) ? Index + 1 : Index - 1;
		if (PedestrianNodes.IsValidIndex(PairIndex))
		{
			AddUniqueConnection(PedestrianNodes[Index].ConnectedNodeIndices, PairIndex);
		}
	}
	for (int32 A = 0; A < PedestrianNodes.Num(); ++A)
	{
		for (int32 B = A + 1; B < PedestrianNodes.Num(); ++B)
		{
			if (FVector::DistSquared2D(PedestrianNodes[A].LocalPosition, PedestrianNodes[B].LocalPosition) < FMath::Square(520.0f))
			{
				AddUniqueConnection(PedestrianNodes[A].ConnectedNodeIndices, B);
				AddUniqueConnection(PedestrianNodes[B].ConnectedNodeIndices, A);
			}
		}
	}
	PedestrianNetwork->SetNodes(PedestrianNodes);

	for (int32 Index = 0; Index < VehicleCount && RoadNodes.Num() > 0; ++Index)
	{
		const int32 StartNode = (Index * 11) % RoadNodes.Num();
		if (ASolCityAutonomousVehicle* Vehicle = SpawnAlways<ASolCityAutonomousVehicle>(World, RoadNodes[StartNode].LocalPosition))
		{
			Vehicle->InitializeOnRoad(RoadNetwork, StartNode);
		}
	}
	for (int32 Index = 0; Index < CatCitizenCount && PedestrianNodes.Num() > 0; ++Index)
	{
		const int32 StartNode = (Index * 7) % PedestrianNodes.Num();
		if (ASolCityCatCitizen* Citizen = SpawnAlways<ASolCityCatCitizen>(World, PedestrianNodes[StartNode].LocalPosition))
		{
			Citizen->InitializeOnSidewalk(PedestrianNetwork, StartNode);
		}
	}
}
