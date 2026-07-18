#include "City/SolCityLotLayout.h"

namespace SolCityLotLayout
{
	namespace
	{
		constexpr float VertexMergeTolerance = 12.0f;
		constexpr float MinimumGraphEdgeLength = 40.0f;
		constexpr float MinimumBlockArea = 2500000.0f;

		struct FPlanarVertex
		{
			FVector2D Position = FVector2D::ZeroVector;
			TArray<int32> OutgoingHalfEdges;
		};

		struct FHalfEdge
		{
			int32 From = INDEX_NONE;
			int32 To = INDEX_NONE;
			int32 Twin = INDEX_NONE;
			int32 SourceRoadIndex = INDEX_NONE;
			float RoadHalfWidth = 0.0f;
			float Angle = 0.0f;
			bool bVisited = false;
		};

		float Cross2D(const FVector2D& A, const FVector2D& B)
		{
			return A.X * B.Y - A.Y * B.X;
		}

		bool FindSegmentIntersection(
			const FVector2D& A,
			const FVector2D& B,
			const FVector2D& C,
			const FVector2D& D,
			float& OutT,
			float& OutU)
		{
			const FVector2D R = B - A;
			const FVector2D S = D - C;
			const float Denominator = Cross2D(R, S);
			if (FMath::Abs(Denominator) <= KINDA_SMALL_NUMBER)
			{
				return false;
			}

			const FVector2D Delta = C - A;
			OutT = Cross2D(Delta, S) / Denominator;
			OutU = Cross2D(Delta, R) / Denominator;
			constexpr float EndpointTolerance = 0.001f;
			return OutT >= -EndpointTolerance && OutT <= 1.0f + EndpointTolerance &&
				OutU >= -EndpointTolerance && OutU <= 1.0f + EndpointTolerance;
		}

		int32 FindOrAddVertex(TArray<FPlanarVertex>& Vertices, const FVector2D& Position)
		{
			for (int32 Index = 0; Index < Vertices.Num(); ++Index)
			{
				if (FVector2D::DistSquared(Vertices[Index].Position, Position) <= FMath::Square(VertexMergeTolerance))
				{
					return Index;
				}
			}

			FPlanarVertex& Vertex = Vertices.AddDefaulted_GetRef();
			Vertex.Position = Position;
			return Vertices.Num() - 1;
		}

		float SignedArea(const TArray<FVector2D>& Polygon)
		{
			float TwiceArea = 0.0f;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				TwiceArea += Cross2D(Polygon[Index], Polygon[(Index + 1) % Polygon.Num()]);
			}
			return TwiceArea * 0.5f;
		}

