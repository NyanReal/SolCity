#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityWorldActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

/**
 * A lightweight oriented box describing one pedestrian-capable bridge deck.
 * Transform is expressed in world space when returned by GetBridgeAreas().
 */
USTRUCT(BlueprintType)
struct SOLCITY_API FCityBridgeArea
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sol City|Bridge")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sol City|Bridge")
	FVector Extent = FVector::ZeroVector;

	bool Contains(const FVector& WorldPoint) const
	{
		const FVector LocalPoint = Transform.InverseTransformPosition(WorldPoint);
		return FMath::Abs(LocalPoint.X) <= Extent.X
			&& FMath::Abs(LocalPoint.Y) <= Extent.Y
			&& FMath::Abs(LocalPoint.Z) <= Extent.Z;
	}
};

/**
 * Procedurally builds the flat, colourful environment used by Sol City.
 *
 * All placement is made from instanced / hierarchical-instanced Engine basic
 * shapes, so the city remains inexpensive even without any authored meshes.
 * Query functions accept and return world-space positions.
 */
UCLASS(Blueprintable)
class SOLCITY_API ACityWorldActor : public AActor
{
	GENERATED_BODY()

public:
	ACityWorldActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Rebuilds every procedural instance using the current settings. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sol City|Generation")
	void GenerateCity();

	/** True only on the drivable part of a road (sidewalks are excluded). */
	UFUNCTION(BlueprintPure, Category = "Sol City|Queries")
	bool IsRoadAt(const FVector& WorldLocation) const;

	/** True for the geometric river footprint, including water below bridges. */
	UFUNCTION(BlueprintPure, Category = "Sol City|Queries")
	bool IsRiverAt(const FVector& WorldLocation) const;

	/** True on grass, parks, sidewalks, and bridge sidewalks, but not buildings or roads. */
	UFUNCTION(BlueprintPure, Category = "Sol City|Queries")
	bool IsWalkableAt(const FVector& WorldLocation) const;

	/** Returns a world-space point suitable for a citizen. */
	UFUNCTION(BlueprintCallable, Category = "Sol City|Queries")
	FVector GetRandomWalkablePoint() const;

	/** Returns world-space bridge volumes. */
	UFUNCTION(BlueprintPure, Category = "Sol City|Queries")
	TArray<FCityBridgeArea> GetBridgeAreas() const;

	UFUNCTION(BlueprintPure, Category = "Sol City|Queries")
	float GetCityHalfSize() const;

	/** Number of full road-to-road intervals on either side of the origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Layout", meta = (ClampMin = "1", UIMin = "1", UIMax = "12"))
	int32 GridHalfExtent = 5;

	/** Radius that keeps the original dense city profile before suburb falloff begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Layout", meta = (ClampMin = "1", UIMin = "1", UIMax = "8"))
	int32 CoreGridHalfExtent = 3;

	/** Visual target size for an individual building lot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Layout", meta = (ClampMin = "250.0", UIMin = "250.0"))
	float BlockSize = 560.0f;

	/** Size of one simple ground / water tile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Layout", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float CellSize = 400.0f;

	/** Distance between the centre lines of adjacent parallel roads. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Layout", meta = (ClampMin = "800.0", UIMin = "800.0"))
	float RoadSpacing = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Road", meta = (ClampMin = "200.0", UIMin = "200.0"))
	float RoadWidth = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Road", meta = (ClampMin = "60.0", UIMin = "60.0"))
	float SidewalkWidth = 150.0f;

	/** River centre in actor-local X. Keeping it between road lines avoids a road running down the river. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River")
	float RiverX = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River", meta = (ClampMin = "150.0", UIMin = "150.0"))
	float RiverHalfWidth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River", meta = (ClampMin = "0.0"))
	float WaterFlowSpeed = 0.065f;

	/** Optional override. A conventional /Game/.../M_WaterAnimated asset is also auto-detected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River")
	TSoftObjectPtr<UMaterialInterface> WaterMaterialAsset;

	/** Counts are consumed by the gameplay spawner; this actor deliberately does not own those classes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Population", meta = (ClampMin = "0", UIMin = "0"))
	int32 VehicleCount = 28;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Population", meta = (ClampMin = "0", UIMin = "0"))
	int32 CitizenCount = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation")
	int32 GenerationSeed = 5731;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float ParkRatio = 0.22f;

	/** Occupancy chance at the outermost block; the core keeps its original 0.84 chance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation", meta = (ClampMin = "0.05", ClampMax = "0.84"))
	float OuterLotOccupancy = 0.18f;

	/** Footprint scale of buildings at the map edge relative to core buildings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation", meta = (ClampMin = "0.45", ClampMax = "1.0"))
	float OuterBuildingScale = 0.68f;

	/** Maximum edge-building height, blended from the core's original 1060uu maximum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation", meta = (ClampMin = "300.0", ClampMax = "900.0"))
	float OuterMaxBuildingHeight = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation")
	bool bGenerateOnConstruction = true;

protected:
	virtual void BeginPlay() override;

private:
	void ClearInstances();
	void ConfigureMaterials();
	void GenerateGroundAndRiver();
	void GenerateRoadNetwork();
	void GenerateBlocks();
	void GenerateRoadsideTrees(FRandomStream& RandomStream);
	void EnsureDaylightActors();
	void AddBuilding(const FVector2D& Centre, const FVector2D& Size, float Height, int32 PaletteIndex, FRandomStream& RandomStream);
	void AddTree(const FVector2D& Centre, float SizeScale = 1.0f);
	void AddWindows(const FVector2D& Centre, const FVector2D& Size, float Height);

	bool IsRoadAtLocal(const FVector2D& LocalPoint) const;
	bool IsRiverAtLocal(const FVector2D& LocalPoint) const;
	bool IsBridgeAtLocal(const FVector2D& LocalPoint) const;
	bool IsWalkableAtLocal(const FVector2D& LocalPoint) const;
	float GetSurfaceHeightAtLocal(const FVector2D& LocalPoint) const;
	float GetSafeRoadSpacing() const;
	float GetSafeRoadWidth() const;
	UMaterialInstanceDynamic* MakeColourMaterial(const FLinearColor& Colour, const TCHAR* DebugName);

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ParkTiles;

	/** Narrow decorative paths layered over grass-based park blocks. */
	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ParkPaths;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RiverTiles;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Roads;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Sidewalks;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadMarkings;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Bridges;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BridgePillars;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingPeach;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingYellow;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingMint;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingBlue;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingCafe;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingApartment;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingTownhouse;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Roofs;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Windows;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeTrunks;

	UPROPERTY(VisibleAnywhere, Category = "Sol City|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeCrowns;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;

	TArray<FCityBridgeArea> LocalBridgeAreas;
	TArray<FBox2D> BuildingFootprints;

	float WaterAnimationTime = 0.0f;
	bool bUsingAnimatedWaterMaterial = false;
};
