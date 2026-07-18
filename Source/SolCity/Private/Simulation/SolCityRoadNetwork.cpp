#include "Simulation/SolCityRoadNetwork.h"

#include "Components/SceneComponent.h"
#include "Simulation/SolCityTrafficSignal.h"

ASolCityRoadNetwork::ASolCityRoadNetwork()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolCityRoadNetwork::SetNodes(const TArray<FSolCityRoadWaypoint>& InNodes)
{
	Nodes = InNodes;
}

bool ASolCityRoadNetwork::IsValidNode(const int32 NodeIndex) const
{
	return Nodes.IsValidIndex(NodeIndex);
}

const FSolCityRoadWaypoint* ASolCityRoadNetwork::GetNode(const int32 NodeIndex) const
{
	return Nodes.IsValidIndex(NodeIndex) ? &Nodes[NodeIndex] : nullptr;
}

FVector ASolCityRoadNetwork::GetNodeWorldPosition(const int32 NodeIndex) const
{
	const FSolCityRoadWaypoint* Node = GetNode(NodeIndex);
	return Node ? GetActorTransform().TransformPosition(Node->LocalPosition) : GetActorLocation();
}

int32 ASolCityRoadNetwork::FindNearestNode(const FVector& WorldPosition) const
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

int32 ASolCityRoadNetwork::ChooseNextNode(const int32 NodeIndex, FRandomStream& RandomStream) const
{
	const FSolCityRoadWaypoint* Node = GetNode(NodeIndex);
	if (!Node || Node->OutgoingNodeIndices.IsEmpty())
	{
		return INDEX_NONE;
	}

	const int32 StartOffset = RandomStream.RandRange(0, Node->OutgoingNodeIndices.Num() - 1);
	for (int32 Offset = 0; Offset < Node->OutgoingNodeIndices.Num(); ++Offset)
	{
		const int32 Candidate = Node->OutgoingNodeIndices[(StartOffset + Offset) % Node->OutgoingNodeIndices.Num()];
		if (IsValidNode(Candidate))
		{
			return Candidate;
		}
	}
	return INDEX_NONE;
}

bool ASolCityRoadNetwork::IsNodeOpenForTraffic(const int32 NodeIndex) const
{
	const FSolCityRoadWaypoint* Node = GetNode(NodeIndex);
	if (!Node || !Node->TrafficSignal || Node->MovementAxis == ESolCityTrafficAxis::Uncontrolled)
	{
		return Node != nullptr;
	}
	return Node->TrafficSignal->IsMovementAllowed(Node->MovementAxis);
}
