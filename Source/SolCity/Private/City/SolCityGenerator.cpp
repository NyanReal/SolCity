#include "City/SolCityGenerator.h"

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
	constexpr float BuildingGap = StandardLaneWidth * 0.65f;
	constexpr float RiverBuildingSetback = StandardLaneWidth * 0.75f;
	constexpr float ProceduralRoadSetback = StandardLaneWidth * 0.65f;
	constexpr float BuildingRoadSearchDepth = StandardLaneWidth * 4.0f;
	constexpr float MinimumBuildingFootprint = StandardLaneWidth * 2.0f;
	constexpr float MaximumBuildingFootprint = StandardLaneWidth * 3.35f;
	constexpr float MaximumAuthoredBuildingHeight = StandardLaneWidth * 7.5f;
	constexpr float TargetParcelPitch = StandardLaneWidth * 9.45f;
	constexpr int32 AuthoredBuildingInterval = 4;
	constexpr float RoadSurfaceZ = 7.0f;
	constexpr float SidewalkSurfaceZ = 16.0f;
	constexpr float BridgeDeckZ = 105.0f;

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
	AuthoredRoadSplineMesh = AuthoredRoadSplineAsset.Object;
	AuthoredRoadJunctionMesh = AuthoredRoadJunctionAsset.Object;
	AuthoredBridgeMesh = AuthoredBridgeAsset.Object;
	AuthoredTreeMesh = AuthoredTreeAsset.Object;
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
	GenerateBuildings();
	GenerateTrees();
	GenerateBridge();
	GenerateTrafficFurniture();
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
	SidewalkInstances = nullptr;
	BuildingBaseModuleInstances = nullptr;
	BuildingMiddleModuleInstances = nullptr;
	BuildingCrownModuleInstances = nullptr;
	AuthoredBuildingInstances = nullptr;
	AuthoredCornerRetailInstances = nullptr;
	AuthoredSteppedTowerInstances = nullptr;
	BridgeInstances = nullptr;
	AuthoredBridgeInstances = nullptr;
	JunctionInstances = nullptr;
	TreeInstances = nullptr;
	DetailInstances = nullptr;
	CylinderInstances = nullptr;
	WaterSurfaceMesh = nullptr;
	WaterMID = nullptr;
	bHasGenerated = false;
}

void ASolCityGenerator::CreateInstanceGroups()
{
	RoadInstances = CreateInstanceGroup(TEXT("GeneratedRoads"), CubeMesh, RoadMaterial, FLinearColor(0.12f, 0.16f, 0.20f), true);
	SidewalkInstances = CreateInstanceGroup(TEXT("GeneratedSidewalks"), CubeMesh, SidewalkMaterial, FLinearColor(0.72f, 0.69f, 0.62f), true);
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
		AddBox(bBridge ? BridgeInstances : RoadInstances, Center, FVector(Length, Width, SolCityGeneration::RoadSurfaceZ), Yaw);
	}

	FSolCityRoadSegment& Segment = RoadSegments.AddDefaulted_GetRef();
	Segment.Start = Start;
	Segment.End = End;
	Segment.HalfWidth = Width * 0.5f;
	Segment.RoadClass = bBridge ? ESolCityRoadClass::Bridge : RoadClass;
	Segment.bBridge = bBridge;

	if (bAddSidewalks)
	{
		const FVector Direction = SolCityGeneration::SafeNormal2D(Delta);
		const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
		const float SidewalkWidth = RoadClass == ESolCityRoadClass::Arterial ? 180.0f : 135.0f;
		const float Offset = Width * 0.5f + SidewalkWidth * 0.5f + 20.0f;
		for (float Side : {-1.0f, 1.0f})
		{
			const FVector WalkCenter = Center + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ * 0.5f);
			if (bCreateVisual)
			{
				AddBox(bBridge ? BridgeInstances : SidewalkInstances, WalkCenter, FVector(Length, SidewalkWidth, SolCityGeneration::SidewalkSurfaceZ), Yaw);
			}
			PedestrianWaypoints.Add(Start + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ));
			PedestrianWaypoints.Add(End + Perpendicular * Offset + FVector(0.0f, 0.0f, SolCityGeneration::SidewalkSurfaceZ));
		}
	}
}

