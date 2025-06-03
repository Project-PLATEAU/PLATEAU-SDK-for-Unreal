// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "PLATEAUFeatureInfoDisplay.h"
#include "PLATEAUGeometry.h"
#include "ExtentEditor/PLATEAUExtentEditorVPClient.h"
#include "ExtentEditor/PLATEAUAsyncLoadedFeatureInfoPanel.h"

#include <plateau/basemap/tile_projection.h>
#include <plateau/basemap/vector_tile_downloader.h>

#include <Async/Async.h>
#include <plateau/dataset/mesh_code.h>
#include <plateau/dataset/i_dataset_accessor.h>

#include "PLATEAURuntime.h"
#include "StaticMeshAttributes.h"
#include "PLATEAUTextureLoader.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace plateau::dataset;
using namespace UE::Tasks;

namespace {
    /**
     * @brief 範囲選択画面に表示する画像名
     */
    constexpr TCHAR BuildingIcon[]      = TEXT("building.png");
    constexpr TCHAR TrafficIcon[]       = TEXT("traffic.png");
    constexpr TCHAR PropsIcon[]         = TEXT("props.png");
    constexpr TCHAR BridgeIcon[]        = TEXT("bridge.png");
    constexpr TCHAR PlantsIcon[]        = TEXT("plants.png");
    constexpr TCHAR UndergroundIcon[]   = TEXT("underground.png");
    constexpr TCHAR TerrainIcon[]       = TEXT("terrain.png");
    constexpr TCHAR OtherIcon[]         = TEXT("other.png");
    
    /**
     * @brief キーに対応するテクスチャのパスを取得します。
     * @return
     */
    FString GetTexturePath(const FPLATEAUFeatureInfoMaterialKey& MaterialKey) {
        const FString WithTextDirName = "AreaIcon_WithText/";
        const FString NoTextDirName = "AreaIcon_NoText/";
        const TArray<FString> LodDirNames = { "LOD01/", "LOD01/", "LOD2/", "LOD3/", "LOD4/" };

        FString Path = FPLATEAURuntimeModule::GetContentDir() + "/";
        Path += MaterialKey.bDetailed ? WithTextDirName : NoTextDirName;

        const auto Lod = FMath::Clamp(MaterialKey.Lod, plateau::Feature::MinLod, plateau::Feature::MaxLod);
        Path += LodDirNames[Lod];
        Path += FPLATEAUFeatureInfoDisplay::GetIconFileName(MaterialKey.Package);

        return Path;
    }

    UMaterialInstanceDynamic* CreateMaterial(UMaterial* BaseMaterial, const FPLATEAUFeatureInfoMaterialKey& MaterialKey) {
        const auto Material = UMaterialInstanceDynamic::Create(BaseMaterial, GetTransientPackage());
        const auto Texture = FPLATEAUTextureLoader::LoadTransient(GetTexturePath(MaterialKey));
        Material->SetTextureParameterValue(TEXT("Texture"), Texture);
        Material->SetScalarParameterValue(TEXT("Multiplier"), 1.0f);
        Material->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
        return Material;
    }

    UMaterialInstanceDynamic* CreateBackPanelMaterial(UMaterial* BaseMaterial) {
        const auto Texture = FPLATEAUTextureLoader::LoadTransient(FPLATEAURuntimeModule::GetContentDir() + "/round-button.png");
        const auto Material = UMaterialInstanceDynamic::Create(BaseMaterial, GetTransientPackage());
        Material->SetTextureParameterValue(TEXT("Texture"), Texture);
        Material->SetScalarParameterValue(TEXT("Multiplier"), 0.01f);
        Material->SetScalarParameterValue(TEXT("Opacity"), 0.6f);
        return Material;
    }

    std::shared_ptr<std::vector<GmlFile>> FindGmlFiles(
        const IDatasetAccessor& InDatasetAccessor,
        const std::shared_ptr<GridCode>& InGridCode,
        const PredefinedCityModelPackage InPackage) {

        return InDatasetAccessor
            .filterByGridCodes({ InGridCode })
            ->getGmlFiles(InPackage);
    }
}

