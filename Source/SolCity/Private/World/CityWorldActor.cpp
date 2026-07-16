#include "World/CityWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace SolCityWorld
{
	constexpr float BasicMeshSize = 100.0f;
	constexpr float GroundZ = 0.0f;
	constexpr float WaterZ = 4.0f;
	constexpr float RoadZ = 10.0f;
	constexpr float SidewalkZ = 16.0f;
	// Keep bridges visually flat because the simulation has no terrain elevation
	// model. Cars can therefore cross without needing a vertical spline ramp.
	constexpr float BridgeDeckZ = 7.0f;
	constexpr float BridgeRoadZ = 14.0f;
	constexpr float BuildingBaseZ = 18.0f;

	const FLinearColor GrassColour = FLinearColor(0.36f, 0.78f, 0.49f, 1.0f);
	const FLinearColor ParkColour = FLinearColor(0.22f, 0.68f, 0.38f, 1.0f);
	const FLinearColor ParkPathColour = FLinearColor(0.94f, 0.79f, 0.55f, 1.0f);
	const FLinearColor RoadColour = FLinearColor(0.18f, 0.24f, 0.31f, 1.0f);
	const FLinearColor SidewalkColour = FLinearColor(0.80f, 0.82f, 0.79f, 1.0f);
	const FLinearColor MarkingColour = FLinearColor(1.00f, 0.86f, 0.32f, 1.0f);
	const FLinearColor WaterColour = FLinearColor(0.08f, 0.72f, 0.77f, 1.0f);
	const FLinearColor BridgeColour = FLinearColor(0.75f, 0.57f, 0.42f, 1.0f);
	const FLinearColor RoofColour = FLinearColor(0.76f, 0.27f, 0.28f, 1.0f);
	const FLinearColor WindowColour = FLinearColor(1.00f, 0.88f, 0.48f, 1.0f);
	const FLinearColor TrunkColour = FLinearColor(0.43f, 0.25f, 0.16f, 1.0f);
	const FLinearColor CrownColour = FLinearColor(0.18f, 0.62f, 0.30f, 1.0f);
}

ACityWorldActor::ACityWorldActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	auto CreateHISM = [this](const FName Name, bool bCollision, bool bAffectsNavigation)
	{
		UHierarchicalInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(SceneRoot);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Component->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Component->SetCanEverAffectNavigation(bAffectsNavigation);
		Component->SetGenerateOverlapEvents(false);
		return Component;
	};

	GroundTiles = CreateHISM(TEXT("GroundTiles"), true, true);
	ParkTiles = CreateHISM(TEXT("ParkTiles"), false, false);
	ParkPaths = CreateHISM(TEXT("ParkPaths"), false, false);
	RiverTiles = CreateHISM(TEXT("RiverTiles"), false, false);
	Roads = CreateHISM(TEXT("Roads"), true, true);
	Sidewalks = CreateHISM(TEXT("Sidewalks"), true, true);
	RoadMarkings = CreateHISM(TEXT("RoadMarkings"), false, false);
	Bridges = CreateHISM(TEXT("Bridges"), true, true);
	BridgePillars = CreateHISM(TEXT("BridgePillars"), true, false);
	BuildingPeach = CreateHISM(TEXT("BuildingPeach"), true, true);
	BuildingYellow = CreateHISM(TEXT("BuildingYellow"), true, true);
	BuildingMint = CreateHISM(TEXT("BuildingMint"), true, true);
	BuildingBlue = CreateHISM(TEXT("BuildingBlue"), true, true);
	BuildingCafe = CreateHISM(TEXT("BuildingCafe"), true, true);
	BuildingApartment = CreateHISM(TEXT("BuildingApartment"), true, true);
	BuildingTownhouse = CreateHISM(TEXT("BuildingTownhouse"), true, true);
	Roofs = CreateHISM(TEXT("Roofs"), false, false);
	RoofsMetal = CreateHISM(TEXT("RoofsMetal"), false, false);
	RoofsGarden = CreateHISM(TEXT("RoofsGarden"), false, false);
	Windows = CreateHISM(TEXT("Windows"), false, false);
	TreeTrunks = CreateHISM(TEXT("TreeTrunks"), true, true);
	TreeCrowns = CreateHISM(TEXT("TreeCrowns"), false, false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectMaterialFinder(
		TEXT("/Game/Generated/Fallback/M_FallbackTintedLit.M_FallbackTintedLit"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* PlaneMesh = PlaneFinder.Succeeded() ? PlaneFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object.Get() : CubeMesh;
	BasicShapeMaterial = ProjectMaterialFinder.Succeeded()
		? ProjectMaterialFinder.Object.Get()
		: (MaterialFinder.Succeeded() ? MaterialFinder.Object.Get() : nullptr);

	GroundTiles->SetStaticMesh(PlaneMesh);
	ParkTiles->SetStaticMesh(PlaneMesh);
	ParkPaths->SetStaticMesh(PlaneMesh);
	RiverTiles->SetStaticMesh(PlaneMesh);
	Roads->SetStaticMesh(CubeMesh);
	Sidewalks->SetStaticMesh(CubeMesh);
	RoadMarkings->SetStaticMesh(CubeMesh);
	Bridges->SetStaticMesh(CubeMesh);
	BridgePillars->SetStaticMesh(CylinderMesh);
	BuildingPeach->SetStaticMesh(CubeMesh);
	BuildingYellow->SetStaticMesh(CubeMesh);
	BuildingMint->SetStaticMesh(CubeMesh);
	BuildingBlue->SetStaticMesh(CubeMesh);
	BuildingCafe->SetStaticMesh(CubeMesh);
	BuildingApartment->SetStaticMesh(CubeMesh);
	BuildingTownhouse->SetStaticMesh(CubeMesh);
	Roofs->SetStaticMesh(CubeMesh);
	RoofsMetal->SetStaticMesh(CubeMesh);
	RoofsGarden->SetStaticMesh(CubeMesh);
	Windows->SetStaticMesh(CubeMesh);
	TreeTrunks->SetStaticMesh(CylinderMesh);
	TreeCrowns->SetStaticMesh(CylinderMesh);

	RiverTiles->SetTranslucentSortPriority(1);
	WaterMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Art/Materials/M_WaterAnimated.M_WaterAnimated")));
}

void ACityWorldActor::BeginPlay()
{
	Super::BeginPlay();

	if (GroundTiles->GetInstanceCount() == 0)
	{
		GenerateCity();
	}
	else
	{
		EnsureDaylightActors();
	}
}

void ACityWorldActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bGenerateOnConstruction && !HasAnyFlags(RF_ClassDefaultObject))
	{
		GenerateCity();
	}
}

void ACityWorldActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	WaterAnimationTime = FMath::Fmod(WaterAnimationTime + DeltaSeconds, 10000.0f);
	const float UVOffset = FMath::Fmod(WaterAnimationTime * WaterFlowSpeed, 1.0f);

	if (WaterMaterial)
	{
		// Supporting both scalar- and vector-authored versions makes the drop-in
		// M_WaterAnimated material contract forgiving for external art production.
		WaterMaterial->SetScalarParameterValue(TEXT("FlowSpeed"), WaterFlowSpeed);
		WaterMaterial->SetScalarParameterValue(TEXT("UVOffset"), UVOffset);
		WaterMaterial->SetVectorParameterValue(TEXT("UVOffset"), FLinearColor(UVOffset, UVOffset * 0.37f, 0.0f, 0.0f));
	}

	if (!bUsingAnimatedWaterMaterial && RiverTiles)
	{
		const float Wave = FMath::Sin(WaterAnimationTime * 1.4f);
		const float CrossWave = FMath::Sin(WaterAnimationTime * 0.83f + 1.2f);
		RiverTiles->SetRelativeLocation(FVector(Wave * 0.45f, CrossWave * 0.35f, Wave * 0.22f));
		RiverTiles->SetRelativeScale3D(FVector(1.0f + Wave * 0.0015f, 1.0f + CrossWave * 0.0012f, 1.0f));
	}
}

void ACityWorldActor::GenerateCity()
{
	ClearInstances();
	ConfigureMaterials();
	GenerateGroundAndRiver();
	GenerateRoadNetwork();
	GenerateBlocks();

	FRandomStream RoadsideRandom(GenerationSeed ^ 0x5A17);
	GenerateRoadsideTrees(RoadsideRandom);
	EnsureDaylightActors();
}

void ACityWorldActor::EnsureDaylightActors()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bHasDirectionalLight = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		bHasDirectionalLight = true;
		break;
	}
	if (!bHasDirectionalLight)
	{
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-48.0f, -32.0f, 0.0f), SpawnParameters);
		if (Sun)
		{
			UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
			if (Light)
			{
				Light->SetMobility(EComponentMobility::Movable);
				// Use a physical daylight level; the city camera locks exposure to
				// EV100 ~= 13 so pastel albedos keep their hue instead of washing out.
				Light->SetIntensity(65000.0f);
				Light->SetLightColor(FLinearColor(1.0f, 0.91f, 0.77f));
				Light->SetCastShadows(true);
				Light->SetAtmosphereSunLight(true);
				Light->SetAtmosphereSunLightIndex(0);
			}
		}
	}

	bool bHasSkyLight = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		bHasSkyLight = true;
		break;
	}
	if (!bHasSkyLight)
	{
		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if (SkyLight && SkyLight->GetLightComponent())
		{
			USkyLightComponent* Light = SkyLight->GetLightComponent();
			Light->SetMobility(EComponentMobility::Movable);
			// Lift north-facing facades so the authored anime textures remain
			// legible while the directional sun still supplies crisp soft shadows.
			Light->SetIntensity(1.55f);
			Light->SetLightColor(FLinearColor(0.78f, 0.87f, 1.0f));
			Light->SetRealTimeCaptureEnabled(true);
		}
	}

	bool bHasAtmosphere = false;
	for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
	{
		bHasAtmosphere = true;
		break;
	}
	if (!bHasAtmosphere)
	{
		World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	}

	bool bHasFog = false;
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		bHasFog = true;
		break;
	}
	if (!bHasFog)
	{
		AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0.0f, 0.0f, -250.0f), FRotator::ZeroRotator, SpawnParameters);
		if (Fog && Fog->GetComponent())
		{
			UExponentialHeightFogComponent* FogComponent = Fog->GetComponent();
			FogComponent->SetFogDensity(0.0025f);
			FogComponent->SetFogHeightFalloff(0.16f);
			FogComponent->SetFogInscatteringColor(FLinearColor(0.80f, 0.89f, 1.0f));
			FogComponent->SetFogMaxOpacity(0.18f);
			FogComponent->SetStartDistance(2600.0f);
		}
	}
}

void ACityWorldActor::ClearInstances()
{
	const TArray<UHierarchicalInstancedStaticMeshComponent*> Components = {
		GroundTiles, ParkTiles, ParkPaths, RiverTiles, Roads, Sidewalks, RoadMarkings,
		Bridges, BridgePillars, BuildingPeach, BuildingYellow, BuildingMint,
		BuildingBlue, BuildingCafe, BuildingApartment, BuildingTownhouse,
		Roofs, RoofsMetal, RoofsGarden, Windows, TreeTrunks, TreeCrowns
	};

	for (UHierarchicalInstancedStaticMeshComponent* Component : Components)
	{
		if (Component)
		{
			Component->ClearInstances();
			Component->SetRelativeLocation(FVector::ZeroVector);
			Component->SetRelativeScale3D(FVector::OneVector);
		}
	}

	LocalBridgeAreas.Reset();
	BuildingFootprints.Reset();
	WaterAnimationTime = 0.0f;
}

