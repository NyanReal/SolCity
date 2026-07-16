#include "Traffic/CityVehicle.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Traffic/CityRoadNetwork.h"
#include "Traffic/CityTrafficLight.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void SetTintParameters(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
	{
		if (!Material)
		{
			return;
		}
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetVectorParameterValue(TEXT("Tint"), Color);
	}

	void SetMeshTintParameters(UStaticMeshComponent* Mesh, const FLinearColor& Color)
	{
		if (!Mesh)
		{
			return;
		}

		const FVector ColorVector(Color.R, Color.G, Color.B);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), ColorVector);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), ColorVector);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("Tint"), ColorVector);
	}
}

ACityVehicle::ACityVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(SceneRoot);
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(SceneRoot);
	Hood = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hood"));
	Hood->SetupAttachment(SceneRoot);
	Trunk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trunk"));
	Trunk->SetupAttachment(SceneRoot);
	Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
	Cabin->SetupAttachment(SceneRoot);
	Roof = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Roof"));
	Roof->SetupAttachment(SceneRoot);
	FrontBumper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontBumper"));
	FrontBumper->SetupAttachment(SceneRoot);
	RearBumper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearBumper"));
	RearBumper->SetupAttachment(SceneRoot);
	WheelFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFrontLeft"));
	WheelFrontLeft->SetupAttachment(SceneRoot);
	WheelFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFrontRight"));
	WheelFrontRight->SetupAttachment(SceneRoot);
	WheelRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelRearLeft"));
	WheelRearLeft->SetupAttachment(SceneRoot);
	WheelRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelRearRight"));
	WheelRearRight->SetupAttachment(SceneRoot);
	WheelCapFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelCapFrontLeft"));
	WheelCapFrontLeft->SetupAttachment(SceneRoot);
	WheelCapFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelCapFrontRight"));
	WheelCapFrontRight->SetupAttachment(SceneRoot);
	WheelCapRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelCapRearLeft"));
	WheelCapRearLeft->SetupAttachment(SceneRoot);
	WheelCapRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelCapRearRight"));
	WheelCapRearRight->SetupAttachment(SceneRoot);
	HeadlightLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadlightLeft"));
	HeadlightLeft->SetupAttachment(SceneRoot);
	HeadlightRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadlightRight"));
	HeadlightRight->SetupAttachment(SceneRoot);
	TailLightLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailLightLeft"));
	TailLightLeft->SetupAttachment(SceneRoot);
	TailLightRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailLightRight"));
	TailLightRight->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectMaterial(
		TEXT("/Game/Generated/Fallback/M_FallbackTintedLit.M_FallbackTintedLit"));
	UMaterialInterface* ShapeMaterial = ProjectMaterial.Succeeded()
		? ProjectMaterial.Object.Get()
		: (BasicMaterial.Succeeded() ? BasicMaterial.Object.Get() : nullptr);

	if (CubeMesh.Succeeded())
	{
		for (UStaticMeshComponent* CubeComponent : {
			Chassis.Get(), Body.Get(), Hood.Get(), Trunk.Get(), Cabin.Get(), Roof.Get(),
			FrontBumper.Get(), RearBumper.Get(), HeadlightLeft.Get(), HeadlightRight.Get(),
			TailLightLeft.Get(), TailLightRight.Get() })
		{
			CubeComponent->SetStaticMesh(CubeMesh.Object);
		}
	}
	if (CylinderMesh.Succeeded())
	{
		for (UStaticMeshComponent* CylinderComponent : {
			WheelFrontLeft.Get(), WheelFrontRight.Get(), WheelRearLeft.Get(), WheelRearRight.Get(),
			WheelCapFrontLeft.Get(), WheelCapFrontRight.Get(), WheelCapRearLeft.Get(), WheelCapRearRight.Get() })
		{
			CylinderComponent->SetStaticMesh(CylinderMesh.Object);
		}
	}

	for (UStaticMeshComponent* MeshComponent : {
		Chassis.Get(), Body.Get(), Hood.Get(), Trunk.Get(), Cabin.Get(), Roof.Get(),
		FrontBumper.Get(), RearBumper.Get(), WheelFrontLeft.Get(), WheelFrontRight.Get(),
		WheelRearLeft.Get(), WheelRearRight.Get(), WheelCapFrontLeft.Get(), WheelCapFrontRight.Get(),
		WheelCapRearLeft.Get(), WheelCapRearRight.Get(), HeadlightLeft.Get(), HeadlightRight.Get(),
		TailLightLeft.Get(), TailLightRight.Get() })
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
		MeshComponent->SetCastShadow(true);
		if (ShapeMaterial)
		{
			MeshComponent->SetMaterial(0, ShapeMaterial);
		}
	}

	// Engine cubes are 100uu wide, so these scales describe a conventional
	// 3.3 m x 1.5 m compact sedan rather than a narrow open-wheel racer.
	// The visual nose remains close to VehicleHalfLength so traffic spacing is
	// unchanged while the wheels tuck naturally beneath the body sides.
	Chassis->SetRelativeLocation(FVector(0.0f, 0.0f, 38.0f));
	Chassis->SetRelativeScale3D(FVector(3.30f, 1.38f, 0.18f));
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, 61.0f));
	Body->SetRelativeScale3D(FVector(3.16f, 1.48f, 0.40f));
	Hood->SetRelativeLocation(FVector(105.0f, 0.0f, 84.0f));
	Hood->SetRelativeScale3D(FVector(1.08f, 1.38f, 0.27f));
	Trunk->SetRelativeLocation(FVector(-119.0f, 0.0f, 84.0f));
	Trunk->SetRelativeScale3D(FVector(0.68f, 1.38f, 0.29f));
	Cabin->SetRelativeLocation(FVector(-15.0f, 0.0f, 108.0f));
	Cabin->SetRelativeScale3D(FVector(1.48f, 1.16f, 0.63f));
	Roof->SetRelativeLocation(FVector(-18.0f, 0.0f, 143.0f));
	Roof->SetRelativeScale3D(FVector(1.28f, 1.22f, 0.12f));
	FrontBumper->SetRelativeLocation(FVector(166.0f, 0.0f, 52.0f));
	FrontBumper->SetRelativeScale3D(FVector(0.14f, 1.48f, 0.17f));
	RearBumper->SetRelativeLocation(FVector(-166.0f, 0.0f, 52.0f));
	RearBumper->SetRelativeScale3D(FVector(0.14f, 1.48f, 0.17f));

	HeadlightLeft->SetRelativeLocation(FVector(174.0f, -46.0f, 73.0f));
	HeadlightRight->SetRelativeLocation(FVector(174.0f, 46.0f, 73.0f));
	HeadlightLeft->SetRelativeScale3D(FVector(0.035f, 0.28f, 0.12f));
	HeadlightRight->SetRelativeScale3D(FVector(0.035f, 0.28f, 0.12f));
	TailLightLeft->SetRelativeLocation(FVector(-174.0f, -48.0f, 72.0f));
	TailLightRight->SetRelativeLocation(FVector(-174.0f, 48.0f, 72.0f));
	TailLightLeft->SetRelativeScale3D(FVector(0.035f, 0.25f, 0.12f));
	TailLightRight->SetRelativeScale3D(FVector(0.035f, 0.25f, 0.12f));

	const FRotator WheelRotation(0.0f, 0.0f, 90.0f);
	const FVector WheelScale(0.52f, 0.52f, 0.18f);
	WheelFrontLeft->SetRelativeLocation(FVector(106.0f, -69.0f, 34.0f));
	WheelFrontRight->SetRelativeLocation(FVector(106.0f, 69.0f, 34.0f));
	WheelRearLeft->SetRelativeLocation(FVector(-106.0f, -69.0f, 34.0f));
	WheelRearRight->SetRelativeLocation(FVector(-106.0f, 69.0f, 34.0f));
	for (UStaticMeshComponent* Wheel : {
		WheelFrontLeft.Get(), WheelFrontRight.Get(), WheelRearLeft.Get(), WheelRearRight.Get() })
	{
		Wheel->SetRelativeRotation(WheelRotation);
		Wheel->SetRelativeScale3D(WheelScale);
	}

	const FVector WheelCapScale(0.31f, 0.31f, 0.045f);
	WheelCapFrontLeft->SetRelativeLocation(FVector(106.0f, -79.0f, 34.0f));
	WheelCapFrontRight->SetRelativeLocation(FVector(106.0f, 79.0f, 34.0f));
	WheelCapRearLeft->SetRelativeLocation(FVector(-106.0f, -79.0f, 34.0f));
	WheelCapRearRight->SetRelativeLocation(FVector(-106.0f, 79.0f, 34.0f));
	for (UStaticMeshComponent* WheelCap : {
		WheelCapFrontLeft.Get(), WheelCapFrontRight.Get(), WheelCapRearLeft.Get(), WheelCapRearRight.Get() })
	{
		WheelCap->SetRelativeRotation(WheelRotation);
		WheelCap->SetRelativeScale3D(WheelCapScale);
	}
}

