// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RnIntersection.h"
#include "RnTrackBuilder.generated.h"

class URnIntersectionEdge;
class URnTrack;
class URnIntersection;

/**
 * FBuildTrackOption
 *
 * トラック生成時の各種オプション
 */
USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FBuildTrackOption {
    GENERATED_BODY()

    // スプラインの Tangent の長さ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackOption")
    float TangentLength = 10.f;

    // 同じ道路への侵入を許可するか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackOption")
    bool AllowSelfTrack = false;

    // 生成前に既存の Track をクリアするか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackOption")
    bool ClearTracks = true;

    // 未生成のトラックのみを対象とするか（ClearTracksがtrueだと実質全Trackが対象となる）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackOption")
    bool UnCreatedTrackOnly = true;

    // 指定された LineString を境界とするトラックのみを対象とする（空の場合は全てのトラックが対象）
    // ※ URnLineString は別途実装済みのクラスとする
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackOption")
    TSet<URnLineString*> TargetBorderLineStrings;

    // Default コンストラクタ
    FBuildTrackOption()
        : TangentLength(10.f)
        , AllowSelfTrack(false)
        , ClearTracks(true)
        , UnCreatedTrackOnly(true) {
    }

    /**
     * ビルド対象のトラックかどうかを判定する
     * @param Intersection 対象交差点。nullptr 判定も含む
     * @param From 入口側の隣接情報。From->Border は URnWay である前提
     * @param To   出口側の隣接情報。To->Border は URnWay である前提
     */
    bool IsBuildTarget(URnIntersection* Intersection, URnIntersectionEdge* From, URnIntersectionEdge* To) const;

    /**
     * 全トラックを再生成するためのデフォルトオプションを取得
     */
    static FBuildTrackOption Default();

    /**
     * 未生成のトラックのみ対象とするオプションを取得
     */
    static FBuildTrackOption UnBuiltTracks();

    /**
     * 指定した境界線に関連するトラックのみ対象とするオプションを取得
     * @param Borders 関連する URnLineString の配列
     */
    static FBuildTrackOption WithBorder(const TArray<URnLineString*>& Borders);
};

/**
 * URnTracksBuilder
 *
 * 交差点におけるトラック生成処理（RnTracksBuilder.cs 相当）
 */
struct PLATEAURUNTIME_API FRnTracksBuilder
{
public:
    // 内部クラス: 出口情報
    struct FOutBound {
        ERnTurnType TurnType;
        // 出口グループ（FRnIntersectionEx::FEdgeGroup 相当）
        FRnIntersectionEx::FEdgeGroup* ToEg;
        // 出口側の隣接情報
        URnIntersectionEdge* To;

        FOutBound() : TurnType(ERnTurnType::Straight), ToEg(nullptr), To(nullptr) {}
        FOutBound(ERnTurnType InTurnType, FRnIntersectionEx::FEdgeGroup* InToEg, URnIntersectionEdge* InTo)
            : TurnType(InTurnType), ToEg(InToEg), To(InTo) {
        }
    };

    // デフォルトコンストラクタ
    FRnTracksBuilder();

    /**
     * トラックの再生成処理
     * @param Intersection  対象交差点（URnIntersection）
     * @param Option        オプション。nullptrの場合はデフォルト値を用いる
     */
    void BuildTracks(URnIntersection* Intersection, const FBuildTrackOption& Option);

    /**
     * from/to をつなぐトラックを生成する
     */
    URnTrack* MakeTrack(URnIntersection* Intersection
        , URnIntersectionEdge* From
        , const FBuildTrackOption& Option
        , FRnIntersectionEx::FEdgeGroup* FromEg
        , const struct FOutBound& OutBound);

    /**
     * トラック生成（FromBorder/toBorderおよびSpline生成）
     */
    URnTrack* CreateTrackOrDefault(URnIntersection* Intersection, const FBuildTrackOption& Option,
        URnWay* Way, const TArray<float>& WidthTable, URnIntersectionEdge* From, URnIntersectionEdge* To, ERnTurnType EdgeTurnType);


    /**
     * Calculate turn type from two 3D vectors based on the specified plane.
     * @param From – incoming direction vector.
     * @param To – outgoing direction vector.
     * @param Axis – projection axis (0: XY, 1: XZ, 2: YZ).
     * @return the calculated turn type.
     */
    static ERnTurnType GetTurnType(const FVector& From, const FVector& To);

    /**
     * Calculate turn type from two 2D vectors.
     * @param From – incoming 2D direction vector.
     * @param To – outgoing 2D direction vector.
     * @return the calculated turn type.
     */
    static ERnTurnType GetTurnType(const FVector2D& From, const FVector2D& To);
};
