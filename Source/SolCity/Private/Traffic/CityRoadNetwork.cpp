#include "Traffic/CityRoadNetwork.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Traffic/CityTrafficLight.h"
#include "Traffic/CityVehicle.h"

ACityRoadNetwork::ACityRoadNetwork()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrafficLightClass = ACityTrafficLight::StaticClass();
	VehicleClass = ACityVehicle::StaticClass();
}

void ACityRoadNetwork::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeOnBeginPlay && !bInitialized)
	{
		Initialize(GridHalfExtent, RoadSpacing, CellSize, RiverColumn);
	}
}

void ACityRoadNetwork::Initialize(const int32 InGridHalfExtent, const float InRoadSpacing,
	const float InCellSize, const int32 InRiverColumn)
{
	ClearSpawnedTraffic();

	GridHalfExtent = FMath::Clamp(InGridHalfExtent, 1, 50);
	RoadSpacing = FMath::Max(InRoadSpacing, 500.0f);
	CellSize = FMath::Max(InCellSize, 100.0f);
	RiverColumn = FMath::Clamp(InRiverColumn, -GridHalfExtent, GridHalfExtent);

	// Keep enough room for two compact cars while staying well inside a block.
	RoadHalfWidth = FMath::Min(FMath::Max(CellSize * 0.55f, 140.0f), RoadSpacing * 0.28f);
	LaneOffset = RoadHalfWidth * 0.43f;
	StopLineOffset = RoadHalfWidth * 0.86f;

	RebuildRoadGraph();
	bInitialized = true;

	if (bSpawnSignalsOnInitialize)
	{
		SpawnTrafficLights();
	}

	if (InitialVehicleCount > 0)
	{
		SpawnVehicles(InitialVehicleCount);
	}
}

void ACityRoadNetwork::RebuildRoadGraph()
{
	RoadNodes.Reset();

	for (int32 X = -GridHalfExtent; X <= GridHalfExtent; ++X)
	{
		for (int32 Y = -GridHalfExtent; Y <= GridHalfExtent; ++Y)
		{
			FCityRoadNode Node;
			Node.GridCoordinate = FIntPoint(X, Y);
			Node.WorldLocation = GridToWorldIntersection(Node.GridCoordinate);

			for (const ECityRoadDirection Direction : {
				ECityRoadDirection::North, ECityRoadDirection::East,
				ECityRoadDirection::South, ECityRoadDirection::West })
			{
				const FIntPoint Neighbor = Node.GridCoordinate + DirectionToGridDelta(Direction);
				if (IsValidIntersection(Neighbor))
				{
					Node.Connections.Add(Neighbor);
				}
			}

			// The river column deliberately does not remove any connection. Those
			// links are the logical bridge spans used by vehicles and world visuals.
			RoadNodes.Add(Node.GridCoordinate, MoveTemp(Node));
		}
	}
}

bool ACityRoadNetwork::IsValidIntersection(const FIntPoint GridCoordinate) const
{
	return FMath::Abs(GridCoordinate.X) <= GridHalfExtent
		&& FMath::Abs(GridCoordinate.Y) <= GridHalfExtent;
}

bool ACityRoadNetwork::GetRoadNode(const FIntPoint GridCoordinate, FCityRoadNode& OutNode) const
{
	if (const FCityRoadNode* Node = FindRoadNode(GridCoordinate))
	{
		OutNode = *Node;
		return true;
	}
	return false;
}

const FCityRoadNode* ACityRoadNetwork::FindRoadNode(const FIntPoint GridCoordinate) const
{
	return RoadNodes.Find(GridCoordinate);
}

FVector ACityRoadNetwork::GridToWorldIntersection(const FIntPoint GridCoordinate) const
{
	const FVector LocalLocation(
		static_cast<float>(GridCoordinate.X) * RoadSpacing,
		static_cast<float>(GridCoordinate.Y) * RoadSpacing,
		RoadSurfaceZ);
	return GetActorTransform().TransformPosition(LocalLocation);
}

FIntPoint ACityRoadNetwork::WorldToNearestIntersection(const FVector WorldLocation,
	const bool bClampToNetwork) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	int32 X = FMath::RoundToInt(LocalLocation.X / RoadSpacing);
	int32 Y = FMath::RoundToInt(LocalLocation.Y / RoadSpacing);

	if (bClampToNetwork)
	{
		X = FMath::Clamp(X, -GridHalfExtent, GridHalfExtent);
		Y = FMath::Clamp(Y, -GridHalfExtent, GridHalfExtent);
	}
	return FIntPoint(X, Y);
}

