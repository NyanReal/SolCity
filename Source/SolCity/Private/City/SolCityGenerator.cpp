#include "City/SolCityGenerator.h"
#include "City/SolCityLotLayout.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace SolCityGeneration
{
	constexpr float CubeSize = 100.0f;
	// One real-world traffic lane is the visual ruler for procedural architecture.
	constexpr float StandardLaneWidth = 360.0f;
	constexpr float BuildingEdgeMargin = StandardLaneWidth * 1.25f;
	constexpr float BuildingGap = StandardLaneWidth * 0.12f;
	constexpr float RiverBuildingSetback = StandardLaneWidth * 0.75f;
	constexpr float ProceduralRoadSetback = StandardLaneWidth * 0.65f;
	constexpr float MinimumBuildingFootprint = StandardLaneWidth * 2.0f;
	constexpr float MaximumBuildingFootprint = StandardLaneWidth * 5.20f;
	constexpr float MaximumAuthoredBuildingHeight = StandardLaneWidth * 7.5f;
	constexpr int32 AuthoredBuildingInterval = 4;
	constexpr float RoadSurfaceZ = 7.0f;
	constexpr float SidewalkSurfaceZ = 16.0f;
	constexpr float BridgeDeckZ = 105.0f;
	constexpr float LocalRoadWidth = StandardLaneWidth;
	constexpr float CollectorRoadWidth = StandardLaneWidth * 2.0f;
	constexpr float ArterialRoadWidth = StandardLaneWidth * 4.0f;
	constexpr float LocalSidewalkWidth = 160.0f;
	constexpr float CollectorSidewalkWidth = 240.0f;
	constexpr float ArterialSidewalkWidth = 320.0f;
	constexpr float GroundZoneSurfaceZ = 2.0f;

	int32 LaneCountForRoadClass(const ESolCityRoadClass RoadClass)
	{
		switch (RoadClass)
		{
		case ESolCityRoadClass::Arterial:
		case ESolCityRoadClass::Bridge: return 4;
		case ESolCityRoadClass::Collector: return 2;
		default: return 1;
		}
	}

	float SidewalkWidthForRoadClass(const ESolCityRoadClass RoadClass)
	{
		switch (RoadClass)
		{
		case ESolCityRoadClass::Arterial:
		case ESolCityRoadClass::Bridge: return ArterialSidewalkWidth;
		case ESolCityRoadClass::Collector: return CollectorSidewalkWidth;
		default: return LocalSidewalkWidth;
		}
	}

	FVector SafeNormal2D(const FVector& Value)
	{
		const FVector Flat(Value.X, Value.Y, 0.0f);
		return Flat.GetSafeNormal();
	}

	FVector2D RotatedExtent2D(const FVector2D& Size, float YawDegrees)
	{
		const FVector2D LocalExtent = Size * 0.5f;
		const float YawRadians = FMath::DegreesToRadians(YawDegrees);
		const float AbsCos = FMath::Abs(FMath::Cos(YawRadians));
		const float AbsSin = FMath::Abs(FMath::Sin(YawRadians));
		return FVector2D(
			AbsCos * LocalExtent.X + AbsSin * LocalExtent.Y,
			AbsSin * LocalExtent.X + AbsCos * LocalExtent.Y);
	}
}