UMaterialInstanceDynamic* ACityWorldActor::MakeColourMaterial(const FLinearColor& Colour, const TCHAR* DebugName)
{
	if (!BasicShapeMaterial)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this, FName(DebugName));
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Colour);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Colour);
		Material->SetVectorParameterValue(TEXT("Base Color"), Colour);
	}
	return Material;
}

void ACityWorldActor::ConfigureMaterials()
{
	auto GeneratedOrFallback = [](const TCHAR* AssetPath, UMaterialInterface* Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Generated = LoadObject<UMaterialInterface>(nullptr, AssetPath, nullptr, LOAD_NoWarn))
		{
			return Generated;
		}
		return Fallback;
	};

	UMaterialInterface* GrassMaterial = GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_GroundGrass_Anime.M_GroundGrass_Anime"),
		MakeColourMaterial(SolCityWorld::GrassColour, TEXT("MI_Grass_Runtime")));
	// Prefer the corrected seamless revision as soon as the external generation
	// watcher imports it, while retaining the first version as an offline fallback.
	GrassMaterial = GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_GroundGrass_Anime_v2.M_GroundGrass_Anime_v2"),
		GrassMaterial);
	GroundTiles->SetMaterial(0, GrassMaterial);
	ParkTiles->SetMaterial(0, GrassMaterial);
	ParkPaths->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_ParkPath_Anime.M_ParkPath_Anime"),
		MakeColourMaterial(SolCityWorld::ParkPathColour, TEXT("MI_ParkPath_Runtime"))));
	Roads->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_RoadAsphalt_Anime.M_RoadAsphalt_Anime"),
		MakeColourMaterial(SolCityWorld::RoadColour, TEXT("MI_Road_Runtime"))));
	Sidewalks->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_Sidewalk_Anime.M_Sidewalk_Anime"),
		MakeColourMaterial(SolCityWorld::SidewalkColour, TEXT("MI_Sidewalk_Runtime"))));
	RoadMarkings->SetMaterial(0, MakeColourMaterial(SolCityWorld::MarkingColour, TEXT("MI_Marking_Runtime")));
	Bridges->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_BridgeDeck_Anime.M_BridgeDeck_Anime"),
		MakeColourMaterial(SolCityWorld::BridgeColour, TEXT("MI_Bridge_Runtime"))));
	BridgePillars->SetMaterial(0, MakeColourMaterial(SolCityWorld::SidewalkColour, TEXT("MI_BridgePillar_Runtime")));
	BuildingPeach->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeWarm_Anime.M_FacadeWarm_Anime"),
		MakeColourMaterial(FLinearColor(1.00f, 0.53f, 0.47f), TEXT("MI_Peach_Runtime"))));
	BuildingYellow->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeWarm_Anime.M_FacadeWarm_Anime"),
		MakeColourMaterial(FLinearColor(1.00f, 0.78f, 0.34f), TEXT("MI_Yellow_Runtime"))));
	BuildingMint->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeCool_Anime.M_FacadeCool_Anime"),
		MakeColourMaterial(FLinearColor(0.38f, 0.84f, 0.66f), TEXT("MI_Mint_Runtime"))));
	BuildingBlue->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeCool_Anime.M_FacadeCool_Anime"),
		MakeColourMaterial(FLinearColor(0.38f, 0.69f, 0.94f), TEXT("MI_Blue_Runtime"))));
	BuildingCafe->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeCafe_Anime.M_FacadeCafe_Anime"),
		MakeColourMaterial(FLinearColor(0.96f, 0.61f, 0.38f), TEXT("MI_Cafe_Runtime"))));
	BuildingApartment->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeApartment_Anime.M_FacadeApartment_Anime"),
		MakeColourMaterial(FLinearColor(0.55f, 0.75f, 0.94f), TEXT("MI_Apartment_Runtime"))));
	BuildingTownhouse->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_FacadeTownhouse_Anime.M_FacadeTownhouse_Anime"),
		MakeColourMaterial(FLinearColor(0.58f, 0.70f, 0.45f), TEXT("MI_Townhouse_Runtime"))));
	Roofs->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_RoofTile_Anime.M_RoofTile_Anime"),
		MakeColourMaterial(SolCityWorld::RoofColour, TEXT("MI_Roof_Runtime"))));
	RoofsMetal->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_RoofMetal_Anime.M_RoofMetal_Anime"),
		MakeColourMaterial(FLinearColor(0.28f, 0.68f, 0.74f), TEXT("MI_RoofMetal_Runtime"))));
	RoofsGarden->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_RoofGarden_Anime.M_RoofGarden_Anime"),
		MakeColourMaterial(FLinearColor(0.42f, 0.72f, 0.38f), TEXT("MI_RoofGarden_Runtime"))));
	Windows->SetMaterial(0, MakeColourMaterial(SolCityWorld::WindowColour, TEXT("MI_Window_Runtime")));
	TreeTrunks->SetMaterial(0, MakeColourMaterial(SolCityWorld::TrunkColour, TEXT("MI_TreeTrunk_Runtime")));
	TreeCrowns->SetMaterial(0, GeneratedOrFallback(
		TEXT("/Game/Generated/Materials/M_TreeFoliage_Anime.M_TreeFoliage_Anime"),
		MakeColourMaterial(SolCityWorld::CrownColour, TEXT("MI_TreeCrown_Runtime"))));

	UMaterialInterface* WaterSource = nullptr;
	if (!WaterMaterialAsset.IsNull())
	{
		WaterSource = WaterMaterialAsset.Get();
		if (!WaterSource)
		{
			WaterSource = LoadObject<UMaterialInterface>(nullptr, *WaterMaterialAsset.ToSoftObjectPath().ToString(), nullptr, LOAD_NoWarn);
		}
	}

	if (!WaterSource)
	{
		const TCHAR* CandidatePaths[] = {
			TEXT("/Game/Materials/M_WaterAnimated.M_WaterAnimated"),
			TEXT("/Game/Environment/Materials/M_WaterAnimated.M_WaterAnimated"),
			TEXT("/Game/Generated/Materials/M_WaterAnimated.M_WaterAnimated")
		};

		for (const TCHAR* Path : CandidatePaths)
		{
			WaterSource = LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn);
			if (WaterSource)
			{
				break;
			}
		}
	}

	bUsingAnimatedWaterMaterial = WaterSource != nullptr;
	if (!WaterSource)
	{
		WaterSource = BasicShapeMaterial;
	}

	WaterMaterial = WaterSource ? UMaterialInstanceDynamic::Create(WaterSource, this, TEXT("MI_Water_Runtime")) : nullptr;
	if (WaterMaterial)
	{
		if (!bUsingAnimatedWaterMaterial)
		{
			WaterMaterial->SetVectorParameterValue(TEXT("Color"), SolCityWorld::WaterColour);
			WaterMaterial->SetVectorParameterValue(TEXT("BaseColor"), SolCityWorld::WaterColour);
			WaterMaterial->SetVectorParameterValue(TEXT("Base Color"), SolCityWorld::WaterColour);
		}
		RiverTiles->SetMaterial(0, WaterMaterial);
	}

}

