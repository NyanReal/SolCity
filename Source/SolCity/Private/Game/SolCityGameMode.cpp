#include "Game/SolCityGameMode.h"

#include "City/SolCityGenerator.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
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
	DEFINE_LOG_CATEGORY_STATIC(LogSolCityEnvironment, Log, All);

	template<typename T>
	T* SpawnAlways(UWorld* World, const FVector& Location = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World ? World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params) : nullptr;
	}

	template<typename T>
	T* ReuseOrSpawnEnvironmentActor(
		UWorld* World,
		const TCHAR* EnvironmentRole,
		const FVector& SpawnLocation = FVector::ZeroVector,
		const FRotator& SpawnRotation = FRotator::ZeroRotator,
		bool* bOutSpawned = nullptr)
	{
		if (bOutSpawned)
		{
			*bOutSpawned = false;
		}
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<T> It(World); It; ++It)
		{
			T* ExistingActor = *It;
			if (IsValid(ExistingActor) && !ExistingActor->IsActorBeingDestroyed())
			{
				UE_LOG(LogSolCityEnvironment, Log, TEXT("Environment %s: reused %s"), EnvironmentRole, *ExistingActor->GetPathName());
				return ExistingActor;
			}
		}

		T* SpawnedActor = SpawnAlways<T>(World, SpawnLocation, SpawnRotation);
		if (SpawnedActor)
		{
			if (bOutSpawned)
			{
				*bOutSpawned = true;
			}
#if WITH_EDITOR
			SpawnedActor->SetActorLabel(FString::Printf(TEXT("SolCity Runtime %s"), EnvironmentRole));
#endif
			UE_LOG(LogSolCityEnvironment, Log, TEXT("Environment %s: spawned %s"), EnvironmentRole, *SpawnedActor->GetPathName());
		}
		else
		{
			UE_LOG(LogSolCityEnvironment, Error, TEXT("Environment %s: spawn failed"), EnvironmentRole);
		}
		return SpawnedActor;
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
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, AssetPath);
		if (!Material)
		{
			UE_LOG(LogSolCityEnvironment, Error,
				TEXT("Runtime material load failed: %s. Verify ProjectPackagingSettings cook directories."), AssetPath);
		}
		return Material;
	}

	void SpawnDistantGround(UWorld* World, UMaterialInterface* GroundMaterial)
	{
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!World || !PlaneMesh)
		{
			return;
		}

		if (AStaticMeshActor* DistantGround = SpawnAlways<AStaticMeshActor>(World, FVector(0.0f, 0.0f, -280.0f)))
		{
			DistantGround->SetActorScale3D(FVector(3000.0f));
			DistantGround->SetActorEnableCollision(false);
			UStaticMeshComponent* Mesh = DistantGround->GetStaticMeshComponent();
			Mesh->SetStaticMesh(PlaneMesh);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCastShadow(false);
			if (GroundMaterial)
			{
				Mesh->SetMaterial(0, GroundMaterial);
			}
		}
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
	ReuseOrSpawnEnvironmentActor<ASkyAtmosphere>(World, TEXT("Sky Atmosphere"));
	bool bSpawnedSun = false;
	if (ADirectionalLight* Sun = ReuseOrSpawnEnvironmentActor<ADirectionalLight>(
		World, TEXT("Directional Sun"), FVector(0.0f, 0.0f, 8000.0f), FRotator(180.0f, -21.0f, 122.0f), &bSpawnedSun))
	{
		// The requested location/rotation/scale are applied by the spawn path. An
		// authored sun keeps its transform while receiving the shared look settings.
		if (bSpawnedSun)
		{
			Sun->SetActorScale3D(FVector(2.5f));
		}
		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunComponent->SetMobility(EComponentMobility::Movable);
			SunComponent->SetIntensity(3.4f);
			SunComponent->SetIndirectLightingIntensity(1.05f);
			SunComponent->SetLightColor(FLinearColor(1.0f, 0.94f, 0.86f));
			SunComponent->SetVolumetricScatteringIntensity(0.8f);
			SunComponent->SetLightSourceAngle(1.0f);
			SunComponent->SetShadowSourceAngleFactor(1.0f);
			SunComponent->ContactShadowLength = 0.035f;
			SunComponent->ContactShadowLengthInWS = false;
			SunComponent->ContactShadowCastingIntensity = 0.65f;
			SunComponent->ContactShadowNonCastingIntensity = 0.0f;
			SunComponent->SetAtmosphereSunLight(true);
			SunComponent->bCastCloudShadows = true;
			SunComponent->CloudShadowStrength = 0.35f;
			SunComponent->CloudShadowOnSurfaceStrength = 0.35f;
			SunComponent->CloudShadowOnAtmosphereStrength = 0.25f;
			SunComponent->MarkRenderStateDirty();
		}
		else
		{
			UE_LOG(LogSolCityEnvironment, Error, TEXT("Directional Sun has no directional light component: %s"), *Sun->GetPathName());
		}
	}

	if (AExponentialHeightFog* Fog = ReuseOrSpawnEnvironmentActor<AExponentialHeightFog>(
		World, TEXT("Exponential Height Fog"), FVector(0.0f, 0.0f, -100.0f)))
	{
		UExponentialHeightFogComponent* FogComponent = Fog->GetComponent();
		FogComponent->SetFogDensity(0.012f);
		FogComponent->SetFogHeightFalloff(0.16f);
		FogComponent->SetFogInscatteringColor(FLinearColor(0.36f, 0.49f, 0.62f));
		FogComponent->SetDirectionalInscatteringColor(FLinearColor(0.75f, 0.70f, 0.62f));
		FogComponent->SetDirectionalInscatteringStartDistance(18000.0f);
		FogComponent->SetFogMaxOpacity(0.72f);
		FogComponent->SetStartDistance(2200.0f);
		FogComponent->SetEndDistance(220000.0f);
		FogComponent->SetVolumetricFog(true);
		FogComponent->SetVolumetricFogScatteringDistribution(0.2f);
		FogComponent->SetVolumetricFogAlbedo(FColor(205, 220, 232));
		FogComponent->SetVolumetricFogExtinctionScale(0.65f);
		FogComponent->SetVolumetricFogDistance(100000.0f);
	}

	if (AVolumetricCloud* Cloud = ReuseOrSpawnEnvironmentActor<AVolumetricCloud>(World, TEXT("Volumetric Cloud")))
	{
		if (UVolumetricCloudComponent* CloudComponent = Cloud->FindComponentByClass<UVolumetricCloudComponent>())
		{
			CloudComponent->SetLayerBottomAltitude(5.0f);
			CloudComponent->SetLayerHeight(8.0f);
			CloudComponent->SetGroundAlbedo(FColor(110, 138, 102));
			CloudComponent->SetSkyLightCloudBottomOcclusion(0.18f);
			CloudComponent->SetViewSampleCountScale(0.75f);
			CloudComponent->SetShadowViewSampleCountScale(0.75f);
			CloudComponent->SetbUsePerSampleAtmosphericLightTransmittance(true);
			CloudComponent->SetMaterial(TryLoadMaterial(TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")));
		}
	}
	if (ASkyLight* Sky = ReuseOrSpawnEnvironmentActor<ASkyLight>(World, TEXT("Sky Light")))
	{
		USkyLightComponent* SkyComponent = Sky->GetLightComponent();
		SkyComponent->SetMobility(EComponentMobility::Movable);
		SkyComponent->SourceType = SLS_CapturedScene;
		SkyComponent->SetLightColor(FLinearColor(0.68f, 0.82f, 1.0f));
		SkyComponent->SetIntensity(2.35f);
		SkyComponent->SetIndirectLightingIntensity(1.45f);
		SkyComponent->bLowerHemisphereIsBlack = false;
		SkyComponent->SetLowerHemisphereColor(FLinearColor(0.22f, 0.34f, 0.52f));
		SkyComponent->SetRealTimeCaptureEnabled(true);
		SkyComponent->RecaptureSky();
	}

	if (APostProcessVolume* PostProcess = ReuseOrSpawnEnvironmentActor<APostProcessVolume>(World, TEXT("Post Process Volume")))
	{
		PostProcess->bEnabled = true;
		PostProcess->bUnbound = true;
		PostProcess->BlendWeight = 1.0f;
		PostProcess->Priority = 1.0f;

		FPostProcessSettings& Settings = PostProcess->Settings;
		Settings.bOverride_AutoExposureMethod = true;
		Settings.AutoExposureMethod = AEM_Manual;
		Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		Settings.AutoExposureApplyPhysicalCameraExposure = false;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.0f;

		Settings.bOverride_AmbientOcclusionIntensity = true;
		Settings.AmbientOcclusionIntensity = 0.45f;
		Settings.bOverride_AmbientOcclusionRadius = true;
		Settings.AmbientOcclusionRadius = 120.0f;
		Settings.bOverride_AmbientOcclusionPower = true;
		Settings.AmbientOcclusionPower = 1.2f;

		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(0.97f, 0.97f, 0.97f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(1.02f, 1.02f, 1.02f, 1.0f);

		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.25f;
		Settings.bOverride_BloomThreshold = true;
		Settings.BloomThreshold = 1.4f;

		UE_LOG(LogSolCityEnvironment, Log,
			TEXT("Post process: Manual exposure bias=0.0, AO=0.45 radius=120 power=1.2, saturation=0.97, contrast=1.02, bloom=0.25 threshold=1.4"));
	}

	UMaterialInterface* GroundMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeGrass.M_AnimeGrass"));
	UMaterialInterface* DistantGroundMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_DistantGround.M_DistantGround"));
	SpawnDistantGround(World, DistantGroundMaterial ? DistantGroundMaterial : GroundMaterial);

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
	Generator->GroundMaterial = GroundMaterial;
	Generator->RoadMaterial = TryLoadMaterial(TEXT("/Game/Art/Materials/M_AnimeAsphalt.M_AnimeAsphalt"));
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
