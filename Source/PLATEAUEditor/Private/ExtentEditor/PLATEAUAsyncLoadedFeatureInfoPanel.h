// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUGeometry.h"
#include "Tasks/Task.h"

struct FPLATEAUFeatureInfoMaterialKey;
class FPLATEAUGridCodeGizmo;

namespace plateau::dataset {
    class MeshCode;
    class GmlFile;
    class IDatasetAccessor;
    enum class PredefinedCityModelPackage : uint32;
}

enum class EPLATEAUFeatureInfoVisibility : uint8_t;

typedef TMap<plateau::dataset::PredefinedCityModelPackage, std::shared_ptr<std::vector<plateau::dataset::GmlFile>>> FPLATEAUFeatureInfoPanelInput;

enum class EPLATEAUFeatureInfoPanelStatus
{
    Idle,
    Loading,
    FullyLoaded
};

/**
 * @brief 各メッシュコード(グリッド)に表示する地物情報のパネルを表します。
 */
class FPLATEAUAsyncLoadedFeatureInfoPanel {
public:
    explicit FPLATEAUAsyncLoadedFeatureInfoPanel(const TWeakPtr<class FPLATEAUFeatureInfoDisplay> Owner,
                                                 const TWeakPtr<class FPLATEAUExtentEditorViewportClient> ViewportClient);

    ~FPLATEAUAsyncLoadedFeatureInfoPanel() {
    }

    /**
     * @brief 地物情報パネルの可視性を取得します。
     */
    EPLATEAUFeatureInfoVisibility GetFeatureInfoVisibility() const;

    /**
     * @brief 地物情報パネルの可視性を設定します。
     */
    void SetFeatureInfoVisibility(const TArray<int>& ShowLods, const EPLATEAUFeatureInfoVisibility Value, const bool bForce = false);
    
    /**
     * @brief GMLファイルの一覧を入力として、非同期に地物の最大LOD情報を読み込みます。
     * パネルの可視化は読み込みが完了した後にTickが呼び出された際に行われます。
     *
     * @param Input GMLファイルの一覧
     * @param InBox パネルの表示範囲
     */
    void LoadMaxLodAsync(const FPLATEAUFeatureInfoPanelInput& Input, const FBox& InBox);

    /**
     * @brief アイコンコンポーネント追加
     */
    bool AddIconComponent();

    int GetIconCount() const {
        return IconComponents.Num();
    }

    EPLATEAUFeatureInfoPanelStatus GetLoadMaxLodTaskStatus() const {
        return MaxLodTaskStatus;
    }

    void RecalculateIconTransform(const TArray<int>& ShowLods);
private:
    TWeakPtr<FPLATEAUFeatureInfoDisplay> Owner;
    TWeakPtr<FPLATEAUExtentEditorViewportClient> ViewportClient;

    UE::Tasks::TTask<TMap<plateau::dataset::PredefinedCityModelPackage, int>> GetMaxLodTask;

    int AddedIconComponentCnt;
    int AddedDetailedIconComponentCnt;
    float DeltaTime;
    TAtomic<EPLATEAUFeatureInfoPanelStatus> MaxLodTaskStatus;
    TAtomic<EPLATEAUFeatureInfoPanelStatus> CreateComponentStatus;
    TAtomic<EPLATEAUFeatureInfoPanelStatus> AddComponentStatus;
    EPLATEAUFeatureInfoVisibility FeatureInfoVisibility;
    FBox Box;
    TArray<USceneComponent*> IconComponents;
    TArray<UMaterialInstanceDynamic*> IconMaterialInstanceDynamics;
    TArray<USceneComponent*> DetailedIconComponents;
    TArray<UMaterialInstanceDynamic*> DetailedIconMaterialInstanceDynamics;
    TArray<FPLATEAUFeatureInfoMaterialKey> FeatureInfoMaterialMaps;

    void ApplyFeatureInfoVisibility(const TArray<int>& ShowLods) const;
    void CreatePanelComponents(const TMap<plateau::dataset::PredefinedCityModelPackage, int>& MaxLods);
};