void ACityWorldActor::GenerateGroundAndRiver()
{
	const float HalfSize = GetCityHalfSize();
	const int32 TileCount = FMath::Max(1, FMath::CeilToInt((HalfSize * 2.0f) / FMath::Max(CellSize, 100.0f)));
	const float ActualTileSize = (HalfSize * 2.0f) / static_cast<float>(TileCount);

	for (int32 X = 0; X < TileCount; ++X)
	{
		for (int32 Y = 0; Y < TileCount; ++Y)
		{
			const FVector Location(
				-HalfSize + (X + 0.5f) * ActualTileSize,
				-HalfSize + (Y + 0.5f) * ActualTileSize,
				SolCityWorld::GroundZ);
			const FVector Scale(ActualTileSize / SolCityWorld::BasicMeshSize, ActualTileSize / SolCityWorld::BasicMeshSize, 1.0f);
			GroundTiles->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));
		}
	}

	const int32 RiverTileCount = FMath::Max(1, FMath::CeilToInt((HalfSize * 2.0f) / FMath::Max(CellSize, 100.0f)));
	const float RiverTileLength = (HalfSize * 2.0f) / static_cast<float>(RiverTileCount);
	for (int32 Y = 0; Y < RiverTileCount; ++Y)
	{
		const FVector Location(RiverX, -HalfSize + (Y + 0.5f) * RiverTileLength, SolCityWorld::WaterZ);
		const FVector Scale((RiverHalfWidth * 2.0f) / SolCityWorld::BasicMeshSize, RiverTileLength / SolCityWorld::BasicMeshSize, 1.0f);
		RiverTiles->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));
	}

	// Slim mint-green riverside promenades soften the water/grass boundary.
	const float BankWidth = FMath::Max(70.0f, SidewalkWidth * 0.65f);
	for (const float Side : {-1.0f, 1.0f})
	{
		const FVector BankLocation(RiverX + Side * (RiverHalfWidth + BankWidth * 0.5f), 0.0f, SolCityWorld::SidewalkZ - 3.0f);
		const FVector BankScale(BankWidth / SolCityWorld::BasicMeshSize, (HalfSize * 2.0f) / SolCityWorld::BasicMeshSize, 0.06f);
		Sidewalks->AddInstance(FTransform(FRotator::ZeroRotator, BankLocation, BankScale));
	}
}

