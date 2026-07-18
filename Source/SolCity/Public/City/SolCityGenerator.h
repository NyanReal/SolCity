#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolCityGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class ESolCityRoadClass : uint8
{
	Arterial,
	Collector,
	Local,
	Bridge
};

USTRUCT(BlueprintType)
struct SOLCITY_API FSolCityRoadSegment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
	FVector End = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
	float HalfWidth = 350.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
	ESolCityRoadClass RoadClass = ESolCityRoadClass::Local;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
	bool bBridge = false;
};

/**
 * Runtime city massing generator.  The output intentionally uses only engine
 * primitive meshes, so the actor also works in a content-empty C++ project.
 * Art materials can be assigned in a Blueprint child without changing the
 * deterministic layout.
 */
UCLASS(BlueprintType, Blueprintable)
class SOLCITY_API ASolCityGenerator : public AActor
{
	GENERATED_BODY()

public:
	ASolCityGenerator();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Sol City|Generation")
	void RegenerateCity();

	UFUNCTION(BlueprintPure, Category = "Sol City|Navigation")
	TArray<FSolCityRoadSegment> GetRoadSegments() const { return RoadSegments; }

	UFUNCTION(BlueprintPure, Category = "Sol City|Navigation")
	TArray<FVector> GetPedestrianWaypoints() const { return PedestrianWaypoints; }

	UFUNCTION(BlueprintPure, Category = "Sol City|Navigation")
	TArray<FVector> GetTrafficSignalLocations() const { return TrafficSignalLocations; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation")
	int32 Seed = 71527;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation", meta = (ClampMin = "6000.0", UIMin = "6000.0"))
	float CityDiameter = 36000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River", meta = (ClampMin = "500.0", ClampMax = "2500.0"))
	float RiverWidth = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River")
	float RiverSurfaceZ = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|River")
	float WaterPanSpeed = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings", meta = (ClampMin = "12", ClampMax = "220"))
	int32 TargetBuildingCount = 112;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation")
	bool bGenerateInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Generation")
	bool bRegenerateAtBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Materials")
	TObjectPtr<UMaterialInterface> GroundMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Materials")
	TObjectPtr<UMaterialInterface> RoadMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Materials")
	TObjectPtr<UMaterialInterface> SidewalkMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Materials")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Materials")
	TObjectPtr<UMaterialInterface> BridgeMaterial;

	/** Optional authored FBX used as a varied mid-rise among procedural masses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings")
	TObjectPtr<UStaticMesh> AuthoredMidRiseMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings")
	TObjectPtr<UStaticMesh> AuthoredCornerRetailMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings")
	TObjectPtr<UStaticMesh> AuthoredSteppedTowerMesh;

	/** Fixed-height authored modules used for every variable-height procedural building. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings|Modules")
	TObjectPtr<UStaticMesh> BuildingBaseModuleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings|Modules")
	TObjectPtr<UStaticMesh> BuildingMiddleModuleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Buildings|Modules")
	TObjectPtr<UStaticMesh> BuildingCrownModuleMesh;

	/** X-forward, longitudinally subdivided road section deformed by spline meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Roads")
	TObjectPtr<UStaticMesh> AuthoredRoadSplineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Roads")
	TObjectPtr<UStaticMesh> AuthoredRoadJunctionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Bridge")
	TObjectPtr<UStaticMesh> AuthoredBridgeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sol City|Foliage")
	TObjectPtr<UStaticMesh> AuthoredTreeMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sol City|Navigation")
	TArray<FSolCityRoadSegment> RoadSegments;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sol City|Navigation")
	TArray<FVector> PedestrianWaypoints;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sol City|Navigation")
	TArray<FVector> TrafficSignalLocations;

protected:
	virtual void BeginPlay() override;

private:
	struct FBuildingFootprint
	{
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Extent = FVector2D::ZeroVector;
	};

	UPROPERTY(VisibleAnywhere, Category = "Sol City")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultSurfaceMaterial;

	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SidewalkInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingBaseModuleInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingMiddleModuleInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingCrownModuleInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> AuthoredBuildingInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> AuthoredCornerRetailInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> AuthoredSteppedTowerInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BridgeInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> AuthoredBridgeInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> JunctionInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DetailInstances;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CylinderInstances;
	TObjectPtr<UProceduralMeshComponent> WaterSurfaceMesh;

	TArray<FBuildingFootprint> OccupiedBuildings;
	TArray<FVector2D> JunctionCapLocations;
	FRandomStream Random;
	float WaterTime = 0.0f;
	bool bHasGenerated = false;

	void ClearGeneratedComponents();
	void CreateInstanceGroups();
	void FinalizeInstanceGroups();
	UHierarchicalInstancedStaticMeshComponent* CreateInstanceGroup(
		const FName Name,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		const FLinearColor& Tint,
		bool bCollision,
		bool bApplyTint = true);

	void GenerateGroundAndRiver();
	void GenerateRoadHierarchy();
	void GenerateBuildings();
	void GenerateTrees();
	void GenerateBridge();
	void GenerateTrafficFurniture();

	void AddBox(UHierarchicalInstancedStaticMeshComponent* Group, const FVector& Center, const FVector& Size, float YawDegrees = 0.0f);
	void AddCylinder(UHierarchicalInstancedStaticMeshComponent* Group, const FVector& Center, float Radius, float Height, float YawDegrees = 0.0f);
	void AddRoadSegment(const FVector& Start, const FVector& End, float Width, ESolCityRoadClass RoadClass, bool bAddSidewalks = true, bool bBridge = false, bool bCreateVisual = true);
	void AddPolylineRoad(const TArray<FVector>& Points, float Width, ESolCityRoadClass RoadClass, bool bAddSidewalks = true);
	void AddSplineRoadVisual(const TArray<FVector>& Points, float Width);
	void AddJunctionCap(const FVector& Position, float Width);
	void AddBuildingMass(const FVector2D& Center, const FVector2D& Footprint, float YawDegrees, int32 Style, float Height);
	bool AddAuthoredBuilding(
		UHierarchicalInstancedStaticMeshComponent* Group,
		UStaticMesh* Mesh,
		const FVector2D& Center,
		float YawDegrees,
		float UniformScale);
	float GetAuthoredBuildingUniformScale(UStaticMesh* Mesh) const;
	FVector2D GetAuthoredBuildingExtent(UStaticMesh* Mesh, float YawDegrees, float UniformScale) const;

	float RiverCenterY(float X) const;
	bool IsInsideRiver(const FVector2D& Point, float Margin) const;
	bool IsBuildingSiteFree(const FVector2D& Center, const FVector2D& Extent) const;
	bool IsBuildingClearOfRoads(const FVector2D& Center, const FVector2D& LocalSize, float YawDegrees) const;
	bool IsNearRoad(const FVector2D& Point, float MaxDistance) const;
	float DistanceToSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B) const;
};
