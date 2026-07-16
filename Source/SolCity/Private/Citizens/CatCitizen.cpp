#include "Citizens/CatCitizen.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// City-builder characters need a mildly exaggerated silhouette at the
	// default overview zoom. The resulting cat is still less than half a car's
	// footprint, while its ears and tail remain readable from above.
	constexpr float CatVisualScale = 1.45f;

	// Probe the torso rather than the feet. The city's walkable slabs top out at
	// Z=22, while buildings, trunks and bridge pillars continue through this
	// raised sphere and therefore remain blocking obstacles.
	constexpr float CatObstacleProbeHeight = 62.0f;
	constexpr float CatObstacleRadius = 34.0f;
	const FName ShapeColorParameter(TEXT("Color"));

	void ConfigureCatMesh(
		UStaticMeshComponent* Component,
		UStaticMesh* Mesh,
		USceneComponent* Parent,
		const FVector& RelativeLocation,
		const FVector& RelativeScale,
		const FRotator& RelativeRotation = FRotator::ZeroRotator)
	{
		check(Component);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(RelativeLocation * CatVisualScale);
		Component->SetRelativeRotation(RelativeRotation);
		Component->SetRelativeScale3D(RelativeScale * CatVisualScale);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(true);
	}

	bool IsPointInsideBoxXY(const FVector& Point, const FBox& Box)
	{
		const float MinX = FMath::Min(Box.Min.X, Box.Max.X);
		const float MaxX = FMath::Max(Box.Min.X, Box.Max.X);
		const float MinY = FMath::Min(Box.Min.Y, Box.Max.Y);
		const float MaxY = FMath::Max(Box.Min.Y, Box.Max.Y);
		return Point.X >= MinX && Point.X <= MaxX && Point.Y >= MinY && Point.Y <= MaxY;
	}

	float DistanceToNearestGridLine(const float Coordinate, const float Spacing)
	{
		const float NearestLine = FMath::RoundToFloat(Coordinate / Spacing) * Spacing;
		return FMath::Abs(Coordinate - NearestLine);
	}
}

ACatCitizen::ACatCitizen()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectMaterialFinder(
		TEXT("/Game/Generated/Fallback/M_FallbackTintedLit.M_FallbackTintedLit"));

	UStaticMesh* const SphereMesh = SphereFinder.Object;
	UStaticMesh* const CylinderMesh = CylinderFinder.Object;
	UStaticMesh* const ConeMesh = ConeFinder.Object;
	BasicShapeMaterial = ProjectMaterialFinder.Succeeded()
		? ProjectMaterialFinder.Object.Get()
		: MaterialFinder.Object.Get();

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	ConfigureCatMesh(Body, SphereMesh, SceneRoot, FVector(0.0f, 0.0f, 48.0f), FVector(0.52f, 0.34f, 0.46f));

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	ConfigureCatMesh(Head, SphereMesh, SceneRoot, FVector(13.0f, 0.0f, 82.0f), FVector(0.37f, 0.35f, 0.35f));

	Muzzle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Muzzle"));
	ConfigureCatMesh(Muzzle, SphereMesh, SceneRoot, FVector(31.0f, 0.0f, 78.5f), FVector(0.11f, 0.16f, 0.10f));

	LeftEar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEar"));
	ConfigureCatMesh(LeftEar, ConeMesh, SceneRoot, FVector(11.0f, -11.0f, 105.0f), FVector(0.105f, 0.105f, 0.20f), FRotator(0.0f, -4.0f, 0.0f));

	RightEar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEar"));
	ConfigureCatMesh(RightEar, ConeMesh, SceneRoot, FVector(11.0f, 11.0f, 105.0f), FVector(0.105f, 0.105f, 0.20f), FRotator(0.0f, 4.0f, 0.0f));

	LeftEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEye"));
	ConfigureCatMesh(LeftEye, SphereMesh, SceneRoot, FVector(30.5f, -7.5f, 85.0f), FVector(0.043f, 0.032f, 0.052f));

	RightEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEye"));
	ConfigureCatMesh(RightEye, SphereMesh, SceneRoot, FVector(30.5f, 7.5f, 85.0f), FVector(0.043f, 0.032f, 0.052f));

	Nose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Nose"));
	ConfigureCatMesh(Nose, SphereMesh, SceneRoot, FVector(37.0f, 0.0f, 79.0f), FVector(0.035f, 0.045f, 0.032f));

	FrontLeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftLeg"));
	ConfigureCatMesh(FrontLeftLeg, CylinderMesh, SceneRoot, FVector(14.0f, -11.0f, 16.0f), FVector(0.075f, 0.075f, 0.25f));

	FrontRightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightLeg"));
	ConfigureCatMesh(FrontRightLeg, CylinderMesh, SceneRoot, FVector(14.0f, 11.0f, 16.0f), FVector(0.075f, 0.075f, 0.25f));

	BackLeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackLeftLeg"));
	ConfigureCatMesh(BackLeftLeg, CylinderMesh, SceneRoot, FVector(-14.0f, -11.0f, 16.0f), FVector(0.08f, 0.08f, 0.25f));

	BackRightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRightLeg"));
	ConfigureCatMesh(BackRightLeg, CylinderMesh, SceneRoot, FVector(-14.0f, 11.0f, 16.0f), FVector(0.08f, 0.08f, 0.25f));

	Tail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tail"));
	ConfigureCatMesh(Tail, CylinderMesh, SceneRoot, FVector(-31.0f, 0.0f, 49.0f), FVector(0.075f, 0.075f, 0.34f), FRotator(-52.0f, 0.0f, 0.0f));

	BaseBodyLocation = Body->GetRelativeLocation();
	BaseTailRotation = Tail->GetRelativeRotation();
}