void ACityVehicle::BeginPlay()
{
	Super::BeginPlay();
	EnsureDynamicMaterials();
	ApplyInitialPastelColor();
}

void ACityVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RoadNetwork)
	{
		RoadNetwork->UnregisterVehicle(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ACityVehicle::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bVehicleInitialized || !RoadNetwork || DeltaSeconds <= 0.0f)
	{
		return;
	}

	switch (DriveState)
	{
	case ECityVehicleDriveState::ApproachingIntersection:
		TickApproach(DeltaSeconds);
		break;
	case ECityVehicleDriveState::CrossingIntersection:
		TickCrossing(DeltaSeconds);
		break;
	default:
		break;
	}
}

bool ACityVehicle::InitializeVehicle(ACityRoadNetwork* InRoadNetwork,
	const FIntPoint FromIntersection, const ECityRoadDirection InitialDirection,
	const float SegmentAlpha)
{
	if (!InRoadNetwork)
	{
		return false;
	}

	const FIntPoint NextIntersection = FromIntersection
		+ ACityRoadNetwork::DirectionToGridDelta(InitialDirection);
	if (!InRoadNetwork->IsValidIntersection(FromIntersection)
		|| !InRoadNetwork->IsValidIntersection(NextIntersection))
	{
		return false;
	}

	if (RoadNetwork && RoadNetwork != InRoadNetwork)
	{
		RoadNetwork->UnregisterVehicle(this);
	}

	RoadNetwork = InRoadNetwork;
	PreviousIntersection = FromIntersection;
	TargetIntersection = NextIntersection;
	TravelDirection = InitialDirection;
	PendingDirection = InitialDirection;
	DriveState = ECityVehicleDriveState::ApproachingIntersection;
	CrossingPath.Reset();
	CrossingPathIndex = 0;

	const FVector Direction = RoadNetwork->GetWorldDirection(TravelDirection);
	const FVector SegmentStart = RoadNetwork->GetLaneExitLocation(PreviousIntersection, TravelDirection);
	const FVector SegmentEnd = RoadNetwork->GetStopLineLocation(TargetIntersection, TravelDirection)
		- Direction * VehicleHalfLength;
	SetActorLocation(FMath::Lerp(SegmentStart, SegmentEnd, FMath::Clamp(SegmentAlpha, 0.02f, 0.90f)));
	SetActorRotation(Direction.Rotation());
	CurrentSpeed = FMath::FRandRange(CruiseSpeed * 0.35f, CruiseSpeed * 0.72f);

	bVehicleInitialized = true;
	RoadNetwork->RegisterVehicle(this);
	return true;
}