void ASolCityGenerator::AddPolylineRoad(const TArray<FVector>& Points, float Width, ESolCityRoadClass RoadClass, bool bAddSidewalks)
{
	TArray<FVector> VisibleChunk;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Midpoint = (Points[Index - 1] + Points[Index]) * 0.5f;
		if (!IsInsideRiver(FVector2D(Midpoint.X, Midpoint.Y), Width * 0.15f))
		{
			if (VisibleChunk.IsEmpty())
			{
				VisibleChunk.Add(Points[Index - 1]);
			}
			VisibleChunk.Add(Points[Index]);
			AddRoadSegment(Points[Index - 1], Points[Index], Width, RoadClass, bAddSidewalks, false, false);
		}
		else if (VisibleChunk.Num() >= 2)
		{
			AddSplineRoadVisual(VisibleChunk, Width);
			VisibleChunk.Reset();
		}
	}
	if (VisibleChunk.Num() >= 2)
	{
		AddSplineRoadVisual(VisibleChunk, Width);
	}
	if (Points.Num() >= 2)
	{
		AddJunctionCap(Points[0], Width);
		AddJunctionCap(Points.Last(), Width);
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

void ASolCityGenerator::AddJunctionCap(const FVector& Position, float Width)
{
	if (!JunctionInstances || !AuthoredRoadJunctionMesh || IsInsideRiver(FVector2D(Position.X, Position.Y), Width * 0.15f))
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
	const FBox Bounds = AuthoredRoadJunctionMesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float TargetSize = 1100.0f * Width / 720.0f;
	const FVector Scale(TargetSize / Size.X, TargetSize / Size.Y, 1.0f);
	const FVector DesiredCenter(Position.X, Position.Y, Bounds.GetSize().Z * 0.5f);
	JunctionInstances->AddInstance(FTransform(FRotator::ZeroRotator, DesiredCenter - Bounds.GetCenter() * Scale, Scale));
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
	AddPolylineRoad(EastWestSpine, 720.0f, ESolCityRoadClass::Arterial);

	const float BridgeApproach = RiverWidth * 0.5f + 560.0f;
	AddPolylineRoad({FVector(0.0f, -H, 0.0f), FVector(0.0f, RiverCenterY(0.0f) - BridgeApproach, 0.0f)}, 720.0f, ESolCityRoadClass::Arterial);
	AddPolylineRoad({FVector(0.0f, RiverCenterY(0.0f) + BridgeApproach, 0.0f), FVector(0.0f, H, 0.0f)}, 720.0f, ESolCityRoadClass::Arterial);

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
	AddPolylineRoad(Ring, 540.0f, ESolCityRoadClass::Collector);

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
			AddPolylineRoad(BranchPoints, BranchIndex == 0 ? 420.0f : 330.0f, BranchIndex == 0 ? ESolCityRoadClass::Collector : ESolCityRoadClass::Local);
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
	const int32 DensityTarget = FMath::Max(12, FMath::RoundToInt(FMath::Square(CityDiameter / SolCityGeneration::TargetParcelPitch)));
	const int32 EffectiveTargetCount = FMath::Min(TargetBuildingCount, DensityTarget);
	int32 Accepted = 0;
	const int32 MaxAttempts = EffectiveTargetCount * 90;

	for (int32 Attempt = 0; Attempt < MaxAttempts && Accepted < EffectiveTargetCount; ++Attempt)
	{
		// Low-discrepancy polar sampling avoids rows while still filling districts.
		const float GoldenAngle = 137.50776f;
		const float Radius = FMath::Sqrt((Attempt + Random.FRand()) / static_cast<float>(MaxAttempts)) * HalfCity * 0.93f;
		const float Angle = FMath::DegreesToRadians(Attempt * GoldenAngle + Random.FRandRange(-12.0f, 12.0f));
		const FVector2D Candidate(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);

		if (!IsNearRoad(Candidate, SolCityGeneration::BuildingRoadSearchDepth))
		{
			continue;
		}

		const int32 Style = Random.RandRange(0, 4);
		const float LongSide = Random.FRandRange(SolCityGeneration::StandardLaneWidth * 2.25f, SolCityGeneration::MaximumBuildingFootprint);
		const float ShortSide = FMath::Max(
			SolCityGeneration::MinimumBuildingFootprint,
			LongSide * Random.FRandRange(0.68f, 0.96f));
		const FVector2D Footprint = Random.RandRange(0, 1) == 0
			? FVector2D(LongSide, ShortSide)
			: FVector2D(ShortSide, LongSide);
		float Height = Random.FRandRange(
			SolCityGeneration::StandardLaneWidth * 2.5f,
			SolCityGeneration::StandardLaneWidth * 5.5f);
		const float CenterBias = 1.0f - FMath::Clamp(Candidate.Size() / HalfCity, 0.0f, 1.0f);
		Height += FMath::Square(CenterBias) * Random.FRandRange(
			SolCityGeneration::StandardLaneWidth * 1.5f,
			SolCityGeneration::StandardLaneWidth * 4.2f);
		const float Yaw = Random.FRandRange(-38.0f, 38.0f);

		UHierarchicalInstancedStaticMeshComponent* AuthoredGroup = nullptr;
		UStaticMesh* AuthoredMesh = nullptr;
		// Every fourth accepted parcel is an authored building. Cycling the three
		// meshes independently of random style/height guarantees a visible mix.
		if (Accepted % SolCityGeneration::AuthoredBuildingInterval == 0)
		{
			switch ((Accepted / SolCityGeneration::AuthoredBuildingInterval) % 3)
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
				// Missing authored assets never block city generation.
				AuthoredGroup = nullptr;
				AuthoredMesh = nullptr;
			}
		}

		// Authored FBX buildings are complete models. Their native mesh bounds,
		// not the procedural target footprint, own the parcel they occupy.
		const float AuthoredUniformScale = GetAuthoredBuildingUniformScale(AuthoredMesh);
		FVector2D LocalOccupiedSize = AuthoredMesh
			? FVector2D(AuthoredMesh->GetBoundingBox().GetSize().X, AuthoredMesh->GetBoundingBox().GetSize().Y) * AuthoredUniformScale
			: Footprint;
		FVector2D OccupiedExtent = AuthoredMesh ? GetAuthoredBuildingExtent(AuthoredMesh, Yaw, AuthoredUniformScale) : SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		if (OccupiedExtent.GetMin() <= KINDA_SMALL_NUMBER)
		{
			AuthoredGroup = nullptr;
			AuthoredMesh = nullptr;
			LocalOccupiedSize = Footprint;
			OccupiedExtent = SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		}
		if (!IsBuildingSiteFree(Candidate, OccupiedExtent) || !IsBuildingClearOfRoads(Candidate, LocalOccupiedSize, Yaw))
		{
			continue;
		}

		if (!AuthoredMesh || !AddAuthoredBuilding(AuthoredGroup, AuthoredMesh, Candidate, Yaw, AuthoredUniformScale))
		{
			AddBuildingMass(Candidate, Footprint, Yaw, Style, Height);
			OccupiedExtent = SolCityGeneration::RotatedExtent2D(Footprint, Yaw);
		}

		FBuildingFootprint& NewFootprint = OccupiedBuildings.AddDefaulted_GetRef();
		NewFootprint.Center = Candidate;
		NewFootprint.Extent = OccupiedExtent;
		++Accepted;
	}
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
	const float MaximumHeightToWidthRatio = Style == 2 ? 3.20f : 2.60f;
	const int32 AspectRatioFloorCap = FMath::Max(1, FMath::FloorToInt(
		(Footprint.GetMin() * MaximumHeightToWidthRatio - BaseModuleHeight - CrownModuleHeight) / MidModuleHeight));
	const int32 StyleFloorCap = Style == 2 ? 8 : (Style == 4 ? 4 : 7);
	const int32 MidFloorCount = FMath::Clamp(RequestedMidFloorCount, 1, FMath::Min(AspectRatioFloorCap, StyleFloorCap));
	float HighestRoofZ = BaseModuleHeight + MidFloorCount * MidModuleHeight;

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
		break;
	}
	case 1: // L-shaped apartment mass
	{
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
		break;
	}
	case 2: // Slender tower with stepped crown
	{
		AddStack(BuildingBaseModuleInstances, BuildingBaseModuleMesh, FVector2D::ZeroVector, Footprint, 0.0f, 1);
		const FVector2D TowerSize = Footprint * 0.82f;
		AddStack(BuildingMiddleModuleInstances, BuildingMiddleModuleMesh, FVector2D::ZeroVector, TowerSize, BaseModuleHeight, MidFloorCount);
		AddStack(BuildingCrownModuleInstances, BuildingCrownModuleMesh, FVector2D::ZeroVector, TowerSize * 0.72f, HighestRoofZ, 1);
		HighestRoofZ += CrownModuleHeight;
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
		break;
	}
	default: // Small anime-neighborhood row houses
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
		break;
	}
	}

	// Rooftop utilities add scale cues at close zoom.
	if (HighestRoofZ > SolCityGeneration::StandardLaneWidth * 2.5f)
	{
		const FVector UtilitySize(
			SolCityGeneration::StandardLaneWidth * 0.28f,
			SolCityGeneration::StandardLaneWidth * 0.38f,
			SolCityGeneration::StandardLaneWidth * 0.36f);
		AddBox(DetailInstances, FVector(Center.X, Center.Y, HighestRoofZ + UtilitySize.Z * 0.5f), UtilitySize, YawDegrees);
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
	const float DeckWidth = 1100.0f;
	const FVector BridgeStart(0.0f, RiverY - BridgeLength * 0.5f, SolCityGeneration::BridgeDeckZ);
	const FVector BridgeEnd(0.0f, RiverY + BridgeLength * 0.5f, SolCityGeneration::BridgeDeckZ);

	// The authored bridge owns the visual roadway. Navigation remains a normal
	// directed road edge and supplies two pedestrian offsets along its walks.
	AddRoadSegment(BridgeStart, BridgeEnd, 720.0f, ESolCityRoadClass::Bridge, true, true, false);
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
		AddJunctionCap(Center, 720.0f);
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
}
