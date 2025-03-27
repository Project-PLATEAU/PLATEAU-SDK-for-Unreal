// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "PLATEAUModelFiltering.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Util/PLATEAUGmlUtil.h"
#include <Misc/DefaultValueHelper.h>
#include <CityGML/PLATEAUCityGmlProxy.h>
#include "Misc/EngineVersionComparison.h"

FPLATEAUModelFiltering::FPLATEAUModelFiltering() {
}

/**
 * @brief 対象コンポーネントとその子コンポーネントのコリジョン設定変更
 * @param ParentComponent コリジョン設定変更対象コンポーネント
 * @param bCollisionResponseBlock コリジョンをブロック設定に変更するか？
 * @param bPropagateToChildren 子コンポーネントのコリジョン設定を変更するか？
 */
void FPLATEAUModelFiltering::ApplyCollisionResponseBlockToChannel(USceneComponent* ParentComponent, const bool bCollisionResponseBlock, const bool bPropagateToChildren) {
    if (const auto& ParentStaticMeshComponent = Cast<UStaticMeshComponent>(ParentComponent); ParentStaticMeshComponent != nullptr) {
        ParentStaticMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, bCollisionResponseBlock ? ECR_Block : ECR_Ignore);
    }

    if (!bPropagateToChildren)
        return;

    if (const TArray<USceneComponent*>& AttachedChildren = ParentComponent->GetAttachChildren(); 0 < AttachedChildren.Num()) {
        TInlineComponentArray<USceneComponent*, NumInlinedActorComponents> ComponentStack;
        ComponentStack.Append(AttachedChildren);
        while (0 < ComponentStack.Num()) {
#if UE_VERSION_NEWER_THAN(5, 5, 0)
            if (const auto& CurrentComp = ComponentStack.Pop(/*bAllowShrinking=*/ EAllowShrinking::No); CurrentComp != nullptr) {
#else
            if (const auto& CurrentComp = ComponentStack.Pop(/*bAllowShrinking=*/ false); CurrentComp != nullptr) {
#endif
                ComponentStack.Append(CurrentComp->GetAttachChildren());
                if (const auto& StaticMeshComponent = Cast<UStaticMeshComponent>(CurrentComp); StaticMeshComponent != nullptr) {
                    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, bCollisionResponseBlock ? ECR_Block : ECR_Ignore);
                }
            }
        }
    }
}

void FPLATEAUModelFiltering::FilterLowLods(const USceneComponent* const InGmlComponent, const int MinLod, const int MaxLod) {
    const TArray<USceneComponent*>& AttachedLodChildren = InGmlComponent->GetAttachChildren();

    // 各LODに対して形状データ(コンポーネント)が存在するコンポーネント名を検索
    TMap<int, TSet<FString>> NameMap;
    for (const auto& LodComponent : AttachedLodChildren) {
        const auto Lod = FPLATEAUComponentUtil::ParseLodComponent(LodComponent);
        auto& Value = NameMap.Add(Lod);

        if (Lod < MinLod || Lod > MaxLod)
            continue;

        TArray<USceneComponent*> FeatureComponents;
        LodComponent->GetChildrenComponents(false, FeatureComponents);
        for (const auto FeatureComponent : FeatureComponents) {
            Value.Add(FPLATEAUComponentUtil::GetOriginalComponentName(FeatureComponent));
        }
    }

    // フィルタリング実行
    for (const auto& LodComponent : AttachedLodChildren) {
        const TArray<USceneComponent*>& AttachedFeatureChildren = LodComponent->GetAttachChildren();
        const auto Lod = FPLATEAUComponentUtil::ParseLodComponent(LodComponent);

        if (Lod < MinLod || Lod > MaxLod) {
            // LOD範囲外のLOD形状は非表示化
            for (const auto& FeatureComponent : AttachedFeatureChildren) {
                ApplyCollisionResponseBlockToChannel(FeatureComponent, false, true);
                FeatureComponent->SetVisibility(false, true);
            }
            continue;
        }

        for (const auto& FeatureComponent : AttachedFeatureChildren) {
            auto ComponentName = FPLATEAUComponentUtil::GetOriginalComponentName(FeatureComponent);
            TArray<int> Keys;
            NameMap.GetKeys(Keys);
            auto bIsMaxLod = true;
            for (const auto Key : Keys) {
                if (Key <= Lod)
                    continue;

                // コンポーネントのLODよりも大きいLODが存在する場合非表示
                if (NameMap[Key].Contains(ComponentName))
                    bIsMaxLod = false;
            }

            ApplyCollisionResponseBlockToChannel(FeatureComponent, bIsMaxLod, true);
            FeatureComponent->SetVisibility(bIsMaxLod, true);
        }
    }
}

