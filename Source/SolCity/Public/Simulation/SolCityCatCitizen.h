#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolCityCatCitizen.generated.h"

class ASolCityPedestrianNetwork;
class UStaticMeshComponent;

/** Autonomous cat citizen that can only traverse the pedestrian waypoint network. */
UCLASS(BlueprintType)
class SOLCITY_API ASolCityCatCitizen : public AActor
{
	GENERATED_BODY()

public:
	ASolCityCatCitizen();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat Citizen")
	TObjectPtr<ASolCityPedestrianNetwork> PedestrianNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat Citizen", meta = (ClampMin = "10.0"))
	float WalkSpeed = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat Citizen", meta = (ClampMin = "1.0"))
	float WaypointAcceptanceRadius = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat Citizen", meta = (ClampMin = "0.0"))
	FVector2D PauseDurationRange = FVector2D(0.3f, 2.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat Citizen", meta = (ClampMin = "0.1"))
	float RotationResponsiveness = 8.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	int32 CurrentNodeIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	int32 TargetNodeIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Cat Citizen")
	bool InitializeOnSidewalk(ASolCityPedestrianNetwork* InNetwork, int32 StartNodeIndex = -1);

protected:
	bool SelectNextTarget();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	TObjectPtr<UStaticMeshComponent> CatMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	TObjectPtr<UStaticMeshComponent> LeftEarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	TObjectPtr<UStaticMeshComponent> RightEarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cat Citizen")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	UPROPERTY(Transient)
	int32 PreviousNodeIndex = INDEX_NONE;

	UPROPERTY(Transient)
	float PauseTimeRemaining = 0.0f;

	FRandomStream RandomStream;
};
