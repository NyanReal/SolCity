#include "Editor/SolCityLandscapeLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInterface.h"
#endif

namespace SolCityLandscape
{
	constexpr int32 QuadsPerSection = 63;
	constexpr int32 SectionsPerComponent = 2;
	constexpr int32 ComponentCount = 4;
	constexpr int32 QuadCount = QuadsPerSection * SectionsPerComponent * ComponentCount;
	constexpr int32 VertexCount = QuadCount + 1;
	constexpr float LandscapeZScale = 4.0f;
	constexpr float RiverBedZ = -230.0f;

	float RiverCenterY(float X, float CityDiameter)
	{
		const float LayoutScale = CityDiameter / 18000.0f;
		const float NormalizedX = X / FMath::Max(CityDiameter * 0.5f, 1.0f);
		return (FMath::Sin(NormalizedX * PI * 1.35f) * 470.0f
			+ FMath::Sin(NormalizedX * PI * 3.1f + 0.65f) * 125.0f) * LayoutScale;
	}

	uint16 EncodeHeight(float HeightCentimeters)
	{
		const int32 Encoded = FMath::RoundToInt(32768.0f + HeightCentimeters * 128.0f / LandscapeZScale);
		return static_cast<uint16>(FMath::Clamp(Encoded, 0, 65535));
	}
}

bool USolCityLandscapeLibrary::RebuildSolCityLandscape(float CityDiameter, float RiverWidth, float RiverSurfaceZ)
{
#if WITH_EDITOR
	if (!GEditor || CityDiameter <= 0.0f || RiverWidth <= 0.0f)
	{
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return false;
	}

	TArray<ALandscape*> ExistingLandscapes;
	for (TActorIterator<ALandscape> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("SolCityLandscape")) || It->GetActorLabel().Equals(TEXT("SolCity_Landscape")))
		{
			ExistingLandscapes.Add(*It);
		}
	}
	for (ALandscape* Existing : ExistingLandscapes)
	{
		World->DestroyActor(Existing);
	}

	const float HalfCity = CityDiameter * 0.5f;
	const float XYScale = CityDiameter / static_cast<float>(SolCityLandscape::QuadCount);
	ALandscape* Landscape = World->SpawnActor<ALandscape>(FVector(-HalfCity, -HalfCity, 0.0f), FRotator::ZeroRotator);
	if (!Landscape)
	{
		return false;
	}

	Landscape->SetActorLabel(TEXT("SolCity_Landscape"));
	Landscape->Tags.AddUnique(TEXT("SolCityLandscape"));
	Landscape->SetActorRelativeScale3D(FVector(XYScale, XYScale, SolCityLandscape::LandscapeZScale));
	Landscape->StaticLightingLOD = 0;
	Landscape->LandscapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_AnimeGrass.M_AnimeGrass"));

	TArray<uint16> HeightData;
	HeightData.SetNumUninitialized(SolCityLandscape::VertexCount * SolCityLandscape::VertexCount);
	const float BedHalfWidth = RiverWidth * 0.46f;
	const float BankOuterHalfWidth = RiverWidth * 0.5f + 520.0f;
	for (int32 YIndex = 0; YIndex < SolCityLandscape::VertexCount; ++YIndex)
	{
		const float WorldY = -HalfCity + YIndex * XYScale;
		for (int32 XIndex = 0; XIndex < SolCityLandscape::VertexCount; ++XIndex)
		{
			const float WorldX = -HalfCity + XIndex * XYScale;
			const float DistanceToRiver = FMath::Abs(WorldY - SolCityLandscape::RiverCenterY(WorldX, CityDiameter));
			const float BankAlpha = FMath::Clamp(
				(DistanceToRiver - BedHalfWidth) / FMath::Max(BankOuterHalfWidth - BedHalfWidth, 1.0f),
				0.0f,
				1.0f);
			const float SmoothBankAlpha = BankAlpha * BankAlpha * (3.0f - 2.0f * BankAlpha);
			const float Height = FMath::Lerp(SolCityLandscape::RiverBedZ, 0.0f, SmoothBankAlpha);
			HeightData[YIndex * SolCityLandscape::VertexCount + XIndex] = SolCityLandscape::EncodeHeight(Height);
		}
	}

	TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
	HeightDataPerLayer.Add(FGuid(), MoveTemp(HeightData));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerData;
	MaterialLayerData.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	Landscape->Import(
		FGuid::NewGuid(),
		0,
		0,
		SolCityLandscape::QuadCount,
		SolCityLandscape::QuadCount,
		SolCityLandscape::SectionsPerComponent,
		SolCityLandscape::QuadsPerSection,
		HeightDataPerLayer,
		TEXT(""),
		MaterialLayerData,
		ELandscapeImportAlphamapType::Additive,
		TArrayView<const FLandscapeLayer>());

	if (ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo())
	{
		LandscapeInfo->UpdateLayerInfoMap(Landscape);
	}
	Landscape->MarkPackageDirty();
	Landscape->PostEditChange();
	World->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("SolCity Landscape rebuilt: %dx%d vertices, %.2fcm XY scale, river surface %.1fcm"),
		SolCityLandscape::VertexCount,
		SolCityLandscape::VertexCount,
		XYScale,
		RiverSurfaceZ);
	return true;
#else
	return false;
#endif
}