bool ACityRoadNetwork::IsRoadLocation(const FVector WorldLocation, const float ExtraTolerance) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float Tolerance = FMath::Max(0.0f, RoadHalfWidth + ExtraTolerance);
	const float NetworkLimit = static_cast<float>(GridHalfExtent) * RoadSpacing + Tolerance;

	if (FMath::Abs(LocalLocation.X) > NetworkLimit || FMath::Abs(LocalLocation.Y) > NetworkLimit)
	{
		return false;
	}

	const float NearestRoadX = FMath::RoundToFloat(LocalLocation.X / RoadSpacing) * RoadSpacing;
	const float NearestRoadY = FMath::RoundToFloat(LocalLocation.Y / RoadSpacing) * RoadSpacing;
	return FMath::Abs(LocalLocation.X - NearestRoadX) <= Tolerance
		|| FMath::Abs(LocalLocation.Y - NearestRoadY) <= Tolerance;
}

bool ACityRoadNetwork::IsRiverBridgeIntersection(const FIntPoint GridCoordinate) const
{
	return IsValidIntersection(GridCoordinate) && GridCoordinate.X == RiverColumn;
}

FVector ACityRoadNetwork::GetLaneWaypoint(const FIntPoint Intersection,
	const ECityRoadDirection TravelDirection) const
{
	FVector LocalLocation(
		static_cast<float>(Intersection.X) * RoadSpacing,
		static_cast<float>(Intersection.Y) * RoadSpacing,
		RoadSurfaceZ);

	switch (TravelDirection)
	{
	case ECityRoadDirection::North:
		LocalLocation.X += LaneOffset;
		break;
	case ECityRoadDirection::East:
		LocalLocation.Y -= LaneOffset;
		break;
	case ECityRoadDirection::South:
		LocalLocation.X -= LaneOffset;
		break;
	case ECityRoadDirection::West:
		LocalLocation.Y += LaneOffset;
		break;
	default:
		break;
	}

	return GetActorTransform().TransformPosition(LocalLocation);
}

FVector ACityRoadNetwork::GetStopLineLocation(const FIntPoint Intersection,
	const ECityRoadDirection IncomingDirection) const
{
	return GetLaneWaypoint(Intersection, IncomingDirection)
		- GetWorldDirection(IncomingDirection) * StopLineOffset;
}

FVector ACityRoadNetwork::GetLaneExitLocation(const FIntPoint Intersection,
	const ECityRoadDirection OutgoingDirection) const
{
	return GetLaneWaypoint(Intersection, OutgoingDirection)
		+ GetWorldDirection(OutgoingDirection) * StopLineOffset;
}

FVector ACityRoadNetwork::GetWorldDirection(const ECityRoadDirection Direction) const
{
	FVector LocalDirection = FVector::ForwardVector;
	switch (Direction)
	{
	case ECityRoadDirection::North:
		LocalDirection = FVector(0.0f, 1.0f, 0.0f);
		break;
	case ECityRoadDirection::East:
		LocalDirection = FVector(1.0f, 0.0f, 0.0f);
		break;
	case ECityRoadDirection::South:
		LocalDirection = FVector(0.0f, -1.0f, 0.0f);
		break;
	case ECityRoadDirection::West:
		LocalDirection = FVector(-1.0f, 0.0f, 0.0f);
		break;
	default:
		break;
	}
	return GetActorTransform().TransformVectorNoScale(LocalDirection).GetSafeNormal();
}

void ACityRoadNetwork::GetOutgoingDirections(const FIntPoint Intersection,
	const ECityRoadDirection IncomingDirection, TArray<ECityRoadDirection>& OutDirections) const
{
	OutDirections.Reset();
	const ECityRoadDirection Reverse = OppositeDirection(IncomingDirection);

	for (const ECityRoadDirection Candidate : {
		ECityRoadDirection::North, ECityRoadDirection::East,
		ECityRoadDirection::South, ECityRoadDirection::West })
	{
		if (Candidate != Reverse
			&& IsValidIntersection(Intersection + DirectionToGridDelta(Candidate)))
		{
			OutDirections.Add(Candidate);
		}
	}
}