void ACityWorldActor::GenerateRoadNetwork()
{
	const float HalfSize = GetCityHalfSize();
	const float Spacing = GetSafeRoadSpacing();
	const float Width = GetSafeRoadWidth();
	const float FullLength = HalfSize * 2.0f;
	const float SlabHeight = 12.0f;

	for (int32 LineIndex = -GridHalfExtent; LineIndex <= GridHalfExtent; ++LineIndex)
	{
		const float Coordinate = LineIndex * Spacing;

		Roads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Coordinate, SolCityWorld::RoadZ),
			FVector(FullLength / SolCityWorld::BasicMeshSize, Width / SolCityWorld::BasicMeshSize, SlabHeight / SolCityWorld::BasicMeshSize)));
		Roads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Coordinate, 0.0f, SolCityWorld::RoadZ),
			FVector(Width / SolCityWorld::BasicMeshSize, FullLength / SolCityWorld::BasicMeshSize, SlabHeight / SolCityWorld::BasicMeshSize)));

		const float SideOffset = (Width + SidewalkWidth) * 0.5f;
		for (const float Side : {-1.0f, 1.0f})
		{
			Sidewalks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Coordinate + Side * SideOffset, SolCityWorld::SidewalkZ),
				FVector(FullLength / SolCityWorld::BasicMeshSize, SidewalkWidth / SolCityWorld::BasicMeshSize, 0.12f)));
			Sidewalks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Coordinate + Side * SideOffset, 0.0f, SolCityWorld::SidewalkZ),
				FVector(SidewalkWidth / SolCityWorld::BasicMeshSize, FullLength / SolCityWorld::BasicMeshSize, 0.12f)));
		}

		// Every horizontal road receives a raised bridge over the north/south river.
		const float BridgeLength = RiverHalfWidth * 2.0f + SidewalkWidth * 3.0f;
		const float BridgeWidth = Width + SidewalkWidth * 2.0f;
		const FVector BridgeCentre(RiverX, Coordinate, SolCityWorld::BridgeDeckZ);
		Bridges->AddInstance(FTransform(FRotator::ZeroRotator, BridgeCentre,
			FVector(BridgeLength / SolCityWorld::BasicMeshSize, BridgeWidth / SolCityWorld::BasicMeshSize, 0.18f)));

		// Restore a dark roadway and two pale pedestrian shoulders above the bridge deck.
		Roads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(RiverX, Coordinate, SolCityWorld::BridgeRoadZ),
			FVector(BridgeLength / SolCityWorld::BasicMeshSize, Width / SolCityWorld::BasicMeshSize, 0.05f)));
		for (const float Side : {-1.0f, 1.0f})
		{
			Sidewalks->AddInstance(FTransform(FRotator::ZeroRotator,
				FVector(RiverX, Coordinate + Side * (Width + SidewalkWidth) * 0.5f, SolCityWorld::BridgeRoadZ + 2.0f),
				FVector(BridgeLength / SolCityWorld::BasicMeshSize, SidewalkWidth / SolCityWorld::BasicMeshSize, 0.08f)));
		}

		for (const float PillarSide : {-0.58f, 0.58f})
		{
			const float PillarX = RiverX + PillarSide * RiverHalfWidth;
			BridgePillars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(PillarX, Coordinate - BridgeWidth * 0.36f, 10.0f), FVector(0.55f, 0.55f, 0.30f)));
			BridgePillars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(PillarX, Coordinate + BridgeWidth * 0.36f, 10.0f), FVector(0.55f, 0.55f, 0.30f)));
		}

		FCityBridgeArea LocalArea;
		LocalArea.Transform = FTransform(FRotator::ZeroRotator, BridgeCentre, FVector::OneVector);
		LocalArea.Extent = FVector(BridgeLength * 0.5f, BridgeWidth * 0.5f, 80.0f);
		LocalBridgeAreas.Add(LocalArea);
	}

	// Centre-line dashes. Their height follows raised bridge road surfaces.
	constexpr float DashLength = 150.0f;
	constexpr float DashGap = 150.0f;
	for (int32 LineIndex = -GridHalfExtent; LineIndex <= GridHalfExtent; ++LineIndex)
	{
		const float Coordinate = LineIndex * Spacing;
		for (float Along = -HalfSize + DashLength; Along < HalfSize; Along += DashLength + DashGap)
		{
			const FVector2D HorizontalPoint(Along, Coordinate);
			const float HorizontalZ = IsBridgeAtLocal(HorizontalPoint) ? SolCityWorld::BridgeRoadZ + 5.0f : SolCityWorld::RoadZ + 8.0f;
			RoadMarkings->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Along, Coordinate, HorizontalZ),
				FVector(DashLength / SolCityWorld::BasicMeshSize, 0.10f, 0.025f)));

			const FVector2D VerticalPoint(Coordinate, Along);
			const float VerticalZ = IsBridgeAtLocal(VerticalPoint) ? SolCityWorld::BridgeRoadZ + 5.0f : SolCityWorld::RoadZ + 8.0f;
			RoadMarkings->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Coordinate, Along, VerticalZ),
				FVector(0.10f, DashLength / SolCityWorld::BasicMeshSize, 0.025f)));
		}
	}
}