void ACatCitizen::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(GetUniqueID() ^ FMath::Rand());
	MoveSpeed = RandomStream.FRandRange(
		FMath::Min(MinimumMoveSpeed, MaximumMoveSpeed),
		FMath::Max(MinimumMoveSpeed, MaximumMoveSpeed));
	GroundZ = GetActorLocation().Z;

	if (!bHasCityRules)
	{
		const FVector Origin = GetActorLocation();
		CityBoundsMin = Origin + FVector(-3000.0f, -3000.0f, 0.0f);
		CityBoundsMax = Origin + FVector(3000.0f, 3000.0f, 0.0f);
		CityRoadSpacing = 0.0f;
		CityRoadWidth = 0.0f;
		CityRiverHalfWidth = 0.0f;
	}

	ApplyRandomPalette();

	if (!IsWalkableLocation(GetActorLocation(), PedestrianClearance))
	{
		FVector RecoveryLocation;
		if (TryFindRecoveryLocation(RecoveryLocation))
		{
			SetActorLocation(RecoveryLocation);
		}
	}

	FVector SurfaceAdjustedLocation = GetActorLocation();
	SurfaceAdjustedLocation.Z = ResolveWalkSurfaceZ(SurfaceAdjustedLocation);
	SetActorLocation(SurfaceAdjustedLocation);

	ChooseNewTarget();
}

void ACatCitizen::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	UpdateMovement(DeltaSeconds);
	UpdateWalkAnimation(DeltaSeconds);
}

void ACatCitizen::Initialize(
	const FVector& BoundsMin,
	const FVector& BoundsMax,
	const float RoadSpacing,
	const float RoadWidth,
	const float RiverX,
	const float RiverHalfWidth,
	const TArray<FBox>& BridgeAreas)
{
	CityBoundsMin = FVector(
		FMath::Min(BoundsMin.X, BoundsMax.X),
		FMath::Min(BoundsMin.Y, BoundsMax.Y),
		FMath::Min(BoundsMin.Z, BoundsMax.Z));
	CityBoundsMax = FVector(
		FMath::Max(BoundsMin.X, BoundsMax.X),
		FMath::Max(BoundsMin.Y, BoundsMax.Y),
		FMath::Max(BoundsMin.Z, BoundsMax.Z));
	CityRoadSpacing = FMath::Max(0.0f, RoadSpacing);
	CityRoadWidth = FMath::Max(0.0f, RoadWidth);
	CityRiverX = RiverX;
	CityRiverHalfWidth = FMath::Max(0.0f, RiverHalfWidth);
	CityBridgeAreas = BridgeAreas;
	GroundZ = GetActorLocation().Z;
	bHasCityRules = true;

	if (HasActorBegunPlay())
	{
		if (!IsWalkableLocation(GetActorLocation(), PedestrianClearance))
		{
			FVector RecoveryLocation;
			if (TryFindRecoveryLocation(RecoveryLocation))
			{
				SetActorLocation(RecoveryLocation);
			}
		}
		FVector SurfaceAdjustedLocation = GetActorLocation();
		SurfaceAdjustedLocation.Z = ResolveWalkSurfaceZ(SurfaceAdjustedLocation);
		SetActorLocation(SurfaceAdjustedLocation);
		ChooseNewTarget();
	}
}

