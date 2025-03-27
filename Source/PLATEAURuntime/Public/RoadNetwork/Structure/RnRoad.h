// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RnRoadBase.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RnRoad.generated.h"

class URnLane;
class URnWay;
class URnIntersection;
class UPLATEAUCityObjectGroup;

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnRoad : public URnRoadBase {
private:
    GENERATED_BODY()
public:
    URnRoad();
    explicit URnRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran);
    explicit URnRoad(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans);

    void Init(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran);
    void Init(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans);

    // メインレーンすべて取得
    const auto& GetMainLanes() const { return MainLanes; }

    // 中央分離帯を含めた全てのレーン
    TArray<TRnRef_T<URnLane>> GetAllLanesWithMedian() const;

    // 有効なRoadかどうか
    bool IsValid() const;

    // 全てのレーンに両方向に接続先がある
    bool IsAllBothConnectedLane() const;

    // 全レーンがIsValidかどうか
    bool IsAllLaneValid() const;

    // 左車線/右車線両方あるかどうか
    bool HasBothLane() const;

    // 交差点間に挿入された空Roadかどうか
    bool IsEmptyRoad() const;

    void SetNext(TRnRef_T<URnRoadBase> InNext);

    void SetPrev(TRnRef_T<URnRoadBase> InPrev);

    void SetMedianLane1(TRnRef_T<URnLane> InMedianLane);

    TRnRef_T<URnRoadBase> GetNext() const;

    TRnRef_T<URnRoadBase> GetPrev() const;

    TRnRef_T<URnRoadBase> GetNeighborRoad(EPLATEAURnLaneBorderType BorderType) const;

    TRnRef_T<URnLane> GetMedianLane() const;

    // 指定方向のレーンを取得
    TArray<TRnRef_T<URnLane>> GetLanes(EPLATEAURnDir Dir) const;

    // 指定方向のレーンを取得. Dirが指定されていない場合はMainLanes全体を返す
    bool TryGetLanes(TOptional<EPLATEAURnDir> Dir, TArray<TRnRef_T<URnLane>>& OutLanes) const;

    // レーンが左車線かどうか
    bool IsLeftLane(const TRnRef_T<URnLane>& Lane) const;

    // レーンが右車線かどうか
    bool IsRightLane(const TRnRef_T<URnLane>& Lane) const;

    // レーンの方向を取得
    EPLATEAURnDir GetLaneDir(const TRnRef_T<URnLane>& Lane) const;

    // 境界線情報を取得
    virtual TArray<TRnRef_T<URnWay>> GetBorders() const override;

    // 左側のレーンを取得
    TArray<TRnRef_T<URnLane>> GetLeftLanes() const;

    // 右側のレーンを取得
    TArray<TRnRef_T<URnLane>> GetRightLanes() const;

    // 左側レーン数を取得
    int32 GetLeftLaneCount() const;

    // 右側レーン数を取得
    int32 GetRightLaneCount() const;

    // 中央分離帯の幅を取得
    float GetMedianWidth() const;

    // 中央分離帯を設定
    void SetMedianLane(const TRnRef_T<URnLane>& Lane);

    // 指定したレーンが中央分離帯かどうか
    bool IsMedianLane(const TRnRef_T<const URnLane>& Lane) const;

    // 隣接するRoadを取得
    virtual TArray<TRnRef_T<URnRoadBase>> GetNeighborRoads() const override;


    /// <summary>
    /// #TODO : 左右の隣接情報がないので要修正
    /// MainLanesにlaneを追加する. すでに存在する場合には無視される.ParentRoad情報も更新する
    /// </summary>
    /// <param name="Lane"></param>
    bool AddMainLane(TRnRef_T<URnLane> Lane);

    /*
     * MainLanesのIndex位置にlaneを追加する. すでに存在する場合には無視される. ParentRoad情報も更新する
     */
    bool AddMainLane(TRnRef_T<URnLane> Lane, int32 Index);

    /*
     * MainLanesからLaneを削除. ParentRoad情報も更新する
     */
    bool RemoveMainLane(TRnRef_T<URnLane> Lane);

    // 指定した方向の境界線を取得する(全レーンマージした状態で取得する)
    TRnRef_T<URnWay> GetMergedBorder(EPLATEAURnLaneBorderType BorderType, TOptional<EPLATEAURnDir> Dir = NullOpt) const;

    // 指定した方向のWayを取得する(全レーンマージした状態で取得する)
    TRnRef_T<URnWay> GetMergedSideWay(EPLATEAURnDir Dir) const;

    // dirで指定した側の全レーンの左右のWayを返す
    // dir==nullの時は全レーン共通で返す
    // 例) 左２車線でdir==RnDir.Leftの場合, 一番左の車線の左側のWayと左から２番目の車線の右側のWayを返す
    bool TryGetMergedSideWay(TOptional<EPLATEAURnDir> Dir, TRnRef_T<URnWay>& OutLeftWay, TRnRef_T<URnWay>& OutRightWay) const;

    // 指定したLineStringまでの最短距離を取得する
    bool TryGetNearestDistanceToSideWays(const TRnRef_T<URnLineString>& LineString, float& OutDistance) const;

    // 境界線の向きをそろえる
    void AlignLaneBorder(EPLATEAURnLaneBorderDir BorderDir = EPLATEAURnLaneBorderDir::Left2Right);

    // 境界線を調整するための線分を取得する
    bool TryGetAdjustBorderSegment(EPLATEAURnLaneBorderType BorderType, FLineSegment3D& OutSegment) const;

    // 指定したレーンの境界線を取得する
    virtual TRnRef_T<URnWay> GetBorderWay(const TRnRef_T<URnLane>& Lane, EPLATEAURnLaneBorderType BorderType, EPLATEAURnLaneBorderDir BorderDir) const;

    // 境界線の一覧を取得する. left->rightの順番
    TArray<URnWay*> GetBorderWays(EPLATEAURnLaneBorderType BorderType) const;
    // レーンを置き換える
    void ReplaceLanes(const TArray<TRnRef_T<URnLane>>& NewLanes, EPLATEAURnDir Dir);
    void ReplaceLanes(const TArray<TRnRef_T<URnLane>>& NewLanes);

    // 前後の接続を設定する
    void SetPrevNext(const TRnRef_T<URnRoadBase>& PrevRoad, const TRnRef_T<URnRoadBase>& NextRoad);

    // 反転する
    // Next,Prevを逆転する.
    // その結果, レーンのIsReverseも逆転 / mainLanesの配列順も逆転する
    // keepOneLaneIsLeftがtrueの場合, 1車線しか無い道路だとその1車線がRoadのPrev / Nextを同じ方向になるように(左車線扱い)する
    void Reverse(bool KeepOneLaneIsLeft = true);

    /// <summary>
      /// この境界とつながっているレーンリスト
      /// </summary>
      /// <returns></returns>
    TArray<URnLane*> GetConnectedLanes(URnWay* border);

    // デバッグ用) その道路の中心を表す代表頂点を返す
    virtual FVector GetCentralVertex() const override;

    // 所属するすべてのWayを取得(重複の可能性あり)
    virtual TArray<TRnRef_T<URnWay>> GetAllWays() const override;

    // 自身の接続を切断する
    virtual void DisConnect(bool RemoveFromModel) override;

    // 隣接情報を置き換える
    virtual void ReplaceNeighbor(const TRnRef_T<URnRoadBase>& From, const TRnRef_T<URnRoadBase>& To) override;

    // 隣接情報を置き換える
    virtual void ReplaceNeighbor(URnWay* BorderWay, URnRoadBase* To) override;
    // 左右のWayを結合したものを取得
    TArray<TRnRef_T<URnWay>> GetMergedSideWays() const;
    bool TryMerge2NeighborIntersection(EPLATEAURnLaneBorderType BorderType);
    void SeparateContinuousBorder();

    // RnRoadへキャストする
    virtual TRnRef_T<URnRoad> CastToRoad() override
    {
        return TRnRef_T<URnRoad>(this);
    }

    // RnIntersectionへキャストする
    virtual TRnRef_T<URnIntersection> CastToIntersection() override
    {
        return TRnRef_T<URnIntersection>(nullptr);
    }

    // 構造的に正しいかどうかチェック
    virtual bool Check() override;


    bool TryGetVerticalSliceSegment(
        EPLATEAURnLaneBorderType BorderSide,
        float BorderOffset,
        FLineSegment3D& OutSegment);

    // 道路を作成する
    static TRnRef_T<URnRoad> Create(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran = nullptr);
    static TRnRef_T<URnRoad> Create(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans);

    // 孤立した道路を作成する
    static TRnRef_T<URnRoad> CreateIsolatedRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran, TRnRef_T<URnWay> Way);

    static TRnRef_T<URnRoad> CreateOneLaneRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran, TRnRef_T<URnLane> Lane);

private:

    void OnAddLane(TRnRef_T<URnLane> Lane);
    void OnRemoveLane(TRnRef_T<URnLane> Lane);

public:
    // 接続先(nullの場合は接続なし)
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnRoadBase* Next;

    // 接続元(nullの場合は接続なし)
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnRoadBase* Prev;

    // レーンリスト
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnLane*> MainLanes;

    // 中央分離帯
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnLane* MedianLane;

};

struct FRnRoadEx
{

    static auto IsValidBorderAdjacentNeighbor(const URnRoad* Self, EPLATEAURnLaneBorderType BorderType,
                                              bool NoBorderIsTrue) -> bool;

};