void ACityWorldActor::GenerateBlocks()
{
	FRandomStream RandomStream(GenerationSeed);
	const float Spacing = GetSafeRoadSpacing();
	const float Width = GetSafeRoadWidth();
	const float Setback = Width * 0.5f + SidewalkWidth + 45.0f;
	const float LotTarget = FMath::Max(BlockSize, 250.0f);

	for (int32 BlockX = -GridHalfExtent; BlockX < GridHalfExtent; ++BlockX)
	{
		for (int32 BlockY = -GridHalfExtent; BlockY < GridHalfExtent; ++BlockY)
		{
			const float MinX = BlockX * Spacing + Setback;
			const float MaxX = (BlockX + 1) * Spacing - Setback;
			const float MinY = BlockY * Spacing + Setback;
			const float MaxY = (BlockY + 1) * Spacing - Setback;
			if (MaxX <= MinX || MaxY <= MinY)
			{
				continue;
			}

			// Blocks cut by the river become open waterfront rather than clipping buildings into water.
			const bool bTouchesRiver = MinX < RiverX + RiverHalfWidth + 120.0f && MaxX > RiverX - RiverHalfWidth - 120.0f;
			if (bTouchesRiver)
			{
				continue;
			}

			const FVector2D Centre((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
			const FVector2D InteriorSize(MaxX - MinX, MaxY - MinY);

			// Chebyshev distance follows the square road grid. The old three-block
			// radius remains fully dense, then a smoothstep curve creates two
			// increasingly sparse suburban rings without adding new actor types.
			const float BlockRadius = FMath::Max(
				FMath::Abs(static_cast<float>(BlockX) + 0.5f),
				FMath::Abs(static_cast<float>(BlockY) + 0.5f));
			const float CoreEdge = FMath::Max(0.5f, static_cast<float>(FMath::Min(CoreGridHalfExtent, GridHalfExtent)) - 0.5f);
			const float OuterEdge = FMath::Max(CoreEdge + 1.0f, static_cast<float>(GridHalfExtent) - 0.5f);
			const float LinearFalloff = FMath::Clamp((BlockRadius - CoreEdge) / (OuterEdge - CoreEdge), 0.0f, 1.0f);
			const float OuterFalloff = LinearFalloff * LinearFalloff * (3.0f - 2.0f * LinearFalloff);
			const float LocalParkRatio = FMath::Clamp(ParkRatio, 0.0f, 0.8f) * FMath::Lerp(1.0f, 0.72f, OuterFalloff);
			const bool bPark = RandomStream.FRand() < LocalParkRatio;

			if (bPark)
			{
				ParkTiles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Centre.X, Centre.Y, 2.0f),
					FVector(InteriorSize.X / SolCityWorld::BasicMeshSize, InteriorSize.Y / SolCityWorld::BasicMeshSize, 1.0f)));

				// Keep the block itself grassy and use two thin, independently
				// instanced strips for the park's simple cross-shaped promenade.
				const float PathWidth = FMath::Clamp(FMath::Min(InteriorSize.X, InteriorSize.Y) * 0.10f, 105.0f, 165.0f);
				const float PathEndInset = FMath::Min(90.0f, FMath::Min(InteriorSize.X, InteriorSize.Y) * 0.08f);
				ParkPaths->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Centre.X, Centre.Y, 3.0f),
					FVector((InteriorSize.X - PathEndInset * 2.0f) / SolCityWorld::BasicMeshSize,
						PathWidth / SolCityWorld::BasicMeshSize, 1.0f)));
				ParkPaths->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Centre.X, Centre.Y, 3.2f),
					FVector(PathWidth / SolCityWorld::BasicMeshSize,
						(InteriorSize.Y - PathEndInset * 2.0f) / SolCityWorld::BasicMeshSize, 1.0f)));

				const int32 CoreTreeCount = FMath::Clamp(FMath::RoundToInt(InteriorSize.Size() / 330.0f), 5, 16);
				const int32 TreeCount = FMath::Clamp(
					FMath::RoundToInt(static_cast<float>(CoreTreeCount) * FMath::Lerp(1.0f, 0.55f, OuterFalloff)), 3, 16);
				for (int32 TreeIndex = 0; TreeIndex < TreeCount; ++TreeIndex)
				{
					for (int32 PlacementAttempt = 0; PlacementAttempt < 8; ++PlacementAttempt)
					{
						const FVector2D TreePoint(
							RandomStream.FRandRange(MinX + 100.0f, MaxX - 100.0f),
							RandomStream.FRandRange(MinY + 100.0f, MaxY - 100.0f));
						const float PathClearance = PathWidth * 0.5f + 75.0f;
						if (FMath::Abs(TreePoint.X - Centre.X) > PathClearance
							&& FMath::Abs(TreePoint.Y - Centre.Y) > PathClearance)
						{
							AddTree(TreePoint, RandomStream.FRandRange(0.80f, 1.25f));
							break;
						}
					}
				}
				continue;
			}

			const int32 LotsX = FMath::Clamp(FMath::FloorToInt(InteriorSize.X / LotTarget), 1, 4);
			const int32 LotsY = FMath::Clamp(FMath::FloorToInt(InteriorSize.Y / LotTarget), 1, 4);
			const float LotStepX = InteriorSize.X / static_cast<float>(LotsX);
			const float LotStepY = InteriorSize.Y / static_cast<float>(LotsY);
			const float LotOccupancy = FMath::Lerp(0.84f, FMath::Clamp(OuterLotOccupancy, 0.05f, 0.84f), OuterFalloff);
			const float FootprintScale = FMath::Lerp(1.0f, FMath::Clamp(OuterBuildingScale, 0.45f, 1.0f), OuterFalloff);
			const float VacantTreeChance = FMath::Lerp(1.0f, 0.35f, OuterFalloff);

			for (int32 LotX = 0; LotX < LotsX; ++LotX)
			{
				for (int32 LotY = 0; LotY < LotsY; ++LotY)
				{
					const FVector2D LotCentre(MinX + (LotX + 0.5f) * LotStepX, MinY + (LotY + 0.5f) * LotStepY);
					if (RandomStream.FRand() > LotOccupancy)
					{
						if (RandomStream.FRand() < VacantTreeChance)
						{
							AddTree(LotCentre, RandomStream.FRandRange(0.68f, 1.05f));
						}
						continue;
					}

					const FVector2D BuildingSize(
						LotStepX * RandomStream.FRandRange(0.58f, 0.80f) * FootprintScale,
						LotStepY * RandomStream.FRandRange(0.58f, 0.80f) * FootprintScale);
					const float MinimumHeight = FMath::Lerp(380.0f, 260.0f, OuterFalloff);
					const float MaximumHeight = FMath::Max(
						MinimumHeight + 120.0f,
						FMath::Lerp(1060.0f, FMath::Max(OuterMaxBuildingHeight, 300.0f), OuterFalloff));
					const float Height = FMath::GridSnap(RandomStream.FRandRange(MinimumHeight, MaximumHeight), 120.0f);
					// A spatial hash guarantees all seven facade palettes are spread
					// across nearby blocks instead of occasionally clustering by chance.
					const int32 PaletteHash = GenerationSeed
						+ (BlockX + GridHalfExtent) * 37
						+ (BlockY + GridHalfExtent) * 19
						+ LotX * 5 + LotY * 3;
					AddBuilding(LotCentre, BuildingSize, Height, FMath::Abs(PaletteHash) % 7, RandomStream);
				}
			}
		}
	}
}