void FPLATEAUModelFiltering::FilterByLods(const TArray<TObjectPtr<USceneComponent>>& GmlComponents, const plateau::dataset::PredefinedCityModelPackage InPackage, 
    const TMap<plateau::dataset::PredefinedCityModelPackage, FPLATEAUMinMaxLod>& PackageToLodRangeMap, const bool bOnlyMaxLod) {

    for (const auto& GmlComponent : GmlComponents) {
        const TArray<USceneComponent*>& AttachedLodChildren = GmlComponent->GetAttachChildren();

        // 一度全ての地物メッシュを不可視にする
        for (const auto& LodComponent : AttachedLodChildren) {
            const TArray<USceneComponent*>& AttachedFeatureChildren = LodComponent->GetAttachChildren();
            for (const auto& FeatureComponent : AttachedFeatureChildren) {
                ApplyCollisionResponseBlockToChannel(FeatureComponent, false, true);
                FeatureComponent->SetVisibility(false, true);
            }
        }

        // 選択されていないパッケージを除外
        const auto Package = FPLATEAUGmlUtil::GetCityModelPackage(GmlComponent);
        if ((Package & InPackage) == plateau::dataset::PredefinedCityModelPackage::None)
            continue;

        const auto MinLod = PackageToLodRangeMap[Package].MinLod;
        const auto MaxLod = PackageToLodRangeMap[Package].MaxLod;

        // 各地物について全てのLODを表示する場合の処理
        if (!bOnlyMaxLod) {
            for (const auto& LodComponent : AttachedLodChildren) {
                const auto Lod = FPLATEAUComponentUtil::ParseLodComponent(LodComponent);
                if (MinLod <= Lod && Lod <= MaxLod) {
                    for (const auto& FeatureComponent : LodComponent->GetAttachChildren()) {
                        ApplyCollisionResponseBlockToChannel(FeatureComponent, true, true);
                        FeatureComponent->SetVisibility(true, true);
                    }
                }
                else {
                    for (const auto& FeatureComponent : LodComponent->GetAttachChildren()) {
                        ApplyCollisionResponseBlockToChannel(FeatureComponent, false, true);
                        FeatureComponent->SetVisibility(false, true);
                    }
                }
            }
            continue;
        }

        // 各地物について最大LODのみを表示する場合の処理
        FilterLowLods(GmlComponent, MinLod, MaxLod);
    }
}

void FPLATEAUModelFiltering::FilterByFeatureTypes(const TArray<TObjectPtr<USceneComponent>>& GmlComponents, const citygml::CityObject::CityObjectsType InCityObjectType) {

    for (const auto& GmlComponent : GmlComponents) {
        // BillboardComponentを無視
        if (GmlComponent.GetName().Contains("BillboardComponent"))
            continue;

        // 起伏は重いため意図的に除外
        const auto Package = FPLATEAUGmlUtil::GetCityModelPackage(GmlComponent);
        if (Package == plateau::dataset::PredefinedCityModelPackage::Relief)
            continue;

        for (const auto& LodComponent : GmlComponent->GetAttachChildren()) {
            TArray<USceneComponent*> FeatureComponents;
            LodComponent->GetChildrenComponents(true, FeatureComponents);
            for (const auto& FeatureComponent : FeatureComponents) {
                //この時点で不可視状態ならLodフィルタリングで不可視化されたことになるので無視
                if (!FeatureComponent->IsVisible())
                    continue;

                auto FeatureID = FeatureComponent->GetName();
                // BillboardComponentも混ざってるので無視
                if (FeatureID.Contains("BillboardComponent"))
                    continue;

                if (!FeatureComponent->IsA(UPLATEAUCityObjectGroup::StaticClass()))
                    continue;

                const auto CityObjGrp = StaticCast<UPLATEAUCityObjectGroup*>(FeatureComponent);
                const auto ObjList = CityObjGrp->GetAllRootCityObjects();
                if (ObjList.Num() != 1)
                    continue;

                const int64 CityObjectType = UPLATEAUCityObjectBlueprintLibrary::GetTypeAsInt64(ObjList[0].Type);
                if (static_cast<int64>(InCityObjectType) & CityObjectType)
                    continue;

                ApplyCollisionResponseBlockToChannel(FeatureComponent, false);
                FeatureComponent->SetVisibility(false);
            }
        }
    }
}

