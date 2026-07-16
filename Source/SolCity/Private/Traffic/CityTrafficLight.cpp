#include "Traffic/CityTrafficLight.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Traffic/CityRoadNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACityTrafficLight::ACityTrafficLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.10f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NorthSouthPole = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NorthSouthPole"));
	NorthSouthPole->SetupAttachment(SceneRoot);
	NorthSouthSignal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NorthSouthSignal"));
	NorthSouthSignal->SetupAttachment(SceneRoot);
	EastWestPole = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EastWestPole"));
	EastWestPole->SetupAttachment(SceneRoot);
	EastWestSignal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EastWestSignal"));
	EastWestSignal->SetupAttachment(SceneRoot);

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

	if (CylinderMesh.Succeeded())
	{
		NorthSouthPole->SetStaticMesh(CylinderMesh.Object);
		EastWestPole->SetStaticMesh(CylinderMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		NorthSouthSignal->SetStaticMesh(CubeMesh.Object);
		EastWestSignal->SetStaticMesh(CubeMesh.Object);
	}

	for (UStaticMeshComponent* MeshComponent : {
		NorthSouthPole.Get(), NorthSouthSignal.Get(), EastWestPole.Get(), EastWestSignal.Get() })
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
		MeshComponent->SetCastShadow(true);
		if (ShapeMaterial)
		{
			MeshComponent->SetMaterial(0, ShapeMaterial);
		}
	}

	NorthSouthPole->SetRelativeScale3D(FVector(0.10f, 0.10f, 2.25f));
	EastWestPole->SetRelativeScale3D(FVector(0.10f, 0.10f, 2.25f));
	NorthSouthSignal->SetRelativeScale3D(FVector(0.25f, 0.18f, 0.48f));
	EastWestSignal->SetRelativeScale3D(FVector(0.18f, 0.25f, 0.48f));
	ConfigureVisualPlacement();
}

void ACityTrafficLight::BeginPlay()
{
	Super::BeginPlay();
	EnsureDynamicMaterials();
	UpdatePhaseAndVisuals();
}

void ACityTrafficLight::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePhaseAndVisuals();
}

void ACityTrafficLight::InitializeLight(ACityRoadNetwork* InRoadNetwork,
	const FIntPoint InIntersection, const float InPhaseOffset)
{
	RoadNetwork = InRoadNetwork;
	Intersection = InIntersection;
	PhaseOffset = FMath::Max(0.0f, InPhaseOffset);

	if (RoadNetwork)
	{
		SetActorLocationAndRotation(RoadNetwork->GridToWorldIntersection(Intersection),
			RoadNetwork->GetActorRotation());
	}

	ConfigureVisualPlacement();
	EnsureDynamicMaterials();
	bVisualsInitialized = false;
	UpdatePhaseAndVisuals();
}

bool ACityTrafficLight::CanProceed(const ECityRoadDirection IncomingDirection) const
{
	const bool bNorthSouth = IncomingDirection == ECityRoadDirection::North
		|| IncomingDirection == ECityRoadDirection::South;
	return bNorthSouth
		? CurrentPhase == ECityTrafficPhase::NorthSouthGreen
		: CurrentPhase == ECityTrafficPhase::EastWestGreen;
}

float ACityTrafficLight::GetSignedDistanceToStopLine(const ECityRoadDirection IncomingDirection,
	const FVector VehicleLocation) const
{
	if (!RoadNetwork)
	{
		return TNumericLimits<float>::Max();
	}

	const FVector Direction = RoadNetwork->GetWorldDirection(IncomingDirection);
	return FVector::DotProduct(GetStopLineWorldLocation(IncomingDirection) - VehicleLocation, Direction);
}

FVector ACityTrafficLight::GetStopLineWorldLocation(const ECityRoadDirection IncomingDirection) const
{
	return RoadNetwork
		? RoadNetwork->GetStopLineLocation(Intersection, IncomingDirection)
		: GetActorLocation();
}

bool ACityTrafficLight::ShouldStop(const ECityRoadDirection IncomingDirection,
	const FVector VehicleLocation, const float StopBuffer) const
{
	if (CanProceed(IncomingDirection))
	{
		return false;
	}

	const float SignedDistance = GetSignedDistanceToStopLine(IncomingDirection, VehicleLocation);
	return SignedDistance >= -FMath::Max(0.0f, StopBuffer)
		&& SignedDistance <= StopDetectionDistance;
}

void ACityTrafficLight::UpdatePhaseAndVisuals()
{
	const float SafePhaseDuration = FMath::Max(2.0f, PhaseDuration);
	const float CycleDuration = SafePhaseDuration * 2.0f;
	const float WorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float CyclePosition = FMath::Fmod(WorldSeconds + PhaseOffset, CycleDuration);
	CurrentPhase = CyclePosition < SafePhaseDuration
		? ECityTrafficPhase::NorthSouthGreen
		: ECityTrafficPhase::EastWestGreen;

	if (bVisualsInitialized && LastVisualPhase == CurrentPhase)
	{
		return;
	}

	EnsureDynamicMaterials();
	const bool bNorthSouthGo = CurrentPhase == ECityTrafficPhase::NorthSouthGreen;
	SetMaterialTint(NorthSouthMaterial, bNorthSouthGo ? GoColor : StopColor);
	SetMaterialTint(EastWestMaterial, bNorthSouthGo ? StopColor : GoColor);
	LastVisualPhase = CurrentPhase;
	bVisualsInitialized = true;
}

void ACityTrafficLight::ConfigureVisualPlacement()
{
	const float CornerOffset = RoadNetwork
		? FMath::Max(150.0f, RoadNetwork->GetRoadHalfWidth() * 0.80f)
		: 180.0f;

	NorthSouthPole->SetRelativeLocation(FVector(CornerOffset, CornerOffset, 112.0f));
	NorthSouthSignal->SetRelativeLocation(FVector(CornerOffset, CornerOffset, 246.0f));
	NorthSouthSignal->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	EastWestPole->SetRelativeLocation(FVector(-CornerOffset, -CornerOffset, 112.0f));
	EastWestSignal->SetRelativeLocation(FVector(-CornerOffset, -CornerOffset, 246.0f));
	EastWestSignal->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
}

void ACityTrafficLight::EnsureDynamicMaterials()
{
	if (!NorthSouthMaterial && NorthSouthSignal)
	{
		NorthSouthMaterial = NorthSouthSignal->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!EastWestMaterial && EastWestSignal)
	{
		EastWestMaterial = EastWestSignal->CreateAndSetMaterialInstanceDynamic(0);
	}

	const FLinearColor PastelPoleColor(0.16f, 0.30f, 0.34f, 1.0f);
	const FVector PastelPoleVector(PastelPoleColor.R, PastelPoleColor.G, PastelPoleColor.B);
	NorthSouthPole->SetVectorParameterValueOnMaterials(TEXT("Color"), PastelPoleVector);
	NorthSouthPole->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), PastelPoleVector);
	EastWestPole->SetVectorParameterValueOnMaterials(TEXT("Color"), PastelPoleVector);
	EastWestPole->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), PastelPoleVector);
}

void ACityTrafficLight::SetMaterialTint(UMaterialInstanceDynamic* Material,
	const FLinearColor& Color)
{
	if (!Material)
	{
		return;
	}

	// BasicShapeMaterial has used different parameter names between engine
	// distributions, so set the common variants. Missing parameters are harmless.
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Material->SetVectorParameterValue(TEXT("Tint"), Color);
}
