// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RnRoadBase.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/Util/PLATEAURnEx.h"
#include "RnIntersection.generated.h"

class URnWay;
class URnPoint;
class UPLATEAUCityObjectGroup;
class URnRoadBase;
class URnWay;
class URnRoad;
struct FBuildTrackOption;
UENUM(BlueprintType)
enum class ERnTurnType : uint8 {
    LeftBack   UMETA(DisplayName = "左後ろ"),
    LeftTurn   UMETA(DisplayName = "左折"),
    LeftFront  UMETA(DisplayName = "左前"),
    Straight   UMETA(DisplayName = "直進"),
    RightFront UMETA(DisplayName = "右前"),
    RightTurn  UMETA(DisplayName = "右折"),
    RightBack  UMETA(DisplayName = "右後ろ"),
    UTurn      UMETA(DisplayName = "Uターン")
};

class PLATEAURUNTIME_API FRnTurnTypeUtil {
public:
    static bool IsLeft(ERnTurnType TurnType);
    static bool IsRight(ERnTurnType TurnType);
};

// 交差点への進行タイプ
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERnFlowTypeMask : uint8 {
    Empty = 0 UMETA(Hidden),
    // 流入
    Inbound = 1 << 0,
    // 流出
    Outbound = 1 << 1,
};
ENUM_CLASS_FLAGS(ERnFlowTypeMask);

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnIntersectionEdge : public UObject
{
    GENERATED_BODY()
public:
    URnIntersectionEdge();
    void Init();
    void Init(URnRoadBase* InRoad, URnWay* InBorder);

    TRnRef_T<URnRoadBase> GetRoad() const { return Road; }

    TRnRef_T<URnWay> GetBorder() const { return Border; }

    void SetRoad(const TRnRef_T<URnRoadBase>& InRoad) { Road = InRoad; }

    void SetBorder(const TRnRef_T<URnWay>& InBorder) { Border = InBorder; }

    // 有効なNeighborかどうか
    bool IsValid() const;

    // 他の道路との境界線かどうか
    bool IsBorder() const;

    // 境界線の中点を取得
    FVector GetCenterPoint() const;

    // 中央分離帯の境界線かどうか
    bool IsMedianBorder() const;

    // 接続されているレーンを取得
    TArray<TRnRef_T<URnLane>> GetConnectedLanes() const;

    // 指定した境界線に接続されているレーンを取得
    TRnRef_T<URnLane> GetConnectedLane(const TRnRef_T<URnWay>& BorderWay) const;

    // この境界線に対して流入/流出するタイプを取得
    ERnFlowTypeMask GetFlowType() const;

private:

    // 接続先の道路
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnRoadBase* Road;

    // 境界線
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnWay* Border;
};


/**
 * URnTrack
 *
 * UE5用トラッククラス。RnTrack の振る舞いをUE5形式に変換したもの。
 */
UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API URnTrack : public UObject {
    GENERATED_BODY()

public:
    // デフォルトコンストラクタ（シリアライズ用）
    URnTrack();

    /**
     * 初期化用関数
     * @param InFromBorder 流入元
     * @param InToBorder   流出先
     * @param InSpline     経路を表すSplineコンポーネント
     * @param InTurnType   曲がり具合（デフォルトは直進）
     */
    UFUNCTION(BlueprintCallable, Category = "URnTrack")
    void Init(URnWay* InFromBorder, URnWay* InToBorder, USplineComponent* InSpline, ERnTurnType InTurnType = ERnTurnType::Straight);

    //---------------------------
    // フィールド
    //---------------------------

    // 流入元
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "URnTrack")
    URnWay* FromBorder;

    // 流出先
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "URnTrack")
    URnWay* ToBorder;

    // 経路(現状未対応)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "URnTrack")
    USplineComponent* Spline;

    // 曲がり具合
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "URnTrack")
    ERnTurnType TurnType;

    //---------------------------
    // 関数
    //---------------------------

    /**
     * 指定した流入元/流出先と一致しているか判定
     * @param OtherFromBorder 比較する流入元
     * @param OtherToBorder   比較する流出先
     * @return 両方一致すればtrueを返す
     */
    UFUNCTION(BlueprintCallable, Category = "URnTrack")
    bool IsSameInOut(const URnWay* OtherFromBorder, const URnWay* OtherToBorder) const;

    /**
     * 他の URnTrack と入口/出口が同じかどうか判定
     * @param Other 比較対象の URnTrack クラス
     * @return 一致していればtrueを返す
     */
    UFUNCTION(BlueprintCallable, Category = "URnTrack")
    bool IsSameInOutWithTrack(const URnTrack* Other) const;

    /**
     * FromBorder または ToBorder が指定の way と一致しているか判定
     * @param Way 比較対象の RnWay
     * @return 一致していればtrueを返す
     */
    UFUNCTION(BlueprintCallable, Category = "URnTrack")
    bool ContainsBorder(const URnWay* Way) const;
};

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API URnIntersection : public URnRoadBase {
    GENERATED_BODY()
public:
    //using Super = URnRoadBase;
public:
    URnIntersection();

    void Init(TObjectPtr<UPLATEAUCityObjectGroup> TargetTran);

    void Init(const TArray<TObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans);


    // 他の道路との境界線Edge取得
    TArray<TRnRef_T<URnIntersectionEdge>> GetNeighbors() const;

    // 輪郭のEdge取得
    const TArray<TRnRef_T<URnIntersectionEdge>>& GetEdges() const { return Edges; }

    // トラック一覧取得
    const TArray<TRnRef_T<URnTrack>>& GetTracks() const { return Tracks; }

    // 有効な交差点かどうか
    bool IsValid() const;

    // 指定したRoadに接続されているEdgeを取得
    TArray<TRnRef_T<URnIntersectionEdge>> GetEdgesBy(const TRnRef_T<URnRoadBase>& Road) const;

    // 指定したRoadに接続されているEdgeを取得
    TRnRef_T<URnIntersectionEdge> GetEdgeBy(const TRnRef_T<URnRoadBase>& Road, const TRnRef_T<URnWay>& Border) const;

    // 指定したRoadに接続されているEdgeを取得
    TRnRef_T<URnIntersectionEdge> GetEdgeBy(const TRnRef_T<URnRoadBase>& Road, const TRnRef_T<URnPoint>& Point) const;

    // 指定したRoadに接続されているEdgeを取得
    TArray<TRnRef_T<URnIntersectionEdge>> GetEdgesBy(const TFunction<bool(const TRnRef_T<URnIntersectionEdge>&)>& Predicate) const;

    // Road/Laneに接続しているEdgeを削除
    void RemoveEdge(const TRnRef_T<URnRoad>& Road, const TRnRef_T<URnLane>& Lane);

    // 指定したRoadに接続されているEdgeを削除
    void RemoveEdges(const TRnRef_T<URnRoadBase>& Road);

    // 指定したRoadに接続されているEdgeを削除
    int32 RemoveEdges(const TFunction<bool(const TRnRef_T<URnIntersectionEdge>&)>& Predicate);

    /*
     * 指定した境界線をエッジから削除する
     */
    int32 RemoveEdges(const TRnRef_T<URnWay>& Way);

    // 指定したRoadに接続されているEdgeを置き換える
    void ReplaceEdges(const TRnRef_T<URnRoad>& Road, EPLATEAURnLaneBorderType BorderType, const TArray<TRnRef_T<URnWay>>& NewBorders);

    // borderを持つEdgeの隣接道路情報をafterRoadに差し替える.
    // 戻り値は差し替えが行われた数
    int32 ReplaceEdgeLink(TRnRef_T<URnWay> Border, TRnRef_T<URnRoadBase> AfterRoad);

    // Edgeを追加(Border==nullptrの場合はfalseが返る)
    bool AddEdge(const TRnRef_T<URnRoadBase>& Road, const TRnRef_T<URnWay>& Border);

    // 指定したRoadに接続されているかどうか
    bool HasEdge(const TRnRef_T<URnRoadBase>& Road) const;

    // 指定したRoadに接続されているかどうか
    bool HasEdge(const TRnRef_T<URnRoadBase>& Road, const TRnRef_T<URnWay>& Border) const;
    bool IsAligned() const;

    // Edgesの順番を整列する
    // 各Edgeが連結かつ時計回りになるように整列する
    void Align();

    // トラックを全削除
    void ClearTracks();

    // 隣接するRoadを取得
    virtual TArray<TRnRef_T<URnRoadBase>> GetNeighborRoads() const override;

    // 境界線情報を取得
    virtual TArray<TRnRef_T<URnWay>> GetBorders() const override;

    // 自身の接続を切断する
    virtual void DisConnect(bool RemoveFromModel) override;

    // 隣接情報を置き換える
    virtual void ReplaceNeighbor(const TRnRef_T<URnRoadBase>& From, const TRnRef_T<URnRoadBase>& To) override;

    // デバッグ用) その道路の中心を表す代表頂点を返す
    virtual FVector GetCentralVertex() const override;

    // 所属するすべてのWayを取得(重複の可能性あり)
    virtual TArray<TRnRef_T<URnWay>> GetAllWays() const override;

    // トラック情報を追加/更新する.
    // 同じfrom/toのトラックがすでにある場合は上書きする. そうでない場合は追加する
    bool TryAddOrUpdateTrack(TRnRef_T<URnTrack> track);

    // RnRoadへキャストする
    virtual TRnRef_T<URnRoad> CastToRoad() override {
        return TRnRef_T<URnRoad>(nullptr);
    }

    // RnIntersectionへキャストする
    virtual TRnRef_T<URnIntersection> CastToIntersection() override {
        return TRnRef_T<URnIntersection>(this);
    }
    void SeparateContinuousBorder();

    void BuildTracks();
    void BuildTracks(const FBuildTrackOption& Option);
    // 構造的に正しいかどうかチェック
    virtual bool Check() override;
    void MergeContinuousNonBorderEdge();

    // ReplaceNeighbor(URnWay* borderWay, URnRoadBase* to)
    // 指定した境界線（borderWay）に対応する隣接道路情報を to に置き換えます。
    virtual void ReplaceNeighbor(URnWay* BorderWay, URnRoadBase* To) override;
    // -------------------
    // static関数
    // -------------------

    // 交差点を作成する
    static TRnRef_T<URnIntersection> Create(TObjectPtr<UPLATEAUCityObjectGroup> TargetTran = nullptr);
    static TRnRef_T<URnIntersection> Create(const TArray<TObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans);

    /// <summary>
    /// 輪郭線の法線方向を外側向くように整える
    /// </summary>
    /// <param name="edge"></param>
    static void AlignEdgeNormal(TRnRef_T<URnIntersectionEdge> edge);


private:
    /*
     * エッジの削除.関係するトラックも削除される
     */
    bool RemoveEdge(URnIntersectionEdge* Edge);

private:

    // 交差点の外形情報
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnIntersectionEdge*> Edges;

    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnTrack*> Tracks;
};

struct FRnIntersectionEx
{
    struct FEdgeGroup : public FPLATEAURnEx::FKeyEdgeGroup<TRnRef_T<URnRoadBase>, TRnRef_T<URnIntersectionEdge>>
    {
    public:
        bool IsBorder() const
        {
            return (bool)Key;
        }

        bool IsValid() const;
                
        TArray<URnIntersectionEdge*> GetInBoundEdges() const;

        TArray<URnIntersectionEdge*> GetOutBoundEdges() const;

        FVector GetNormal() const;

        FEdgeGroup* RightSide = nullptr;
        FEdgeGroup* LeftSide = nullptr;
    };

    static TArray<FEdgeGroup> CreateEdgeGroup(TRnRef_T<URnIntersection> Intersection);

    static FVector GetEdgeNormal(TRnRef_T<URnIntersectionEdge> Edge);
    static FVector2D GetEdgeNormal2D(TRnRef_T<URnIntersectionEdge> Edge);
    static FVector GetEdgeCenter(TRnRef_T<URnIntersectionEdge> Edge);

    static FVector2D GetEdgeCenter2D(TRnRef_T<URnIntersectionEdge> Edge);
};
