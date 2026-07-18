#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/SolCitySimulationTypes.h"
#include "SolCityTrafficSignal.generated.h"

class UPointLightComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSolCityTrafficPhaseChanged, ESolCityTrafficPhase, NewPhase);

/** Two-axis signal controller. A and B can represent any pair of conflicting road movements. */
UCLASS(BlueprintType)
class SOLCITY_API ASolCityTrafficSignal : public AActor
{
	GENERATED_BODY()

public:
	ASolCityTrafficSignal();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Signal", meta = (ClampMin = "0.1"))
	float GreenDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Signal", meta = (ClampMin = "0.1"))
	float YellowDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Signal", meta = (ClampMin = "0.0"))
	float AllRedDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentPhase, Category = "Traffic Signal")
	ESolCityTrafficPhase CurrentPhase = ESolCityTrafficPhase::AxisAGreen;

	UPROPERTY(BlueprintAssignable, Category = "Traffic Signal")
	FSolCityTrafficPhaseChanged OnPhaseChanged;

	UFUNCTION(BlueprintPure, Category = "Traffic Signal")
	bool IsMovementAllowed(ESolCityTrafficAxis Axis) const;

	UFUNCTION(BlueprintCallable, Category = "Traffic Signal")
	void SetPhase(ESolCityTrafficPhase NewPhase, bool bResetTimer = true);

	UFUNCTION(BlueprintPure, Category = "Traffic Signal")
	float GetPhaseTimeRemaining() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_CurrentPhase();

	void AdvancePhase();
	void UpdateLightVisuals();
	float GetCurrentPhaseDuration() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisARedLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisAYellowLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisAGreenLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisBRedLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisBYellowLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Signal|Visual")
	TObjectPtr<UPointLightComponent> AxisBGreenLight;

	UPROPERTY(Transient)
	float PhaseElapsedTime = 0.0f;
};