void ACityVehicle::SetVehicleColor(const FLinearColor NewColor)
{
	EnsureDynamicMaterials();
	SetTintParameters(BodyMaterial, NewColor);
	for (UStaticMeshComponent* PaintedPart : {
		Hood.Get(), Trunk.Get(), Roof.Get() })
	{
		SetMeshTintParameters(PaintedPart, NewColor);
	}

	const FLinearColor GlassColor = FLinearColor::LerpUsingHSV(
		FLinearColor(0.08f, 0.27f, 0.38f, 1.0f), NewColor, 0.08f);
	SetTintParameters(CabinMaterial, GlassColor);

	const FLinearColor BumperColor = FLinearColor::LerpUsingHSV(
		NewColor, FLinearColor(0.18f, 0.24f, 0.27f, 1.0f), 0.22f);
	SetMeshTintParameters(FrontBumper, BumperColor);
	SetMeshTintParameters(RearBumper, BumperColor);
}

void ACityVehicle::TickApproach(const float DeltaSeconds)
{
	const FVector Direction = RoadNetwork->GetWorldDirection(TravelDirection);
	const FVector StopLine = RoadNetwork->GetStopLineLocation(TargetIntersection, TravelDirection);
	const FVector VehicleCenterStop = StopLine - Direction * VehicleHalfLength;
	const float DistanceToCenterStop = FVector::DotProduct(VehicleCenterStop - GetActorLocation(), Direction);

	float DesiredSpeed = CruiseSpeed;
	const ACityTrafficLight* Light = RoadNetwork->GetTrafficLightAt(TargetIntersection);
	const bool bSignalRequestsStop = Light
		&& Light->ShouldStop(TravelDirection, GetActorLocation(), VehicleHalfLength + 30.0f);
	if (bSignalRequestsStop)
	{
		const float BrakingRoom = FMath::Max(0.0f, DistanceToCenterStop);
		const float SignalSafeSpeed = FMath::Sqrt(2.0f * BrakeDeceleration * BrakingRoom);
		DesiredSpeed = FMath::Min(DesiredSpeed, SignalSafeSpeed);
		if (BrakingRoom <= 5.0f)
		{
			DesiredSpeed = 0.0f;
		}
	}

	DesiredSpeed = ApplyFollowingLimit(DesiredSpeed);
	const bool bAtStopPosition = AdvanceToward(VehicleCenterStop, DeltaSeconds, DesiredSpeed);
	if (bAtStopPosition)
	{
		if (!Light || Light->CanProceed(TravelDirection))
		{
			BeginCrossing();
		}
		else
		{
			CurrentSpeed = 0.0f;
		}
	}
}