FPLATEAUFeatureInfoDisplay::FPLATEAUFeatureInfoDisplay(
    const FPLATEAUGeoReference& InGeoReference,
    const TSharedPtr<FPLATEAUExtentEditorViewportClient> InViewportClient)
    : GeoReference(InGeoReference)
    , ViewportClient(InViewportClient)
{
    ShowLods.Reset();
    for (int Lod = 0; Lod <= plateau::Feature::MaxLod; ++Lod) {
        ShowLods.Emplace(Lod);
    }
    
    InitializeMaterials();
}

FPLATEAUFeatureInfoDisplay::~FPLATEAUFeatureInfoDisplay() {}

bool FPLATEAUFeatureInfoDisplay::CreatePanelAsync(const FPLATEAUGridCodeGizmo& MeshCodeGizmo, const IDatasetAccessor& InDatasetAccessor) {
    // 生成済みの場合はスキップ
    if (GridCodeGizmoContains(MeshCodeGizmo))
        return false;

    const auto AsyncLoadedTile = MakeShared<FPLATEAUAsyncLoadedFeatureInfoPanel>(SharedThis(this), ViewportClient);
    AsyncLoadedPanels.Add(MeshCodeGizmo.GetRegionGridCodeID(), AsyncLoadedTile);

    FPLATEAUFeatureInfoPanelInput Input;
    const auto Packages = GetDisplayedPackages();
    for (const auto& Package : GetDisplayedPackages()) {
        Input.Add(Package, FindGmlFiles(InDatasetAccessor, MeshCodeGizmo.GetGridCode(), Package));
    }

    const auto TileExtent = MeshCodeGizmo.GetGridCode()->getExtent();
    const auto RawTileMax = GeoReference.GetData().project(TileExtent.max);
    const auto RawTileMin = GeoReference.GetData().project(TileExtent.min);
    const FBox Box{FVector(RawTileMin.x, RawTileMin.y, RawTileMin.z), FVector(RawTileMax.x, RawTileMax.y, RawTileMax.z)};

    AsyncLoadedTile->LoadMaxLodAsync(Input, Box);

    return true;
}

int FPLATEAUFeatureInfoDisplay::CountLoadingPanels() {
    int Count = 0;
    for (const auto& Entry : AsyncLoadedPanels) {
        if (Entry.Value->GetLoadMaxLodTaskStatus() == EPLATEAUFeatureInfoPanelStatus::Loading)
            ++Count;
    }
    return Count;
}

bool FPLATEAUFeatureInfoDisplay::AddComponent(const FPLATEAUGridCodeGizmo& MeshCodeGizmo) {
    if (GridCodeGizmoContains(MeshCodeGizmo)) {
        return AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->AddIconComponent();
    }

    return false;
}

UMaterialInstanceDynamic* FPLATEAUFeatureInfoDisplay::GetFeatureInfoIconMaterial(
    const FPLATEAUFeatureInfoMaterialKey& Key) {
    if (!FeatureInfoMaterials.Find(Key))
        return nullptr;

    return FeatureInfoMaterials[Key];
}

UMaterialInstanceDynamic* FPLATEAUFeatureInfoDisplay::GetBackPanelMaterial() const {
    return BackPanelMaterial;
}

EPLATEAUFeatureInfoVisibility FPLATEAUFeatureInfoDisplay::GetVisibility() const {
    return Visibility;
}

void FPLATEAUFeatureInfoDisplay::SetVisibility(const FPLATEAUGridCodeGizmo& MeshCodeGizmo, const EPLATEAUFeatureInfoVisibility Value) {
    Visibility = Value;
    if (GridCodeGizmoContains(MeshCodeGizmo)) {
        AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->RecalculateIconTransform(ShowLods);
        AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->SetFeatureInfoVisibility(ShowLods, Visibility);
    }
}

TArray<PredefinedCityModelPackage> FPLATEAUFeatureInfoDisplay::GetDisplayedPackages() {
    static TArray DisplayedPackages = {
        PredefinedCityModelPackage::Building,
        PredefinedCityModelPackage::Road,
        PredefinedCityModelPackage::UrbanPlanningDecision,
        PredefinedCityModelPackage::LandUse,
        PredefinedCityModelPackage::CityFurniture,
        PredefinedCityModelPackage::Vegetation,
        PredefinedCityModelPackage::Relief,
        PredefinedCityModelPackage::DisasterRisk,
        PredefinedCityModelPackage::Railway,
        PredefinedCityModelPackage::Waterway,
        PredefinedCityModelPackage::WaterBody,
        PredefinedCityModelPackage::Bridge,
        PredefinedCityModelPackage::Track,
        PredefinedCityModelPackage::Square,
        PredefinedCityModelPackage::Tunnel,
        PredefinedCityModelPackage::UndergroundFacility,
        PredefinedCityModelPackage::UndergroundBuilding,
        PredefinedCityModelPackage::Area,
        PredefinedCityModelPackage::OtherConstruction,
        PredefinedCityModelPackage::Generic,
        PredefinedCityModelPackage::Unknown,
    };

    return DisplayedPackages;
}

