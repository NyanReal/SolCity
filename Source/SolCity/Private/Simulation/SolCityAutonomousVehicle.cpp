#include "Simulation/SolCityAutonomousVehicle.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Simulation/SolCityRoadNetwork.h"
#include "UObject/ConstructorHelpers.h"

ASolCityAutonomousVehicle::ASolCityAutonomousVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	VehicleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VehicleRoot"));
	SetRootComponent(VehicleRoot);

	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	VehicleMesh->SetupAttachment(VehicleRoot);
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VehicleMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	VehicleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	VehicleMesh->SetRelativeScale3D(FVector(1.15f, 0.55f, 0.28f));

	CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
	CabinMesh->SetupAttachment(VehicleMesh);
	CabinMesh->SetRelativeLocation(FVector(-10.0f, 0.0f, 120.0f));
	CabinMesh->SetRelativeScale3D(FVector(0.52f, 0.88f, 0.75f));
	CabinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredVehicleAsset(TEXT("/Game/Art/Props/SM_SolCity_AutonomousCar_01.SM_SolCity_AutonomousCar_01"));
	if (AuthoredVehicleAsset.Succeeded())
	{
		VehicleMesh->SetStaticMesh(AuthoredVehicleAsset.Object);
		VehicleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -15.0f));
		// After FBX axis conversion the imported visual needs a -90 yaw so its
		// nose matches actor-local +X without rotating navigation or sensing.
		VehicleMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		VehicleMesh->SetRelativeScale3D(FVector(1.0f));
		CabinMesh->SetVisibility(false);
		CabinMesh->SetHiddenInGame(true);
	}
	else if (CubeAsset.Succeeded())
	{
		VehicleMesh->SetStaticMesh(CubeAsset.Object);
		CabinMesh->SetStaticMesh(CubeAsset.Object);
	}
	Tags.Add(TEXT("SolCityVehicle"));
}

void ASolCityAutonomousVehicle::BeginPlay()
{
	Super::BeginPlay();
	RandomStream.Initialize(GetUniqueID());
	if (RoadNetwork)
	{
		InitializeOnRoad(RoadNetwork, CurrentNodeIndex);
	}
}

bool ASolCityAutonomousVehicle::InitializeOnRoad(ASolCityRoadNetwork* InRoadNetwork, int32 StartNodeIndex)
{
	RoadNetwork = InRoadNetwork;
	if (!RoadNetwork)
	{
		CurrentNodeIndex = INDEX_NONE;
		NextNodeIndex = INDEX_NONE;
		return false;
	}

	if (!RoadNetwork->IsValidNode(StartNodeIndex))
	{
		StartNodeIndex = RoadNetwork->FindNearestNode(GetActorLocation());
	}
	if (!RoadNetwork->IsValidNode(StartNodeIndex))
	{
		return false;
	}

	CurrentNodeIndex = StartNodeIndex;
	SetActorLocation(RoadNetwork->GetNodeWorldPosition(CurrentNodeIndex));
	return SelectNextNode();
}

bool ASolCityAutonomousVehicle::SelectNextNode()
{
	NextNodeIndex = RoadNetwork ? RoadNetwork->ChooseNextNode(CurrentNodeIndex, RandomStream) : INDEX_NONE;
	return NextNodeIndex != INDEX_NONE;
}

void ASolCityAutonomousVehicle::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!RoadNetwork || !RoadNetwork->IsValidNode(NextNodeIndex))
	{
		CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, 0.0f, DeltaSeconds, BrakingDeceleration);
		return;
	}

	const FVector TargetPosition = RoadNetwork->GetNodeWorldPosition(NextNodeIndex);
	const FVector ToTarget = TargetPosition - GetActorLocation();
	const float DistanceToTarget = ToTarget.Size2D();
	if (DistanceToTarget <= WaypointAcceptanceRadius)
	{
		SetActorLocation(TargetPosition);
		CurrentNodeIndex = NextNodeIndex;
		if (!SelectNextNode())
		{
			CurrentSpeed = 0.0f;
		}
		return;
	}

	bWaitingForTraffic = HasVehicleAhead() || MustStopAtSignal(DistanceToTarget);
	const FSolCityRoadWaypoint* CurrentNode = RoadNetwork->GetNode(CurrentNodeIndex);
	const float LaneSpeedLimit = CurrentNode ? CurrentNode->SpeedLimit : MaxSpeed;
	const float DesiredSpeed = bWaitingForTraffic ? 0.0f : FMath::Min(MaxSpeed, LaneSpeedLimit);
	const float SpeedChangeRate = DesiredSpeed < CurrentSpeed ? BrakingDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, DesiredSpeed, DeltaSeconds, SpeedChangeRate);

	if (CurrentSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal2D();
	const float TravelDistance = FMath::Min(CurrentSpeed * DeltaSeconds, DistanceToTarget);
	SetActorLocation(GetActorLocation() + Direction * TravelDistance);
	const FRotator DesiredRotation = Direction.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, RotationResponsiveness));
}

bool ASolCityAutonomousVehicle::MustStopAtSignal(const float DistanceToTarget) const
{
	return DistanceToTarget <= StopLineDistance && RoadNetwork && !RoadNetwork->IsNodeOpenForTraffic(NextNodeIndex);
}

bool ASolCityAutonomousVehicle::HasVehicleAhead() const
{
	if (!GetWorld())
	{
		return false;
	}
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	for (TActorIterator<ASolCityAutonomousVehicle> It(GetWorld()); It; ++It)
	{
		const ASolCityAutonomousVehicle* Other = *It;
		if (Other == this || Other->RoadNetwork != RoadNetwork)
		{
			continue;
		}
		const FVector Offset = Other->GetActorLocation() - GetActorLocation();
		const float ForwardDistance = FVector::DotProduct(Offset, Forward);
		if (ForwardDistance <= 0.0f || ForwardDistance >= FollowingDistance)
		{
			continue;
		}
		const FVector LateralOffset = Offset - Forward * ForwardDistance;
		if (LateralOffset.SizeSquared2D() < FMath::Square(140.0f))
		{
			return true;
		}
	}
	return false;
}