bool ACatCitizen::IsWalkableLocation(const FVector& Location, const float ExtraClearance) const
{
	return IsLocationWalkableForCity(
		Location,
		CityBoundsMin,
		CityBoundsMax,
		CityRoadSpacing,
		CityRoadWidth,
		CityRiverX,
		CityRiverHalfWidth,
		CityBridgeAreas,
		ExtraClearance);
}

bool ACatCitizen::IsLocationWalkableForCity(
	const FVector& Location,
	const FVector& BoundsMin,
	const FVector& BoundsMax,
	const float RoadSpacing,
	const float RoadWidth,
	const float RiverX,
	const float RiverHalfWidth,
	const TArray<FBox>& BridgeAreas,
	const float Clearance)
{
	const float SafeClearance = FMath::Max(0.0f, Clearance);
	const float MinimumX = FMath::Min(BoundsMin.X, BoundsMax.X) + SafeClearance;
	const float MaximumX = FMath::Max(BoundsMin.X, BoundsMax.X) - SafeClearance;
	const float MinimumY = FMath::Min(BoundsMin.Y, BoundsMax.Y) + SafeClearance;
	const float MaximumY = FMath::Max(BoundsMin.Y, BoundsMax.Y) - SafeClearance;

	if (Location.X < MinimumX || Location.X > MaximumX || Location.Y < MinimumY || Location.Y > MaximumY)
	{
		return false;
	}

	if (RoadSpacing > KINDA_SMALL_NUMBER && RoadWidth > KINDA_SMALL_NUMBER)
	{
		const float HalfRoadWithClearance = RoadWidth * 0.5f + SafeClearance;
		if (DistanceToNearestGridLine(Location.X, RoadSpacing) <= HalfRoadWithClearance ||
			DistanceToNearestGridLine(Location.Y, RoadSpacing) <= HalfRoadWithClearance)
		{
			return false;
		}
	}

	if (RiverHalfWidth > KINDA_SMALL_NUMBER && FMath::Abs(Location.X - RiverX) <= RiverHalfWidth + SafeClearance)
	{
		for (const FBox& Bridge : BridgeAreas)
		{
			if (IsPointInsideBoxXY(Location, Bridge))
			{
				return true;
			}
		}
		return false;
	}

	return true;
}

void ACatCitizen::ApplyRandomPalette()
{
	if (!BasicShapeMaterial)
	{
		return;
	}

	static const FLinearColor FurPalette[] =
	{
		FLinearColor(0.98f, 0.34f, 0.24f), // warm coral
		FLinearColor(0.24f, 0.66f, 0.98f), // clear sky blue
		FLinearColor(0.30f, 0.82f, 0.50f), // mint green
		FLinearColor(0.65f, 0.45f, 0.94f), // lavender
		FLinearColor(1.00f, 0.68f, 0.18f), // sunny cream
		FLinearColor(0.98f, 0.46f, 0.70f), // sakura pink
		FLinearColor(0.20f, 0.78f, 0.82f)  // turquoise
	};

	const FLinearColor FurColor = FurPalette[RandomStream.RandRange(0, UE_ARRAY_COUNT(FurPalette) - 1)];
	const FLinearColor AccentColor = FLinearColor::LerpUsingHSV(FurColor, FLinearColor(1.0f, 0.97f, 0.88f), 0.62f);
	const FLinearColor EyeColor(0.018f, 0.028f, 0.055f, 1.0f);
	const FLinearColor NoseColor(1.0f, 0.33f, 0.48f, 1.0f);

	auto MakeMaterial = [this](const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
		if (Material)
		{
			Material->SetVectorParameterValue(ShapeColorParameter, Color);
			DynamicMaterials.Add(Material);
		}
		return Material;
	};

	UMaterialInstanceDynamic* const FurMaterial = MakeMaterial(FurColor);
	UMaterialInstanceDynamic* const AccentMaterial = MakeMaterial(AccentColor);
	UMaterialInstanceDynamic* const EyeMaterial = MakeMaterial(EyeColor);
	UMaterialInstanceDynamic* const NoseMaterial = MakeMaterial(NoseColor);

	for (UStaticMeshComponent* Component :
		{Body.Get(), Head.Get(), FrontLeftLeg.Get(), FrontRightLeg.Get(), BackLeftLeg.Get(), BackRightLeg.Get(), Tail.Get()})
	{
		Component->SetMaterial(0, FurMaterial);
	}

	Muzzle->SetMaterial(0, AccentMaterial);
	LeftEar->SetMaterial(0, AccentMaterial);
	RightEar->SetMaterial(0, AccentMaterial);
	LeftEye->SetMaterial(0, EyeMaterial);
	RightEye->SetMaterial(0, EyeMaterial);
	Nose->SetMaterial(0, NoseMaterial);
}

