#include "Simulation/SolCityCatCitizen.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Simulation/SolCityPedestrianNetwork.h"
#include "UObject/ConstructorHelpers.h"

ASolCityCatCitizen::ASolCityCatCitizen()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* CitizenRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CitizenRoot"));
	SetRootComponent(CitizenRoot);

	CatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CatMesh"));
	CatMesh->SetupAttachment(CitizenRoot);
	CatMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CatMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	CatMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	CatMesh->SetRelativeScale3D(FVector(0.42f, 0.24f, 0.28f));

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(CatMesh);
	HeadMesh->SetRelativeLocation(FVector(105.0f, 0.0f, 35.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.72f, 0.95f, 0.95f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftEarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEarMesh"));
	LeftEarMesh->SetupAttachment(HeadMesh);
	LeftEarMesh->SetRelativeLocation(FVector(5.0f, -45.0f, 48.0f));
	LeftEarMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.42f));
	LeftEarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RightEarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEarMesh"));
	RightEarMesh->SetupAttachment(HeadMesh);
	RightEarMesh->SetRelativeLocation(FVector(5.0f, 45.0f, 48.0f));
	RightEarMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.42f));
	RightEarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	TailMesh->SetupAttachment(CatMesh);
	TailMesh->SetRelativeLocation(FVector(-115.0f, 0.0f, 35.0f));
	TailMesh->SetRelativeRotation(FRotator(0.0f, -35.0f, 62.0f));
	TailMesh->SetRelativeScale3D(FVector(0.20f, 0.20f, 0.75f));
	TailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredCatAsset(TEXT("/Game/Art/Props/SM_SolCity_VoxelCat_01.SM_SolCity_VoxelCat_01"));
	if (AuthoredCatAsset.Succeeded())
	{
		CatMesh->SetStaticMesh(AuthoredCatAsset.Object);
		CatMesh->SetRelativeLocation(FVector::ZeroVector);
		// After FBX axis conversion the imported visual needs a -90 yaw. Keep the
		// pedestrian actor's local +X path heading and rotate only its hierarchy.
		CatMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		CatMesh->SetRelativeScale3D(FVector(0.62f));
		for (UStaticMeshComponent* FallbackPart : {HeadMesh.Get(), LeftEarMesh.Get(), RightEarMesh.Get(), TailMesh.Get()})
		{
			FallbackPart->SetVisibility(false);
			FallbackPart->SetHiddenInGame(true);
		}
	}
	else if (SphereAsset.Succeeded())
	{
		CatMesh->SetStaticMesh(SphereAsset.Object);
		HeadMesh->SetStaticMesh(SphereAsset.Object);
		if (ConeAsset.Succeeded())
		{
			LeftEarMesh->SetStaticMesh(ConeAsset.Object);
			RightEarMesh->SetStaticMesh(ConeAsset.Object);
		}
		if (CylinderAsset.Succeeded())
		{
			TailMesh->SetStaticMesh(CylinderAsset.Object);
		}
	}
	Tags.Add(TEXT("SolCityCatCitizen"));
}

void ASolCityCatCitizen::BeginPlay()
{
	Super::BeginPlay();
	RandomStream.Initialize(GetUniqueID());
	if (PedestrianNetwork)
	{
		InitializeOnSidewalk(PedestrianNetwork, CurrentNodeIndex);
	}
}

bool ASolCityCatCitizen::InitializeOnSidewalk(ASolCityPedestrianNetwork* InNetwork, int32 StartNodeIndex)
{
	PedestrianNetwork = InNetwork;
	if (!PedestrianNetwork)
	{
		return false;
	}
	if (!PedestrianNetwork->IsValidNode(StartNodeIndex))
	{
		StartNodeIndex = PedestrianNetwork->FindNearestNode(GetActorLocation());
	}
	if (!PedestrianNetwork->IsValidNode(StartNodeIndex))
	{
		return false;
	}

	CurrentNodeIndex = StartNodeIndex;
	PreviousNodeIndex = INDEX_NONE;
	SetActorLocation(PedestrianNetwork->GetNodeWorldPosition(CurrentNodeIndex));
	return SelectNextTarget();
}

bool ASolCityCatCitizen::SelectNextTarget()
{
	TargetNodeIndex = PedestrianNetwork
		? PedestrianNetwork->ChooseConnectedNode(CurrentNodeIndex, PreviousNodeIndex, RandomStream)
		: INDEX_NONE;
	return TargetNodeIndex != INDEX_NONE;
}

void ASolCityCatCitizen::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!PedestrianNetwork || !PedestrianNetwork->IsValidNode(TargetNodeIndex))
	{
		return;
	}
	if (PauseTimeRemaining > 0.0f)
	{
		PauseTimeRemaining = FMath::Max(0.0f, PauseTimeRemaining - DeltaSeconds);
		return;
	}

	const FVector TargetPosition = PedestrianNetwork->GetNodeWorldPosition(TargetNodeIndex);
	const FVector ToTarget = TargetPosition - GetActorLocation();
	const float DistanceToTarget = ToTarget.Size2D();
	if (DistanceToTarget <= WaypointAcceptanceRadius)
	{
		SetActorLocation(TargetPosition);
		PreviousNodeIndex = CurrentNodeIndex;
		CurrentNodeIndex = TargetNodeIndex;
		PauseTimeRemaining = RandomStream.FRandRange(
			FMath::Min(PauseDurationRange.X, PauseDurationRange.Y),
			FMath::Max(PauseDurationRange.X, PauseDurationRange.Y));
		SelectNextTarget();
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal2D();
	const float TravelDistance = FMath::Min(WalkSpeed * DeltaSeconds, DistanceToTarget);
	SetActorLocation(GetActorLocation() + Direction * TravelDistance);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, RotationResponsiveness));
}