ASolCityGenerator::ASolCityGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SceneRoot->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredMidRiseAsset(TEXT("/Game/Art/Buildings/SM_SolCity_MidRise_01.SM_SolCity_MidRise_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredCornerRetailAsset(TEXT("/Game/Art/Buildings/SM_SolCity_CornerRetail_01.SM_SolCity_CornerRetail_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredSteppedTowerAsset(TEXT("/Game/Art/Buildings/SM_SolCity_SteppedTower_01.SM_SolCity_SteppedTower_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildingBaseModuleAsset(TEXT("/Game/Art/Buildings/SM_SolCity_BuildingBase_01.SM_SolCity_BuildingBase_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildingMiddleModuleAsset(TEXT("/Game/Art/Buildings/SM_SolCity_BuildingMiddle_01.SM_SolCity_BuildingMiddle_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildingCrownModuleAsset(TEXT("/Game/Art/Buildings/SM_SolCity_BuildingCrown_01.SM_SolCity_BuildingCrown_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredRoadSplineAsset(TEXT("/Game/Art/Props/SM_SolCity_RoadSpline_01.SM_SolCity_RoadSpline_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredRoadJunctionAsset(TEXT("/Game/Art/Props/SM_SolCity_RoadJunction_01.SM_SolCity_RoadJunction_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredBridgeAsset(TEXT("/Game/Art/Props/SM_SolCity_Bridge_01.SM_SolCity_Bridge_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredTreeAsset(TEXT("/Game/Art/Props/SM_SolCity_Tree_01.SM_SolCity_Tree_01"));

	CubeMesh = CubeAsset.Object;
	CylinderMesh = CylinderAsset.Object;
	DefaultSurfaceMaterial = ShapeMaterial.Object;
	AuthoredMidRiseMesh = AuthoredMidRiseAsset.Object;
	AuthoredCornerRetailMesh = AuthoredCornerRetailAsset.Object;
	AuthoredSteppedTowerMesh = AuthoredSteppedTowerAsset.Object;
	BuildingBaseModuleMesh = BuildingBaseModuleAsset.Object;
	BuildingMiddleModuleMesh = BuildingMiddleModuleAsset.Object;
	BuildingCrownModuleMesh = BuildingCrownModuleAsset.Object;
	CornerBaseModuleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_CornerBase_01.SM_SolCity_CornerBase_01"));
	CornerMiddleModuleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_CornerMiddle_01.SM_SolCity_CornerMiddle_01"));
	CornerCrownModuleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_CornerCrown_01.SM_SolCity_CornerCrown_01"));
	RooftopHVACMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopHVAC_01.SM_SolCity_RooftopHVAC_01"));
	RooftopWaterTankMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopWaterTank_01.SM_SolCity_RooftopWaterTank_01"));
	RooftopSolarMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopSolar_01.SM_SolCity_RooftopSolar_01"));
	RooftopAntennaMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopAntenna_01.SM_SolCity_RooftopAntenna_01"));
	RooftopPergolaMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopPergola_01.SM_SolCity_RooftopPergola_01"));
	RooftopHelipadMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopHelipad_01.SM_SolCity_RooftopHelipad_01"));
	RooftopWarningBeaconMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_RooftopWarningBeacon_01.SM_SolCity_RooftopWarningBeacon_01"));
	SkybridgeConnectorMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_SolCity_SkybridgeConnector_01.SM_SolCity_SkybridgeConnector_01"));
	AuthoredRoadSplineMesh = AuthoredRoadSplineAsset.Object;
	AuthoredRoadJunctionMesh = AuthoredRoadJunctionAsset.Object;
	AuthoredBridgeMesh = AuthoredBridgeAsset.Object;
	AuthoredTreeMesh = AuthoredTreeAsset.Object;
	BenchMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_Bench_01.SM_SolCity_Bench_01"));
	TrashBinMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_TrashBin_01.SM_SolCity_TrashBin_01"));
	StreetLampMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_StreetLamp_01.SM_SolCity_StreetLamp_01"));
	PlanterMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_Planter_01.SM_SolCity_Planter_01"));
	BollardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_Bollard_01.SM_SolCity_Bollard_01"));
	ParkingWheelStopMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_ParkingWheelStop_01.SM_SolCity_ParkingWheelStop_01"));
	BillboardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Props/SM_SolCity_Billboard_01.SM_SolCity_Billboard_01"));
	BillboardAdMaterial01 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Props/Materials/MI_SolCity_Billboard_Ad_01.MI_SolCity_Billboard_Ad_01"));
	BillboardAdMaterial02 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Props/Materials/MI_SolCity_Billboard_Ad_02.MI_SolCity_Billboard_Ad_02"));
	BillboardAdMaterial03 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Props/Materials/MI_SolCity_Billboard_Ad_03.MI_SolCity_Billboard_Ad_03"));
	BillboardAdMaterial04 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Props/Materials/MI_SolCity_Billboard_Ad_04.MI_SolCity_Billboard_Ad_04"));
}

void ASolCityGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (bRegenerateAtBeginPlay || !bHasGenerated)
	{
		RegenerateCity();
	}
}

void ASolCityGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	if (bGenerateInEditor && (!GetWorld() || !GetWorld()->IsGameWorld()))
	{
		RegenerateCity();
	}
#endif
}

void ASolCityGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WaterTime += DeltaSeconds * WaterPanSpeed;
	if (WaterMID)
	{
		WaterMID->SetScalarParameterValue(TEXT("UVOffset"), FMath::Fmod(WaterTime, 1.0f));
		WaterMID->SetScalarParameterValue(TEXT("Time"), WaterTime);
		WaterMID->SetScalarParameterValue(TEXT("PanSpeed"), WaterPanSpeed);
	}
}

void ASolCityGenerator::RegenerateCity()
{
	ClearGeneratedComponents();
	Random.Initialize(Seed);
	RoadSegments.Reset();
	PedestrianWaypoints.Reset();
	TrafficSignalLocations.Reset();
	OccupiedBuildings.Reset();
	JunctionCapLocations.Reset();
	WaterTime = 0.0f;

	if (!CubeMesh || !CylinderMesh)
	{
		return;
	}

	CreateInstanceGroups();
	GenerateGroundAndRiver();
	GenerateRoadHierarchy();
	GenerateDistrictSurfaces();
	GenerateBuildings();
	GenerateBridge();
	GenerateTrafficFurniture();
	GenerateTrees();
	FinalizeInstanceGroups();
	UE_LOG(LogTemp, Display, TEXT("SolCity authored buildings: MidRise=%d CornerRetail=%d SteppedTower=%d"),
		AuthoredBuildingInstances ? AuthoredBuildingInstances->GetInstanceCount() : 0,
		AuthoredCornerRetailInstances ? AuthoredCornerRetailInstances->GetInstanceCount() : 0,
		AuthoredSteppedTowerInstances ? AuthoredSteppedTowerInstances->GetInstanceCount() : 0);
	UE_LOG(LogTemp, Display, TEXT("SolCity authored uniform scales (360 cm lane basis): MidRise=%.3f CornerRetail=%.3f SteppedTower=%.3f"),
		GetAuthoredBuildingUniformScale(AuthoredMidRiseMesh),
		GetAuthoredBuildingUniformScale(AuthoredCornerRetailMesh),
		GetAuthoredBuildingUniformScale(AuthoredSteppedTowerMesh));
	UE_LOG(LogTemp, Display, TEXT("SolCity procedural stack modules: Base=%d Middle=%d Crown=%d"),
		BuildingBaseModuleInstances ? BuildingBaseModuleInstances->GetInstanceCount() : 0,
		BuildingMiddleModuleInstances ? BuildingMiddleModuleInstances->GetInstanceCount() : 0,
		BuildingCrownModuleInstances ? BuildingCrownModuleInstances->GetInstanceCount() : 0);
	UE_LOG(LogTemp, Display, TEXT("SolCity urban props: benches=%d bins=%d lamps=%d planters=%d bollards=%d wheelstops=%d billboards=%d"),
		BenchInstances ? BenchInstances->GetInstanceCount() : 0,
		TrashBinInstances ? TrashBinInstances->GetInstanceCount() : 0,
		StreetLampInstances ? StreetLampInstances->GetInstanceCount() : 0,
		PlanterInstances ? PlanterInstances->GetInstanceCount() : 0,
		BollardInstances ? BollardInstances->GetInstanceCount() : 0,
		ParkingWheelStopInstances ? ParkingWheelStopInstances->GetInstanceCount() : 0,
		(BillboardInstances01 ? BillboardInstances01->GetInstanceCount() : 0) +
		(BillboardInstances02 ? BillboardInstances02->GetInstanceCount() : 0) +
		(BillboardInstances03 ? BillboardInstances03->GetInstanceCount() : 0) +
		(BillboardInstances04 ? BillboardInstances04->GetInstanceCount() : 0));
	UE_LOG(LogTemp, Display, TEXT("SolCity authored modules: corner=%d/%d/%d rooftop=%d skybridges=%d"),
		CornerBaseModuleInstances ? CornerBaseModuleInstances->GetInstanceCount() : 0,
		CornerMiddleModuleInstances ? CornerMiddleModuleInstances->GetInstanceCount() : 0,
		CornerCrownModuleInstances ? CornerCrownModuleInstances->GetInstanceCount() : 0,
		(RooftopHVACInstances ? RooftopHVACInstances->GetInstanceCount() : 0) +
		(RooftopWaterTankInstances ? RooftopWaterTankInstances->GetInstanceCount() : 0) +
		(RooftopSolarInstances ? RooftopSolarInstances->GetInstanceCount() : 0) +
		(RooftopAntennaInstances ? RooftopAntennaInstances->GetInstanceCount() : 0) +
		(RooftopPergolaInstances ? RooftopPergolaInstances->GetInstanceCount() : 0) +
		(RooftopHelipadInstances ? RooftopHelipadInstances->GetInstanceCount() : 0) +
		(RooftopWarningBeaconInstances ? RooftopWarningBeaconInstances->GetInstanceCount() : 0),
		SkybridgeConnectorInstances ? SkybridgeConnectorInstances->GetInstanceCount() : 0);
	bHasGenerated = true;
}

void ASolCityGenerator::FinalizeInstanceGroups()
{
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (UHierarchicalInstancedStaticMeshComponent* Group = Cast<UHierarchicalInstancedStaticMeshComponent>(Component))
		{
			Group->BuildTreeIfOutdated(false, true);
		}
	}
}

void ASolCityGenerator::ClearGeneratedComponents()
{
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	GeneratedComponents.Reset();
	RoadInstances = nullptr;
	CollectorRoadInstances = nullptr;
	ArterialRoadInstances = nullptr;
	RoadMarkingInstances = nullptr;
	LaneMarkingInstances = nullptr;
	SidewalkInstances = nullptr;
	ResidentialGroundInstances = nullptr;
	CommercialGroundInstances = nullptr;
	ParkGroundInstances = nullptr;
	ParkingGroundInstances = nullptr;
	BuildingBaseModuleInstances = nullptr;
	BuildingMiddleModuleInstances = nullptr;
	BuildingCrownModuleInstances = nullptr;
	CornerBaseModuleInstances = nullptr;
	CornerMiddleModuleInstances = nullptr;
	CornerCrownModuleInstances = nullptr;
	RooftopHVACInstances = nullptr;
	RooftopWaterTankInstances = nullptr;
	RooftopSolarInstances = nullptr;
	RooftopAntennaInstances = nullptr;
	RooftopPergolaInstances = nullptr;
	RooftopHelipadInstances = nullptr;
	RooftopWarningBeaconInstances = nullptr;
	SkybridgeConnectorInstances = nullptr;
	AuthoredBuildingInstances = nullptr;
	AuthoredCornerRetailInstances = nullptr;
	AuthoredSteppedTowerInstances = nullptr;
	BridgeInstances = nullptr;
	AuthoredBridgeInstances = nullptr;
	JunctionInstances = nullptr;
	TreeInstances = nullptr;
	BenchInstances = nullptr;
	TrashBinInstances = nullptr;
	StreetLampInstances = nullptr;
	PlanterInstances = nullptr;
	BollardInstances = nullptr;
	ParkingWheelStopInstances = nullptr;
	BillboardInstances01 = nullptr;
	BillboardInstances02 = nullptr;
	BillboardInstances03 = nullptr;
	BillboardInstances04 = nullptr;
	DetailInstances = nullptr;
	CylinderInstances = nullptr;
	WaterSurfaceMesh = nullptr;
	WaterMID = nullptr;
	bHasGenerated = false;
}

void ASolCityGenerator::CreateInstanceGroups()
{
	RoadInstances = CreateInstanceGroup(TEXT("GeneratedLocalRoads"), CubeMesh, LocalRoadMaterial ? LocalRoadMaterial.Get() : RoadMaterial.Get(), FLinearColor(0.74f, 0.78f, 0.80f), true);
	CollectorRoadInstances = CreateInstanceGroup(TEXT("GeneratedCollectorRoads"), CubeMesh, CollectorRoadMaterial ? CollectorRoadMaterial.Get() : RoadMaterial.Get(), FLinearColor(0.62f, 0.69f, 0.73f), true);
	ArterialRoadInstances = CreateInstanceGroup(TEXT("GeneratedArterialRoads"), CubeMesh, ArterialRoadMaterial ? ArterialRoadMaterial.Get() : RoadMaterial.Get(), FLinearColor(0.50f, 0.58f, 0.64f), true);
	RoadMarkingInstances = CreateInstanceGroup(TEXT("GeneratedRoadCenterMarkings"), CubeMesh, RoadMarkingMaterial, FLinearColor(0.96f, 0.72f, 0.18f), false);
	LaneMarkingInstances = CreateInstanceGroup(TEXT("GeneratedRoadLaneMarkings"), CubeMesh, RoadMarkingMaterial, FLinearColor(0.92f, 0.93f, 0.90f), false);
	SidewalkInstances = CreateInstanceGroup(TEXT("GeneratedSidewalks"), CubeMesh, SidewalkMaterial, FLinearColor(0.72f, 0.69f, 0.62f), true);
	ResidentialGroundInstances = CreateInstanceGroup(TEXT("GeneratedResidentialGround"), CubeMesh, ResidentialGroundMaterial, FLinearColor(0.72f, 0.88f, 0.72f), false);
	CommercialGroundInstances = CreateInstanceGroup(TEXT("GeneratedCommercialGround"), CubeMesh, CommercialGroundMaterial, FLinearColor(0.78f, 0.72f, 0.63f), false);
	ParkGroundInstances = CreateInstanceGroup(TEXT("GeneratedParkGround"), CubeMesh, ParkGroundMaterial ? ParkGroundMaterial.Get() : GroundMaterial.Get(), FLinearColor(0.54f, 1.02f, 0.58f), false);
	ParkingGroundInstances = CreateInstanceGroup(TEXT("GeneratedParkingGround"), CubeMesh, ParkingGroundMaterial, FLinearColor(0.88f, 0.88f, 0.82f), false);
	if (BuildingBaseModuleMesh)
	{
		BuildingBaseModuleInstances = CreateInstanceGroup(TEXT("GeneratedBuildingBaseModules"), BuildingBaseModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	if (BuildingMiddleModuleMesh)
	{
		BuildingMiddleModuleInstances = CreateInstanceGroup(TEXT("GeneratedBuildingMiddleModules"), BuildingMiddleModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	if (BuildingCrownModuleMesh)
	{
		BuildingCrownModuleInstances = CreateInstanceGroup(TEXT("GeneratedBuildingCrownModules"), BuildingCrownModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	if (CornerBaseModuleMesh)
	{
		CornerBaseModuleInstances = CreateInstanceGroup(TEXT("GeneratedCornerBaseModules"), CornerBaseModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	if (CornerMiddleModuleMesh)
	{
		CornerMiddleModuleInstances = CreateInstanceGroup(TEXT("GeneratedCornerMiddleModules"), CornerMiddleModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	if (CornerCrownModuleMesh)
	{
		CornerCrownModuleInstances = CreateInstanceGroup(TEXT("GeneratedCornerCrownModules"), CornerCrownModuleMesh, nullptr, FLinearColor::White, true, false);
	}
	auto CreateOptionalRoofGroup = [this](const FName Name, UStaticMesh* Mesh)
	{
		return Mesh ? CreateInstanceGroup(Name, Mesh, nullptr, FLinearColor::White, true, false) : nullptr;
	};
	RooftopHVACInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopHVAC"), RooftopHVACMesh);
	RooftopWaterTankInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopWaterTanks"), RooftopWaterTankMesh);
	RooftopSolarInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopSolar"), RooftopSolarMesh);
	RooftopAntennaInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopAntennas"), RooftopAntennaMesh);
	RooftopPergolaInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopPergolas"), RooftopPergolaMesh);
	RooftopHelipadInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopHelipads"), RooftopHelipadMesh);
	RooftopWarningBeaconInstances = CreateOptionalRoofGroup(TEXT("GeneratedRooftopWarningBeacons"), RooftopWarningBeaconMesh);
	SkybridgeConnectorInstances = CreateOptionalRoofGroup(TEXT("GeneratedSkybridgeConnectors"), SkybridgeConnectorMesh);
	if (AuthoredMidRiseMesh)
	{
		AuthoredBuildingInstances = CreateInstanceGroup(TEXT("GeneratedAuthoredMidRise"), AuthoredMidRiseMesh, nullptr, FLinearColor::White, true, false);
	}
	if (AuthoredCornerRetailMesh)
	{
		AuthoredCornerRetailInstances = CreateInstanceGroup(TEXT("GeneratedAuthoredCornerRetail"), AuthoredCornerRetailMesh, nullptr, FLinearColor::White, true, false);
	}
	if (AuthoredSteppedTowerMesh)
	{
		AuthoredSteppedTowerInstances = CreateInstanceGroup(TEXT("GeneratedAuthoredSteppedTower"), AuthoredSteppedTowerMesh, nullptr, FLinearColor::White, true, false);
	}
	BridgeInstances = CreateInstanceGroup(TEXT("GeneratedBridge"), CubeMesh, BridgeMaterial, FLinearColor(0.80f, 0.78f, 0.69f), true);
	if (AuthoredBridgeMesh)
	{
		AuthoredBridgeInstances = CreateInstanceGroup(TEXT("GeneratedAuthoredBridge"), AuthoredBridgeMesh, nullptr, FLinearColor::White, true, false);
	}
	if (AuthoredRoadJunctionMesh)
	{
		JunctionInstances = CreateInstanceGroup(TEXT("GeneratedRoadJunctions"), AuthoredRoadJunctionMesh, nullptr, FLinearColor::White, true, false);
	}
	if (AuthoredTreeMesh)
	{
		TreeInstances = CreateInstanceGroup(TEXT("GeneratedTrees"), AuthoredTreeMesh, nullptr, FLinearColor::White, false, false);
	}
	if (BenchMesh)
	{
		BenchInstances = CreateInstanceGroup(TEXT("GeneratedBenches"), BenchMesh, nullptr, FLinearColor::White, true, false);
	}
	if (TrashBinMesh)
	{
		TrashBinInstances = CreateInstanceGroup(TEXT("GeneratedTrashBins"), TrashBinMesh, nullptr, FLinearColor::White, true, false);
	}
	if (StreetLampMesh)
	{
		StreetLampInstances = CreateInstanceGroup(TEXT("GeneratedStreetLamps"), StreetLampMesh, nullptr, FLinearColor::White, true, false);
	}
	if (PlanterMesh)
	{
		PlanterInstances = CreateInstanceGroup(TEXT("GeneratedPlanters"), PlanterMesh, nullptr, FLinearColor::White, true, false);
	}
	if (BollardMesh)
	{
		BollardInstances = CreateInstanceGroup(TEXT("GeneratedBollards"), BollardMesh, nullptr, FLinearColor::White, true, false);
	}
	if (ParkingWheelStopMesh)
	{
		ParkingWheelStopInstances = CreateInstanceGroup(TEXT("GeneratedParkingWheelStops"), ParkingWheelStopMesh, nullptr, FLinearColor::White, true, false);
	}
	if (BillboardMesh)
	{
		BillboardInstances01 = CreateInstanceGroup(TEXT("GeneratedBillboards01"), BillboardMesh, nullptr, FLinearColor::White, true, false);
		BillboardInstances02 = CreateInstanceGroup(TEXT("GeneratedBillboards02"), BillboardMesh, nullptr, FLinearColor::White, true, false);
		BillboardInstances03 = CreateInstanceGroup(TEXT("GeneratedBillboards03"), BillboardMesh, nullptr, FLinearColor::White, true, false);
		BillboardInstances04 = CreateInstanceGroup(TEXT("GeneratedBillboards04"), BillboardMesh, nullptr, FLinearColor::White, true, false);
		if (BillboardAdMaterial01) BillboardInstances01->SetMaterial(1, BillboardAdMaterial01);
		if (BillboardAdMaterial02) BillboardInstances02->SetMaterial(1, BillboardAdMaterial02);
		if (BillboardAdMaterial03) BillboardInstances03->SetMaterial(1, BillboardAdMaterial03);
		if (BillboardAdMaterial04) BillboardInstances04->SetMaterial(1, BillboardAdMaterial04);
	}
	DetailInstances = CreateInstanceGroup(TEXT("GeneratedDetails"), CubeMesh, BridgeMaterial, FLinearColor(0.17f, 0.24f, 0.27f), true);
	CylinderInstances = CreateInstanceGroup(TEXT("GeneratedCylinders"), CylinderMesh, BridgeMaterial, FLinearColor(0.68f, 0.66f, 0.60f), true);

	UMaterialInterface* WaterBase = WaterMaterial.Get() ? WaterMaterial.Get() : DefaultSurfaceMaterial.Get();
	if (WaterBase)
	{
		WaterMID = UMaterialInstanceDynamic::Create(WaterBase, this);
		WaterMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.66f, 0.91f));
		WaterMID->SetVectorParameterValue(TEXT("BaseColorTint"), FLinearColor(0.16f, 0.66f, 0.91f));
		WaterMID->SetVectorParameterValue(TEXT("WaterTint"), FLinearColor(0.10f, 0.60f, 0.88f, 0.82f));
	}
}

UHierarchicalInstancedStaticMeshComponent* ASolCityGenerator::CreateInstanceGroup(
	const FName Name,
	UStaticMesh* Mesh,
	UMaterialInterface* Material,
	const FLinearColor& Tint,
	bool bCollision,
	bool bApplyTint)
{
	UHierarchicalInstancedStaticMeshComponent* Group = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, Name);
	Group->SetupAttachment(SceneRoot);
	Group->SetStaticMesh(Mesh);
	Group->SetMobility(EComponentMobility::Static);
	Group->bAutoRebuildTreeOnInstanceChanges = false;
	Group->SetCanEverAffectNavigation(bCollision);
	Group->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Group->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Group->RegisterComponent();
	AddInstanceComponent(Group);
	GeneratedComponents.Add(Group);

	UMaterialInterface* BaseMaterial = Material ? Material : DefaultSurfaceMaterial.Get();
	if (bApplyTint && BaseMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MID->SetVectorParameterValue(TEXT("Color"), Tint);
		MID->SetVectorParameterValue(TEXT("BaseColorTint"), Tint);
		Group->SetMaterial(0, MID);
	}
	return Group;
}

void ASolCityGenerator::AddBox(UHierarchicalInstancedStaticMeshComponent* Group, const FVector& Center, const FVector& Size, float YawDegrees)
{
	if (!Group || Size.GetMin() <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FTransform InstanceTransform(FRotator(0.0f, YawDegrees, 0.0f), Center, Size / SolCityGeneration::CubeSize);
	Group->AddInstance(InstanceTransform);
}

void ASolCityGenerator::AddCylinder(UHierarchicalInstancedStaticMeshComponent* Group, const FVector& Center, float Radius, float Height, float YawDegrees)
{
	if (!Group || Radius <= 0.0f || Height <= 0.0f)
	{
		return;
	}
	const FVector Scale((Radius * 2.0f) / SolCityGeneration::CubeSize, (Radius * 2.0f) / SolCityGeneration::CubeSize, Height / SolCityGeneration::CubeSize);
	Group->AddInstance(FTransform(FRotator(0.0f, YawDegrees, 0.0f), Center, Scale));
}

float ASolCityGenerator::RiverCenterY(float X) const
{
	const float LayoutScale = CityDiameter / 18000.0f;
	const float NormalizedX = X / FMath::Max(CityDiameter * 0.5f, 1.0f);
	return (FMath::Sin(NormalizedX * PI * 1.35f) * 470.0f + FMath::Sin(NormalizedX * PI * 3.1f + 0.65f) * 125.0f) * LayoutScale;
}

bool ASolCityGenerator::IsInsideRiver(const FVector2D& Point, float Margin) const
{
	return FMath::Abs(Point.Y - RiverCenterY(Point.X)) < RiverWidth * 0.5f + Margin;
}

void ASolCityGenerator::GenerateGroundAndRiver()
{
	const float HalfCity = CityDiameter * 0.5f;
	constexpr int32 SegmentCount = 192;
	const float WaterHalfWidth = RiverWidth * 0.46f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve((SegmentCount + 1) * 2);
	Normals.Reserve((SegmentCount + 1) * 2);
	UV0.Reserve((SegmentCount + 1) * 2);
	VertexColors.Reserve((SegmentCount + 1) * 2);
	Tangents.Reserve((SegmentCount + 1) * 2);
	Triangles.Reserve(SegmentCount * 6);

	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Alpha = Index / static_cast<float>(SegmentCount);
		const float X = FMath::Lerp(-HalfCity, HalfCity, Alpha);
		const float CenterY = RiverCenterY(X);
		Vertices.Add(FVector(X, CenterY - WaterHalfWidth, RiverSurfaceZ));
		Vertices.Add(FVector(X, CenterY + WaterHalfWidth, RiverSurfaceZ));
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(Alpha * CityDiameter / 1200.0f, 0.0f));
		UV0.Add(FVector2D(Alpha * CityDiameter / 1200.0f, 1.0f));
		VertexColors.Add(FLinearColor::White);
		VertexColors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		if (Index < SegmentCount)
		{
			const int32 Base = Index * 2;
			// Unreal's front face is clockwise when viewed from above.
			Triangles.Append({Base, Base + 1, Base + 2, Base + 1, Base + 3, Base + 2});
		}
	}

	WaterSurfaceMesh = NewObject<UProceduralMeshComponent>(this, TEXT("GeneratedContinuousRiver"));
	WaterSurfaceMesh->SetupAttachment(SceneRoot);
	WaterSurfaceMesh->SetMobility(EComponentMobility::Static);
	WaterSurfaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurfaceMesh->SetCanEverAffectNavigation(false);
	WaterSurfaceMesh->RegisterComponent();
	AddInstanceComponent(WaterSurfaceMesh);
	GeneratedComponents.Add(WaterSurfaceMesh);
	WaterSurfaceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
	if (WaterMID)
	{
		WaterSurfaceMesh->SetMaterial(0, WaterMID);
	}
}

void ASolCityGenerator::AddRoadSegment(const FVector& Start, const FVector& End, float Width, ESolCityRoadClass RoadClass, bool bAddSidewalks, bool bBridge, bool bCreateVisual)
{
	const FVector Delta = End - Start;
	const float Length = Delta.Size2D();
	if (Length < 10.0f)
	{
		return;
	}

	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const FVector Center = (Start + End) * 0.5f + FVector(0.0f, 0.0f, SolCityGeneration::RoadSurfaceZ * 0.5f);
	if (bCreateVisual)
	{
		UHierarchicalInstancedStaticMeshComponent* RoadGroup = RoadInstances;
		if (RoadClass == ESolCityRoadClass::Arterial)
		{
			RoadGroup = ArterialRoadInstances;
		}
		else if (RoadClass == ESolCityRoadClass::Collector)
		{
			RoadGroup = CollectorRoadInstances;
		}
		AddBox(bBridge ? BridgeInstances.Get() : RoadGroup, Center, FVector(Length, Width, SolCityGeneration::RoadSurfaceZ), Yaw);
		AddRoadMarkings(Start, End, Width, RoadClass, bBridge);
	}

	FSolCityRoadSegment& Segment = RoadSegments.AddDefaulted_GetRef();
	Segment.Start = Start;
	Segment.End = End;
	Segment.HalfWidth = Width * 0.5f;
	Segment.RoadClass = bBridge ? ESolCityRoadClass::Bridge : RoadClass;
	Segment.LaneCount = SolCityGeneration::LaneCountForRoadClass(RoadClass);
	Segment.bBridge = bBridge;

	if (bAddSidewalks)
	{
		const FVector Direction = SolCityGeneration::SafeNormal2D(Delta);
		const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
		const float SidewalkWidth = SolCityGeneration::SidewalkWidthForRoadClass(RoadClass);
		const float Offset = Width * 0.5f + SidewalkWidth * 0.5f + 20.0f;
		for (float Side : {-1.0f, 1.0f})
		{
			const FVector WalkCenter = (Start + End) * 0.5f + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ * 0.5f);
			if (bCreateVisual)
			{
				AddBox(bBridge ? BridgeInstances : SidewalkInstances, WalkCenter, FVector(Length, SidewalkWidth, SolCityGeneration::SidewalkSurfaceZ), Yaw);
			}
			PedestrianWaypoints.Add(Start + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ));
			PedestrianWaypoints.Add(End + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ));
		}
	}
}

void ASolCityGenerator::AddRoadMarkings(const FVector& Start, const FVector& End, float Width, ESolCityRoadClass RoadClass, bool bBridge)
{
	if (bBridge || RoadClass == ESolCityRoadClass::Local || !RoadMarkingInstances || !LaneMarkingInstances)
	{
		return;
	}

	const FVector Delta = End - Start;
	const float Length = Delta.Size2D();
	const FVector Direction = SolCityGeneration::SafeNormal2D(Delta);
	const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const FVector Midpoint = (Start + End) * 0.5f;
	constexpr float MarkingZ = SolCityGeneration::RoadSurfaceZ + 1.5f;
	constexpr float MarkingHeight = 2.0f;

	auto AddDashedLine = [&](UHierarchicalInstancedStaticMeshComponent* Group, const float LateralOffset)
	{
		constexpr float DashLength = 240.0f;
		constexpr float DashGap = 160.0f;
		const float Pitch = DashLength + DashGap;
		const int32 DashCount = FMath::Max(1, FMath::FloorToInt((Length + DashGap) / Pitch));
		for (int32 DashIndex = 0; DashIndex < DashCount; ++DashIndex)
		{
			const float Along = -Length * 0.5f + DashLength * 0.5f + DashIndex * Pitch;
			if (Along + DashLength * 0.5f > Length * 0.5f)
			{
				break;
			}
			AddBox(Group, Midpoint + Direction * Along + Perpendicular * LateralOffset + FVector(0.0f, 0.0f, MarkingZ), FVector(DashLength, 12.0f, MarkingHeight), Yaw);
		}
	};

	if (RoadClass == ESolCityRoadClass::Collector)
	{
		AddDashedLine(RoadMarkingInstances, 0.0f);
		return;
	}

	// Four-lane arterial: double yellow center line, two dashed same-direction
	// dividers, and solid white edge lines.
	for (const float Offset : {-18.0f, 18.0f})
	{
		AddBox(RoadMarkingInstances, Midpoint + Perpendicular * Offset + FVector(0.0f, 0.0f, MarkingZ), FVector(Length, 10.0f, MarkingHeight), Yaw);
	}
	for (const float Offset : {-SolCityGeneration::StandardLaneWidth, SolCityGeneration::StandardLaneWidth})
	{
		AddDashedLine(LaneMarkingInstances, Offset);
	}
	for (const float Offset : {-Width * 0.5f + 24.0f, Width * 0.5f - 24.0f})
	{
		AddBox(LaneMarkingInstances, Midpoint + Perpendicular * Offset + FVector(0.0f, 0.0f, MarkingZ), FVector(Length, 10.0f, MarkingHeight), Yaw);
	}
}

void ASolCityGenerator::AddPolylineRoad(const TArray<FVector>& Points, float Width, ESolCityRoadClass RoadClass, bool bAddSidewalks)
{
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Midpoint = (Points[Index - 1] + Points[Index]) * 0.5f;
		if (!IsInsideRiver(FVector2D(Midpoint.X, Midpoint.Y), Width * 0.15f))
		{
			// The carriageway, markings and sidewalks are deliberately emitted as
			// separate HISM layers. This keeps sidewalk texture scale independent
			// from road width and avoids baking sidewalks into the spline mesh.
			AddRoadSegment(Points[Index - 1], Points[Index], Width, RoadClass, bAddSidewalks, false, true);
		}
	}
	for (const FVector& Point : Points)
	{
		AddJunctionCap(Point, Width, RoadClass);
	}
}

void ASolCityGenerator::AddSplineRoadVisual(const TArray<FVector>& Points, float Width)
{
	if (Points.Num() < 2)
	{
		return;
	}

	if (!AuthoredRoadSplineMesh)
	{
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const FVector Delta = Points[Index] - Points[Index - 1];
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			AddBox(RoadInstances, (Points[Index - 1] + Points[Index]) * 0.5f + FVector(0.0f, 0.0f, SolCityGeneration::RoadSurfaceZ * 0.5f), FVector(Delta.Size2D(), Width, SolCityGeneration::RoadSurfaceZ), Yaw);
		}
		return;
	}

	USplineComponent* Spline = NewObject<USplineComponent>(this);
	Spline->SetupAttachment(SceneRoot);
	Spline->SetMobility(EComponentMobility::Static);
	Spline->ClearSplinePoints(false);
	for (const FVector& Point : Points)
	{
		Spline->AddSplinePoint(Point + FVector(0.0f, 0.0f, 2.0f), ESplineCoordinateSpace::Local, false);
	}
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Spline->SetSplinePointType(Index, ESplinePointType::CurveClamped, false);
	}
	Spline->UpdateSpline();
	Spline->RegisterComponent();
	AddInstanceComponent(Spline);
	GeneratedComponents.Add(Spline);

	const float TransverseScale = Width / 720.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(this);
		Segment->SetupAttachment(Spline);
		Segment->SetMobility(EComponentMobility::Static);
		Segment->SetStaticMesh(AuthoredRoadSplineMesh);
		if (RoadMaterial)
		{
			Segment->SetMaterial(0, RoadMaterial);
		}
		Segment->SetForwardAxis(ESplineMeshAxis::X, false);
		Segment->SetStartScale(FVector2D(TransverseScale, 1.0f), false);
		Segment->SetEndScale(FVector2D(TransverseScale, 1.0f), false);
		Segment->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Segment->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Segment->SetCanEverAffectNavigation(true);
		const FVector StartPosition = Spline->GetLocationAtSplinePoint(Index - 1, ESplineCoordinateSpace::Local);
		const FVector EndPosition = Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		const FVector StartTangent = Spline->GetLeaveTangentAtSplinePoint(Index - 1, ESplineCoordinateSpace::Local);
		const FVector EndTangent = Spline->GetArriveTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		Segment->SetStartAndEnd(StartPosition, StartTangent, EndPosition, EndTangent, false);
		Segment->RegisterComponent();
		Segment->UpdateMesh();
		AddInstanceComponent(Segment);
		GeneratedComponents.Add(Segment);
	}
}

void ASolCityGenerator::AddJunctionCap(const FVector& Position, float Width, ESolCityRoadClass RoadClass)
{
	if (IsInsideRiver(FVector2D(Position.X, Position.Y), Width * 0.15f))
	{
		return;
	}
	const FVector2D Position2D(Position.X, Position.Y);
	for (const FVector2D& Existing : JunctionCapLocations)
	{
		if (FVector2D::Distance(Existing, Position2D) < 250.0f)
		{
			return;
		}
	}
	UHierarchicalInstancedStaticMeshComponent* RoadGroup = RoadInstances;
	if (RoadClass == ESolCityRoadClass::Arterial)
	{
		RoadGroup = ArterialRoadInstances;
	}
	else if (RoadClass == ESolCityRoadClass::Collector)
	{
		RoadGroup = CollectorRoadInstances;
	}
	AddBox(RoadGroup, Position + FVector(0.0f, 0.0f, SolCityGeneration::RoadSurfaceZ * 0.5f - 0.1f), FVector(Width, Width, SolCityGeneration::RoadSurfaceZ - 0.2f));
	JunctionCapLocations.Add(Position2D);
}

void ASolCityGenerator::GenerateRoadHierarchy()
{
	const float H = CityDiameter * 0.5f;
	const float LayoutScale = CityDiameter / 18000.0f;

	// Two asymmetrical arterial spines.  One bends with the landscape and the
	// other is deliberately split at the river for the authored bridge.
	TArray<FVector> EastWestSpine;
	EastWestSpine.Add(FVector(-H, -2700.0f * LayoutScale, 0.0f));
	EastWestSpine.Add(FVector(-H * 0.55f, -2250.0f * LayoutScale, 0.0f));
	EastWestSpine.Add(FVector(-H * 0.15f, -2750.0f * LayoutScale, 0.0f));
	EastWestSpine.Add(FVector(H * 0.28f, -2150.0f * LayoutScale, 0.0f));
	EastWestSpine.Add(FVector(H * 0.66f, -2450.0f * LayoutScale, 0.0f));
	EastWestSpine.Add(FVector(H, -1900.0f * LayoutScale, 0.0f));
	AddPolylineRoad(EastWestSpine, SolCityGeneration::ArterialRoadWidth, ESolCityRoadClass::Arterial);

	const float BridgeApproach = RiverWidth * 0.5f + 560.0f;
	AddPolylineRoad({FVector(0.0f, -H, 0.0f), FVector(0.0f, RiverCenterY(0.0f) - BridgeApproach, 0.0f)}, SolCityGeneration::ArterialRoadWidth, ESolCityRoadClass::Arterial);
	AddPolylineRoad({FVector(0.0f, RiverCenterY(0.0f) + BridgeApproach, 0.0f), FVector(0.0f, H, 0.0f)}, SolCityGeneration::ArterialRoadWidth, ESolCityRoadClass::Arterial);

	// Irregular ring collectors keep connectivity without a square grid.
	const TArray<FVector> CanonicalRing = {
		FVector(-6100.0f, -3300.0f, 0.0f), FVector(-3900.0f, -5900.0f, 0.0f),
		FVector(-500.0f, -6500.0f, 0.0f), FVector(3300.0f, -5600.0f, 0.0f),
		FVector(6100.0f, -2900.0f, 0.0f), FVector(6500.0f, 800.0f, 0.0f),
		FVector(5200.0f, 4300.0f, 0.0f), FVector(2200.0f, 6300.0f, 0.0f),
		FVector(-1800.0f, 6200.0f, 0.0f), FVector(-5100.0f, 4700.0f, 0.0f),
		FVector(-6500.0f, 1600.0f, 0.0f), FVector(-6100.0f, -3300.0f, 0.0f)
	};
	TArray<FVector> Ring;
	Ring.Reserve(CanonicalRing.Num());
	for (const FVector& Point : CanonicalRing)
	{
		Ring.Add(Point * FVector(LayoutScale, LayoutScale, 1.0f));
	}
	AddPolylineRoad(Ring, SolCityGeneration::CollectorRoadWidth, ESolCityRoadClass::Collector);

	// A second collector loop compresses the central street spacing into a
	// recognizable CBD instead of scaling every block up with the terrain.
	const TArray<FVector> CanonicalCbdRing = {
		FVector(-3200.0f, -1700.0f, 0.0f), FVector(-1800.0f, -3300.0f, 0.0f),
		FVector(500.0f, -3600.0f, 0.0f), FVector(2800.0f, -2500.0f, 0.0f),
		FVector(3400.0f, -400.0f, 0.0f), FVector(2500.0f, 1900.0f, 0.0f),
		FVector(400.0f, 3000.0f, 0.0f), FVector(-2100.0f, 2200.0f, 0.0f),
		FVector(-3200.0f, -1700.0f, 0.0f)
	};
	TArray<FVector> CbdRing;
	CbdRing.Reserve(CanonicalCbdRing.Num());
	for (const FVector& Point : CanonicalCbdRing)
	{
		CbdRing.Add(Point * FVector(LayoutScale, LayoutScale, 1.0f));
	}
	AddPolylineRoad(CbdRing, SolCityGeneration::CollectorRoadWidth, ESolCityRoadClass::Collector);

	// Branching streets grow from district seeds. Each branch changes heading,
	// producing curved blocks and T-junctions rather than a repeated grid.
	const TArray<FVector2D> CanonicalDistrictSeeds = {
		FVector2D(-4300.0f, -3900.0f), FVector2D(3600.0f, -4000.0f),
		FVector2D(-4300.0f, 3300.0f), FVector2D(3600.0f, 3900.0f),
		FVector2D(-500.0f, 3200.0f), FVector2D(5900.0f, 800.0f)
	};
	TArray<FVector2D> DistrictSeeds;
	DistrictSeeds.Reserve(CanonicalDistrictSeeds.Num());
	for (const FVector2D& Point : CanonicalDistrictSeeds)
	{
		DistrictSeeds.Add(Point * LayoutScale);
	}

	for (int32 DistrictIndex = 0; DistrictIndex < DistrictSeeds.Num(); ++DistrictIndex)
	{
		const FVector2D SeedPoint = DistrictSeeds[DistrictIndex];
		const int32 BranchCount = 3 + (DistrictIndex % 2);
		for (int32 BranchIndex = 0; BranchIndex < BranchCount; ++BranchIndex)
		{
			const float BaseAngle = DistrictIndex * 47.0f + BranchIndex * (360.0f / BranchCount) + Random.FRandRange(-16.0f, 16.0f);
			FVector Current(SeedPoint.X, SeedPoint.Y, 0.0f);
			float Heading = BaseAngle;
			TArray<FVector> BranchPoints;
			BranchPoints.Add(Current);
			const int32 SegmentCount = Random.RandRange(3, 6);
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				Heading += Random.FRandRange(-19.0f, 19.0f);
				const float Length = Random.FRandRange(680.0f, 1050.0f) * LayoutScale;
				const FVector Direction = FRotator(0.0f, Heading, 0.0f).Vector();
				const FVector Candidate = Current + Direction * Length;
				if (FMath::Abs(Candidate.X) > H - 500.0f || FMath::Abs(Candidate.Y) > H - 500.0f)
				{
					break;
				}
				BranchPoints.Add(Candidate);
				Current = Candidate;
			}
			AddPolylineRoad(BranchPoints,
				BranchIndex == 0 ? SolCityGeneration::CollectorRoadWidth : SolCityGeneration::LocalRoadWidth,
				BranchIndex == 0 ? ESolCityRoadClass::Collector : ESolCityRoadClass::Local);
		}
	}
}

void ASolCityGenerator::GenerateDistrictSurfaces()
{
	// Broad, deterministic land-use cells sit below roads and sidewalks. They
	// communicate zoning at city-camera height while remaining non-colliding so
	// later lot/building placement can proceed independently above them.
	constexpr int32 ColumnCount = 6;
	constexpr int32 RowsPerBank = 3;
	const float HalfCity = CityDiameter * 0.5f;
	const float RiverClearance = RiverWidth * 0.5f + CityDiameter * 0.045f;
	const float ColumnPitch = CityDiameter / ColumnCount;
	const float BankDepth = FMath::Max(1200.0f, HalfCity - RiverClearance);
	const float RowPitch = BankDepth / RowsPerBank;
	const FVector CellSize(ColumnPitch - 180.0f, RowPitch - 180.0f, SolCityGeneration::GroundZoneSurfaceZ);

	for (int32 BankIndex = 0; BankIndex < 2; ++BankIndex)
	{
		const float BankSign = BankIndex == 0 ? -1.0f : 1.0f;
		for (int32 RowIndex = 0; RowIndex < RowsPerBank; ++RowIndex)
		{
			const float DistanceFromRiver = RiverClearance + (RowIndex + 0.5f) * RowPitch;
			const float Y = BankSign * DistanceFromRiver;
			for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
			{
				const float X = -HalfCity + (ColumnIndex + 0.5f) * ColumnPitch;
				const int32 Pattern = ColumnIndex + RowIndex * 3 + BankIndex * 5;
				UHierarchicalInstancedStaticMeshComponent* ZoneGroup = ResidentialGroundInstances;

				const bool bNearArterial = ColumnIndex == 2 || ColumnIndex == 3 || (RowIndex == 0 && ColumnIndex >= 1 && ColumnIndex <= 4);
				const bool bParkCell = (RowIndex == 0 && (Pattern % 4 == 0)) || (RowIndex == 2 && (ColumnIndex == 0 || ColumnIndex == 5));
				const bool bParkingCell = bNearArterial && (Pattern % 5 == 1);
				if (bParkCell)
				{
					ZoneGroup = ParkGroundInstances;
				}
				else if (bParkingCell)
				{
					ZoneGroup = ParkingGroundInstances;
				}
				else if (bNearArterial)
				{
					ZoneGroup = CommercialGroundInstances;
				}

				AddBox(ZoneGroup, FVector(X, Y, SolCityGeneration::GroundZoneSurfaceZ * 0.5f), CellSize);
			}
		}
	}
}

float ASolCityGenerator::DistanceToSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B) const
{
	const FVector2D AB = B - A;
	const float Denominator = AB.SizeSquared();
	if (Denominator <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::Distance(Point, A);
	}
	const float T = FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / Denominator, 0.0f, 1.0f);
	return FVector2D::Distance(Point, A + AB * T);
}

bool ASolCityGenerator::IsNearRoad(const FVector2D& Point, float MaxDistance) const
{
	for (const FSolCityRoadSegment& Segment : RoadSegments)
	{
		const FVector2D SegmentStart(Segment.Start.X, Segment.Start.Y);
		const FVector2D SegmentEnd(Segment.End.X, Segment.End.Y);
		const float Distance = DistanceToSegment2D(Point, SegmentStart, SegmentEnd);
		if (Distance < Segment.HalfWidth + MaxDistance)
		{
			return true;
		}
	}
	return false;
}

bool ASolCityGenerator::IsBuildingSiteFree(const FVector2D& Center, const FVector2D& Extent) const
{
	const float HalfCity = CityDiameter * 0.5f - SolCityGeneration::BuildingEdgeMargin;
	if (FMath::Abs(Center.X) + Extent.X > HalfCity || FMath::Abs(Center.Y) + Extent.Y > HalfCity || IsInsideRiver(Center, Extent.GetMax() + SolCityGeneration::RiverBuildingSetback))
	{
		return false;
	}
	for (const FBuildingFootprint& Existing : OccupiedBuildings)
	{
		const FVector2D Delta = (Center - Existing.Center).GetAbs();
		if (Delta.X < Extent.X + Existing.Extent.X + SolCityGeneration::BuildingGap && Delta.Y < Extent.Y + Existing.Extent.Y + SolCityGeneration::BuildingGap)
		{
			return false;
		}
	}
	return true;
}

bool ASolCityGenerator::IsBuildingClearOfRoads(const FVector2D& Center, const FVector2D& LocalSize, float YawDegrees) const
{
	const FVector2D HalfSize = LocalSize * 0.5f;
	const float YawRadians = FMath::DegreesToRadians(YawDegrees);
	const FVector2D LocalX(FMath::Cos(YawRadians), FMath::Sin(YawRadians));
	const FVector2D LocalY(-LocalX.Y, LocalX.X);

	for (const FSolCityRoadSegment& Segment : RoadSegments)
	{
		const FVector2D Start(Segment.Start.X, Segment.Start.Y);
		const FVector2D End(Segment.End.X, Segment.End.Y);
		const FVector2D AlongRoad = End - Start;
		const float LengthSquared = AlongRoad.SizeSquared();
		if (LengthSquared <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float T = FMath::Clamp(FVector2D::DotProduct(Center - Start, AlongRoad) / LengthSquared, 0.0f, 1.0f);
		const FVector2D NearestPoint = Start + AlongRoad * T;
		const FVector2D Separation = Center - NearestPoint;
		const float Distance = Separation.Size();
		const FVector2D RoadNormal = Distance > KINDA_SMALL_NUMBER
			? Separation / Distance
			: FVector2D(-AlongRoad.Y, AlongRoad.X).GetSafeNormal();
		const float ProjectedBuildingRadius =
			FMath::Abs(FVector2D::DotProduct(LocalX, RoadNormal)) * HalfSize.X +
			FMath::Abs(FVector2D::DotProduct(LocalY, RoadNormal)) * HalfSize.Y;
		if (Distance < Segment.HalfWidth + SolCityGeneration::ProceduralRoadSetback + ProjectedBuildingRadius)
		{
			return false;
		}
	}
	return true;
}

void ASolCityGenerator::GenerateBuildings()
{
	const float HalfCity = CityDiameter * 0.5f;
	const int32 EffectiveTargetCount = FMath::Clamp(TargetBuildingCount, 12, 480);
	int32 Accepted = 0;

	TArray<SolCityLotLayout::FRoadCenterline> LotRoads;
	LotRoads.Reserve(RoadSegments.Num());
	for (int32 RoadIndex = 0; RoadIndex < RoadSegments.Num(); ++RoadIndex)
	{
		const FSolCityRoadSegment& Segment = RoadSegments[RoadIndex];
		if (Segment.bBridge)
		{
			continue;
		}
		SolCityLotLayout::FRoadCenterline& Road = LotRoads.AddDefaulted_GetRef();
		Road.Start = FVector2D(Segment.Start.X, Segment.Start.Y);
		Road.End = FVector2D(Segment.End.X, Segment.End.Y);
		Road.HalfWidth = Segment.HalfWidth;
		Road.SourceRoadIndex = RoadIndex;
	}

	const TArray<SolCityLotLayout::FCityBlock> Blocks = SolCityLotLayout::ExtractBlocks(LotRoads, CityDiameter);
	TArray<TArray<SolCityLotLayout::FCityLot>> LotsByBlock;
	LotsByBlock.SetNum(Blocks.Num());
	SolCityLotLayout::FLotSettings LotSettings;
	LotSettings.MinimumFrontage = SolCityGeneration::StandardLaneWidth * 2.1f;
	LotSettings.MaximumFrontage = SolCityGeneration::StandardLaneWidth * 3.6f;
	LotSettings.MinimumDepth = SolCityGeneration::StandardLaneWidth * 2.1f;
	LotSettings.MaximumDepth = SolCityGeneration::StandardLaneWidth * 3.2f;
	LotSettings.FrontSetback = SolCityGeneration::ProceduralRoadSetback + 40.0f;
	const float CbdRadius = HalfCity * 0.42f;

	struct FOpenSpaceRect
	{
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		float Yaw = 0.0f;
		bool bValid = false;
	};

	auto FindOpenSpaceRect = [](const SolCityLotLayout::FCityBlock& Block) -> FOpenSpaceRect
	{
		FOpenSpaceRect Rect;
		Rect.Center = Block.Centroid;
		const FVector2D BoundsSize = Block.Bounds.GetSize();
		Rect.Size = FVector2D(
			FMath::Clamp(BoundsSize.X * 0.42f, 900.0f, 2800.0f),
			FMath::Clamp(BoundsSize.Y * 0.42f, 900.0f, 2400.0f));
		for (const SolCityLotLayout::FBlockEdge& Edge : Block.Edges)
		{
			const FVector2D Delta = Edge.End - Edge.Start;
			if (Delta.SizeSquared() > FMath::Square(Rect.Size.GetMax()))
			{
				Rect.Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
				break;
			}
		}

		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			const float Radians = FMath::DegreesToRadians(Rect.Yaw);
			const FVector2D AxisX(FMath::Cos(Radians), FMath::Sin(Radians));
			const FVector2D AxisY(-AxisX.Y, AxisX.X);
			const FVector2D HalfSize = Rect.Size * 0.5f;
			const FVector2D Corners[] = {
				Rect.Center - AxisX * HalfSize.X - AxisY * HalfSize.Y,
				Rect.Center + AxisX * HalfSize.X - AxisY * HalfSize.Y,
				Rect.Center + AxisX * HalfSize.X + AxisY * HalfSize.Y,
				Rect.Center - AxisX * HalfSize.X + AxisY * HalfSize.Y
			};
			bool bAllInside = true;
			for (const FVector2D& Corner : Corners)
			{
				bAllInside &= SolCityLotLayout::IsPointInsidePolygon(Corner, Block.Boundary);
			}
			if (bAllInside)
			{
				Rect.bValid = true;
				return Rect;
			}
			Rect.Size *= 0.82f;
		}
		return Rect;
	};

	auto ReserveOpenSpace = [this](const FOpenSpaceRect& Rect)
	{
		FBuildingFootprint& Reserved = OccupiedBuildings.AddDefaulted_GetRef();
		Reserved.Center = Rect.Center;
		Reserved.Extent = SolCityGeneration::RotatedExtent2D(Rect.Size, Rect.Yaw);
	};
	auto AddGroundedProp = [](UHierarchicalInstancedStaticMeshComponent* Group, const FVector2D& Position, float Yaw)
	{
		if (Group)
		{
			Group->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), FVector(Position.X, Position.Y, 16.0f), FVector::OneVector));
		}
	};
	auto LocalToWorld2D = [](const FOpenSpaceRect& Rect, const FVector2D& LocalOffset)
	{
		const float Radians = FMath::DegreesToRadians(Rect.Yaw);
		const FVector2D AxisX(FMath::Cos(Radians), FMath::Sin(Radians));
		const FVector2D AxisY(-AxisX.Y, AxisX.X);
		return Rect.Center + AxisX * LocalOffset.X + AxisY * LocalOffset.Y;
	};

	int32 ParkCount = 0;
	int32 ParkingCount = 0;
	int32 CourtyardCount = 0;
	for (int32 BlockIndex = 0; BlockIndex < Blocks.Num(); ++BlockIndex)
	{
		const SolCityLotLayout::FCityBlock& Block = Blocks[BlockIndex];
		SolCityLotLayout::FLotSettings BlockLotSettings = LotSettings;
		if (Block.Centroid.Size() < CbdRadius)
		{
			// CBD frontages range from narrow infill lots to merged tower podiums.
			BlockLotSettings.MinimumFrontage = SolCityGeneration::StandardLaneWidth * 1.9f;
			BlockLotSettings.MaximumFrontage = SolCityGeneration::StandardLaneWidth * 5.2f;
			BlockLotSettings.MinimumDepth = SolCityGeneration::StandardLaneWidth * 2.4f;
			BlockLotSettings.MaximumDepth = SolCityGeneration::StandardLaneWidth * 5.0f;
			BlockLotSettings.SideSetback = SolCityGeneration::BuildingGap;
		}
		SolCityLotLayout::SubdivideBlockFrontages(Block, BlockIndex, Random, BlockLotSettings, LotsByBlock[BlockIndex]);

		if (Block.Area < 6500000.0f || IsInsideRiver(Block.Centroid, 300.0f))
		{
			continue;
		}

		const int32 SpaceSelector = (BlockIndex * 7 + Seed) % 13;
		if (SpaceSelector > 2)
		{
			continue;
		}
		const FOpenSpaceRect Rect = FindOpenSpaceRect(Block);
		if (!Rect.bValid || Rect.Size.GetMin() < 650.0f)
		{
			continue;
		}

		ReserveOpenSpace(Rect);
		const FVector Center3D(Rect.Center.X, Rect.Center.Y, 0.0f);
		if (SpaceSelector == 0)
		{
			// A grass pocket park uses two paved desire lines and a planted center.
			AddBox(SidewalkInstances, Center3D + FVector(0.0f, 0.0f, 5.0f), FVector(Rect.Size.X, 150.0f, 10.0f), Rect.Yaw);
			AddBox(SidewalkInstances, Center3D + FVector(0.0f, 0.0f, 5.0f), FVector(Rect.Size.Y, 150.0f, 10.0f), Rect.Yaw + 90.0f);
			AddCylinder(CylinderInstances, Center3D + FVector(0.0f, 0.0f, 30.0f), 150.0f, 60.0f);
			AddGroundedProp(BenchInstances, LocalToWorld2D(Rect, FVector2D(-Rect.Size.X * 0.24f, -220.0f)), Rect.Yaw);
			AddGroundedProp(BenchInstances, LocalToWorld2D(Rect, FVector2D(Rect.Size.X * 0.24f, 220.0f)), Rect.Yaw + 180.0f);
			AddGroundedProp(TrashBinInstances, LocalToWorld2D(Rect, FVector2D(-Rect.Size.X * 0.12f, -250.0f)), Rect.Yaw);
			AddGroundedProp(StreetLampInstances, LocalToWorld2D(Rect, FVector2D(-Rect.Size.X * 0.38f, Rect.Size.Y * 0.36f)), Rect.Yaw);
			AddGroundedProp(StreetLampInstances, LocalToWorld2D(Rect, FVector2D(Rect.Size.X * 0.38f, -Rect.Size.Y * 0.36f)), Rect.Yaw + 180.0f);
			AddGroundedProp(PlanterInstances, Rect.Center, Rect.Yaw);
			++ParkCount;
		}
		else if (SpaceSelector == 1)
		{
			// Parking remains a distinct asphalt parcel instead of another road.
			AddBox(RoadInstances, Center3D + FVector(0.0f, 0.0f, 4.0f), FVector(Rect.Size.X, Rect.Size.Y, 8.0f), Rect.Yaw);
			const int32 StallCount = FMath::Clamp(FMath::FloorToInt(Rect.Size.X / 270.0f), 2, 9);
			for (int32 StallIndex = 1; StallIndex < StallCount; ++StallIndex)
			{
				const float Alpha = StallIndex / static_cast<float>(StallCount) - 0.5f;
				const FVector Offset = FRotator(0.0f, Rect.Yaw, 0.0f).RotateVector(FVector(Alpha * Rect.Size.X, 0.0f, 0.0f));
				AddBox(SidewalkInstances, Center3D + Offset + FVector(0.0f, 0.0f, 10.0f), FVector(10.0f, Rect.Size.Y * 0.42f, 4.0f), Rect.Yaw);
			}
			for (int32 StallIndex = 0; StallIndex < StallCount; ++StallIndex)
			{
				const float Alpha = (StallIndex + 0.5f) / StallCount - 0.5f;
				AddGroundedProp(ParkingWheelStopInstances, LocalToWorld2D(Rect, FVector2D(Alpha * Rect.Size.X, Rect.Size.Y * 0.28f)), Rect.Yaw);
			}
			AddGroundedProp(StreetLampInstances, LocalToWorld2D(Rect, FVector2D(0.0f, -Rect.Size.Y * 0.42f)), Rect.Yaw);
			AddGroundedProp(BollardInstances, LocalToWorld2D(Rect, FVector2D(-120.0f, -Rect.Size.Y * 0.46f)), Rect.Yaw);
			AddGroundedProp(BollardInstances, LocalToWorld2D(Rect, FVector2D(120.0f, -Rect.Size.Y * 0.46f)), Rect.Yaw);
			++ParkingCount;
		}
		else
		{
			// Courtyards are paved but keep a planter and clear central circulation.
			AddBox(SidewalkInstances, Center3D + FVector(0.0f, 0.0f, 4.0f), FVector(Rect.Size.X, Rect.Size.Y, 8.0f), Rect.Yaw);
			AddCylinder(CylinderInstances, Center3D + FVector(0.0f, 0.0f, 38.0f), 180.0f, 76.0f);
			AddGroundedProp(PlanterInstances, Rect.Center, Rect.Yaw);
			AddGroundedProp(BenchInstances, LocalToWorld2D(Rect, FVector2D(-Rect.Size.X * 0.28f, 0.0f)), Rect.Yaw + 90.0f);
			AddGroundedProp(BenchInstances, LocalToWorld2D(Rect, FVector2D(Rect.Size.X * 0.28f, 0.0f)), Rect.Yaw - 90.0f);
			AddGroundedProp(TrashBinInstances, LocalToWorld2D(Rect, FVector2D(0.0f, -Rect.Size.Y * 0.30f)), Rect.Yaw);
			AddGroundedProp(StreetLampInstances, LocalToWorld2D(Rect, FVector2D(0.0f, Rect.Size.Y * 0.34f)), Rect.Yaw + 180.0f);
			++CourtyardCount;
		}
	}

	int32 TaperedBuildingCount = 0;
	int32 TwinTowerBuildingCount = 0;
	auto TryPlaceBuilding = [this, HalfCity, &TaperedBuildingCount, &TwinTowerBuildingCount](
		const FVector2D& Candidate,
		const FVector2D& RequestedFootprint,
		float Yaw,
		int32 Style,
		float Height,
		int32 AcceptedIndex) -> bool
	{
		if (FMath::Abs(Candidate.X) > HalfCity || FMath::Abs(Candidate.Y) > HalfCity)
		{
			return false;
		}

		const FVector2D Footprint(
			FMath::Clamp(RequestedFootprint.X, SolCityGeneration::MinimumBuildingFootprint * 0.78f, SolCityGeneration::MaximumBuildingFootprint),
			FMath::Clamp(RequestedFootprint.Y, SolCityGeneration::MinimumBuildingFootprint * 0.78f, SolCityGeneration::MaximumBuildingFootprint));
		const bool bLargeBuilding = Footprint.X * Footprint.Y >= 850000.0f && Height >= 1800.0f;
		if (TwinTowerBuildingCount == 0 && Style != 1 && bLargeBuilding)
		{
			// Claim the first viable large non-corner parcel for the landmark. This
			// also covers frontage lots when the planar block graph is incomplete.
			Style = 5;
		}
		else if (Style == 5 && !bLargeBuilding)
		{
			Style = 0;
		}
		else if (Style != 1 && Style != 5 && bLargeBuilding)
		{
			Style = 2;
		}

		UHierarchicalInstancedStaticMeshComponent* AuthoredGroup = nullptr;
		UStaticMesh* AuthoredMesh = nullptr;
		if (AcceptedIndex % SolCityGeneration::AuthoredBuildingInterval == 0)
		{
			switch ((AcceptedIndex / SolCityGeneration::AuthoredBuildingInterval) % 3)
			{
			case 0:
				AuthoredGroup = AuthoredBuildingInstances;
				AuthoredMesh = AuthoredMidRiseMesh;
				break;
			case 1:
				AuthoredGroup = AuthoredCornerRetailInstances;
				AuthoredMesh = AuthoredCornerRetailMesh;
				break;
			default:
				AuthoredGroup = AuthoredSteppedTowerInstances;
				AuthoredMesh = AuthoredSteppedTowerMesh;
				break;
			}
			if (!AuthoredGroup || !AuthoredMesh)
			{
				AuthoredGroup = nullptr;
				AuthoredMesh = nullptr;
			}
		}

		float AuthoredUniformScale = GetAuthoredBuildingUniformScale(AuthoredMesh);
		if (AuthoredMesh)
		{
			const FVector MeshSize = AuthoredMesh->GetBoundingBox().GetSize();
			if (MeshSize.X > KINDA_SMALL_NUMBER && MeshSize.Y > KINDA_SMALL_NUMBER)
			{
				const float ParcelScale = FMath::Min(Footprint.X / MeshSize.X, Footprint.Y / MeshSize.Y) * 0.92f;
				AuthoredUniformScale = FMath::Min(AuthoredUniformScale, ParcelScale);
			}
			if (AuthoredUniformScale < 0.24f)
			{
				AuthoredGroup = nullptr;
				AuthoredMesh = nullptr;
			}
		}
		FVector2D LocalOccupiedSize = AuthoredMesh
			? FVector2D(AuthoredMesh->GetBoundingBox().GetSize().X, AuthoredMesh->GetBoundingBox().GetSize().Y) * AuthoredUniformScale
			: Footprint;
		FVector2D OccupiedExtent = AuthoredMesh
			? GetAuthoredBuildingExtent(AuthoredMesh, Yaw, AuthoredUniformScale)
			: SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		if (OccupiedExtent.GetMin() <= KINDA_SMALL_NUMBER)
		{
			AuthoredGroup = nullptr;
			AuthoredMesh = nullptr;
			LocalOccupiedSize = Footprint;
			OccupiedExtent = SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		}
		if (!IsBuildingSiteFree(Candidate, OccupiedExtent) || !IsBuildingClearOfRoads(Candidate, LocalOccupiedSize, Yaw))
		{
			return false;
		}

		if (!AuthoredMesh || !AddAuthoredBuilding(AuthoredGroup, AuthoredMesh, Candidate, Yaw, AuthoredUniformScale))
		{
			AddBuildingMass(Candidate, Footprint, Yaw, Style, Height);
			TaperedBuildingCount += Style == 2 ? 1 : 0;
			TwinTowerBuildingCount += Style == 5 ? 1 : 0;
			OccupiedExtent = SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		}

		FBuildingFootprint& NewFootprint = OccupiedBuildings.AddDefaulted_GetRef();
		NewFootprint.Center = Candidate;
		NewFootprint.Extent = OccupiedExtent;
		return true;
	};

	int32 MaximumLotsInBlock = 0;
	for (const TArray<SolCityLotLayout::FCityLot>& BlockLots : LotsByBlock)
	{
		MaximumLotsInBlock = FMath::Max(MaximumLotsInBlock, BlockLots.Num());
	}
	for (int32 Layer = 0; Layer < MaximumLotsInBlock && Accepted < EffectiveTargetCount; ++Layer)
	{
		for (int32 BlockIndex = 0; BlockIndex < LotsByBlock.Num() && Accepted < EffectiveTargetCount; ++BlockIndex)
		{
			if (!LotsByBlock[BlockIndex].IsValidIndex(Layer))
			{
				continue;
			}
			const SolCityLotLayout::FCityLot& Lot = LotsByBlock[BlockIndex][Layer];
			const float CenterBias = 1.0f - FMath::Clamp(Lot.Center.Size() / HalfCity, 0.0f, 1.0f);
			const float Height = Random.FRandRange(
				SolCityGeneration::StandardLaneWidth * (2.8f + CenterBias * 2.2f),
				SolCityGeneration::StandardLaneWidth * (5.5f + CenterBias * 12.5f));
			int32 Style = Lot.bCornerLot ? 1 : Random.RandRange(0, 5);
			if (!Lot.bCornerLot && TwinTowerBuildingCount == 0 &&
				Lot.Footprint.X * Lot.Footprint.Y >= 850000.0f && Height >= 1800.0f)
			{
				// Every deterministic city contains at least one landmark twin tower.
				Style = 5;
			}
			if (TryPlaceBuilding(Lot.Center, Lot.Footprint, Lot.YawDegrees, Style, Height, Accepted))
			{
				++Accepted;
			}
		}
	}

	// Open or disconnected street graphs do not enclose every district. Fill
	// those areas with deterministic frontage lots, never city-wide point scatter.
	for (int32 RoadIndex = 0; RoadIndex < RoadSegments.Num() && Accepted < EffectiveTargetCount; ++RoadIndex)
	{
		const FSolCityRoadSegment& Segment = RoadSegments[RoadIndex];
		if (Segment.bBridge)
		{
			continue;
		}
		const FVector2D Start(Segment.Start.X, Segment.Start.Y);
		const FVector2D End(Segment.End.X, Segment.End.Y);
		const FVector2D Delta = End - Start;
		const float Length = Delta.Size();
		if (Length < 1200.0f)
		{
			continue;
		}
		const FVector2D Along = Delta / Length;
		const FVector2D Normal(-Along.Y, Along.X);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
		const float Pitch = Random.FRandRange(780.0f, 1050.0f);
		const int32 SlotCount = FMath::Max(1, FMath::FloorToInt((Length - 300.0f) / Pitch));
		for (int32 SlotIndex = 0; SlotIndex < SlotCount && Accepted < EffectiveTargetCount; ++SlotIndex)
		{
			const float T = (SlotIndex + 0.5f) / SlotCount;
			const FVector2D FrontCenter = FMath::Lerp(Start, End, T);
			for (const float Side : {-1.0f, 1.0f})
			{
				const FVector2D Footprint(
					FMath::Clamp(Pitch * 0.78f, 600.0f, SolCityGeneration::MaximumBuildingFootprint),
					Random.FRandRange(650.0f, 920.0f));
				const FVector2D Candidate = FrontCenter + Normal * Side * (
					Segment.HalfWidth + SolCityGeneration::ProceduralRoadSetback + Footprint.Y * 0.5f + 45.0f);
				const float CenterBias = 1.0f - FMath::Clamp(Candidate.Size() / HalfCity, 0.0f, 1.0f);
				const float Height = Random.FRandRange(1000.0f, 1700.0f + CenterBias * 1400.0f);
				if (TryPlaceBuilding(Candidate, Footprint, Yaw, Random.RandRange(0, 5), Height, Accepted))
				{
					++Accepted;
					if (Accepted >= EffectiveTargetCount)
					{
						break;
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("SolCity parcel layout: blocks=%d buildings=%d/%d parks=%d parking=%d courtyards=%d tapered=%d twinSkybridge=%d"),
		Blocks.Num(), Accepted, EffectiveTargetCount, ParkCount, ParkingCount, CourtyardCount, TaperedBuildingCount, TwinTowerBuildingCount);
}

float ASolCityGenerator::GetAuthoredBuildingUniformScale(UStaticMesh* Mesh) const
{
	if (!Mesh)
	{
		return 1.0f;
	}

	const FVector MeshSize = Mesh->GetBoundingBox().GetSize();
	if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const float FootprintScale = SolCityGeneration::MaximumBuildingFootprint / FMath::Max(MeshSize.X, MeshSize.Y);
	const float HeightScale = SolCityGeneration::MaximumAuthoredBuildingHeight / MeshSize.Z;
	return FMath::Clamp(FMath::Min(FootprintScale, HeightScale), 0.10f, 1.0f);
}

FVector2D ASolCityGenerator::GetAuthoredBuildingExtent(UStaticMesh* Mesh, float YawDegrees, float UniformScale) const
{
	if (!Mesh)
	{
		return FVector2D::ZeroVector;
	}

	const FVector MeshSize = Mesh->GetBoundingBox().GetSize();
	if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}

	return SolCityGeneration::RotatedExtent2D(FVector2D(MeshSize.X, MeshSize.Y) * UniformScale, YawDegrees);
}

bool ASolCityGenerator::AddAuthoredBuilding(
	UHierarchicalInstancedStaticMeshComponent* Group,
	UStaticMesh* Mesh,
	const FVector2D& Center,
	float YawDegrees,
	float UniformScale)
{
	if (!Group || !Mesh)
	{
		return false;
	}

	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector MeshSize = Bounds.GetSize();
	if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FRotator Rotation(0.0f, YawDegrees, 0.0f);
	const FVector UniformScale3D(UniformScale);
	const FVector DesiredCenter(Center.X, Center.Y, MeshSize.Z * UniformScale * 0.5f);
	const FVector Translation = DesiredCenter - Rotation.RotateVector(Bounds.GetCenter() * UniformScale3D);
	Group->AddInstance(FTransform(Rotation, Translation, UniformScale3D));
	return true;
}

void ASolCityGenerator::AddBuildingMass(const FVector2D& Center, const FVector2D& Footprint, float YawDegrees, int32 Style, float Height)
{
	if (!BuildingBaseModuleInstances || !BuildingMiddleModuleInstances || !BuildingCrownModuleInstances ||
		!BuildingBaseModuleMesh || !BuildingMiddleModuleMesh || !BuildingCrownModuleMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SolCity skipped a procedural building because one or more authored stack modules are unavailable."));
		return;
	}

	const float BaseModuleHeight = BuildingBaseModuleMesh->GetBoundingBox().GetSize().Z;
	const float MidModuleHeight = BuildingMiddleModuleMesh->GetBoundingBox().GetSize().Z;
	const float CrownModuleHeight = BuildingCrownModuleMesh->GetBoundingBox().GetSize().Z;
	if (FMath::Min3(BaseModuleHeight, MidModuleHeight, CrownModuleHeight) <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const int32 RequestedMidFloorCount = FMath::Max(1,
		FMath::RoundToInt((Height - BaseModuleHeight - CrownModuleHeight) / MidModuleHeight));
	const float MaximumHeightToWidthRatio = (Style == 2 || Style == 5) ? 3.20f : 2.60f;
	const int32 AspectRatioFloorCap = FMath::Max(1, FMath::FloorToInt(
		(Footprint.GetMin() * MaximumHeightToWidthRatio - BaseModuleHeight - CrownModuleHeight) / MidModuleHeight));
	const int32 StyleFloorCap = (Style == 2 || Style == 5) ? 16 : (Style == 4 ? 4 : 10);
	const int32 MidFloorCount = FMath::Clamp(RequestedMidFloorCount, 1, FMath::Min(AspectRatioFloorCap, StyleFloorCap));
	float HighestRoofZ = BaseModuleHeight + MidFloorCount * MidModuleHeight;
	FVector2D RoofFootprint = Footprint;
	FVector2D RoofLocalOffset = FVector2D::ZeroVector;
	bool bTwinTower = false;
	FVector2D TwinTowerOffset = FVector2D::ZeroVector;

	auto AddStack = [this, Center, YawDegrees](
		UHierarchicalInstancedStaticMeshComponent* Group,
		UStaticMesh* ModuleMesh,
		const FVector2D& LocalOffset,
		const FVector2D& ModuleFootprint,
		float StartZ,
		int32 ModuleCount)
	{
		if (!Group || !ModuleMesh || ModuleCount <= 0)
		{
			return;
		}
		const FBox ModuleBounds = ModuleMesh->GetBoundingBox();
		const FVector ModuleSize = ModuleBounds.GetSize();
		if (ModuleSize.GetMin() <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		// Modules keep their authored storey height. Only one shared XY scalar is
		// chosen from the lane-derived parcel footprint; Z always remains 1.0.
		const float XYScale = FMath::Min(ModuleFootprint.X / ModuleSize.X, ModuleFootprint.Y / ModuleSize.Y);
		const FVector ModuleScale(XYScale, XYScale, 1.0f);
		const float ModuleHeight = ModuleSize.Z;
		const FRotator ModuleRotation(0.0f, YawDegrees, 0.0f);
		const FVector RotatedOffset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(LocalOffset.X, LocalOffset.Y, 0.0f));
		for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
		{
			const float ModuleCenterZ = StartZ + (ModuleIndex + 0.5f) * ModuleHeight;
			const FVector DesiredCenter = FVector(Center.X, Center.Y, ModuleCenterZ) + RotatedOffset;
			const FVector Translation = DesiredCenter - ModuleRotation.RotateVector(ModuleBounds.GetCenter() * ModuleScale);
			Group->AddInstance(FTransform(ModuleRotation, Translation, ModuleScale));
		}
	};

	switch (Style)
	{
	case 0: // Setback mid-rise
	{
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, FVector2D::ZeroVector, Footprint, 0.0f, 1);
		const FVector2D TowerSize = Footprint * 0.72f;
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, FVector2D::ZeroVector, TowerSize, BaseModuleHeight, MidFloorCount);
		AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, FVector2D::ZeroVector, TowerSize, HighestRoofZ, 1);
		HighestRoofZ += CrownModuleHeight;
		RoofFootprint = TowerSize;
		break;
	}
	case 1: // Authored corner module set, with a procedural L-shaped fallback.
	{
		if (CornerBaseModuleInstances && CornerMiddleModuleInstances && CornerCrownModuleInstances &&
			CornerBaseModuleMesh && CornerMiddleModuleMesh && CornerCrownModuleMesh)
		{
			const float CornerBaseHeight = CornerBaseModuleMesh->GetBoundingBox().GetSize().Z;
			const float CornerMiddleHeight = CornerMiddleModuleMesh->GetBoundingBox().GetSize().Z;
			const float CornerCrownHeight = CornerCrownModuleMesh->GetBoundingBox().GetSize().Z;
			AddStack(CornerBaseModuleInstances, CornerBaseModuleMesh, FVector2D::ZeroVector, Footprint, 0.0f, 1);
			AddStack(CornerMiddleModuleInstances, CornerMiddleModuleMesh, FVector2D::ZeroVector, Footprint, CornerBaseHeight, MidFloorCount);
			HighestRoofZ = CornerBaseHeight + CornerMiddleHeight * MidFloorCount;
			AddStack(CornerCrownModuleInstances, CornerCrownModuleMesh, FVector2D::ZeroVector, Footprint, HighestRoofZ, 1);
			HighestRoofZ += CornerCrownHeight;
			RoofFootprint = Footprint;
			break;
		}

		const FVector2D MainWing(Footprint.X, Footprint.Y * 0.46f);
		const FVector2D SideWing(Footprint.X * 0.42f, Footprint.Y * 0.52f);
		const FVector2D SideOffset(-Footprint.X * 0.29f, Footprint.Y * 0.25f);
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, FVector2D::ZeroVector, MainWing, 0.0f, 1);
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, SideOffset, SideWing, 0.0f, 1);
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, FVector2D::ZeroVector, MainWing, BaseModuleHeight, MidFloorCount);
		const int32 SideFloorCount = FMath::Max(1, MidFloorCount - 1);
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, SideOffset, SideWing, BaseModuleHeight, SideFloorCount);
		AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, FVector2D::ZeroVector, MainWing, HighestRoofZ, 1);
		HighestRoofZ += CrownModuleHeight;
		RoofFootprint = MainWing;
		break;
	}
	case 2: // Monotonically tapered tower; upper floors never exceed lower floors.
	{
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, FVector2D::ZeroVector, Footprint, 0.0f, 1);
		FVector2D PreviousSize = Footprint * 0.90f;
		FVector2D PreviousOffset = FVector2D::ZeroVector;
		const float AnchorX = Random.RandRange(0, 1) == 0 ? -1.0f : 1.0f;
		const float AnchorY = Random.RandRange(0, 1) == 0 ? -1.0f : 1.0f;
		for (int32 FloorIndex = 0; FloorIndex < MidFloorCount; ++FloorIndex)
		{
			FVector2D FloorSize = PreviousSize;
			FVector2D FloorOffset = PreviousOffset;
			if (FloorIndex > 0 && FloorIndex % 2 == 0)
			{
				const FVector2D ShrinkFactor(
					Random.FRandRange(0.84f, 0.94f),
					Random.FRandRange(0.86f, 0.96f));
				FloorSize = FVector2D(
					FMath::Max(Footprint.X * 0.56f, PreviousSize.X * ShrinkFactor.X),
					FMath::Max(Footprint.Y * 0.56f, PreviousSize.Y * ShrinkFactor.Y));
				const FVector2D ContainmentShift = (PreviousSize - FloorSize) * 0.5f;
				FloorOffset += FVector2D(ContainmentShift.X * AnchorX, ContainmentShift.Y * AnchorY);
			}
			AddStack(
				BuildingMiddleModuleInstances,
				BuildingMiddleModuleMesh,
				FloorOffset,
				FloorSize,
				BaseModuleHeight + FloorIndex * MidModuleHeight,
				1);
			PreviousSize = FloorSize;
			PreviousOffset = FloorOffset;
		}
		const FVector2D CrownSize = PreviousSize * 0.88f;
		const FVector2D CrownShift = (PreviousSize - CrownSize) * 0.5f;
		const FVector2D CrownOffset = PreviousOffset + FVector2D(CrownShift.X * AnchorX, CrownShift.Y * AnchorY);
		AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, CrownOffset, CrownSize, HighestRoofZ, 1);
		HighestRoofZ += CrownModuleHeight;
		RoofFootprint = CrownSize;
		RoofLocalOffset = CrownOffset;
		break;
	}
	case 3: // Courtyard cluster
	{
		const float WingWidth = FMath::Clamp(
			FMath::Min(Footprint.X, Footprint.Y) * 0.34f,
			SolCityGeneration::StandardLaneWidth * 0.75f,
			FMath::Min(Footprint.X, Footprint.Y) * 0.40f);
		const FVector2D CrossWing(Footprint.X, WingWidth);
		const FVector2D SideWing(WingWidth, Footprint.Y - WingWidth);
		const FVector2D SideOffset(0.0f, (Footprint.Y - WingWidth) * 0.5f);
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, FVector2D::ZeroVector, CrossWing, 0.0f, 1);
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, SideOffset, SideWing, 0.0f, 1);
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, -SideOffset, SideWing, 0.0f, 1);
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, FVector2D::ZeroVector, CrossWing, BaseModuleHeight, MidFloorCount);
		const int32 WingFloorCount = FMath::Max(1, MidFloorCount - 1);
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, SideOffset, SideWing, BaseModuleHeight, WingFloorCount);
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, -SideOffset, SideWing, BaseModuleHeight, WingFloorCount);
		AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, FVector2D::ZeroVector, CrossWing, HighestRoofZ, 1);
		HighestRoofZ += CrownModuleHeight;
		RoofFootprint = CrossWing;
		break;
	}
	case 5: // Symmetric twin towers connected by an elevated skybridge.
	{
		const FVector2D TowerSize(Footprint.X * 0.38f, Footprint.Y * 0.82f);
		TwinTowerOffset = FVector2D((Footprint.X - TowerSize.X) * 0.44f, 0.0f);
		for (const float Side : {-1.0f, 1.0f})
		{
			const FVector2D TowerOffset = TwinTowerOffset * Side;
			AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, TowerOffset, TowerSize, 0.0f, 1);
			AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, TowerOffset, TowerSize, BaseModuleHeight, MidFloorCount);
			AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, TowerOffset, TowerSize * 0.90f, HighestRoofZ, 1);
		}

		const int32 BridgeFloor = FMath::Clamp(FMath::FloorToInt(MidFloorCount * 0.62f), 1, FMath::Max(1, MidFloorCount - 1));
		const float BridgeBottomZ = BaseModuleHeight + BridgeFloor * MidModuleHeight;
		const float DesiredBridgeLength = FMath::Max(300.0f, TwinTowerOffset.X * 2.0f - TowerSize.X + 140.0f);
		if (SkybridgeConnectorInstances && SkybridgeConnectorMesh)
		{
			const FVector MeshSize = SkybridgeConnectorMesh->GetBoundingBox().GetSize();
			if (MeshSize.GetMin() > KINDA_SMALL_NUMBER)
			{
				const FVector Scale(
					DesiredBridgeLength / MeshSize.X,
					FMath::Min(1.0f, TowerSize.Y * 0.42f / MeshSize.Y),
					1.0f);
				SkybridgeConnectorInstances->AddInstance(FTransform(
					FRotator(0.0f, YawDegrees, 0.0f),
					FVector(Center.X, Center.Y, BridgeBottomZ),
					Scale));
			}
		}
		else
		{
			AddBox(
				DetailInstances,
				FVector(Center.X, Center.Y, BridgeBottomZ + MidModuleHeight * 0.42f),
				FVector(DesiredBridgeLength, TowerSize.Y * 0.34f, MidModuleHeight * 0.84f),
				YawDegrees);
		}

		HighestRoofZ += CrownModuleHeight;
		RoofFootprint = TowerSize * 0.90f;
		RoofLocalOffset = TwinTowerOffset;
		bTwinTower = true;
		break;
	}
	case 4: // Small anime-neighborhood row houses
	{
		const int32 MaximumHouseCount = FMath::Clamp(
			FMath::FloorToInt(Footprint.X / (SolCityGeneration::StandardLaneWidth * 1.35f)), 1, 2);
		const int32 HouseCount = Random.RandRange(1, MaximumHouseCount);
		for (int32 HouseIndex = 0; HouseIndex < HouseCount; ++HouseIndex)
		{
			const float Alpha = (HouseIndex + 0.5f) / HouseCount - 0.5f;
			const FVector2D HouseOffset(Alpha * Footprint.X, 0.0f);
			const FVector2D HouseFootprint(Footprint.X / HouseCount - SolCityGeneration::StandardLaneWidth * 0.08f, Footprint.Y);
			const int32 HouseMidFloorCount = FMath::Clamp(MidFloorCount + Random.RandRange(-1, 1), 1, 4);
			AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, HouseOffset, HouseFootprint, 0.0f, 1);
			AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, HouseOffset, HouseFootprint * FVector2D(0.92f, 0.92f), BaseModuleHeight, HouseMidFloorCount);
			const float HouseRoofZ = BaseModuleHeight + HouseMidFloorCount * MidModuleHeight;
			AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, HouseOffset, HouseFootprint, HouseRoofZ, 1);
			HighestRoofZ = FMath::Max(HighestRoofZ, HouseRoofZ + CrownModuleHeight);
		}
		RoofFootprint = Footprint;
		break;
	}
	default:
		break;
	}

	// Authored rooftop modules are selected by usable roof area. They never
	// enlarge the occupied floor plate and therefore preserve the taper rule.
	if (HighestRoofZ > SolCityGeneration::StandardLaneWidth * 2.5f)
	{
		auto AddRoofModule = [Center, YawDegrees, HighestRoofZ, RoofFootprint, RoofLocalOffset](
			UHierarchicalInstancedStaticMeshComponent* Group,
			UStaticMesh* Mesh,
			const FVector2D& AdditionalLocalOffset,
			float AdditionalYaw = 0.0f) -> bool
		{
			if (!Group || !Mesh)
			{
				return false;
			}
			const FVector MeshSize = Mesh->GetBoundingBox().GetSize();
			if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER ||
				MeshSize.X > RoofFootprint.X * 0.92f || MeshSize.Y > RoofFootprint.Y * 0.92f)
			{
				return false;
			}
			const FVector2D LocalOffset = RoofLocalOffset + AdditionalLocalOffset;
			const FVector RotatedOffset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(LocalOffset.X, LocalOffset.Y, 0.0f));
			Group->AddInstance(FTransform(
				FRotator(0.0f, YawDegrees + AdditionalYaw, 0.0f),
				FVector(Center.X, Center.Y, HighestRoofZ) + RotatedOffset,
				FVector::OneVector));
			return true;
		};

		const float BaseArea = Footprint.X * Footprint.Y;
		const float RoofArea = RoofFootprint.X * RoofFootprint.Y;
		const bool bLargeHighRise = BaseArea >= 850000.0f && HighestRoofZ >= 2200.0f;
		bool bPlacedPrimary = false;
		if (bLargeHighRise && RoofArea >= 650000.0f && Random.RandRange(0, 2) == 0)
		{
			bPlacedPrimary = AddRoofModule(RooftopHelipadInstances, RooftopHelipadMesh, FVector2D::ZeroVector);
		}
		if (bLargeHighRise && !bPlacedPrimary)
		{
			bPlacedPrimary = AddRoofModule(RooftopWaterTankInstances, RooftopWaterTankMesh, FVector2D::ZeroVector);
		}
		if (!bPlacedPrimary)
		{
			switch (Random.RandRange(0, 3))
			{
			case 0:
				bPlacedPrimary = AddRoofModule(RooftopHVACInstances, RooftopHVACMesh, FVector2D::ZeroVector);
				break;
			case 1:
				bPlacedPrimary = AddRoofModule(RooftopSolarInstances, RooftopSolarMesh, FVector2D::ZeroVector, 90.0f);
				break;
			case 2:
				bPlacedPrimary = AddRoofModule(RooftopPergolaInstances, RooftopPergolaMesh, FVector2D::ZeroVector);
				break;
			default:
				bPlacedPrimary = AddRoofModule(RooftopAntennaInstances, RooftopAntennaMesh, FVector2D::ZeroVector);
				break;
			}
		}

		if (bLargeHighRise)
		{
			if (bTwinTower)
			{
				const FVector2D OtherTowerOffset = -TwinTowerOffset * 2.0f;
				AddRoofModule(RooftopAntennaInstances, RooftopAntennaMesh, OtherTowerOffset);
				AddRoofModule(RooftopWarningBeaconInstances, RooftopWarningBeaconMesh, OtherTowerOffset + RoofFootprint * FVector2D(0.24f, 0.24f));
				AddRoofModule(RooftopWarningBeaconInstances, RooftopWarningBeaconMesh, RoofFootprint * FVector2D(-0.24f, -0.24f));
			}
			else
			{
				const FVector2D BeaconOffset = RoofFootprint * FVector2D(0.30f, 0.30f);
				AddRoofModule(RooftopWarningBeaconInstances, RooftopWarningBeaconMesh, BeaconOffset);
				if (RoofArea >= 800000.0f)
				{
					AddRoofModule(RooftopWarningBeaconInstances, RooftopWarningBeaconMesh, -BeaconOffset);
				}
			}
		}

		if (!bPlacedPrimary)
		{
			const FVector UtilitySize(
				SolCityGeneration::StandardLaneWidth * 0.28f,
				SolCityGeneration::StandardLaneWidth * 0.38f,
				SolCityGeneration::StandardLaneWidth * 0.36f);
			const FVector LocalUtilityOffset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(RoofLocalOffset.X, RoofLocalOffset.Y, 0.0f));
			AddBox(DetailInstances, FVector(Center.X, Center.Y, HighestRoofZ + UtilitySize.Z * 0.5f) + LocalUtilityOffset, UtilitySize, YawDegrees);
		}
	}
}