void ACityVehicle::TickCrossing(const float DeltaSeconds)
{
	if (!CrossingPath.IsValidIndex(CrossingPathIndex))
	{
		PreviousIntersection = TargetIntersection;
		TravelDirection = PendingDirection;
		TargetIntersection = PreviousIntersection
			+ ACityRoadNetwork::DirectionToGridDelta(TravelDirection);
		DriveState = ECityVehicleDriveState::ApproachingIntersection;
		CrossingPath.Reset();
		CrossingPathIndex = 0;
		return;
	}

	const float BaseDesiredSpeed = PendingDirection == TravelDirection ? CruiseSpeed : TurnSpeed;
	const float DesiredSpeed = ApplyFollowingLimit(BaseDesiredSpeed);
	if (AdvanceToward(CrossingPath[CrossingPathIndex], DeltaSeconds, DesiredSpeed))
	{
		++CrossingPathIndex;
	}
}

bool ACityVehicle::BeginCrossing()
{
	TArray<ECityRoadDirection> Choices;
	RoadNetwork->GetOutgoingDirections(TargetIntersection, TravelDirection, Choices);
	if (Choices.IsEmpty())
	{
		return false;
	}

	PendingDirection = ChooseOutgoingDirection(Choices);
	RoadNetwork->BuildIntersectionPath(TargetIntersection, TravelDirection, PendingDirection,
		CrossingPath, 8);
	if (CrossingPath.IsEmpty())
	{
		return false;
	}

	CrossingPathIndex = 0;
	DriveState = ECityVehicleDriveState::CrossingIntersection;
	return true;
}

bool ACityVehicle::AdvanceToward(const FVector& Target, const float DeltaSeconds,
	const float DesiredSpeed)
{
	const FVector ToTarget = Target - GetActorLocation();
	const float Distance = FVector(ToTarget.X, ToTarget.Y, 0.0f).Size();
	if (Distance <= 2.0f)
	{
		SetActorLocation(Target);
		return true;
	}

	const float SafeDesiredSpeed = FMath::Max(0.0f, DesiredSpeed);
	const float SpeedChangeRate = SafeDesiredSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, SafeDesiredSpeed, DeltaSeconds, SpeedChangeRate);
	if (CurrentSpeed <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector MoveDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	const float MoveDistance = FMath::Min(CurrentSpeed * DeltaSeconds, Distance);
	SetActorLocation(GetActorLocation() + MoveDirection * MoveDistance);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), MoveDirection.Rotation(), DeltaSeconds, 8.0f));

	if (MoveDistance >= Distance - 1.0f)
	{
		SetActorLocation(Target);
		return true;
	}
	return false;
}