void ACityWorldActor::AddBuilding(const FVector2D& Centre, const FVector2D& Size, float Height, int32 PaletteIndex, FRandomStream& RandomStream)
{
	UHierarchicalInstancedStaticMeshComponent* BodyComponent = BuildingPeach;
	switch (PaletteIndex % 7)
	{
	case 1: BodyComponent = BuildingYellow; break;
	case 2: BodyComponent = BuildingMint; break;
	case 3: BodyComponent = BuildingBlue; break;
	case 4: BodyComponent = BuildingCafe; break;
	case 5: BodyComponent = BuildingApartment; break;
	case 6: BodyComponent = BuildingTownhouse; break;
	default: break;
	}

	const float BodyCentreZ = SolCityWorld::BuildingBaseZ + Height * 0.5f;
	BodyComponent->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Centre.X, Centre.Y, BodyCentreZ),
		FVector(Size.X / SolCityWorld::BasicMeshSize, Size.Y / SolCityWorld::BasicMeshSize, Height / SolCityWorld::BasicMeshSize)));

	UHierarchicalInstancedStaticMeshComponent* RoofComponent = Roofs;
	switch (PaletteIndex % 7)
	{
	case 1:
	case 2:
	case 5:
		RoofComponent = RoofsMetal;
		break;
	case 3:
	case 6:
		RoofComponent = RoofsGarden;
		break;
	default:
		break;
	}

	const float RoofZ = SolCityWorld::BuildingBaseZ + Height + 22.0f;
	RoofComponent->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Centre.X, Centre.Y, RoofZ),
		FVector(Size.X * 1.07f / SolCityWorld::BasicMeshSize, Size.Y * 1.07f / SolCityWorld::BasicMeshSize, 0.38f)));

	// A small rooftop box breaks up the skyline while keeping the same box-model language.
	if (RoofComponent != RoofsGarden && RandomStream.FRand() < 0.58f)
	{
		const FVector2D UnitSize(Size.X * RandomStream.FRandRange(0.22f, 0.38f), Size.Y * RandomStream.FRandRange(0.22f, 0.38f));
		const float UnitHeight = RandomStream.FRandRange(70.0f, 140.0f);
		RoofComponent->AddInstance(FTransform(FRotator::ZeroRotator,
			FVector(Centre.X + Size.X * 0.18f, Centre.Y - Size.Y * 0.16f, RoofZ + UnitHeight * 0.5f),
			FVector(UnitSize.X / SolCityWorld::BasicMeshSize, UnitSize.Y / SolCityWorld::BasicMeshSize, UnitHeight / SolCityWorld::BasicMeshSize)));
	}

	// The direct imagegen facades already contain authored windows, balconies,
	// doors, awnings, pipes, and shop details. Keep the inexpensive box-window
	// overlay only on the simpler legacy facades so details do not double up.
	if ((PaletteIndex % 7) < 4)
	{
		AddWindows(Centre, Size, Height);
	}
	BuildingFootprints.Add(FBox2D(Centre - Size * 0.54f, Centre + Size * 0.54f));
}

void ACityWorldActor::AddWindows(const FVector2D& Centre, const FVector2D& Size, float Height)
{
	const int32 FloorCount = FMath::Clamp(FMath::FloorToInt((Height - 80.0f) / 145.0f), 2, 7);
	const int32 WindowsX = FMath::Clamp(FMath::FloorToInt(Size.X / 180.0f), 1, 4);
	const int32 WindowsY = FMath::Clamp(FMath::FloorToInt(Size.Y / 180.0f), 1, 4);
	const float WindowWidth = 68.0f;
	const float WindowHeight = 74.0f;
	const float WindowDepth = 8.0f;

	for (int32 Floor = 0; Floor < FloorCount; ++Floor)
	{
		const float Z = SolCityWorld::BuildingBaseZ + 105.0f + Floor * 145.0f;
		for (int32 WindowIndex = 0; WindowIndex < WindowsX; ++WindowIndex)
		{
			const float Alpha = (WindowIndex + 1.0f) / (WindowsX + 1.0f);
			const float X = Centre.X - Size.X * 0.5f + Alpha * Size.X;
			for (const float Side : {-1.0f, 1.0f})
			{
				Windows->AddInstance(FTransform(FRotator::ZeroRotator,
					FVector(X, Centre.Y + Side * (Size.Y * 0.5f + WindowDepth), Z),
					FVector(WindowWidth / SolCityWorld::BasicMeshSize, WindowDepth / SolCityWorld::BasicMeshSize, WindowHeight / SolCityWorld::BasicMeshSize)));
			}
		}

		for (int32 WindowIndex = 0; WindowIndex < WindowsY; ++WindowIndex)
		{
			const float Alpha = (WindowIndex + 1.0f) / (WindowsY + 1.0f);
			const float Y = Centre.Y - Size.Y * 0.5f + Alpha * Size.Y;
			for (const float Side : {-1.0f, 1.0f})
			{
				Windows->AddInstance(FTransform(FRotator::ZeroRotator,
					FVector(Centre.X + Side * (Size.X * 0.5f + WindowDepth), Y, Z),
					FVector(WindowDepth / SolCityWorld::BasicMeshSize, WindowWidth / SolCityWorld::BasicMeshSize, WindowHeight / SolCityWorld::BasicMeshSize)));
			}
		}
	}
}

void ACityWorldActor::AddTree(const FVector2D& Centre, float SizeScale)
{
	const float TrunkHeight = 150.0f * SizeScale;
	const float CrownHeight = 150.0f * SizeScale;
	const float CrownDiameter = 175.0f * SizeScale;
	TreeTrunks->AddInstance(FTransform(FRotator::ZeroRotator,
		FVector(Centre.X, Centre.Y, SolCityWorld::BuildingBaseZ + TrunkHeight * 0.5f),
		FVector(0.42f * SizeScale, 0.42f * SizeScale, TrunkHeight / SolCityWorld::BasicMeshSize)));
	TreeCrowns->AddInstance(FTransform(FRotator::ZeroRotator,
		FVector(Centre.X, Centre.Y, SolCityWorld::BuildingBaseZ + TrunkHeight + CrownHeight * 0.35f),
		FVector(CrownDiameter / SolCityWorld::BasicMeshSize, CrownDiameter / SolCityWorld::BasicMeshSize, CrownHeight / SolCityWorld::BasicMeshSize)));
	TreeCrowns->AddInstance(FTransform(FRotator::ZeroRotator,
		FVector(Centre.X, Centre.Y, SolCityWorld::BuildingBaseZ + TrunkHeight + CrownHeight * 1.05f),
		FVector(CrownDiameter * 0.70f / SolCityWorld::BasicMeshSize, CrownDiameter * 0.70f / SolCityWorld::BasicMeshSize, CrownHeight * 0.72f / SolCityWorld::BasicMeshSize)));
}

void ACityWorldActor::GenerateRoadsideTrees(FRandomStream& RandomStream)
{
	const float HalfSize = GetCityHalfSize();
	const float Step = FMath::Max(GetSafeRoadSpacing() * 0.42f, 700.0f);
	for (const float Side : {-1.0f, 1.0f})
	{
		const float X = RiverX + Side * (RiverHalfWidth + SidewalkWidth + 105.0f);
		for (float Y = -HalfSize + Step * 0.5f; Y < HalfSize; Y += Step)
		{
			const FVector2D Point(X, Y + RandomStream.FRandRange(-70.0f, 70.0f));
			if (!IsRoadAtLocal(Point) && !IsRiverAtLocal(Point))
			{
				AddTree(Point, RandomStream.FRandRange(0.75f, 1.05f));
			}
		}
	}
}

