// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include <optional>

#include "RnRoad.h"
#include "RnRoadBase.h"
#include "RnRoadGroup.generated.h"
class URnIntersection;
class URnRoad;
class URnSideWalk;
class URnWay;
class URnLane;
class URnLineString;

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnRoadGroup : public UObject
{
    GENERATED_BODY()
public:
    URnRoadGroup() = default;
    URnRoadGroup(TRnRef_T<URnIntersection> InPrevIntersection,
        TRnRef_T<URnIntersection> InNextIntersection,
        const TArray<TRnRef_T<URnRoad>>& InRoads);

    void Init(TRnRef_T<URnIntersection> InPrevIntersection,
        TRnRef_T<URnIntersection> InNextIntersection,
        const TArray<TRnRef_T<URnRoad>>& InRoads);

    // 有効なRoadGroupかどうか
    bool IsValid() const;

    // 全レーンが有効かどうか
    bool IsAllLaneValid() const;

    // 左側のレーン数を取得
    int32 GetLeftLaneCount() const;

    // 右側のレーン数を取得
    int32 GetRightLaneCount() const;

    // 右側のレーンを取得
    TArray<TRnRef_T<URnLane>> GetRightLanes() const;

    // 左側のレーンを取得
    TArray<TRnRef_T<URnLane>> GetLeftLanes() const;
public:
    // RnDirで指定した側のレーンを取得
    TArray<TRnRef_T<URnLane>> GetLanes(EPLATEAURnDir Dir) const;

    // 中央分離帯を取得
    void GetMedians(TArray<TRnRef_T<URnWay>>& OutLeft, TArray<TRnRef_T<URnWay>>& OutRight) const;

    // 中央分離帯があるかどうか
    bool HasMedians() const;

    // 中央分離帯を作成する
    bool CreateMedianOrSkip(float MedianWidth = 1.0f, float MaxMedianLaneRate = 0.5f);

    // 中央分離帯があるかどうか
    bool HasMedian() const;

public:
    // 向きがそろっているかどうか
    bool IsAligned() const;

    // 複数道路がきれいに整列されているかどうか
    bool IsDeepAligned() const;

    // LaneのPrev/Nextの向きをそろえる
    bool Align();

    // LaneのPrev/Nextの向きをそろえる
    bool Align() const;
    // 複数のRoadsを1つのRoadにまとめる
    bool MergeRoads();

    // 交差点との境界線の角度を調整する
    void AdjustBorder();

    // Static Methods
    static TRnRef_T<URnRoadGroup> CreateRoadGroupOrDefault(TRnRef_T<URnIntersection> PrevIntersection, TRnRef_T<URnIntersection> NextIntersection);
    static bool IsSameRoadGroup(TRnRef_T<URnRoadGroup> A, TRnRef_T<URnRoadGroup> B);

    static TRnRef_T<URnRoadGroup> CreateRoadGroupOrDefault(TRnRef_T<URnRoad> Road)
    {
        if (!Road)
            return nullptr;
        TArray roads{ Road };

        auto GetOppositeRoad = [](TRnRef_T<URnRoad> Self, TRnRef_T<URnRoadBase> Other) -> TRnRef_T<URnRoadBase>
        {
            auto Prev = Self->GetPrev();
            auto Next = Self->GetNext();
            if (Prev == Other)
                return Next == Other ? nullptr : Next;
            if (Next == Other)
                return Prev == Other ? nullptr : Prev;
            return nullptr;
        };

        auto Search = [&](TRnRef_T<URnRoadBase> src, TRnRef_T<URnRoadBase> target, bool isPrev) -> TRnRef_T<URnIntersection> {
            while (target && target->CastToRoad()) {
                auto road = target->CastToRoad();
                // ループしていたら終了
                if (roads.Contains(road))
                    break;
                if (isPrev)
                    roads.Insert(road, 0);
                else
                    roads.Add(road);
                // roadの接続先でselfじゃない方
                target = GetOppositeRoad(road, src);
                src = road;

            }
            if (!target)
                return nullptr;
            return target->CastToIntersection();
            };
        auto prevIntersection = Search(Road, Road->GetPrev(), true);
        auto nextIntersection = Search(Road, Road->GetNext(), false);
        return RnNew<URnRoadGroup>(prevIntersection, nextIntersection, roads);
    }

private:


    TMap<TRnRef_T<URnRoad>, TArray<TRnRef_T<URnLane>>> SplitLane(
        int32 Num,
        TOptional<EPLATEAURnDir> Dir,
        const TFunction<float(int32)>& GetSplitRate = nullptr);

    // レーン分割する
    void SetLaneCountImpl(int32 Count, EPLATEAURnDir Dir, bool RebuildTrack);

    // レーン数を変更する
    void SetLaneCountWithoutMedian(int32 LeftCount, int32 RightCount, bool RebuildTrack);

public:
    // レーン数を変更する
    void SetLaneCount(int32 LeftCount, int32 RightCount, bool RebuildTrack = true);

    // 左側レーン数を変更する
    void SetLeftLaneCount(int32 Count, bool RebuildTrack = true);

    // 右側レーン数を変更する
    void SetRightLaneCount(int32 Count, bool RebuildTrack = true);

    // 指定した側のレーン数を設定する
    void SetLaneCount(EPLATEAURnDir Dir, int32 Count, bool RebuildTrack = true);

    // 中央分離帯を考慮したレーン分割
    void SetLaneCountWithMedian(int32 LeftCount, int32 RightCount, float MedianWidthRate);

public:
    // 開始ノード
    TRnRef_T<URnIntersection> PrevIntersection;

    // 終了ノード
    TRnRef_T<URnIntersection> NextIntersection;

    // 道路リスト
    TArray<TRnRef_T<URnRoad>> Roads;
};