void ACatCitizen::ChooseNewTarget()
{
	const FVector CurrentLocation = GetActorLocation();
	const float MinimumRadius = FMath::Min(150.0f, FMath::Max(50.0f, WanderRadius));
	const float MaximumRadius = FMath::Max(MinimumRadius, WanderRadius);

	for (int32 Attempt = 0; Attempt < 42; ++Attempt)
	{
		const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
		const float Distance = RandomStream.FRandRange(MinimumRadius, MaximumRadius);
		FVector Candidate = CurrentLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
		Candidate.X = FMath::Clamp(Candidate.X, CityBoundsMin.X + PedestrianClearance, CityBoundsMax.X - PedestrianClearance);
		Candidate.Y = FMath::Clamp(Candidate.Y, CityBoundsMin.Y + PedestrianClearance, CityBoundsMax.Y - PedestrianClearance);
		Candidate.Z = GroundZ;

		if (IsWalkableLocation(Candidate, PedestrianClearance) && IsPathWalkable(CurrentLocation, Candidate))
		{
			WanderTarget = Candidate;
			TimeUntilRetarget = RandomStream.FRandRange(4.0f, 9.0f);
			return;
		}
	}

	WanderTarget = CurrentLocation;
	TimeUntilRetarget = RandomStream.FRandRange(0.35f, 0.8f);
}

void ACatCitizen::UpdateMovement(const float DeltaSeconds)
{
	TimeUntilRetarget -= DeltaSeconds;
	FVector CurrentLocation = GetActorLocation();
	CurrentLocation.Z = GroundZ;

	if (!IsWalkableLocation(CurrentLocation, PedestrianClearance))
	{
		FVector RecoveryLocation;
		if (TryFindRecoveryLocation(RecoveryLocation))
		{
			SetActorLocation(RecoveryLocation);
			CurrentLocation = RecoveryLocation;
			CurrentVelocity = FVector::ZeroVector;
		}
	}

	const float DistanceToTarget = FVector::Dist2D(CurrentLocation, WanderTarget);
	if (TimeUntilRetarget <= 0.0f || DistanceToTarget < 55.0f || !IsWalkableLocation(WanderTarget, PedestrianClearance))
	{
		ChooseNewTarget();
	}

	FVector DesiredDirection = WanderTarget - CurrentLocation;
	DesiredDirection.Z = 0.0f;
	DesiredDirection = DesiredDirection.GetSafeNormal();
	DesiredDirection = GetAvoidanceDirection(DesiredDirection);

	const FVector DesiredVelocity = DesiredDirection * MoveSpeed;
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, DesiredVelocity, DeltaSeconds, SteeringSharpness);
	CurrentVelocity.Z = 0.0f;

	FVector ProposedLocation = CurrentLocation + CurrentVelocity * DeltaSeconds;
	ProposedLocation.Z = GroundZ;

	if (CurrentVelocity.SizeSquared2D() > 4.0f && IsPathWalkable(CurrentLocation, ProposedLocation))
	{
		ProposedLocation.Z = ResolveWalkSurfaceZ(ProposedLocation);
		SetActorLocation(ProposedLocation, false, nullptr, ETeleportType::None);

		const FRotator DesiredRotation(0.0f, CurrentVelocity.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, RotationSharpness));
	}
	else if (CurrentVelocity.SizeSquared2D() > 4.0f)
	{
		CurrentVelocity *= -0.2f;
		TimeUntilRetarget = 0.0f;
	}
}

void ACatCitizen::UpdateWalkAnimation(const float DeltaSeconds)
{
	const float SpeedAlpha = FMath::Clamp(CurrentVelocity.Size2D() / FMath::Max(1.0f, MoveSpeed), 0.0f, 1.0f);
	AnimationTime += DeltaSeconds * FMath::Lerp(2.5f, 8.5f, SpeedAlpha);

	const float BobAmount = FMath::Sin(AnimationTime * 2.0f) * 2.3f * SpeedAlpha;
	Body->SetRelativeLocation(BaseBodyLocation + FVector(0.0f, 0.0f, BobAmount));

	const float LegSwing = FMath::Sin(AnimationTime * 2.0f) * 18.0f * SpeedAlpha;
	FrontLeftLeg->SetRelativeRotation(FRotator(LegSwing, 0.0f, 0.0f));
	BackRightLeg->SetRelativeRotation(FRotator(LegSwing, 0.0f, 0.0f));
	FrontRightLeg->SetRelativeRotation(FRotator(-LegSwing, 0.0f, 0.0f));
	BackLeftLeg->SetRelativeRotation(FRotator(-LegSwing, 0.0f, 0.0f));

	FRotator AnimatedTail = BaseTailRotation;
	AnimatedTail.Yaw += FMath::Sin(AnimationTime * 0.72f) * FMath::Lerp(12.0f, 25.0f, SpeedAlpha);
	AnimatedTail.Roll += FMath::Sin(AnimationTime * 0.43f + 0.8f) * 7.0f;
	Tail->SetRelativeRotation(AnimatedTail);
}

