// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "../RGraph/RGraphDef.h"
#include <memory>
#include <functional>

#include "PLATEAUInstancedCityModel.h"
#include "RoadNetwork/RGraph/RGraphFactory.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "RoadNetworkFactory.generated.h"

class APLATEAURnStructureModel;
class UPLATEAUCityObjectGroup;
class URnModel;
class URnRoad;
class URnIntersection;
class URnSideWalk;
class URnWay;
class URnLineString;
class URnLane;
class URnPoint;


USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FRoadNetworkFactory
{
    GENERATED_BODY()
public:
    static const FString FactoryVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float RoadSize = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float TerminateAllowEdgeAngle = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float TerminateSkipAngle = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float Lod1SideWalkSize = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    float Lod1SideWalkThresholdRoadWidth = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bAddSideWalk = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bCheckMedian = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bIgnoreHighway = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bAddTrafficSignalLights = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bSaveTmpData = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bUseContourMesh = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bMergeRoadGroup = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bCalibrateIntersection = true;

    // 道路で別の道路との境界線が繋がっている場合(間に輪郭線が入っていない)場合に少しずらして挿入する
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bSeparateContinuousBorder = true;

    // 車線分割を行うかどうか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bSplitLane = true;

    // 交差点のトラック生成を行う
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bBuildTracks = true;

    // LOD3.1以上のLane情報を見る
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    bool bCheckLane = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FRGraphFactory GraphFactory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FRnModelCalibrateIntersectionBorderOption CalibrateIntersectionOption;

};

struct FRoadNetworkFactoryEx
{
   
    struct FCreateRnModelRequest {
        APLATEAUInstancedCityModel* Actor;
        TWeakObjectPtr<USceneComponent> Transform;
        TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;
        //PLATEAURnStructureModel* OriginalMesh;
    };

    static void CreateRnModel(const FRoadNetworkFactory& Self, APLATEAUInstancedCityModel* Actor, APLATEAURnStructureModel* DestActor);

    // Targetが生成対象かどうか
    static bool IsConvertTarget(UPLATEAUCityObjectGroup* Target);
private:

    static TRnRef_T<URnModel> CreateRoadNetwork(
        const FRoadNetworkFactory& Self
        , APLATEAUInstancedCityModel* TargetCityModel
        , APLATEAURnStructureModel* Actor
        , TArray<UPLATEAUCityObjectGroup*>& CityObjectGroups
    );

    // 最小地物に分解する
    static void CreateSubDividedCityObjects(const FRoadNetworkFactory& Self, APLATEAUInstancedCityModel* Actor
        , AActor* DestActor
        , USceneComponent* Root
        , TArray<UPLATEAUCityObjectGroup*>& CityObjectGroups
        , TArray<FSubDividedCityObject>& OutSubDividedCityObjects);

    // RGraphを作成する
    static void CreateRGraph(const FRoadNetworkFactory& Self, APLATEAUInstancedCityModel* Actor
        , AActor* DestActor
        , USceneComponent* Root
        , TArray<FSubDividedCityObject>& SubDividedCityObjects
        , RGraphRef_t<URGraph>& OutGraph);

    // RnModelを作成する
    static  TRnRef_T<URnModel> CreateRnModel(
        const FRoadNetworkFactory& Self
        , RGraphRef_t<URGraph> Graph
        , URnModel* OutModel);
};


/**
 * FLineStringFactoryWork
 *
 * URnPoint 配列から URnLineString および URnWay を生成する際のキャッシュ機能を実装します。
 */
class FPLATEAULineStringFactoryWork {
public:
    // キャッシュにおけるエントリ。point リストのコピーと、それに対応する LineString を保持。
    struct FPointCache {
        // URnPoint ポインタの配列のコピー
        TArray<URnPoint*> Points;
        // 上記ポイントから生成された URnLineString オブジェクト
        URnLineString* LineString = nullptr;
    };

    // キャッシュマップ。
    // キーは各ポイントの DebugMyId を XOR 演算した値、値は FPointCache の配列。
    TMap<uint64, TArray<FPointCache>> RnPointList2LineStringMap;

public:
    FPLATEAULineStringFactoryWork() {}

    /**
     * 2 つの URnPoint 配列が等しいかを判定します（逆順の場合も許容）。
     * @param A 比較元の配列
     * @param B 比較対象の配列
     * @param bIsReversed [アウト] B が A の逆順の場合 true に設定
     * @return 等しければ true、そうでなければ false を返す
     */
    static bool IsEqual(const TArray<URnPoint*>& A, const TArray<URnPoint*>& B, bool& bIsReversed);

    /**
     * URnPoint 配列から URnLineString を生成します。
     * キャッシュが有効であれば、既存のオブジェクトを返します。
     *
     * @param Points           : URnPoint ポインタの配列
     * @param bIsCached [アウト]: キャッシュが使用された場合 true に設定
     * @param bIsReversed [アウト]: 生成された LineString の順序が逆の場合 true に設定
     * @param bUseCache        : キャッシュを利用するか否か (デフォルトは true)
     * @param CreateLineStringFunc: カスタム生成関数（省略可）。シグネチャは URnLineString*(const TArray<URnPoint*>&)
     * @return 生成またはキャッシュから取得した URnLineString オブジェクト
     */
    URnLineString* CreateLineString(
        const TArray<URnPoint*>& Points,
        bool& bIsCached,
        bool& bIsReversed,
        bool bUseCache = true,
        TFunction<URnLineString* (const TArray<URnPoint*>&)> CreateLineStringFunc = nullptr);

    /**
     * URnPoint 配列から URnWay を生成します。
     * 内部で URnLineString の生成（キャッシュ利用可能）を行います。
     *
     * @param Points           : URnPoint ポインタの配列
     * @param bIsCached [アウト]: URnLineString の生成においてキャッシュが利用された場合 true に設定
     * @param bUseCache        : キャッシュを利用するか (デフォルトは true)
     * @return 生成された URnWay オブジェクト
     */
    URnWay* CreateWay(
        const TArray<URnPoint*>& Points,
        bool& bIsCached,
        bool bUseCache = true);

    /**
     * bIsCached のアウト引数なしバージョン
     *
     * @param Points : URnPoint ポインタの配列
     * @return 生成された URnWay オブジェクト
     */
    URnWay* CreateWay(const TArray<URnPoint*>& Points);
};