FString FPLATEAUFeatureInfoDisplay::GetIconFileName(const PredefinedCityModelPackage Package) {
    switch (Package) {
    case PredefinedCityModelPackage::Building:              return BuildingIcon;
    case PredefinedCityModelPackage::Road:                  return TrafficIcon;
    case PredefinedCityModelPackage::UrbanPlanningDecision: return OtherIcon;
    case PredefinedCityModelPackage::LandUse:               return OtherIcon;
    case PredefinedCityModelPackage::CityFurniture:         return PropsIcon;
    case PredefinedCityModelPackage::Vegetation:            return PlantsIcon;
    case PredefinedCityModelPackage::Relief:                return TerrainIcon;
    case PredefinedCityModelPackage::DisasterRisk:          return OtherIcon;
    case PredefinedCityModelPackage::Railway:               return TrafficIcon;
    case PredefinedCityModelPackage::Waterway:              return TrafficIcon;
    case PredefinedCityModelPackage::WaterBody:             return OtherIcon;
    case PredefinedCityModelPackage::Bridge:                return BridgeIcon;
    case PredefinedCityModelPackage::Track:                 return TrafficIcon;
    case PredefinedCityModelPackage::Square:                return TrafficIcon;
    case PredefinedCityModelPackage::Tunnel:                return BridgeIcon;
    case PredefinedCityModelPackage::UndergroundFacility:   return UndergroundIcon;
    case PredefinedCityModelPackage::UndergroundBuilding:   return UndergroundIcon;
    case PredefinedCityModelPackage::Area:                  return OtherIcon;
    case PredefinedCityModelPackage::OtherConstruction:     return OtherIcon;
    case PredefinedCityModelPackage::Generic:               return OtherIcon;
    case PredefinedCityModelPackage::Unknown:               return OtherIcon;
    default:
        UE_LOG(LogTemp, Error, TEXT("An icon for an unregistered package was requested."));
        return OtherIcon;
    }
}

TArray<FString> FPLATEAUFeatureInfoDisplay::GetIconFileNameList() {
    return TArray<FString> {BuildingIcon, TrafficIcon, PropsIcon, BridgeIcon, PlantsIcon, UndergroundIcon, TerrainIcon, OtherIcon};
}

void FPLATEAUFeatureInfoDisplay::SwitchFeatureInfoDisplay(const TArray<FPLATEAUGridCodeGizmo>& MeshCodeGizmos, const int Lod, const bool bCheck) {
    if (bCheck) {
        ShowLods.AddUnique(Lod);
    } else {
        ShowLods.Remove(Lod);
    }

    for (const auto& MeshCodeGizmo : MeshCodeGizmos) {
        if (GridCodeGizmoContains(MeshCodeGizmo)) {
            AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->RecalculateIconTransform(ShowLods);
            AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->SetFeatureInfoVisibility(ShowLods, Visibility, true);
        }
    }
}

void FPLATEAUFeatureInfoDisplay::InitializeMaterials() {
    const auto BaseMaterial = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/PLATEAU-SDK-for-Unreal/FeatureInfoPanel_PanelIcon")));

    BackPanelMaterial = CreateBackPanelMaterial(BaseMaterial);

    // パッケージ、LOD等、地物情報アイコンを全種類列挙
    TArray<FPLATEAUFeatureInfoMaterialKey> Keys;
    for (const auto& Package : GetDisplayedPackages()) {
        for (int Lod = 0; Lod <= plateau::Feature::MaxLod; ++Lod) {
            Keys.Add({ Package, Lod, false });
            Keys.Add({ Package, Lod, true });
        }
    }

    // 全てのパターンについて地物情報アイコンのマテリアルを生成して保持
    for (const auto& Key : Keys) {
        FeatureInfoMaterials.Add(Key, CreateMaterial(BaseMaterial, Key));
    }
}
