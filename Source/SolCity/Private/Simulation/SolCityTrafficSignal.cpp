#include "Simulation/SolCityTrafficSignal.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	UPointLightComponent* MakeSignalLight(AActor* Owner, const FName Name, USceneComponent* Parent,
		const FVector& RelativeLocation, const FLinearColor& Color)
	{
		UPointLightComponent* Light = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
		Light->SetupAttachment(Parent);
		Light->SetRelativeLocation(RelativeLocation);
		Light->SetLightColor(Color);
		Light->SetIntensity(0.0f);
		Light->SetAttenuationRadius(250.0f);
		Light->SetCastShadows(false);
		return Light;
	}
}

ASolCityTrafficSignal::ASolCityTrafficSignal()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	AxisARedLight = MakeSignalLight(this, TEXT("AxisARed"), SceneRoot, FVector(0, 0, 220), FLinearColor::Red);
	AxisAYellowLight = MakeSignalLight(this, TEXT("AxisAYellow"), SceneRoot, FVector(0, 0, 200), FLinearColor::Yellow);
	AxisAGreenLight = MakeSignalLight(this, TEXT("AxisAGreen"), SceneRoot, FVector(0, 0, 180), FLinearColor::Green);
	AxisBRedLight = MakeSignalLight(this, TEXT("AxisBRed"), SceneRoot, FVector(0, 40, 220), FLinearColor::Red);
	AxisBYellowLight = MakeSignalLight(this, TEXT("AxisBYellow"), SceneRoot, FVector(0, 40, 200), FLinearColor::Yellow);
	AxisBGreenLight = MakeSignalLight(this, TEXT("AxisBGreen"), SceneRoot, FVector(0, 40, 180), FLinearColor::Green);
	UpdateLightVisuals();
}

void ASolCityTrafficSignal::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}
	PhaseElapsedTime += DeltaSeconds;
	if (PhaseElapsedTime >= GetCurrentPhaseDuration())
	{
		AdvancePhase();
	}
}

bool ASolCityTrafficSignal::IsMovementAllowed(const ESolCityTrafficAxis Axis) const
{
	return Axis == ESolCityTrafficAxis::Uncontrolled
		|| (Axis == ESolCityTrafficAxis::AxisA && CurrentPhase == ESolCityTrafficPhase::AxisAGreen)
		|| (Axis == ESolCityTrafficAxis::AxisB && CurrentPhase == ESolCityTrafficPhase::AxisBGreen);
}

void ASolCityTrafficSignal::SetPhase(const ESolCityTrafficPhase NewPhase, const bool bResetTimer)
{
	CurrentPhase = NewPhase;
	if (bResetTimer)
	{
		PhaseElapsedTime = 0.0f;
	}
	UpdateLightVisuals();
	OnPhaseChanged.Broadcast(CurrentPhase);
}

float ASolCityTrafficSignal::GetCurrentPhaseDuration() const
{
	switch (CurrentPhase)
	{
	case ESolCityTrafficPhase::AxisAGreen:
	case ESolCityTrafficPhase::AxisBGreen:
		return GreenDuration;
	case ESolCityTrafficPhase::AxisAYellow:
	case ESolCityTrafficPhase::AxisBYellow:
		return YellowDuration;
	default:
		return FMath::Max(AllRedDuration, KINDA_SMALL_NUMBER);
	}
}

float ASolCityTrafficSignal::GetPhaseTimeRemaining() const
{
	return FMath::Max(0.0f, GetCurrentPhaseDuration() - PhaseElapsedTime);
}

void ASolCityTrafficSignal::AdvancePhase()
{
	const uint8 Next = (static_cast<uint8>(CurrentPhase) + 1u) % 6u;
	SetPhase(static_cast<ESolCityTrafficPhase>(Next));
}

void ASolCityTrafficSignal::UpdateLightVisuals()
{
	const bool bAGreen = CurrentPhase == ESolCityTrafficPhase::AxisAGreen;
	const bool bAYellow = CurrentPhase == ESolCityTrafficPhase::AxisAYellow;
	const bool bBGreen = CurrentPhase == ESolCityTrafficPhase::AxisBGreen;
	const bool bBYellow = CurrentPhase == ESolCityTrafficPhase::AxisBYellow;
	constexpr float LitIntensity = 3500.0f;
	AxisARedLight->SetIntensity(!bAGreen && !bAYellow ? LitIntensity : 0.0f);
	AxisAYellowLight->SetIntensity(bAYellow ? LitIntensity : 0.0f);
	AxisAGreenLight->SetIntensity(bAGreen ? LitIntensity : 0.0f);
	AxisBRedLight->SetIntensity(!bBGreen && !bBYellow ? LitIntensity : 0.0f);
	AxisBYellowLight->SetIntensity(bBYellow ? LitIntensity : 0.0f);
	AxisBGreenLight->SetIntensity(bBGreen ? LitIntensity : 0.0f);
}

void ASolCityTrafficSignal::OnRep_CurrentPhase()
{
	UpdateLightVisuals();
	OnPhaseChanged.Broadcast(CurrentPhase);
}

void ASolCityTrafficSignal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASolCityTrafficSignal, CurrentPhase);
}
