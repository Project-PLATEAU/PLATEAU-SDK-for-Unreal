// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Component/PLATEAUSceneComponent.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/Util/PLATEAURnEx.h"
#include "RnModel.generated.h"
class URnRoadGroup;
class URnRoad;
class URnIntersection;
class URnSideWalk;
class URnLane;
class URnIntersectionEdge;
class URnLineString;
class URnWay;
class UPLATEAUCityObjectGroup;
class URnRoadBase;
struct FLineSegment3D;

USTRUCT(BlueprintType)
struct FRnModelCalibrateIntersectionBorderOption
{
    GENERATED_BODY()
public:
    // 交差点の停止線を道路側に移動させる量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float MaxOffsetMeter = 5.0f;

    // 道路の長さがこれ以下にならないように交差点の移動量を減らす
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float NeedRoadLengthMeter = 23.0f;

    // デバッグ用. 切断のみ行い交差点へのマージ処理はスキップする
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool SkipMergeRoads = false;

};

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnModel : public UPLATEAUSceneComponent
{
public:
    const FString& GetFactoryVersion() const;

    void SetFactoryVersion(const FString& InFactoryVersion);

private:
    GENERATED_BODY()

public:
    static constexpr float Epsilon = SMALL_NUMBER;

    URnModel();

    void Init();

    // 道路を追加
    void AddRoadBase(const TRnRef_T<URnRoadBase>& RoadBase);

    // 道路を追加
    void AddRoad(const TRnRef_T<URnRoad>& Road);

    // 道路を削除
    void RemoveRoad(const TRnRef_T<URnRoad>& Road);

    // 交差点を追加
    void AddIntersection(const TRnRef_T<URnIntersection>& Intersection);

    // 交差点を削除
    void RemoveIntersection(const TRnRef_T<URnIntersection>& Intersection);

    // 歩道を追加
    void AddSideWalk(const TRnRef_T<URnSideWalk>& SideWalk);

    // 歩道を削除
    void RemoveSideWalk(const TRnRef_T<URnSideWalk>& SideWalk);

    // 道路を取得
    const TArray<TRnRef_T<URnRoad>>& GetRoads() const;

    // 交差点を取得
    const TArray<TRnRef_T<URnIntersection>>& GetIntersections() const;

    // 歩道を取得
    const TArray<TRnRef_T<URnSideWalk>>& GetSideWalks() const;

    // 指定したCityObjectGroupを含む道路を取得
    TRnRef_T<URnRoad> GetRoadBy(UPLATEAUCityObjectGroup* TargetTran) const;

    // 指定したCityObjectGroupを含む交差点を取得
    TRnRef_T<URnIntersection> GetIntersectionBy(UPLATEAUCityObjectGroup* TargetTran) const;

    // 指定したCityObjectGroupを含む歩道を取得
    TRnRef_T<URnSideWalk> GetSideWalkBy(UPLATEAUCityObjectGroup* TargetTran) const;

    // 指定したCityObjectGroupを含む道路/交差点を取得
    TRnRef_T<URnRoadBase> GetRoadBaseBy(UPLATEAUCityObjectGroup* TargetTran) const;

    // 指定した道路/交差点に接続されている道路/交差点を取得
    TArray<TRnRef_T<URnRoadBase>> GetNeighborRoadBases(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定した道路/交差点に接続されている道路を取得
    TArray<TRnRef_T<URnRoad>> GetNeighborRoads(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定した道路/交差点に接続されている交差点を取得
    TArray<TRnRef_T<URnIntersection>> GetNeighborIntersections(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定した道路/交差点に接続されている歩道を取得
    TArray<TRnRef_T<URnSideWalk>> GetNeighborSideWalks(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 交差点の境界線を調整する
    void CalibrateIntersectionBorderForAllRoad(const FRnModelCalibrateIntersectionBorderOption& Option);
    bool TrySliceRoadHorizontalNearByBorder(URnRoad* Road, const FRnModelCalibrateIntersectionBorderOption& Option,
                                            URnRoad*& OutPrevSideRoad, URnRoad*& OutCenterSideRoad,
                                            URnRoad*& OutNextSideRoad);

    // 道路ネットワークを作成する
    static TRnRef_T<URnModel> Create();

    // 指定したRoadBaseに接続されている道路/交差点を取得
    TArray<TRnRef_T<URnRoadBase>> GetConnectedRoadBases(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定したRoadBaseに接続されている道路を取得
    TArray<TRnRef_T<URnRoad>> GetConnectedRoads(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定したRoadBaseに接続されている交差点を取得
    TArray<TRnRef_T<URnIntersection>> GetConnectedIntersections(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定したRoadBaseに接続されている歩道を取得
    TArray<TRnRef_T<URnSideWalk>> GetConnectedSideWalks(const TRnRef_T<URnRoadBase>& RoadBase) const;

    // 指定したRoadBaseに接続されているRoadBaseを取得
    TArray<TRnRef_T<URnRoadBase>> GetConnectedRoadBasesRecursive(const TRnRef_T<URnRoadBase>& RoadBase) const;

    /// <summary>
    /// 連続した道路を一つのRoadにまとめる
    /// </summary>
    void MergeRoadGroup();

    // roadWidthの道路幅を基準にレーンを分割する
    void SplitLaneByWidth(float RoadWidth, bool rebuildTrack, TArray<FString>& failedRoads, TFunction<bool(URnRoadGroup*)> IsLaneSplitTarget);

    // 不正チェック
    bool Check() const;


    enum ERoadCutResult {
        Success,
        // 道路自体が不正
        InvalidRoad,
        // 不正な歩道を持っている
        InvalidSideWalk,
        // 切断線が不正
        InvalidCutLine,
        // 分断できないレーンがあった
        UnSlicedLaneExist,
        // 一部だけ分断された歩道が存在
        PartiallySlicedSideWalkExist,
        // 切断線が端点と近すぎる(ほぼ分断しない状態)
        TerminateCutLine,
        // 切断線が交差している
        CrossCutLine,

        // 交差点自体が不正
        InvalidIntersection,
        // 交差点の境界線を切断
        IntersectionBorderSliced,
        // 交差点のEdgeが片側だけ切断された
        IntersectionPartiallySlicedEdge,
        // 交差点で複数の入口を切断するのはダメ
        IntersectionMultipleEdgeSliced,
        // 交差点で分断された入口が存在しなかった
        IntersectionNoEdgeSliced,
    };

    struct FSliceRoadHorizontalResult {
        // 切断結果
        ERoadCutResult Result;
        // 元のRoadのPrev側(元のRoadはこれになる)
        URnRoad* PrevRoad;
        // 元のRoadのNext側
        URnRoad* NextRoad;
    };

    static ERoadCutResult CanSliceRoadHorizontal(URnRoad* Road, const FLineSegment3D& LineSegment, FPLATEAURnEx::FLineCrossPointResult& OutResult);

    FSliceRoadHorizontalResult SliceRoadHorizontal(URnRoad* Road, const FLineSegment3D& LineSegment);

    void SeparateContinuousBorder();

private:

    // 自動生成で作成されたときのバージョン
    FString FactoryVersion;

    // 道路リスト
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnRoad*> Roads;

    // 交差点リスト
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnIntersection*> Intersections;

    // 歩道リスト
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnSideWalk*> SideWalks;

};