bool ACatCitizen::IsPathWalkable(const FVector& Start, const FVector& End) const
{
	const float Distance = FVector::Dist2D(Start, End);
	const int32 StepCount = FMath::Clamp(FMath::CeilToInt(Distance / 32.0f), 1, 64);

	for (int32 Step = 0; Step <= StepCount; ++Step)
	{
		const FVector TestPoint = FMath::Lerp(Start, End, static_cast<float>(Step) / static_cast<float>(StepCount));
		if (!IsWalkableLocation(TestPoint, PedestrianClearance))
		{
			return false;
		}
	}

	const UWorld* const World = GetWorld();
	if (!World)
	{
		return true;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const FVector TraceStart(Start.X, Start.Y, GroundZ + CatObstacleProbeHeight);
	const FVector TraceEnd(End.X, End.Y, GroundZ + CatObstacleProbeHeight);
	return !World->SweepTestByChannel(
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(CatObstacleRadius),
		QueryParams);
}

bool ACatCitizen::IsPointClearOfStaticObstacles(const FVector& Location) const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return true;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const FVector ProbeLocation(Location.X, Location.Y, GroundZ + CatObstacleProbeHeight);
	return !World->OverlapBlockingTestByChannel(
		ProbeLocation,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(CatObstacleRadius),
		QueryParams);
}

float ACatCitizen::ResolveWalkSurfaceZ(const FVector& Location) const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return GroundZ;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	FHitResult SurfaceHit;
	const FVector TraceStart(Location.X, Location.Y, GroundZ + 120.0f);
	const FVector TraceEnd(Location.X, Location.Y, GroundZ - 60.0f);
	if (World->LineTraceSingleByChannel(SurfaceHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		const UPrimitiveComponent* const HitComponent = SurfaceHit.GetComponent();
		if (HitComponent)
		{
			const FName ComponentName = HitComponent->GetFName();
			if (ComponentName == TEXT("GroundTiles") || ComponentName == TEXT("Sidewalks") ||
				ComponentName == TEXT("Bridges") || ComponentName == TEXT("Roads"))
			{
				return SurfaceHit.ImpactPoint.Z + 0.5f;
			}
		}
	}

	return GroundZ;
}

bool ACatCitizen::TryFindRecoveryLocation(FVector& OutLocation)
{
	for (int32 Attempt = 0; Attempt < 96; ++Attempt)
	{
		FVector Candidate(
			RandomStream.FRandRange(CityBoundsMin.X + PedestrianClearance, CityBoundsMax.X - PedestrianClearance),
			RandomStream.FRandRange(CityBoundsMin.Y + PedestrianClearance, CityBoundsMax.Y - PedestrianClearance),
			GroundZ);

		if (IsWalkableLocation(Candidate, PedestrianClearance) && IsPointClearOfStaticObstacles(Candidate))
		{
			Candidate.Z = ResolveWalkSurfaceZ(Candidate);
			OutLocation = Candidate;
			return true;
		}
	}

	return false;
}

FVector ACatCitizen::GetAvoidanceDirection(const FVector& DesiredDirection) const
{
	if (DesiredDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector CurrentLocation = GetActorLocation();
	const float ProbeDistance = 95.0f + CurrentVelocity.Size2D() * 0.35f;
	if (IsPathWalkable(CurrentLocation, CurrentLocation + DesiredDirection * ProbeDistance))
	{
		return DesiredDirection;
	}

	static const float AvoidanceAngles[] = {34.0f, -34.0f, 68.0f, -68.0f, 102.0f, -102.0f};
	for (const float Angle : AvoidanceAngles)
	{
		const FVector CandidateDirection = DesiredDirection.RotateAngleAxis(Angle, FVector::UpVector).GetSafeNormal();
		if (IsPathWalkable(CurrentLocation, CurrentLocation + CandidateDirection * ProbeDistance))
		{
			return CandidateDirection;
		}
	}

	return -DesiredDirection;
}