		FVector2D PolygonCentroid(const TArray<FVector2D>& Polygon, float SignedPolygonArea)
		{
			if (Polygon.Num() < 3 || FMath::IsNearlyZero(SignedPolygonArea))
			{
				FVector2D Average = FVector2D::ZeroVector;
				for (const FVector2D& Point : Polygon)
				{
					Average += Point;
				}
				return Polygon.IsEmpty() ? FVector2D::ZeroVector : Average / Polygon.Num();
			}

			FVector2D Weighted = FVector2D::ZeroVector;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				const float Weight = Cross2D(A, B);
				Weighted += (A + B) * Weight;
			}
			return Weighted / (6.0f * SignedPolygonArea);
		}

		void AddGraphEdge(
			TArray<FPlanarVertex>& Vertices,
			TArray<FHalfEdge>& HalfEdges,
			TMap<uint64, int32>& ExistingEdges,
			const FVector2D& Start,
			const FVector2D& End,
			const FRoadCenterline& SourceRoad)
		{
			if (FVector2D::Distance(Start, End) < MinimumGraphEdgeLength)
			{
				return;
			}

			const int32 From = FindOrAddVertex(Vertices, Start);
			const int32 To = FindOrAddVertex(Vertices, End);
			if (From == To)
			{
				return;
			}

			const uint32 Low = static_cast<uint32>(FMath::Min(From, To));
			const uint32 High = static_cast<uint32>(FMath::Max(From, To));
			const uint64 Key = (static_cast<uint64>(Low) << 32) | High;
			if (int32* ExistingIndex = ExistingEdges.Find(Key))
			{
				FHalfEdge& Existing = HalfEdges[*ExistingIndex];
				Existing.RoadHalfWidth = FMath::Max(Existing.RoadHalfWidth, SourceRoad.HalfWidth);
				HalfEdges[Existing.Twin].RoadHalfWidth = Existing.RoadHalfWidth;
				return;
			}

			const int32 ForwardIndex = HalfEdges.Num();
			FHalfEdge& Forward = HalfEdges.AddDefaulted_GetRef();
			Forward.From = From;
			Forward.To = To;
			Forward.Twin = ForwardIndex + 1;
			Forward.SourceRoadIndex = SourceRoad.SourceRoadIndex;
			Forward.RoadHalfWidth = SourceRoad.HalfWidth;
			Forward.Angle = FMath::Atan2(End.Y - Start.Y, End.X - Start.X);

			FHalfEdge& Reverse = HalfEdges.AddDefaulted_GetRef();
			Reverse.From = To;
			Reverse.To = From;
			Reverse.Twin = ForwardIndex;
			Reverse.SourceRoadIndex = SourceRoad.SourceRoadIndex;
			Reverse.RoadHalfWidth = SourceRoad.HalfWidth;
			Reverse.Angle = FMath::Atan2(Start.Y - End.Y, Start.X - End.X);

			Vertices[From].OutgoingHalfEdges.Add(ForwardIndex);
			Vertices[To].OutgoingHalfEdges.Add(ForwardIndex + 1);
			ExistingEdges.Add(Key, ForwardIndex);
		}

		bool AreLotCornersInside(const FCityLot& Lot, const TArray<FVector2D>& Polygon)
		{
			for (const FVector2D& Corner : Lot.Boundary)
			{
				if (!IsPointInsidePolygon(Corner, Polygon))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
	{
		if (Polygon.Num() < 3)
		{
			return false;
		}

		bool bInside = false;
		for (int32 Index = 0, Previous = Polygon.Num() - 1; Index < Polygon.Num(); Previous = Index++)
		{
			const FVector2D& A = Polygon[Index];
			const FVector2D& B = Polygon[Previous];
			const bool bStraddles = (A.Y > Point.Y) != (B.Y > Point.Y);
			if (bStraddles)
			{
				const float CrossingX = (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X;
				if (Point.X < CrossingX)
				{
					bInside = !bInside;
				}
			}
		}
		return bInside;
	}

	TArray<FCityBlock> ExtractBlocks(const TArray<FRoadCenterline>& Roads, float CityDiameter)
	{
		TArray<FCityBlock> Result;
		if (Roads.Num() < 3)
		{
			return Result;
		}

		TArray<TArray<float>> SplitParameters;
		SplitParameters.SetNum(Roads.Num());
		for (TArray<float>& Parameters : SplitParameters)
		{
			Parameters.Append({0.0f, 1.0f});
		}

		for (int32 AIndex = 0; AIndex < Roads.Num(); ++AIndex)
		{
			for (int32 BIndex = AIndex + 1; BIndex < Roads.Num(); ++BIndex)
			{
				float T = 0.0f;
				float U = 0.0f;
				if (FindSegmentIntersection(Roads[AIndex].Start, Roads[AIndex].End, Roads[BIndex].Start, Roads[BIndex].End, T, U))
				{
					SplitParameters[AIndex].Add(FMath::Clamp(T, 0.0f, 1.0f));
					SplitParameters[BIndex].Add(FMath::Clamp(U, 0.0f, 1.0f));
				}
			}
		}

		TArray<FPlanarVertex> Vertices;
		TArray<FHalfEdge> HalfEdges;
		TMap<uint64, int32> ExistingEdges;
		for (int32 RoadIndex = 0; RoadIndex < Roads.Num(); ++RoadIndex)
		{
			TArray<float>& Parameters = SplitParameters[RoadIndex];
			Parameters.Sort();
			for (int32 Index = Parameters.Num() - 1; Index > 0; --Index)
			{
				if (FMath::IsNearlyEqual(Parameters[Index], Parameters[Index - 1], 0.0005f))
				{
					Parameters.RemoveAt(Index);
				}
			}

			for (int32 Index = 1; Index < Parameters.Num(); ++Index)
			{
				const FVector2D Start = FMath::Lerp(Roads[RoadIndex].Start, Roads[RoadIndex].End, Parameters[Index - 1]);
				const FVector2D End = FMath::Lerp(Roads[RoadIndex].Start, Roads[RoadIndex].End, Parameters[Index]);
				AddGraphEdge(Vertices, HalfEdges, ExistingEdges, Start, End, Roads[RoadIndex]);
			}
		}

		for (FPlanarVertex& Vertex : Vertices)
		{
			Vertex.OutgoingHalfEdges.Sort([&HalfEdges](int32 A, int32 B)
			{
				return HalfEdges[A].Angle < HalfEdges[B].Angle;
			});
		}

		const float MaximumBlockArea = FMath::Square(CityDiameter) * 0.48f;
		for (int32 StartEdgeIndex = 0; StartEdgeIndex < HalfEdges.Num(); ++StartEdgeIndex)
		{
			if (HalfEdges[StartEdgeIndex].bVisited)
			{
				continue;
			}

			TArray<int32> FaceEdges;
			int32 CurrentEdgeIndex = StartEdgeIndex;
			for (int32 Step = 0; Step <= HalfEdges.Num(); ++Step)
			{
				FHalfEdge& CurrentEdge = HalfEdges[CurrentEdgeIndex];
				if (CurrentEdge.bVisited && CurrentEdgeIndex != StartEdgeIndex)
				{
					FaceEdges.Reset();
					break;
				}

				CurrentEdge.bVisited = true;
				FaceEdges.Add(CurrentEdgeIndex);
				const TArray<int32>& Outgoing = Vertices[CurrentEdge.To].OutgoingHalfEdges;
				const int32 TwinPosition = Outgoing.IndexOfByKey(CurrentEdge.Twin);
				if (TwinPosition == INDEX_NONE || Outgoing.IsEmpty())
				{
					FaceEdges.Reset();
					break;
				}

				const int32 NextPosition = (TwinPosition - 1 + Outgoing.Num()) % Outgoing.Num();
				CurrentEdgeIndex = Outgoing[NextPosition];
				if (CurrentEdgeIndex == StartEdgeIndex)
				{
					break;
				}
			}

			if (FaceEdges.Num() < 3 || CurrentEdgeIndex != StartEdgeIndex)
			{
				continue;
			}

			FCityBlock Block;
			Block.Boundary.Reserve(FaceEdges.Num());
			Block.Edges.Reserve(FaceEdges.Num());
			for (const int32 EdgeIndex : FaceEdges)
			{
				const FHalfEdge& Edge = HalfEdges[EdgeIndex];
				const FVector2D Start = Vertices[Edge.From].Position;
				const FVector2D End = Vertices[Edge.To].Position;
				Block.Boundary.Add(Start);
				FBlockEdge& BlockEdge = Block.Edges.AddDefaulted_GetRef();
				BlockEdge.Start = Start;
				BlockEdge.End = End;
				BlockEdge.RoadHalfWidth = Edge.RoadHalfWidth;
				BlockEdge.SourceRoadIndex = Edge.SourceRoadIndex;
			}

			const float Area = SignedArea(Block.Boundary);
			if (Area < MinimumBlockArea || Area > MaximumBlockArea)
			{
				continue;
			}

			Block.Area = Area;
			Block.Centroid = PolygonCentroid(Block.Boundary, Area);
			Block.Bounds = FBox2D(Block.Boundary);
			Result.Add(MoveTemp(Block));
		}

		Result.Sort([](const FCityBlock& A, const FCityBlock& B)
		{
			if (!FMath::IsNearlyEqual(A.Centroid.X, B.Centroid.X, 1.0f))
			{
				return A.Centroid.X < B.Centroid.X;
			}
			return A.Centroid.Y < B.Centroid.Y;
		});
		return Result;
	}

	void SubdivideBlockFrontages(
		const FCityBlock& Block,
		int32 BlockIndex,
		FRandomStream& Random,
		const FLotSettings& Settings,
		TArray<FCityLot>& OutLots)
	{
		for (int32 EdgeIndex = 0; EdgeIndex < Block.Edges.Num(); ++EdgeIndex)
		{
			const FBlockEdge& Edge = Block.Edges[EdgeIndex];
			const FVector2D EdgeVector = Edge.End - Edge.Start;
			const float EdgeLength = EdgeVector.Size();
			if (EdgeLength < Settings.MinimumFrontage + Settings.CornerClearance * 2.0f)
			{
				continue;
			}

			const FVector2D Along = EdgeVector / EdgeLength;
			const FVector2D Inward(-Along.Y, Along.X);
			const float UsableLength = EdgeLength - Settings.CornerClearance * 2.0f;
			const float DesiredFrontage = Random.FRandRange(Settings.MinimumFrontage, Settings.MaximumFrontage);
			const int32 LotCount = FMath::Max(1, FMath::FloorToInt(UsableLength / DesiredFrontage));
			const float ParcelFrontage = UsableLength / LotCount;
			if (ParcelFrontage < Settings.MinimumFrontage * 0.72f)
			{
				continue;
			}

			for (int32 LotIndex = 0; LotIndex < LotCount; ++LotIndex)
			{
				const float FrontCenterDistance = Settings.CornerClearance + (LotIndex + 0.5f) * ParcelFrontage;
				const float ParcelDepth = Random.FRandRange(Settings.MinimumDepth, Settings.MaximumDepth);
				const float BuildableWidth = FMath::Max(500.0f, ParcelFrontage - Settings.SideSetback * 2.0f);
				const float BuildingDepth = ParcelDepth * Random.FRandRange(0.72f, 0.92f);
				const float BuildingWidth = BuildableWidth * Random.FRandRange(0.86f, 0.98f);
				const FVector2D FrontCenter = Edge.Start + Along * FrontCenterDistance;
				const float FrontOffset = Edge.RoadHalfWidth + Settings.FrontSetback;

				FCityLot Lot;
				Lot.BlockIndex = BlockIndex;
				Lot.RoadIndex = Edge.SourceRoadIndex;
				Lot.bCornerLot = LotIndex == 0 || LotIndex == LotCount - 1;
				Lot.YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
				Lot.Footprint = FVector2D(BuildingWidth, BuildingDepth);
				Lot.Center = FrontCenter + Inward * (FrontOffset + BuildingDepth * 0.5f);

				const float HalfFrontage = ParcelFrontage * 0.5f;
				Lot.Boundary = {
					FrontCenter - Along * HalfFrontage + Inward * FrontOffset,
					FrontCenter + Along * HalfFrontage + Inward * FrontOffset,
					FrontCenter + Along * HalfFrontage + Inward * (FrontOffset + ParcelDepth),
					FrontCenter - Along * HalfFrontage + Inward * (FrontOffset + ParcelDepth)
				};

				if (IsPointInsidePolygon(Lot.Center, Block.Boundary) && AreLotCornersInside(Lot, Block.Boundary))
				{
					OutLots.Add(MoveTemp(Lot));
				}
			}
		}
	}
}
