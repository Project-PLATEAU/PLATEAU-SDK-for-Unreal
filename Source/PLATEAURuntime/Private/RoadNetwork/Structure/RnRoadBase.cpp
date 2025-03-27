// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnRoadBase.h"

#include "RoadNetwork/Factory/RoadNetworkFactory.h"
#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "RoadNetwork/Structure/RnModel.h"

URnRoadBase::URnRoadBase()
{
}

bool URnRoadBase::AddSideWalk(const TRnRef_T<URnSideWalk>& SideWalk) {
    if (!SideWalk)
        return false;
    // すでに存在する場合は無視 
    if (SideWalks.Contains(SideWalk)) 
        return false;

    // 以前の親からは削除
    if (SideWalk->GetParentRoad()) {
        SideWalk->GetParentRoad()->RemoveSideWalk(SideWalk);
    }
    SideWalk->SetParent(TRnRef_T<URnRoadBase>(this));
    SideWalks.Add(SideWalk);
    return true;
}

void URnRoadBase::RemoveSideWalk(const TRnRef_T<URnSideWalk>& SideWalk) {
    if (!SideWalk) return;
    SideWalk->SetParent(nullptr);
    SideWalks.Remove(SideWalk);
}

void URnRoadBase::AddTargetTran(UPLATEAUCityObjectGroup* TargetTran) {
    if (!TargetTrans.Contains(TargetTran)) {
        TargetTrans.Add(TargetTran);
    }
}

void URnRoadBase::AddTargetTran(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran)
{
    if (!TargetTrans.Contains(TargetTran)) {
        TargetTrans.Add(TargetTran);
    }
}

void URnRoadBase::AddTargetTrans(const TArray<UPLATEAUCityObjectGroup*>& InTargetTrans) {
    for (const auto& Tran : InTargetTrans) {
        AddTargetTran(Tran);
    }
}

void URnRoadBase::AddTargetTrans(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& InTargetTrans)
{
    for (const auto& Tran : InTargetTrans) {
        AddTargetTran(Tran);
    }
}

TArray<TRnRef_T<URnWay>> URnRoadBase::GetAllWays() const {
    TArray<TRnRef_T<URnWay>> Ways;
    for (const auto& SideWalk : SideWalks) {
        for (const auto& Way : SideWalk->GetAllWays()) {
            Ways.Add(Way);
        }
    }
    return Ways;
}

void URnRoadBase::UnLink(const TRnRef_T<URnRoadBase>& Other)
{
    if(Other)
        ReplaceNeighbor(Other, nullptr);
}

void URnRoadBase::DisConnect(bool RemoveFromModel) {
    if (RemoveFromModel) {
        for (const auto& SideWalk : SideWalks) {
            if (ParentModel) {
                ParentModel->RemoveSideWalk(SideWalk);
            }
        }
    }
}

void URnRoadBase::UnLinkEachOther(const TRnRef_T<URnRoadBase>& Other) {
    if (this) UnLink(Other);
    if (Other) Other->UnLink(TRnRef_T<URnRoadBase>(this));
}

FString URnRoadBase::GetTargetTransName() const {
    if (!this) 
        return TEXT("null");

    TArray<FString> Names;
    for (const auto& Tr : TargetTrans) {
        auto Trans = RnFrom(Tr);
        Names.Add(Trans ? Trans->GetName() : TEXT("null"));
    }
    return FString::Join(Names, TEXT(","));
}

TSet<TRnRef_T<URnLineString>> URnRoadBase::GetAllLineStringsDistinct() const {
    TSet<TRnRef_T<URnLineString>> LineStrings;
    if (!this) return LineStrings;

    for (const auto& Way : GetAllWays()) {
        if (Way && Way->LineString) {
            LineStrings.Add(Way->LineString);
        }
    }
    return LineStrings;
}

TRnRef_T<URnModel> URnRoadBase::GetParentModel() const
{
    return RnFrom(ParentModel);
}

void URnRoadBase::SetParentModel(const TRnRef_T<URnModel>& InParentModel)
{ ParentModel = InParentModel; }

bool URnRoadBase::RemoveSideWalk(URnSideWalk* InSideWalk, bool bRemoveFromModel) {
    if (!InSideWalk) {
        return false;
    }
    if (!SideWalks.Contains(InSideWalk)) {
        return false;
    }

    // 歩道の親をリセットする
    InSideWalk->SetParent(nullptr);
    SideWalks.Remove(InSideWalk);

    if (bRemoveFromModel && ParentModel) {
        ParentModel->RemoveSideWalk(InSideWalk);
    }

    return true;
}

void URnRoadBase::MergeConnectedSideWalks() {
    // 全ての歩道に対してループ
    for (int32 i = 0; i < SideWalks.Num(); ++i) {
        URnSideWalk* DstSideWalk = SideWalks[i];
        bool bFound = true;
        while (bFound) {
            bFound = false;
            // 統合可能な隣接歩道を探す
            for (int32 j = i + 1; j < SideWalks.Num(); ++j) {
                URnSideWalk* SrcSideWalk = SideWalks[j];
                // 隣接歩道の統合を試みる
                if (DstSideWalk->TryMergeNeighborSideWalk(SrcSideWalk)) {
                    // 統合に成功した場合、歩道を削除 (モデルからも削除)
                    RemoveSideWalk(SrcSideWalk, true);
                    bFound = true;
                    break;  // 更なる統合の可能性のため、内側ループを再開
                }
            }
        }
    }
}

void URnRoadBase::MergeSideWalks(const TArray<URnSideWalk*>& AddSideWalks) {
    // 渡された歩道を追加
    for (URnSideWalk* sw : AddSideWalks) {
        AddSideWalk(sw);
    }
    // 連結された歩道の統合処理を呼び出す
    MergeConnectedSideWalks();
}

void URnRoadBase::MergeSamePointLineStrings() {
    // 全ての歩道にある Way を収集する（重複ありの場合もあるため TSet を利用）
    TSet<URnWay*> Ways;
    for (auto Way : this->GetAllWays()) {
        if (Way == nullptr)
            continue;
        Ways.Add(Way);
    }

    // LineString 用のファクトリーオブジェクト (FLineStringFactoryWork) を生成
    FPLATEAULineStringFactoryWork LineStringFactory;

    // 各 Way に対して処理を行う
    for (URnWay* Way : Ways) {
        // LineStringのキャッシュなのでLineStringのPointsをキーにする
        auto Points = Way->LineString->GetPoints();

        bool bIsCached = false;
        bool bIsReversed = false;
        URnLineString* LS = LineStringFactory.CreateLineString(Points, bIsCached, bIsReversed, true,
            // キャッシュが無い時は今のLineStringをそのまま使いたいので生成関数を差し替える
            [Way](const TArray<URnPoint*>&) -> URnLineString* {
                return Way->LineString;
            });

        // キャッシュが存在する場合、Way の LineString を更新する
        if (bIsCached) {
            Way->LineString = LS;
            if (bIsReversed) {
                Way->Reverse(true);
            }
        }
    }
}

bool URnRoadBase::Check()
{
    for(auto Sw : GetSideWalks())
    {
        if ( Sw && Sw->Check() == false)
            return false;
    }
    return true;
}