void ASolCityGenerator::GenerateTrees()
{
	if (!TreeInstances || !AuthoredTreeMesh)
	{
		return;
	}
	const FBox Bounds = AuthoredTreeMesh->GetBoundingBox();
	const float HalfCity = CityDiameter * 0.5f - 450.0f;
	const int32 TargetTrees = FMath::Clamp(FMath::RoundToInt(CityDiameter / 180.0f), 90, 220);
	int32 Added = 0;
	for (int32 Attempt = 0; Attempt < TargetTrees * 25 && Added < TargetTrees; ++Attempt)
	{
		const FVector2D Candidate(Random.FRandRange(-HalfCity, HalfCity), Random.FRandRange(-HalfCity, HalfCity));
		if (IsInsideRiver(Candidate, 180.0f) || !IsNearRoad(Candidate, 860.0f) || IsNearRoad(Candidate, 430.0f))
		{
			continue;
		}
		bool bOverlapsBuilding = false;
		for (const FBuildingFootprint& Building : OccupiedBuildings)
		{
			const FVector2D Delta = (Candidate - Building.Center).GetAbs();
			if (Delta.X < Building.Extent.X + 150.0f && Delta.Y < Building.Extent.Y + 150.0f)
			{
				bOverlapsBuilding = true;
				break;
			}
		}
		if (bOverlapsBuilding)
		{
			continue;
		}
		const float UniformScale = Random.FRandRange(0.82f, 1.18f);
		const FRotator Rotation(0.0f, Random.FRandRange(-180.0f, 180.0f), 0.0f);
		const FVector Scale(UniformScale);
		const FVector DesiredCenter(Candidate.X, Candidate.Y, Bounds.GetSize().Z * UniformScale * 0.5f);
		TreeInstances->AddInstance(FTransform(Rotation, DesiredCenter - Rotation.RotateVector(Bounds.GetCenter() * Scale), Scale));
		++Added;
	}
}