float ACityWorldActor::GetSafeRoadSpacing() const
{
	return FMath::Max(RoadSpacing, 800.0f);
}

float ACityWorldActor::GetSafeRoadWidth() const
{
	return FMath::Clamp(RoadWidth, 200.0f, GetSafeRoadSpacing() * 0.55f);
}

float ACityWorldActor::GetCityHalfSize() const
{
	return FMath::Max(1, GridHalfExtent) * GetSafeRoadSpacing();
}

bool ACityWorldActor::IsRoadAtLocal(const FVector2D& LocalPoint) const
{
	const float HalfSize = GetCityHalfSize();
	if (FMath::Abs(LocalPoint.X) > HalfSize || FMath::Abs(LocalPoint.Y) > HalfSize)
	{
		return false;
	}

	const float Spacing = GetSafeRoadSpacing();
	const float HalfWidth = GetSafeRoadWidth() * 0.5f;
	const float NearestX = FMath::RoundToFloat(LocalPoint.X / Spacing) * Spacing;
	const float NearestY = FMath::RoundToFloat(LocalPoint.Y / Spacing) * Spacing;
	return FMath::Abs(LocalPoint.X - NearestX) <= HalfWidth || FMath::Abs(LocalPoint.Y - NearestY) <= HalfWidth;
}

bool ACityWorldActor::IsRiverAtLocal(const FVector2D& LocalPoint) const
{
	return FMath::Abs(LocalPoint.X - RiverX) <= FMath::Max(RiverHalfWidth, 150.0f)
		&& FMath::Abs(LocalPoint.Y) <= GetCityHalfSize();
}

bool ACityWorldActor::IsBridgeAtLocal(const FVector2D& LocalPoint) const
{
	const float BridgeHalfLength = RiverHalfWidth + SidewalkWidth * 1.5f;
	if (FMath::Abs(LocalPoint.X - RiverX) > BridgeHalfLength)
	{
		return false;
	}

	const float Spacing = GetSafeRoadSpacing();
	const float NearestRoadY = FMath::RoundToFloat(LocalPoint.Y / Spacing) * Spacing;
	const bool bWithinRoadGrid = FMath::Abs(NearestRoadY) <= GridHalfExtent * Spacing + KINDA_SMALL_NUMBER;
	const float BridgeHalfWidth = GetSafeRoadWidth() * 0.5f + SidewalkWidth;
	return bWithinRoadGrid && FMath::Abs(LocalPoint.Y - NearestRoadY) <= BridgeHalfWidth;
}

bool ACityWorldActor::IsWalkableAtLocal(const FVector2D& LocalPoint) const
{
	const float HalfSize = GetCityHalfSize();
	if (FMath::Abs(LocalPoint.X) > HalfSize - 40.0f || FMath::Abs(LocalPoint.Y) > HalfSize - 40.0f)
	{
		return false;
	}

	if (IsRoadAtLocal(LocalPoint))
	{
		return false;
	}

	if (IsRiverAtLocal(LocalPoint) && !IsBridgeAtLocal(LocalPoint))
	{
		return false;
	}

	for (const FBox2D& Footprint : BuildingFootprints)
	{
		if (Footprint.IsInside(LocalPoint))
		{
			return false;
		}
	}
	return true;
}

float ACityWorldActor::GetSurfaceHeightAtLocal(const FVector2D& LocalPoint) const
{
	return IsBridgeAtLocal(LocalPoint) && IsRiverAtLocal(LocalPoint)
		? SolCityWorld::BridgeRoadZ + 8.0f
		: SolCityWorld::SidewalkZ + 4.0f;
}

bool ACityWorldActor::IsRoadAt(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return IsRoadAtLocal(FVector2D(Local.X, Local.Y));
}

bool ACityWorldActor::IsRiverAt(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return IsRiverAtLocal(FVector2D(Local.X, Local.Y));
}

bool ACityWorldActor::IsWalkableAt(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return IsWalkableAtLocal(FVector2D(Local.X, Local.Y));
}

FVector ACityWorldActor::GetRandomWalkablePoint() const
{
	const float HalfSize = GetCityHalfSize() - 80.0f;
	for (int32 Attempt = 0; Attempt < 256; ++Attempt)
	{
		const FVector2D Candidate(FMath::FRandRange(-HalfSize, HalfSize), FMath::FRandRange(-HalfSize, HalfSize));
		if (IsWalkableAtLocal(Candidate))
		{
			return GetActorTransform().TransformPosition(FVector(Candidate.X, Candidate.Y, GetSurfaceHeightAtLocal(Candidate)));
		}
	}

	// Deterministic last resort: a point just outside the central intersection.
	const FVector2D Fallback(GetSafeRoadWidth() * 0.5f + SidewalkWidth * 0.5f, GetSafeRoadWidth() * 0.5f + SidewalkWidth * 0.5f);
	return GetActorTransform().TransformPosition(FVector(Fallback.X, Fallback.Y, GetSurfaceHeightAtLocal(Fallback)));
}

TArray<FCityBridgeArea> ACityWorldActor::GetBridgeAreas() const
{
	TArray<FCityBridgeArea> WorldAreas;
	WorldAreas.Reserve(LocalBridgeAreas.Num());
	for (const FCityBridgeArea& LocalArea : LocalBridgeAreas)
	{
		FCityBridgeArea WorldArea = LocalArea;
		WorldArea.Transform = LocalArea.Transform * GetActorTransform();
		WorldAreas.Add(WorldArea);
	}
	return WorldAreas;
}
