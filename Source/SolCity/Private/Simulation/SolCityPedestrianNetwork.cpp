#include "Simulation/SolCityPedestrianNetwork.h"

#include "Components/SceneComponent.h"

ASolCityPedestrianNetwork::ASolCityPedestrianNetwork()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolCityPedestrianNetwork::SetNodes(const TArray<FSolCityPedestrianWaypoint>& InNodes)
{
	Nodes = InNodes;
}

bool ASolCityPedestrianNetwork::IsValidNode(const int32 NodeIndex) const
{
	return Nodes.IsValidIndex(NodeIndex);
}

FVector ASolCityPedestrianNetwork::GetNodeWorldPosition(const int32 NodeIndex) const
{
	return IsValidNode(NodeIndex) ? GetActorTransform().TransformPosition(Nodes[NodeIndex].LocalPosition) : GetActorLocation();
}

int32 ASolCityPedestrianNetwork::FindNearestNode(const FVector& WorldPosition) const
{
	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const float DistanceSquared = FVector::DistSquared(WorldPosition, GetNodeWorldPosition(Index));
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestIndex = Index;
		}
	}
	return BestIndex;
}

int32 ASolCityPedestrianNetwork::ChooseConnectedNode(const int32 NodeIndex, const int32 PreviousNodeIndex,
	FRandomStream& RandomStream) const
{
	if (!Nodes.IsValidIndex(NodeIndex) || Nodes[NodeIndex].ConnectedNodeIndices.IsEmpty())
	{
		return INDEX_NONE;
	}
	const TArray<int32>& Connections = Nodes[NodeIndex].ConnectedNodeIndices;
	const int32 StartOffset = RandomStream.RandRange(0, Connections.Num() - 1);
	for (int32 Offset = 0; Offset < Connections.Num(); ++Offset)
	{
		const int32 Candidate = Connections[(StartOffset + Offset) % Connections.Num()];
		if (IsValidNode(Candidate) && (Candidate != PreviousNodeIndex || Connections.Num() == 1))
		{
			return Candidate;
		}
	}
	return INDEX_NONE;
}