void FPLATEAUModelFiltering::FilterByFeatureTypesLegacyCacheCityGml(const TArray<TObjectPtr<USceneComponent>>& GmlComponents, const citygml::CityObject::CityObjectsType InCityObjectType, const FString DatasetName) {

    // 処理が重いため先にCityGMLのパースを行って内部的にキャッシュしておく。
    for (const auto& GmlComponent : GmlComponents) {
        // BillboardComponentを無視
        if (GmlComponent.GetName().Contains("BillboardComponent"))
            continue;

        // 起伏は重いため意図的に除外
        const auto Package = FPLATEAUGmlUtil::GetCityModelPackage(GmlComponent);
        if (Package == plateau::dataset::PredefinedCityModelPackage::Relief)
            continue;

        FPLATEAUCityObjectInfo GmlInfo;
        GmlInfo.DatasetName = DatasetName;
        GmlInfo.GmlName = FPLATEAUGmlUtil::GetGmlFileName(GmlComponent);
        const auto CityModel = UPLATEAUCityGmlProxy::Load(GmlInfo);
    }
}

void FPLATEAUModelFiltering::FilterByFeatureTypesLegacyMain(const TArray<TObjectPtr<USceneComponent>>& GmlComponents, const citygml::CityObject::CityObjectsType InCityObjectType, const FString DatasetName) {
    for (const auto& GmlComponent : GmlComponents) {
        // BillboardComponentを無視
        if (GmlComponent.GetName().Contains("BillboardComponent"))
            continue;

        // 起伏は重いため意図的に除外
        const auto Package = FPLATEAUGmlUtil::GetCityModelPackage(GmlComponent);
        if (Package == plateau::dataset::PredefinedCityModelPackage::Relief)
            continue;

        for (const auto& LodComponent : GmlComponent->GetAttachChildren()) {
            TArray<USceneComponent*> FeatureComponents;
            LodComponent->GetChildrenComponents(true, FeatureComponents);
            for (const auto& FeatureComponent : FeatureComponents) {
                //この時点で不可視状態ならLodフィルタリングで不可視化されたことになるので無視
                if (!FeatureComponent->IsVisible())
                    continue;

                auto FeatureID = FeatureComponent->GetName();

                // TODO: 最小地物の場合元の地物IDに_{数値}が入っている場合があるため、最小地物についてのみ処理する。よりロバストな方法検討必要
                if (FeatureComponent->GetAttachParent() != LodComponent) {
                    FeatureID = FPLATEAUComponentUtil::GetOriginalComponentName(FeatureComponent);
                }

                // BillboardComponentも混ざってるので無視
                if (FeatureID.Contains("BillboardComponent"))
                    continue;

                FPLATEAUCityObjectInfo GmlInfo;
                GmlInfo.DatasetName = DatasetName;
                GmlInfo.GmlName = FPLATEAUGmlUtil::GetGmlFileName(GmlComponent);
                const auto CityModel = UPLATEAUCityGmlProxy::Load(GmlInfo);

                if (CityModel == nullptr) {
                    UE_LOG(LogTemp, Error, TEXT("Invalid Dataset or Gml : %s, %s"), *GmlInfo.DatasetName, *GmlInfo.GmlName);
                    continue;
                }

                const auto CityObject = CityModel->getCityObjectById(TCHAR_TO_UTF8(*FeatureID));
                if (CityObject == nullptr) {
                    UE_LOG(LogTemp, Error, TEXT("Invalid ID : %s"), *FeatureID);
                    continue;
                }

                const auto CityObjectType = CityObject->getType();
                if (static_cast<uint64_t>(InCityObjectType & CityObjectType))
                    continue;

                ApplyCollisionResponseBlockToChannel(FeatureComponent, false);
                FeatureComponent->SetVisibility(false);
            }
        }
    }
}