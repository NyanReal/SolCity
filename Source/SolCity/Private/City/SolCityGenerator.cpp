#include "City/SolCityGenerator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace SolCityGeneration
{
	constexpr float CubeSize = 100.0f;
	constexpr float RoadSurfaceZ = 7.0f;
	constexpr float SidewalkSurfaceZ = 16.0f;
	constexpr float GroundThickness = 60.0f;
	constexpr float RiverDepth = 130.0f;
	constexpr float BridgeDeckZ = 105.0f;

	FVector SafeNormal2D(const FVector& Value)
	{
		const FVector Flat(Value.X, Value.Y, 0.0f);
		return Flat.GetSafeNormal();
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
	GroundInstances = nullptr;
	RoadInstances = nullptr;
	SidewalkInstances = nullptr;
	BuildingInstances = nullptr;
	AccentBuildingInstances = nullptr;
	AuthoredBuildingInstances = nullptr;
	AuthoredCornerRetailInstances = nullptr;
	AuthoredSteppedTowerInstances = nullptr;
	RoofInstances = nullptr;
	WaterInstances = nullptr;
	BridgeInstances = nullptr;
	AuthoredBridgeInstances = nullptr;
	JunctionInstances = nullptr;
	TreeInstances = nullptr;
	DetailInstances = nullptr;
	CylinderInstances = nullptr;
	WaterMID = nullptr;
	bHasGenerated = false;
}

void ASolCityGenerator::CreateInstanceGroups()
{
	GroundInstances = CreateInstanceGroup(TEXT("GeneratedGround"), CubeMesh, GroundMaterial, FLinearColor(0.40f, 0.72f, 0.40f), true);
	RoadInstances = CreateInstanceGroup(TEXT("GeneratedRoads"), CubeMesh, RoadMaterial, FLinearColor(0.12f, 0.16f, 0.20f), true);
	SidewalkInstances = CreateInstanceGroup(TEXT("GeneratedSidewalks"), CubeMesh, SidewalkMaterial, FLinearColor(0.72f, 0.69f, 0.62f), true);
	BuildingInstances = CreateInstanceGroup(TEXT("GeneratedBuildings"), CubeMesh, BuildingMaterial, FLinearColor(0.93f, 0.69f, 0.47f), true);
	AccentBuildingInstances = CreateInstanceGroup(TEXT("GeneratedAccentBuildings"), CubeMesh, BuildingMaterial, FLinearColor(0.48f, 0.72f, 0.86f), true);
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
	RoofInstances = CreateInstanceGroup(TEXT("GeneratedRoofs"), CubeMesh, RoofMaterial, FLinearColor(0.80f, 0.25f, 0.22f), true);
	WaterInstances = CreateInstanceGroup(TEXT("GeneratedWater"), CubeMesh, WaterMaterial, FLinearColor(0.16f, 0.66f, 0.91f), false);
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
	if (WaterBase && WaterInstances)
	{
		WaterMID = UMaterialInstanceDynamic::Create(WaterBase, this);
		WaterMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.66f, 0.91f));
		WaterMID->SetVectorParameterValue(TEXT("BaseColorTint"), FLinearColor(0.16f, 0.66f, 0.91f));
		WaterMID->SetVectorParameterValue(TEXT("WaterTint"), FLinearColor(0.10f, 0.60f, 0.88f, 0.82f));
		WaterInstances->SetMaterial(0, WaterMID);
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
	const int32 SliceCount = FMath::Max(8, FMath::CeilToInt(CityDiameter / GroundTileSize));
	const float SliceWidth = CityDiameter / static_cast<float>(SliceCount);

	for (int32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
	{
		const float X = -HalfCity + (SliceIndex + 0.5f) * SliceWidth;
		const float CenterY = RiverCenterY(X);
		const float BankHalfWidth = RiverWidth * 0.5f;

		// Every ground instance is one image tile. The small seam is intentional:
		// it keeps a hand-built board-game quality at distant camera zoom levels.
		for (int32 RowIndex = 0; RowIndex < SliceCount; ++RowIndex)
		{
			const float Y = -HalfCity + (RowIndex + 0.5f) * SliceWidth;
			const FVector2D TileCenter(X, Y);
			if (!IsInsideRiver(TileCenter, SliceWidth * 0.58f))
			{
				AddBox(GroundInstances, FVector(X, Y, -SolCityGeneration::GroundThickness * 0.5f), FVector(SliceWidth - 8.0f, SliceWidth - 8.0f, SolCityGeneration::GroundThickness));
			}
		}

		const float NextX = -HalfCity + (SliceIndex + 1.5f) * SliceWidth;
		const float NextY = RiverCenterY(NextX);
		const FVector Start(X - SliceWidth * 0.55f, CenterY, RiverSurfaceZ - SolCityGeneration::RiverDepth * 0.5f);
		const FVector End(X + SliceWidth * 0.55f, NextY, RiverSurfaceZ - SolCityGeneration::RiverDepth * 0.5f);
		const FVector Delta = End - Start;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		AddBox(WaterInstances, (Start + End) * 0.5f, FVector(Delta.Size2D() + 35.0f, RiverWidth * 0.96f, SolCityGeneration::RiverDepth), Yaw);

		// Stone quay lips make the bank readable from a high city-builder camera.
		AddBox(BridgeInstances, FVector(X, CenterY - BankHalfWidth, -36.0f), FVector(SliceWidth + 10.0f, 42.0f, 72.0f));
		AddBox(BridgeInstances, FVector(X, CenterY + BankHalfWidth, -36.0f), FVector(SliceWidth + 10.0f, 42.0f, 72.0f));
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
	const float HalfCity = CityDiameter * 0.5f - 320.0f;
	if (FMath::Abs(Center.X) + Extent.X > HalfCity || FMath::Abs(Center.Y) + Extent.Y > HalfCity || IsInsideRiver(Center, Extent.GetMax() + 160.0f))
	{
		return false;
	}
	for (const FBuildingFootprint& Existing : OccupiedBuildings)
	{
		const FVector2D Delta = (Center - Existing.Center).GetAbs();
		if (Delta.X < Extent.X + Existing.Extent.X + 120.0f && Delta.Y < Extent.Y + Existing.Extent.Y + 120.0f)
		{
			return false;
		}
	}
	return true;
}

void ASolCityGenerator::GenerateBuildings()
{
	const float HalfCity = CityDiameter * 0.5f;
	int32 Accepted = 0;
	const int32 MaxAttempts = TargetBuildingCount * 55;

	for (int32 Attempt = 0; Attempt < MaxAttempts && Accepted < TargetBuildingCount; ++Attempt)
	{
		// Low-discrepancy polar sampling avoids rows while still filling districts.
		const float GoldenAngle = 137.50776f;
		const float Radius = FMath::Sqrt((Attempt + Random.FRand()) / static_cast<float>(MaxAttempts)) * HalfCity * 0.93f;
		const float Angle = FMath::DegreesToRadians(Attempt * GoldenAngle + Random.FRandRange(-12.0f, 12.0f));
		const FVector2D Candidate(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);

		if (!IsNearRoad(Candidate, 980.0f) || IsNearRoad(Candidate, 140.0f))
		{
			continue;
		}

		const int32 Style = Random.RandRange(0, 4);
		const FVector2D Footprint(Random.FRandRange(300.0f, 620.0f), Random.FRandRange(260.0f, 560.0f));
		const FVector2D Extent = Footprint * 0.5f;
		if (!IsBuildingSiteFree(Candidate, Extent))
		{
			continue;
		}

		float Height = Random.FRandRange(380.0f, 1200.0f);
		const float CenterBias = 1.0f - FMath::Clamp(Candidate.Size() / HalfCity, 0.0f, 1.0f);
		Height += FMath::Square(CenterBias) * Random.FRandRange(900.0f, 2400.0f);
		const float Yaw = Random.FRandRange(-38.0f, 38.0f);
		if (AuthoredSteppedTowerInstances && Style == 2 && Height > 1550.0f)
		{
			AddAuthoredBuilding(AuthoredSteppedTowerInstances, AuthoredSteppedTowerMesh, Candidate, Footprint, Yaw, Height);
		}
		else if (AuthoredBuildingInstances && Style == 0 && Height > 850.0f)
		{
			AddAuthoredBuilding(AuthoredBuildingInstances, AuthoredMidRiseMesh, Candidate, Footprint, Yaw, Height);
		}
		else if (AuthoredCornerRetailInstances && Style == 4 && Height < 1450.0f)
		{
			AddAuthoredBuilding(AuthoredCornerRetailInstances, AuthoredCornerRetailMesh, Candidate, Footprint, Yaw, FMath::Clamp(Height, 520.0f, 1100.0f));
		}
		else
		{
			AddBuildingMass(Candidate, Footprint, Yaw, Style, Height);
		}

		FBuildingFootprint& NewFootprint = OccupiedBuildings.AddDefaulted_GetRef();
		NewFootprint.Center = Candidate;
		NewFootprint.Extent = Extent;
		++Accepted;
	}
}

void ASolCityGenerator::AddAuthoredBuilding(
	UHierarchicalInstancedStaticMeshComponent* Group,
	UStaticMesh* Mesh,
	const FVector2D& Center,
	const FVector2D& Footprint,
	float YawDegrees,
	float Height)
{
	if (!Group || !Mesh)
	{
		AddBuildingMass(Center, Footprint, YawDegrees, 0, Height);
		return;
	}

	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector MeshSize = Bounds.GetSize();
	if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER)
	{
		AddBuildingMass(Center, Footprint, YawDegrees, 0, Height);
		return;
	}

	const FVector Scale(Footprint.X / MeshSize.X, Footprint.Y / MeshSize.Y, Height / MeshSize.Z);
	const FRotator Rotation(0.0f, YawDegrees, 0.0f);
	const FVector DesiredCenter(Center.X, Center.Y, Height * 0.5f);
	const FVector Translation = DesiredCenter - Rotation.RotateVector(Bounds.GetCenter() * Scale);
	Group->AddInstance(FTransform(Rotation, Translation, Scale));
}

void ASolCityGenerator::AddBuildingMass(const FVector2D& Center, const FVector2D& Footprint, float YawDegrees, int32 Style, float Height)
{
	UHierarchicalInstancedStaticMeshComponent* WallGroup = (Style % 2 == 0) ? BuildingInstances : AccentBuildingInstances;
	const FVector BaseCenter(Center.X, Center.Y, Height * 0.5f);

	switch (Style)
	{
	case 0: // Setback mid-rise
	{
		const float PodiumHeight = Height * 0.32f;
		AddBox(WallGroup, FVector(Center.X, Center.Y, PodiumHeight * 0.5f), FVector(Footprint.X, Footprint.Y, PodiumHeight), YawDegrees);
		const FVector2D TowerSize = Footprint * 0.72f;
		AddBox(WallGroup, FVector(Center.X, Center.Y, PodiumHeight + (Height - PodiumHeight) * 0.5f), FVector(TowerSize.X, TowerSize.Y, Height - PodiumHeight), YawDegrees);
		AddBox(RoofInstances, FVector(Center.X, Center.Y, Height + 34.0f), FVector(TowerSize.X + 28.0f, TowerSize.Y + 28.0f, 68.0f), YawDegrees);
		break;
	}
	case 1: // L-shaped apartment mass
	{
		AddBox(WallGroup, BaseCenter, FVector(Footprint.X, Footprint.Y * 0.46f, Height), YawDegrees);
		const FVector LocalOffset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(-Footprint.X * 0.29f, Footprint.Y * 0.25f, 0.0f));
		AddBox(WallGroup, BaseCenter + LocalOffset, FVector(Footprint.X * 0.42f, Footprint.Y * 0.52f, Height * 0.76f), YawDegrees);
		AddBox(RoofInstances, FVector(Center.X, Center.Y, Height + 26.0f), FVector(Footprint.X * 0.88f, Footprint.Y * 0.42f, 52.0f), YawDegrees);
		break;
	}
	case 2: // Slender tower with stepped crown
	{
		AddBox(WallGroup, FVector(Center.X, Center.Y, Height * 0.43f), FVector(Footprint.X, Footprint.Y, Height * 0.86f), YawDegrees);
		AddBox(WallGroup, FVector(Center.X, Center.Y, Height * 0.93f), FVector(Footprint.X * 0.72f, Footprint.Y * 0.72f, Height * 0.14f), YawDegrees + 8.0f);
		AddBox(RoofInstances, FVector(Center.X, Center.Y, Height + 38.0f), FVector(Footprint.X * 0.55f, Footprint.Y * 0.55f, 76.0f), YawDegrees + 8.0f);
		break;
	}
	case 3: // Courtyard cluster
	{
		const float WingWidth = FMath::Min(Footprint.X, Footprint.Y) * 0.24f;
		AddBox(WallGroup, BaseCenter, FVector(Footprint.X, WingWidth, Height), YawDegrees);
		const FVector SideOffset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(0.0f, (Footprint.Y - WingWidth) * 0.5f, 0.0f));
		AddBox(WallGroup, BaseCenter + SideOffset, FVector(WingWidth, Footprint.Y - WingWidth, Height * 0.78f), YawDegrees);
		AddBox(WallGroup, BaseCenter - SideOffset, FVector(WingWidth, Footprint.Y - WingWidth, Height * 0.78f), YawDegrees);
		AddBox(RoofInstances, FVector(Center.X, Center.Y, Height + 24.0f), FVector(Footprint.X, WingWidth + 24.0f, 48.0f), YawDegrees);
		break;
	}
	default: // Small anime-neighborhood row houses
	{
		const int32 HouseCount = 2 + Random.RandRange(0, 2);
		for (int32 HouseIndex = 0; HouseIndex < HouseCount; ++HouseIndex)
		{
			const float Alpha = (HouseIndex + 0.5f) / HouseCount - 0.5f;
			const FVector Offset = FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(Alpha * Footprint.X, 0.0f, 0.0f));
			const float HouseHeight = Height * Random.FRandRange(0.70f, 1.0f);
			AddBox(WallGroup, FVector(Center.X, Center.Y, HouseHeight * 0.5f) + Offset, FVector(Footprint.X / HouseCount - 24.0f, Footprint.Y, HouseHeight), YawDegrees);
			AddBox(RoofInstances, FVector(Center.X, Center.Y, HouseHeight + 32.0f) + Offset, FVector(Footprint.X / HouseCount, Footprint.Y + 30.0f, 64.0f), YawDegrees);
		}
		break;
	}
	}

	// Rooftop utilities add scale cues at close zoom.
	if (Height > 900.0f)
	{
		AddBox(DetailInstances, FVector(Center.X, Center.Y, Height + 105.0f), FVector(90.0f, 130.0f, 140.0f), YawDegrees);
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
