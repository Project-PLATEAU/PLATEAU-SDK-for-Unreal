// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"

#include "RnSideWalk.generated.h"
// Forward declarations
class URnRoadBase;
class URnWay;
class URnPoint;

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnSideWalk : public UObject {
    GENERATED_BODY()
public:
    URnSideWalk();
    void Init();

    void Init(
        const TRnRef_T<URnRoadBase>& Parent,
        const TRnRef_T<URnWay>& OutsideWay,
        const TRnRef_T<URnWay>& InsideWay,
        const TRnRef_T<URnWay>& StartEdgeWay,
        const TRnRef_T<URnWay>& EndEdgeWay,
        EPLATEAURnSideWalkLaneType LaneType = EPLATEAURnSideWalkLaneType::Undefined);
    // 自分が所属するRoadNetworkModel
    TRnRef_T<URnRoadBase> GetParentRoad() const;

    // 道路と反対側のWay
    TRnRef_T<URnWay> GetOutsideWay() const { return OutsideWay; }

    // 道路と同じ側のWay
    TRnRef_T<URnWay> GetInsideWay() const { return InsideWay; }

    // outsideWayとinsideWayの始点を繋ぐWay
    TRnRef_T<URnWay> GetStartEdgeWay() const { return StartEdgeWay; }

    // outsideWayとinsideWayの終点を繋ぐWay
    TRnRef_T<URnWay> GetEndEdgeWay() const { return EndEdgeWay; }

    EPLATEAURnSideWalkLaneType GetLaneType() const { return LaneType; }
    void SetLaneType(EPLATEAURnSideWalkLaneType Type) { LaneType = Type; }

    // 左右のWay(OutsideWay, InsideWay)を列挙
    TArray<TRnRef_T<URnWay>> GetSideWays() const;

    // 開始/終了の境界線のWay(StartEdgeWay, EndEdgeWay)を列挙
    TArray<TRnRef_T<URnWay>> GetEdgeWays() const;

    // 全てのWay(nullは含まない)
    TArray<TRnRef_T<URnWay>> GetAllWays() const;

    // Inside/OutsideのWayが両方ともValidかどうか
    bool IsValid() const;

    EPLATEAURnSideWalkWayTypeMask GetValidWayTypeMask() const;

    // 強制的に親を変更する. 構造壊れるので扱い注意
    void SetParent(const TRnRef_T<URnRoadBase>& Parent);

    // 左右のWayを再設定
    void SetSideWays(const TRnRef_T<URnWay>& OutsideWay, const TRnRef_T<URnWay>& InsideWay);

    // 境界のWayを再設定
    void SetEdgeWays(const TRnRef_T<URnWay>& StartWay, const TRnRef_T<URnWay>& EndWay);
    void SetStartEdgeWay(const TRnRef_T<URnWay>& StartWay);
    void SetEndEdgeWay(const TRnRef_T<URnWay>& EndWay);

    // レーンタイプを入れ替え
    void ReverseLaneType();

    // InSideWay/OutSideWayの方向を合わせる
    void Align();

    // 歩道作成
    static TRnRef_T<URnSideWalk> Create(
        const TRnRef_T<URnRoadBase>& Parent,
        const TRnRef_T<URnWay>& OutsideWay,
        const TRnRef_T<URnWay>& InsideWay,
        const TRnRef_T<URnWay>& StartEdgeWay,
        const TRnRef_T<URnWay>& EndEdgeWay,
        EPLATEAURnSideWalkLaneType LaneType = EPLATEAURnSideWalkLaneType::Undefined,
        bool AddToParent = true
    );


    // 代表点を取得
    FVector GetCentralVertex() const;

    // 隣接判定
    bool IsNeighboring(const TRnRef_T<URnSideWalk>& Other) const;

    // 近接スコア計算
    float CalcRoadProximityScore(const TRnRef_T<URnRoadBase>& Other) const;
    void Reverse();
    URnSideWalk* ReversedSideWalk();
    bool TryMergeNeighborSideWalk(URnSideWalk* SrcSideWalk);

    bool Check() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TWeakObjectPtr<URnRoadBase> ParentRoad;

    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnWay* OutsideWay;
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnWay* InsideWay;
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnWay* StartEdgeWay;
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnWay* EndEdgeWay;
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    EPLATEAURnSideWalkLaneType LaneType;
};
