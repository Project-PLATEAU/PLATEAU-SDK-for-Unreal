// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUAsyncLoadedFeatureInfoPanel.h"
#include "PLATEAUGeometry.h"
#include "PLATEAUGridCodeGizmo.h"

namespace plateau::Feature {
    constexpr int MinLod = 0;
    constexpr int MaxLod = 4;
    constexpr int MaxIconCol = 4;
    constexpr int MaxIconRow = 2;
    constexpr int MaxIconCnt = 8;
}

namespace plateau::dataset {
    class MeshCode;
    class GmlFile;
    class IDatasetAccessor;
    enum class PredefinedCityModelPackage : uint32;
}

enum class EPLATEAUFeatureInfoVisibility : uint8_t {
    Hidden = 0,
    Visible = 1,
    Detailed = 2,
};

/**
 * @brief 地物情報パネルのマテリアルを検索するためのキーです。
 */
struct FPLATEAUFeatureInfoMaterialKey {
    plateau::dataset::PredefinedCityModelPackage Package;
    int Lod;
    bool bDetailed;
};

inline bool operator==(const FPLATEAUFeatureInfoMaterialKey& A, const FPLATEAUFeatureInfoMaterialKey& B) {
    return A.Package == B.Package && A.Lod == B.Lod && A.bDetailed == B.bDetailed;
}

inline bool operator!=(const FPLATEAUFeatureInfoMaterialKey& A, const FPLATEAUFeatureInfoMaterialKey& B) {
    return A.Package != B.Package || A.Lod != B.Lod || A.bDetailed != B.bDetailed;
}

inline uint32 GetTypeHash(const FPLATEAUFeatureInfoMaterialKey& Key) {
    // ビット配列：D00LLL00PPPPP
    return
        FMath::FloorLog2(static_cast<uint32>(Key.Package)) // 0 ~ 31(5bits)
        + (Key.Lod << 7) // 0 ~ 4(3bits)
        + ((Key.bDetailed ? 1 : 0) << 12);
}

/**
 * 全ての地物情報パネルを管理します。UpdateAsyncを毎Tick呼び出す必要があります。
 */
class FPLATEAUFeatureInfoDisplay : public TSharedFromThis<FPLATEAUFeatureInfoDisplay> {
public:
    FPLATEAUFeatureInfoDisplay(const FPLATEAUGeoReference& InGeoReference, const TSharedPtr<class FPLATEAUExtentEditorViewportClient> InViewportClient);
    ~FPLATEAUFeatureInfoDisplay();

    bool CreatePanelAsync(const FPLATEAUGridCodeGizmo& MeshCodeGizmo, const plateau::dataset::IDatasetAccessor& InDatasetAccessor);
    bool AddComponent(const FPLATEAUGridCodeGizmo& MeshCodeGizmo);

    int CountLoadingPanels();

    UMaterialInstanceDynamic* GetFeatureInfoIconMaterial(const FPLATEAUFeatureInfoMaterialKey& Key);
    UMaterialInstanceDynamic* GetBackPanelMaterial() const;

    /**
     * @brief 地物情報パネルの可視性を取得します。
     */
    EPLATEAUFeatureInfoVisibility GetVisibility() const;

    /**
     * @brief 地物情報パネルの可視性を設定します。
     */
    void SetVisibility(const FPLATEAUGridCodeGizmo& MeshCodeGizmo, const EPLATEAUFeatureInfoVisibility Value);

    /**
     * @brief 範囲選択画面に表示する各パッケージリストを取得
     */    
    static TArray<plateau::dataset::PredefinedCityModelPackage> GetDisplayedPackages();

    /**
     * @brief 各パッケージの範囲選択画面に表示するアイコンファイル名リストを取得
     */
    static FString GetIconFileName(const plateau::dataset::PredefinedCityModelPackage Package);

    /**
     * @brief 範囲選択画面に表示する順番でアイコンファイル名リストを取得
     */  
    static TArray<FString> GetIconFileNameList();

    bool GridCodeGizmoContains(const FPLATEAUGridCodeGizmo& MeshCodeGizmo) const {
        return AsyncLoadedPanels.Contains(MeshCodeGizmo.GetRegionGridCodeID());
    }
    
    int GetItemCount(const FPLATEAUGridCodeGizmo& MeshCodeGizmo) {
        if (GridCodeGizmoContains(MeshCodeGizmo)) {
            return AsyncLoadedPanels[MeshCodeGizmo.GetRegionGridCodeID()].Get()->GetIconCount();
        }
        return 0;
    }

    void SwitchFeatureInfoDisplay(const TArray<FPLATEAUGridCodeGizmo>& MeshCodeGizmos, const int Lod, const bool bCheck);
private:
    FPLATEAUGeoReference GeoReference;
    TWeakPtr<class FPLATEAUExtentEditorViewportClient> ViewportClient;

    EPLATEAUFeatureInfoVisibility Visibility;
    TMap<FString, TSharedPtr<FPLATEAUAsyncLoadedFeatureInfoPanel>> AsyncLoadedPanels;
    TMap<FPLATEAUFeatureInfoMaterialKey, UMaterialInstanceDynamic*> FeatureInfoMaterials;
    UMaterialInstanceDynamic* BackPanelMaterial;
    TArray<int> ShowLods;

    void InitializeMaterials();
};
