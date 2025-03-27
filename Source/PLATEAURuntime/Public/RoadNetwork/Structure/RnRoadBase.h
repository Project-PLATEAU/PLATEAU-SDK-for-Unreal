// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "RoadNetwork/GeoGraph/LineSegment3D.h"
#include "RnRoadBase.generated.h"

class URnModel;
class URnSideWalk;
class RnBorder;
class URnWay;
class URnLineString;
class URnLane;
class URnRoad;
class URnIntersection;

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnRoadBase : public UObject
{
    GENERATED_BODY()
public:
    URnRoadBase();
    virtual ~URnRoadBase() override = default;

    // 歩道sideWalkを追加する
    // sideWalkの親情報も書き換える
    bool AddSideWalk(const TRnRef_T<URnSideWalk>& SideWalk);

    // 歩道sideWalkを削除する
    // sideWalkの親情報は変更しない
    void RemoveSideWalk(const TRnRef_T<URnSideWalk>& SideWalk);

    // 境界線情報を取得
    virtual TArray<TRnRef_T<URnWay>> GetBorders() const { return TArray<TRnRef_T<URnWay>>(); }

    // 隣接するRoadを取得
    virtual TArray<TRnRef_T<URnRoadBase>> GetNeighborRoads() const { return TArray<TRnRef_T<URnRoadBase>>(); }

    // 対象のTargetTranを追加
    void AddTargetTran(UPLATEAUCityObjectGroup* TargetTran);
    void AddTargetTran(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran);
    void AddTargetTrans(const TArray<UPLATEAUCityObjectGroup*>& InTargetTrans);
    void AddTargetTrans(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& InTargetTrans);

    // 所属するすべてのWayを取得(重複の可能性あり)
    virtual TArray<TRnRef_T<URnWay>> GetAllWays() const;

    // otherをつながりから削除する. other側の接続は消えない
    void UnLink(const TRnRef_T<URnRoadBase>& Other);

    // 自身の接続を切断する
    // removeFromModel=trueの場合、RnModelからも削除する
    virtual void DisConnect(bool RemoveFromModel);

    // デバッグ用) その道路の中心を表す代表頂点を返す
    virtual FVector GetCentralVertex() const { return FVector::ZeroVector; }

    virtual void ReplaceNeighbor(const TRnRef_T<URnRoadBase>& From, const TRnRef_T<URnRoadBase>& To) {}

    // 相互に接続を解除する
    void UnLinkEachOther(const TRnRef_T<URnRoadBase>& Other);

    // デバッグ表示用. TargetTransの名前を取得
    FString GetTargetTransName() const;

    // selfのすべてのLineStringを取得
    TSet<TRnRef_T<URnLineString>> GetAllLineStringsDistinct() const;

    // 歩道を取得
    const auto& GetSideWalks() const { return SideWalks; }
    auto& GetSideWalks() { return SideWalks; }

    const auto& GetTargetTrans() const { return TargetTrans; }
    auto& GetTargetTrans() { return TargetTrans; }

    TRnRef_T<URnModel> GetParentModel() const;
    void SetParentModel(const TRnRef_T<URnModel>& InParentModel);
    bool RemoveSideWalk(URnSideWalk* InSideWalk, bool bRemoveFromModel);
    void MergeConnectedSideWalks();
    void MergeSideWalks(const TArray<URnSideWalk*>& AddSideWalks);
    void MergeSamePointLineStrings();

    // 構造的に正しいかどうかチェック
    virtual bool Check();

    // RnRoadへキャストする
    virtual TRnRef_T<URnRoad> CastToRoad()
    {
        return nullptr;
    }

    // RnIntersectionへキャストする
    virtual TRnRef_T<URnIntersection> CastToIntersection()
    {
        return nullptr;
    }

    // ReplaceNeighbor(URnWay* borderWay, URnRoadBase* to)
    // 指定した境界線（borderWay）に対応する隣接道路情報を to に置き換えます。
    virtual void ReplaceNeighbor(URnWay* BorderWay, URnRoadBase* To) {}

private:

    // 自分が所属するRoadNetworkModel
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnModel* ParentModel;

    // これに紐づくtranオブジェクトリスト(統合なので複数存在する場合がある)
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>> TargetTrans;

    // 歩道情報
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnSideWalk*> SideWalks;

};
