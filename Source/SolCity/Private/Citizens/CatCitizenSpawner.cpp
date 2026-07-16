#include "Citizens/CatCitizenSpawner.h"

#include "Citizens/CatCitizen.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ACatCitizenSpawner::ACatCitizenSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	CitizenClass = ACatCitizen::StaticClass();
}

TArray<ACatCitizen*> ACatCitizenSpawner::SpawnCitizens(
	const int32 Count,
	const FVector& BoundsMin,
	const FVector& BoundsMax,
	const float RoadSpacing,
	const float RoadWidth,
	const float RiverX,
	const float RiverHalfWidth,
	const TArray<FBox>& BridgeAreas)
{
	TArray<ACatCitizen*> NewCitizens;
	if (Count <= 0 || !GetWorld())
	{
		return NewCitizens;
	}

	SpawnedCitizens.RemoveAll([](const TObjectPtr<ACatCitizen>& Citizen)
	{
		return !IsValid(Citizen.Get());
	});

	RandomStream.Initialize(RandomSeed != 0 ? RandomSeed : FMath::Rand());
	NewCitizens.Reserve(Count);

	UClass* const SpawnClass = CitizenClass ? CitizenClass.Get() : ACatCitizen::StaticClass();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector SpawnLocation;
		if (!TryFindSpawnLocation(
			SpawnLocation,
			BoundsMin,
			BoundsMax,
			RoadSpacing,
			RoadWidth,
			RiverX,
			RiverHalfWidth,
			BridgeAreas))
		{
			continue;
		}

		const FTransform SpawnTransform(
			FRotator(0.0f, RandomStream.FRandRange(-180.0f, 180.0f), 0.0f),
			SpawnLocation);
		ACatCitizen* const Citizen = GetWorld()->SpawnActorDeferred<ACatCitizen>(
			SpawnClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Citizen)
		{
			continue;
		}

		Citizen->Initialize(
			BoundsMin,
			BoundsMax,
			RoadSpacing,
			RoadWidth,
			RiverX,
			RiverHalfWidth,
			BridgeAreas);
		Citizen->FinishSpawning(SpawnTransform);

		SpawnedCitizens.Add(Citizen);
		NewCitizens.Add(Citizen);
	}

	return NewCitizens;
}

void ACatCitizenSpawner::ClearCitizens()
{
	for (ACatCitizen* Citizen : SpawnedCitizens)
	{
		if (IsValid(Citizen))
		{
			Citizen->Destroy();
		}
	}
	SpawnedCitizens.Empty();
}

int32 ACatCitizenSpawner::GetSpawnedCitizenCount() const
{
	int32 Count = 0;
	for (const ACatCitizen* Citizen : SpawnedCitizens)
	{
		Count += IsValid(Citizen) ? 1 : 0;
	}
	return Count;
}

bool ACatCitizenSpawner::TryFindSpawnLocation(
	FVector& OutLocation,
	const FVector& BoundsMin,
	const FVector& BoundsMax,
	const float RoadSpacing,
	const float RoadWidth,
	const float RiverX,
	const float RiverHalfWidth,
	const TArray<FBox>& BridgeAreas)
{
	const float MinimumX = FMath::Min(BoundsMin.X, BoundsMax.X) + SpawnClearance;
	const float MaximumX = FMath::Max(BoundsMin.X, BoundsMax.X) - SpawnClearance;
	const float MinimumY = FMath::Min(BoundsMin.Y, BoundsMax.Y) + SpawnClearance;
	const float MaximumY = FMath::Max(BoundsMin.Y, BoundsMax.Y) - SpawnClearance;
	if (MinimumX >= MaximumX || MinimumY >= MaximumY)
	{
		return false;
	}

	const float GroundZ = (BoundsMin.Z + BoundsMax.Z) * 0.5f + SpawnHeightOffset;
	for (int32 Attempt = 0; Attempt < 160; ++Attempt)
	{
		const FVector Candidate(
			RandomStream.FRandRange(MinimumX, MaximumX),
			RandomStream.FRandRange(MinimumY, MaximumY),
			GroundZ);

		if (!ACatCitizen::IsLocationWalkableForCity(
			Candidate,
			BoundsMin,
			BoundsMax,
			RoadSpacing,
			RoadWidth,
			RiverX,
			RiverHalfWidth,
			BridgeAreas,
			SpawnClearance) ||
			!IsFarEnoughFromExistingCitizens(Candidate))
		{
			continue;
		}

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		// Torso-height probing deliberately clears the city's <=22 cm walkable
		// slabs while still overlapping buildings, tree trunks and bridge pillars.
		const bool bOverlapsStaticObstacle = GetWorld()->OverlapBlockingTestByChannel(
			Candidate + FVector(0.0f, 0.0f, 62.0f),
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(34.0f),
			QueryParams);
		if (!bOverlapsStaticObstacle)
		{
			OutLocation = Candidate;
			return true;
		}
	}

	return false;
}

bool ACatCitizenSpawner::IsFarEnoughFromExistingCitizens(const FVector& Location) const
{
	const float MinimumDistanceSquared = FMath::Square(MinimumCitizenSeparation);
	for (const ACatCitizen* Citizen : SpawnedCitizens)
	{
		if (IsValid(Citizen) && FVector::DistSquared2D(Location, Citizen->GetActorLocation()) < MinimumDistanceSquared)
		{
			return false;
		}
	}
	return true;
}