void ASolCityGenerator::GenerateBridge()
{
	const float RiverY = RiverCenterY(0.0f);
	const float BridgeLength = RiverWidth + 1120.0f;
	const float DeckWidth = SolCityGeneration::ArterialRoadWidth + SolCityGeneration::ArterialSidewalkWidth * 2.0f + 100.0f;
	const FVector BridgeStart(0.0f, RiverY - BridgeLength * 0.5f, SolCityGeneration::BridgeDeckZ);
	const FVector BridgeEnd(0.0f, RiverY + BridgeLength * 0.5f, SolCityGeneration::BridgeDeckZ);

	// The authored bridge owns the visual roadway. Navigation remains a normal
	// directed road edge and supplies two pedestrian offsets along its walks.
	AddRoadSegment(BridgeStart, BridgeEnd, SolCityGeneration::ArterialRoadWidth, ESolCityRoadClass::Bridge, true, true, false);
	if (AuthoredBridgeInstances && AuthoredBridgeMesh)
	{
		const FBox Bounds = AuthoredBridgeMesh->GetBoundingBox();
		const FVector Size = Bounds.GetSize();
		if (Size.GetMin() > KINDA_SMALL_NUMBER)
		{
			const FVector Scale(BridgeLength / Size.X, DeckWidth / Size.Y, 1.0f);
			const FRotator Rotation(0.0f, 90.0f, 0.0f);
			constexpr float AuthoredDeckSurfaceZ = 455.0f;
			const float BottomZ = SolCityGeneration::BridgeDeckZ - AuthoredDeckSurfaceZ * Scale.Z;
			const FVector DesiredCenter(0.0f, RiverY, BottomZ + Size.Z * Scale.Z * 0.5f);
			const FVector Translation = DesiredCenter - Rotation.RotateVector(Bounds.GetCenter() * Scale);
			AuthoredBridgeInstances->AddInstance(FTransform(Rotation, Translation, Scale));
			return;
		}
	}

	const float DeckThickness = 105.0f;
	AddBox(BridgeInstances, FVector(0.0f, RiverY, SolCityGeneration::BridgeDeckZ - DeckThickness * 0.5f), FVector(DeckWidth, BridgeLength, DeckThickness), 0.0f);

	// Carefully separated abutments, twin piers, crossheads and railings.
	for (float EndSign : {-1.0f, 1.0f})
	{
		const float Y = RiverY + EndSign * (BridgeLength * 0.5f - 170.0f);
		AddBox(BridgeInstances, FVector(0.0f, Y, 2.0f), FVector(1320.0f, 280.0f, 205.0f));
		AddBox(BridgeInstances, FVector(0.0f, Y + EndSign * 190.0f, 43.0f), FVector(1180.0f, 140.0f, 120.0f));
	}

	for (float PierY : {RiverY - RiverWidth * 0.24f, RiverY + RiverWidth * 0.24f})
	{
		for (float PierX : {-280.0f, 280.0f})
		{
			const float PierHeight = SolCityGeneration::BridgeDeckZ - RiverSurfaceZ + 90.0f;
			AddCylinder(CylinderInstances, FVector(PierX, PierY, RiverSurfaceZ + PierHeight * 0.5f - 35.0f), 78.0f, PierHeight);
		}
		AddBox(BridgeInstances, FVector(0.0f, PierY, SolCityGeneration::BridgeDeckZ - 62.0f), FVector(900.0f, 150.0f, 125.0f));
	}

	for (float RailX : {-520.0f, 520.0f})
	{
		AddBox(BridgeInstances, FVector(RailX, RiverY, SolCityGeneration::BridgeDeckZ + 86.0f), FVector(38.0f, BridgeLength, 46.0f));
		for (int32 Post = 0; Post <= 14; ++Post)
		{
			const float Alpha = Post / 14.0f;
			const float Y = FMath::Lerp(BridgeStart.Y, BridgeEnd.Y, Alpha);
			AddBox(BridgeInstances, FVector(RailX, Y, SolCityGeneration::BridgeDeckZ + 65.0f), FVector(52.0f, 52.0f, 180.0f));
		}
	}
}