void ACityRoadNetwork::BuildIntersectionPath(const FIntPoint Intersection,
	const ECityRoadDirection IncomingDirection, const ECityRoadDirection OutgoingDirection,
	TArray<FVector>& OutPath, const int32 SampleCount) const
{
	OutPath.Reset();
	const int32 ClampedSamples = FMath::Clamp(SampleCount, 2, 16);
	const FVector Start = GetStopLineLocation(Intersection, IncomingDirection);
	const FVector End = GetLaneExitLocation(Intersection, OutgoingDirection);

	if (IncomingDirection == OutgoingDirection)
	{
		for (int32 Index = 1; Index <= ClampedSamples; ++Index)
		{
			OutPath.Add(FMath::Lerp(Start, End,
				static_cast<float>(Index) / static_cast<float>(ClampedSamples)));
		}
		return;
	}

	const FVector Control = GridToWorldIntersection(Intersection);
	for (int32 Index = 1; Index <= ClampedSamples; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(ClampedSamples);
		const float OneMinusAlpha = 1.0f - Alpha;
		OutPath.Add(OneMinusAlpha * OneMinusAlpha * Start
			+ 2.0f * OneMinusAlpha * Alpha * Control
			+ Alpha * Alpha * End);
	}
}

int32 ACityRoadNetwork::SpawnTrafficLights()
{
	if (!GetWorld())
	{
		return 0;
	}

	if (RoadNodes.IsEmpty())
	{
		RebuildRoadGraph();
	}

	TSubclassOf<ACityTrafficLight> ClassToSpawn = TrafficLightClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACityTrafficLight::StaticClass();
	}
	int32 SpawnedCount = 0;

	for (const TPair<FIntPoint, FCityRoadNode>& Pair : RoadNodes)
	{
		if ((!bSpawnSignalsAtBoundary && Pair.Value.Connections.Num() < 4)
			|| GetTrafficLightAt(Pair.Key))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACityTrafficLight* Light = GetWorld()->SpawnActor<ACityTrafficLight>(
			ClassToSpawn, GridToWorldIntersection(Pair.Key), GetActorRotation(), SpawnParameters);
		if (!Light)
		{
			continue;
		}

		const bool bStaggeredPhase = ((Pair.Key.X + Pair.Key.Y) & 1) != 0;
		Light->InitializeLight(this, Pair.Key, bStaggeredPhase ? 8.0f : 0.0f);
		SpawnedTrafficLights.Add(Light);
		LightByIntersection.Add(Pair.Key, Light);
		++SpawnedCount;
	}

	return SpawnedCount;
}

ACityTrafficLight* ACityRoadNetwork::GetTrafficLightAt(const FIntPoint Intersection) const
{
	if (const TWeakObjectPtr<ACityTrafficLight>* FoundLight = LightByIntersection.Find(Intersection))
	{
		return FoundLight->Get();
	}
	return nullptr;
}

bool ACityRoadNetwork::CanProceedThrough(const FIntPoint Intersection,
	const ECityRoadDirection IncomingDirection) const
{
	const ACityTrafficLight* Light = GetTrafficLightAt(Intersection);
	return !Light || Light->CanProceed(IncomingDirection);
}

int32 ACityRoadNetwork::SpawnVehicles(const int32 Count)
{
	if (!GetWorld() || Count <= 0)
	{
		return 0;
	}

	int32 SpawnedCount = 0;
	const int32 MaxAttempts = FMath::Max(Count * 12, 24);
	for (int32 Attempt = 0; Attempt < MaxAttempts && SpawnedCount < Count; ++Attempt)
	{
		const FIntPoint From(
			FMath::RandRange(-GridHalfExtent, GridHalfExtent),
			FMath::RandRange(-GridHalfExtent, GridHalfExtent));
		const ECityRoadDirection Direction = static_cast<ECityRoadDirection>(FMath::RandRange(0, 3));
		const FIntPoint To = From + DirectionToGridDelta(Direction);
		if (!IsValidIntersection(To))
		{
			continue;
		}

		const float SegmentAlpha = FMath::FRandRange(0.10f, 0.72f);
		const FVector CandidateLocation = FMath::Lerp(
			GetLaneExitLocation(From, Direction), GetStopLineLocation(To, Direction), SegmentAlpha);

		bool bLocationClear = true;
		for (const TWeakObjectPtr<ACityVehicle>& ExistingVehicle : RegisteredVehicles)
		{
			if (ExistingVehicle.IsValid()
				&& FVector::DistSquared2D(CandidateLocation, ExistingVehicle->GetActorLocation())
				< FMath::Square(520.0f))
			{
				bLocationClear = false;
				break;
			}
		}

		if (bLocationClear && SpawnVehicleAt(From, Direction, SegmentAlpha))
		{
			++SpawnedCount;
		}
	}

	return SpawnedCount;
}

