// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RnPoint.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/GeoGraph/AxisPlane.h"
#include "RoadNetwork/GeoGraph/LineSegment2D.h"
#include "RoadNetwork/GeoGraph/LineSegment3D.h"
#include <memory>

#include "RnLineString.generated.h"
UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnLineString : public UObject
{
    GENERATED_BODY()
private:
    inline static constexpr float DefaultDistanceEpsilon = 0.f * FPLATEAURnDef::Meter2Unit;
    inline static constexpr float DefaultDegEpsilon = 0.5f;
    inline static constexpr float DefaultMidPointTolerance = 0.1f * FPLATEAURnDef::Meter2Unit;
public:
    URnLineString();
    URnLineString(int32 InitialSize);
    URnLineString(const TArray<TRnRef_T<URnPoint>>& InPoints);
    void Init();
    void Init(int32 InitialSize);
    void Init(const TArray<TRnRef_T<URnPoint>>& InPoints);

    const TArray<TRnRef_T<URnPoint>>& GetPoints() const
    {
        return Points;    
    }

    TArray<TRnRef_T<URnPoint>>& GetPoints() {
        return Points;
    }

    void SetPoints(const TArray<TRnRef_T<URnPoint>>& InPoints) {
        Points = InPoints;
    }

    int32 Count() const;
    bool IsValid() const;

    TArray<TRnRef_T<URnLineString>> Split(int32 Num, bool InsertNewPoint, TFunction<float(int32)> RateSelector = nullptr);

    bool SplitByIndex(float Index,URnLineString*& OutFront, URnLineString*& OutBack, TFunction<URnPoint*(FVector)> CreatePoint = nullptr) const;
    void AddFrontPoint(TRnRef_T<URnPoint> Point);

    bool Contains(TRnRef_T<URnPoint> Point) const;
    void GetNearestPoint(const FVector& Pos, FVector& OutNearest, float& OutPointIndex, float& OutDistance) const;
    float GetDistance2D(const TRnRef_T<URnLineString> Other, EAxisPlane Plane = EAxisPlane::Xy) const;

    void AddPointOrSkip(TRnRef_T<URnPoint> Point, float DistanceEpsilon = DefaultDistanceEpsilon, float DegEpsilon = DefaultDegEpsilon, float MidPointTolerance = DefaultMidPointTolerance);
    void AddPointFrontOrSkip(TRnRef_T<URnPoint> Point, float DistanceEpsilon = DefaultDistanceEpsilon, float DegEpsilon = DefaultDegEpsilon, float MidPointTolerance = DefaultMidPointTolerance);

    FVector GetVertexNormal(int32 VertexIndex) const;
    FVector GetEdgeNormal(int32 StartVertexIndex) const;

    TRnRef_T<URnLineString> Clone(bool CloneVertex) const;
    int32 ReplacePoint(TRnRef_T<URnPoint> OldPoint, TRnRef_T<URnPoint> NewPoint);

    float CalcLength() const;
    float CalcLength(float StartIndex, float EndIndex) const;
    float CalcTotalAngle2D() const;



    TArray<FLineSegment2D> GetEdges2D(EAxisPlane axis = FPLATEAURnDef::Plane) const;
    TArray<FLineSegment3D> GetEdges() const;

    FVector operator[](int32 Index) const;

    FVector GetVertex(int32 Index) const;

    TRnRef_T<URnPoint> GetPoint(int32 Index) const;

    // floatのindex指定で座標を取ってくる
    // GetVertexByFloatIndex(1.5) -> FMath::Lerp(v[1], v[2], 0.5f)が返る
    FVector GetVertexByFloatIndex(float index) const;

    void SetPoint(int32 Index, const TRnRef_T<URnPoint>& Point);

    FVector GetAdvancedPointFromFront(float Offset, int32& OutStartIndex, int32& OutEndIndex) const;
    FVector GetAdvancedPointFromBack(float Offset, int32& OutStartIndex, int32& OutEndIndex) const;

    FVector GetAdvancedPoint(float Offset, bool bReverse, int32& OutStartIndex, int32& OutEndIndex) const;

    TArray<TTuple<float, FVector>> GetIntersectionBy2D(
        const FLineSegment3D& LineSegment,
        EAxisPlane Plane) const;

    TArray<TTuple<float, FVector>> GetIntersectionBy2D(
        const FRay& Ray,
        EAxisPlane Plane) const;

    bool TryGetNearestIntersectionBy2D(const FRay& Ray, TTuple<float, FVector>& Res, EAxisPlane Plane = FPLATEAURnDef::Plane) const;


    // selfのotherに対する距離スコアを返す(線分同士の距離ではない).低いほど近い
    // selfの各点に対して, otherとの距離を出して, その平均をスコアとする
    TOptional<float> CalcProximityScore(const URnLineString* Other) const;

    /*
     * Static関数
     */



     /*
      * 頂点リストから線分を生成する
      * RemoveDuplicate : 重複する頂点を取り除くかのフラグ
      */
    static TRnRef_T<URnLineString> Create(const TArray<TRnRef_T<URnPoint>>& Vertices, float DistanceEpsilon, float DegEpsilon, float MidPointTolerance);

    /*
     * 頂点リストから線分を生成する
     * RemoveDuplicate : 重複する頂点を取り除くかのフラグ
     */
    static TRnRef_T<URnLineString> Create(const TArray<TRnRef_T<URnPoint>>& Vertices, bool RemoveDuplicate = true);

    /*
     * 頂点リストから線分を生成する
     * RemoveDuplicate : 重複する頂点を取り除くかのフラグ
     */
    static TRnRef_T<URnLineString> Create(const TArray<FVector>& Vertices, bool RemoveDuplicate = true);

    static bool Equals(const TRnRef_T<URnLineString> X, const TRnRef_T<URnLineString> Y);

private:
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    TArray<URnPoint*> Points;
};

struct FRnLineStringEx
{
public:
    static bool IsSequenceEqual(URnLineString* A, URnLineString* B, bool& OutIsReverse);

    /*
     * ポイントを共有しているかどうかを返す
     */
    static bool IsPointShared(const URnLineString* A, const URnLineString* B);
};