void ASolCityGenerator::GenerateTrafficFurniture()
{
	const float LayoutScale = CityDiameter / 18000.0f;
	const float RiverY = RiverCenterY(0.0f);
	const TArray<FVector> SignalCenters = {
		FVector(0.0f, -2500.0f * LayoutScale, 0.0f), FVector(0.0f, 6100.0f * LayoutScale, 0.0f),
		FVector(-6100.0f * LayoutScale, -3300.0f * LayoutScale, 0.0f), FVector(6100.0f * LayoutScale, -2900.0f * LayoutScale, 0.0f),
		FVector(0.0f, RiverY - RiverWidth * 0.5f - 650.0f, 0.0f),
		FVector(0.0f, RiverY + RiverWidth * 0.5f + 650.0f, 0.0f)
	};

	for (const FVector& Center : SignalCenters)
	{
		AddJunctionCap(Center, SolCityGeneration::ArterialRoadWidth, ESolCityRoadClass::Arterial);
		TrafficSignalLocations.Add(Center + FVector(0.0f, 0.0f, 20.0f));
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const float XSign = (Corner & 1) ? 1.0f : -1.0f;
			const float YSign = (Corner & 2) ? 1.0f : -1.0f;
			const FVector PoleBase = Center + FVector(XSign * 470.0f, YSign * 470.0f, 0.0f);
			AddCylinder(CylinderInstances, PoleBase + FVector(0.0f, 0.0f, 190.0f), 20.0f, 380.0f);
			AddBox(DetailInstances, PoleBase + FVector(0.0f, 0.0f, 390.0f), FVector(90.0f, 70.0f, 190.0f), Corner * 90.0f);
		}
	}

	// Language-free billboard art rotates across four material variants. Sites
	// are reserved like small parcels so buildings and trees do not clip them.
	UHierarchicalInstancedStaticMeshComponent* BillboardGroups[] = {
		BillboardInstances01, BillboardInstances02, BillboardInstances03, BillboardInstances04
	};
	int32 BillboardCount = 0;
	for (int32 RoadIndex = 0; RoadIndex < RoadSegments.Num() && BillboardCount < 12; ++RoadIndex)
	{
		const FSolCityRoadSegment& Segment = RoadSegments[RoadIndex];
		if (Segment.bBridge || Segment.RoadClass == ESolCityRoadClass::Local || (RoadIndex + Seed) % 3 != 0)
		{
			continue;
		}
		const FVector2D Start(Segment.Start.X, Segment.Start.Y);
		const FVector2D End(Segment.End.X, Segment.End.Y);
		const FVector2D Delta = End - Start;
		if (Delta.SizeSquared() < FMath::Square(2400.0f))
		{
			continue;
		}
		const FVector2D Along = Delta.GetSafeNormal();
		const FVector2D Normal(-Along.Y, Along.X);
		const float Side = (RoadIndex & 1) == 0 ? 1.0f : -1.0f;
		const float SidewalkWidth = SolCityGeneration::SidewalkWidthForRoadClass(Segment.RoadClass);
		const FVector2D Candidate = (Start + End) * 0.5f + Normal * Side * (
			Segment.HalfWidth + SidewalkWidth + 420.0f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
		const FVector2D Extent = SolCityGeneration::RotatedExtent2D(FVector2D(650.0f, 180.0f), Yaw);
		if (IsInsideRiver(Candidate, 250.0f) || !IsBuildingSiteFree(Candidate, Extent))
		{
			continue;
		}
		UHierarchicalInstancedStaticMeshComponent* Group = BillboardGroups[BillboardCount % 4];
		if (!Group)
		{
			continue;
		}
		Group->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), FVector(Candidate.X, Candidate.Y, 16.0f), FVector::OneVector));
		FBuildingFootprint& Reserved = OccupiedBuildings.AddDefaulted_GetRef();
		Reserved.Center = Candidate;
		Reserved.Extent = Extent;
		++BillboardCount;
	}
}