ACityVehicle* ACityRoadNetwork::SpawnVehicleAt(const FIntPoint FromIntersection,
	const ECityRoadDirection Direction, const float SegmentAlpha)
{
	if (!GetWorld() || !IsValidIntersection(FromIntersection)
		|| !IsValidIntersection(FromIntersection + DirectionToGridDelta(Direction)))
	{
		return nullptr;
	}

	TSubclassOf<ACityVehicle> ClassToSpawn = VehicleClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACityVehicle::StaticClass();
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACityVehicle* Vehicle = GetWorld()->SpawnActor<ACityVehicle>(
		ClassToSpawn, GetLaneExitLocation(FromIntersection, Direction),
		GetWorldDirection(Direction).Rotation(), SpawnParameters);
	if (!Vehicle)
	{
		return nullptr;
	}

	if (!Vehicle->InitializeVehicle(this, FromIntersection, Direction, SegmentAlpha))
	{
		Vehicle->Destroy();
		return nullptr;
	}

	SpawnedVehicles.Add(Vehicle);
	return Vehicle;
}

void ACityRoadNetwork::RegisterVehicle(ACityVehicle* Vehicle)
{
	if (IsValid(Vehicle))
	{
		RegisteredVehicles.AddUnique(Vehicle);
	}
}

void ACityRoadNetwork::UnregisterVehicle(ACityVehicle* Vehicle)
{
	RegisteredVehicles.Remove(Vehicle);
	SpawnedVehicles.Remove(Vehicle);
}

float ACityRoadNetwork::GetDistanceToVehicleAhead(const ACityVehicle* Requestor,
	const FVector& Forward, const float MaxDistance, const float LaneTolerance) const
{
	if (!IsValid(Requestor))
	{
		return MaxDistance;
	}

	const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();
	if (FlatForward.IsNearlyZero())
	{
		return MaxDistance;
	}

	const FVector Right(-FlatForward.Y, FlatForward.X, 0.0f);
	float NearestDistance = MaxDistance;
	for (const TWeakObjectPtr<ACityVehicle>& WeakVehicle : RegisteredVehicles)
	{
		const ACityVehicle* Other = WeakVehicle.Get();
		if (!IsValid(Other) || Other == Requestor)
		{
			continue;
		}

		const FVector Delta = Other->GetActorLocation() - Requestor->GetActorLocation();
		const float ForwardDistance = FVector::DotProduct(Delta, FlatForward);
		const float LateralDistance = FMath::Abs(FVector::DotProduct(Delta, Right));
		if (ForwardDistance > 0.0f && ForwardDistance < NearestDistance
			&& LateralDistance <= LaneTolerance && FMath::Abs(Delta.Z) < 180.0f)
		{
			NearestDistance = ForwardDistance;
		}
	}

	return NearestDistance;
}

FIntPoint ACityRoadNetwork::DirectionToGridDelta(const ECityRoadDirection Direction)
{
	switch (Direction)
	{
	case ECityRoadDirection::North:
		return FIntPoint(0, 1);
	case ECityRoadDirection::East:
		return FIntPoint(1, 0);
	case ECityRoadDirection::South:
		return FIntPoint(0, -1);
	case ECityRoadDirection::West:
		return FIntPoint(-1, 0);
	default:
		return FIntPoint::ZeroValue;
	}
}

ECityRoadDirection ACityRoadNetwork::OppositeDirection(const ECityRoadDirection Direction)
{
	switch (Direction)
	{
	case ECityRoadDirection::North:
		return ECityRoadDirection::South;
	case ECityRoadDirection::East:
		return ECityRoadDirection::West;
	case ECityRoadDirection::South:
		return ECityRoadDirection::North;
	case ECityRoadDirection::West:
		return ECityRoadDirection::East;
	default:
		return ECityRoadDirection::South;
	}
}

void ACityRoadNetwork::ClearSpawnedTraffic()
{
	// Destroy() synchronously reaches ACityVehicle::EndPlay(), which unregisters
	// the vehicle. Move the owned arrays out first so those callbacks only touch
	// the now-empty member arrays instead of invalidating this iteration.
	TArray<TObjectPtr<ACityVehicle>> VehiclesToDestroy = MoveTemp(SpawnedVehicles);
	RegisteredVehicles.Reset();
	for (ACityVehicle* Vehicle : VehiclesToDestroy)
	{
		if (IsValid(Vehicle))
		{
			Vehicle->Destroy();
		}
	}

	TArray<TObjectPtr<ACityTrafficLight>> LightsToDestroy = MoveTemp(SpawnedTrafficLights);
	LightByIntersection.Reset();
	for (ACityTrafficLight* Light : LightsToDestroy)
	{
		if (IsValid(Light))
		{
			Light->Destroy();
		}
	}
}