float ACityVehicle::ApplyFollowingLimit(const float DesiredSpeed) const
{
	if (!RoadNetwork)
	{
		return DesiredSpeed;
	}

	const float DistanceAhead = RoadNetwork->GetDistanceToVehicleAhead(
		this, GetActorForwardVector(), VehicleLookAheadDistance, LaneTolerance);
	if (DistanceAhead >= VehicleLookAheadDistance)
	{
		return DesiredSpeed;
	}

	const float BrakingRoom = DistanceAhead - MinimumFollowingDistance;
	if (BrakingRoom <= 0.0f)
	{
		return 0.0f;
	}

	const float FollowingSafeSpeed = FMath::Sqrt(2.0f * BrakeDeceleration * BrakingRoom);
	return FMath::Min(DesiredSpeed, FollowingSafeSpeed);
}

ECityRoadDirection ACityVehicle::ChooseOutgoingDirection(
	const TArray<ECityRoadDirection>& Choices) const
{
	if (Choices.Contains(TravelDirection) && FMath::FRand() < StraightChoiceWeight)
	{
		return TravelDirection;
	}

	TArray<ECityRoadDirection, TInlineAllocator<3>> TurnChoices;
	for (const ECityRoadDirection Choice : Choices)
	{
		if (Choice != TravelDirection)
		{
			TurnChoices.Add(Choice);
		}
	}

	if (!TurnChoices.IsEmpty())
	{
		return TurnChoices[FMath::RandRange(0, TurnChoices.Num() - 1)];
	}
	return Choices[FMath::RandRange(0, Choices.Num() - 1)];
}

void ACityVehicle::EnsureDynamicMaterials()
{
	if (!BodyMaterial && Body)
	{
		BodyMaterial = Body->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!CabinMaterial && Cabin)
	{
		CabinMaterial = Cabin->CreateAndSetMaterialInstanceDynamic(0);
	}

	SetMeshTintParameters(Chassis, FLinearColor(0.08f, 0.12f, 0.15f, 1.0f));

	const FLinearColor TireColor(0.055f, 0.075f, 0.09f, 1.0f);
	for (UStaticMeshComponent* Wheel : {
		WheelFrontLeft.Get(), WheelFrontRight.Get(), WheelRearLeft.Get(), WheelRearRight.Get() })
	{
		SetMeshTintParameters(Wheel, TireColor);
	}

	const FLinearColor WheelCapColor(0.72f, 0.79f, 0.83f, 1.0f);
	for (UStaticMeshComponent* WheelCap : {
		WheelCapFrontLeft.Get(), WheelCapFrontRight.Get(), WheelCapRearLeft.Get(), WheelCapRearRight.Get() })
	{
		SetMeshTintParameters(WheelCap, WheelCapColor);
	}

	SetMeshTintParameters(HeadlightLeft, FLinearColor(1.0f, 0.92f, 0.58f, 1.0f));
	SetMeshTintParameters(HeadlightRight, FLinearColor(1.0f, 0.92f, 0.58f, 1.0f));
	SetMeshTintParameters(TailLightLeft, FLinearColor(1.0f, 0.14f, 0.18f, 1.0f));
	SetMeshTintParameters(TailLightRight, FLinearColor(1.0f, 0.14f, 0.18f, 1.0f));
}

void ACityVehicle::ApplyInitialPastelColor()
{
	static const FLinearColor PastelPalette[] =
	{
		FLinearColor(1.00f, 0.45f, 0.48f, 1.0f), // sakura coral
		FLinearColor(0.30f, 0.72f, 1.00f, 1.0f), // clear sky blue
		FLinearColor(0.36f, 0.88f, 0.66f, 1.0f), // mint
		FLinearColor(1.00f, 0.78f, 0.30f, 1.0f), // warm yellow
		FLinearColor(0.72f, 0.54f, 1.00f, 1.0f), // lavender
		FLinearColor(1.00f, 0.62f, 0.82f, 1.0f)  // candy pink
	};

	SetVehicleColor(PastelPalette[GetUniqueID() % UE_ARRAY_COUNT(PastelPalette)]);
}
