#pragma once

#include "CoreMinimal.h"

namespace SolCityLotLayout
{
	struct FRoadCenterline
	{
		FVector2D Start = FVector2D::ZeroVector;
		FVector2D End = FVector2D::ZeroVector;
		float HalfWidth = 0.0f;
		int32 SourceRoadIndex = INDEX_NONE;
	};

	struct FBlockEdge
	{
		FVector2D Start = FVector2D::ZeroVector;
		FVector2D End = FVector2D::ZeroVector;
		float RoadHalfWidth = 0.0f;
		int32 SourceRoadIndex = INDEX_NONE;
	};

	struct FCityBlock
	{
		TArray<FVector2D> Boundary;
		TArray<FBlockEdge> Edges;
		FVector2D Centroid = FVector2D::ZeroVector;
		FBox2D Bounds = FBox2D(EForceInit::ForceInit);
		float Area = 0.0f;
	};

	struct FCityLot
	{
		TArray<FVector2D> Boundary;
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Footprint = FVector2D::ZeroVector;
		float YawDegrees = 0.0f;
		int32 BlockIndex = INDEX_NONE;
		int32 RoadIndex = INDEX_NONE;
		bool bCornerLot = false;
	};

	struct FLotSettings
	{
		float MinimumFrontage = 900.0f;
		float MaximumFrontage = 1750.0f;
		float MinimumDepth = 760.0f;
		float MaximumDepth = 1380.0f;
		float FrontSetback = 280.0f;
		float SideSetback = 70.0f;
		float CornerClearance = 360.0f;
	};

	TArray<FCityBlock> ExtractBlocks(const TArray<FRoadCenterline>& Roads, float CityDiameter);
	void SubdivideBlockFrontages(
		const FCityBlock& Block,
		int32 BlockIndex,
		FRandomStream& Random,
		const FLotSettings& Settings,
		TArray<FCityLot>& OutLots);
	bool IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon);
}
