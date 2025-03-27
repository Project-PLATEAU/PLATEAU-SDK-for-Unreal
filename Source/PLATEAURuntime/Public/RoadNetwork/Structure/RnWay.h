// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RnPoint.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/GeoGraph/AxisPlane.h"
#include "RoadNetwork/GeoGraph/LineSegment2D.h"
#include "RoadNetwork/GeoGraph/LineSegment3D.h"
#include "CoreMinimal.h"
#include "RnLineString.h"
#include "RnPoint.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RnWay.generated.h"

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnWay : public UObject
{
    GENERATED_BODY()
public:
    class PointIterator
    {
    public:
        PointIterator(const URnWay* InWay, int32 index)
            : Way(InWay)
            , Index(index) {
        }

        TRnRef_T<URnPoint> operator*() const {
            return Way->GetPoint(Index);
        }

        PointIterator& operator++() {
            ++Index;
            return *this;
        }

        bool operator!=(const PointIterator& Other) const {
            return Index != Other.Index;
        }
        const URnWay* Way;
        int32 Index;
    };

    class PointsEnumerator
    {
    public:
        PointsEnumerator(const URnWay* InWay)
            : Way(InWay)
        {
        }
        PointIterator begin() const {
            return PointIterator(Way, 0);
        }

        PointIterator end() const {
            return PointIterator(Way,  Way ?Way->Count() : 0);
        }

        TArray<TRnRef_T<URnPoint>> ToArray() const {
            TArray<TRnRef_T<URnPoint>> Result;
            if (Way) {
                for (int32 i = 0; i < Way->Count(); ++i) {
                    Result.Add(Way->GetPoint(i));
                }
            }
            return Result;
        }

        const URnWay* Way;
    };

    class VertexIterator {
    public:
        VertexIterator(const URnWay* InWay, int32 index)
            : Way(InWay)
            , Index(index) {
        }

        FVector operator*() const {
            return Way->GetVertex(Index);
        }

        VertexIterator& operator++() {
            ++Index;
            return *this;
        }

        bool operator!=(const VertexIterator& Other) const {
            return Index != Other.Index;
        }
        const URnWay* Way;
        int32 Index;
    };

    class VertexEnumerator {
    public:
        VertexEnumerator(const URnWay* InWay)
            : Way(InWay) {
        }
        VertexIterator begin() const {
            return VertexIterator(Way, 0);
        }

        VertexIterator end() const {
            return VertexIterator(Way,  Way ?Way->Count() : 0);
        }

        TArray<FVector> ToArray() const {
            TArray<FVector> Result;
            if (Way) {
                for (int32 i = 0; i < Way->Count(); ++i) {
                    Result.Add(Way->GetVertex(i));
                }
            }
            return Result;
        }

        const URnWay* Way;
    };

public:
    URnWay();
    URnWay(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed = false, bool bInIsReverseNormal = false);
    void Init();
    void Init(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed = false, bool bInIsReverseNormal = false);


    // URnPointのイテレータ
    PointsEnumerator GetPoints() const
    {
        return PointsEnumerator(this);    
    }

    // FVectorのイテレータ
    VertexEnumerator GetVertices() const {
        return VertexEnumerator(this);
    }

    TRnRef_T<URnLineString> GetLineString() const {
        return LineString;
    }

    int32 Count() const;
    bool IsValid() const;

    TRnRef_T<URnPoint> GetPoint(int32 Index) const;
    TRnRef_T<URnPoint> SetPoint(int32 Index, const TRnRef_T<URnPoint>& Point);
    void SetPoints(const TArray<TRnRef_T<URnPoint>>& Points);

    FVector GetVertex(int32 Index) const;
    FVector operator[](int32 Index) const;

    TRnRef_T<URnWay> ReversedWay() const;
    void Reverse(bool KeepNormalDir);

    FVector GetVertexNormal(int32 VertexIndex) const;
    FVector GetEdgeNormal(int32 StartVertexIndex) const;

    void MoveAlongNormal(float Offset);
    void Move(const FVector& Offset);

    TRnRef_T<URnWay> Clone(bool CloneVertex) const;

    /*
     * 自身の浅いコピーを返す
     */
    TRnRef_T<URnWay> ShallowClone() const;

    TArray<FLineSegment2D> GetEdges2D() const;

    int32 FindPoint(const TRnRef_T<URnPoint>& Point) const;
    int32 FindPointIndex(const TRnRef_T<URnPoint>& Point) const;
    void GetNearestPoint(const FVector& Pos, FVector& OutNearest, float& OutPointIndex, float& OutDistance) const;
    bool IsValidOrDefault() const;
    float CalcLength() const;
    float CalcLength(float StartIndex, float EndIndex) const;
    void AppendBack2LineString(const TRnRef_T<URnWay>& Back);
    void AppendFront2LineString(const TRnRef_T<URnWay>& Front);
    bool IsOutSide(const FVector& V, FVector& OutNearest, float& OutDistance) const;
    void MoveLerpAlongNormal(const FVector& StartOffset, const FVector& EndOffset);
    FVector GetAdvancedPointFromFront(float Offset, int32& OutStartIndex, int32& OutEndIndex) const;
    FVector GetAdvancedPointFromBack(float Offset, int32& OutStartIndex, int32& OutEndIndex) const;
    FVector GetAdvancedPoint(float Offset, bool Reverse, int32& OutStartIndex, int32& OutEndIndex) const;
    FVector GetAdvancedPoint(float Offset, bool Reverse) const;
    float GetDistance2D(const TRnRef_T<URnWay>& Other, EAxisPlane Plane = FPLATEAURnDef::Plane) const;

    /**
     * 線の端からDistanceメートル辿ったときの位置を返します。
     * EndSideがtrueの場合、線を逆（配列のend側）から辿ります。
     * @param Distance - 辿る距離（メートル）
     * @param EndSide - trueの場合、線を逆から辿ります
     * @return 指定した距離の位置
     */
    FVector PositionAtDistance(float Distance, bool EndSide) const;

    // 線分の距離をp : (1-p)で分割した点を返す
    FVector GetLerpPoint(float P) const;
    float GetLerpPoint(float P, FVector& OutMidPoint) const;

    // 同じLineStringを参照しているかどうか
    bool IsSameLineReference(const URnWay* Other) const;

    // 同じ頂点列を持っているかどうか
    bool IsSameLineSequence(const URnWay* Other) const;

    // 自身をnum分割して返す. 分割できない(頂点空）の時は空リストを返す.
    // insertNewPoint=trueの時はselfにも新しい点を追加する
    TArray<TRnRef_T<URnWay>> Split(int32 Num, bool InsertNewPoint, TFunction<float(int32)> RateSelector = nullptr);

    /*
     * ファクトリメソッド
     */
    static TRnRef_T<URnWay> Create(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed = false, bool bInIsReverseNormal = false);

    /*
     * A,BをつなげたWayを作成する
     */
    static TRnRef_T<URnWay> CreateMergedWay(URnWay* A, URnWay* B, bool RemoveDuplicate = true);
private:
    int32 ToRawIndex(int32 Index, bool AllowMinus = false) const;
    int32 SwitchIndex(int32 Index) const;
    float SwitchIndex(float Index) const;

public:
    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    bool IsReversed;

    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    bool IsReverseNormal;

    UPROPERTY(VisibleAnywhere, Category = "PLATEAU")
    URnLineString* LineString;
};

struct FRnWayEx
{
    // Selfの長さを計算する
    static float CalcLengthOrDefault(const URnWay* Self);

    // Self != nullptr && Self->IsValid()
    static bool IsValidWayOrDefault(const URnWay* Self);

    // selfにsrcを結合しようとする(内部のLineStringに結合しようとする).結合できない場合はfalseを返す
    // self (v0, v1, v2, v3...vn) src(v0', v1', v2', v3'...vm')の場合
    // v0 == v0'だと, srcを逆順にselfの先頭に追加 (vm'... v3', v2', v1', v0'(v0), v1, v2, v3...vn)となる
    // vn == vm'だと, srcを逆順にselfの末尾に追加 (v0, v1, v2, v3...vn(vm')... v3', v2', v1', v0')となる
    // v0 == vm'だと, srcをそのままselfの先頭に追加 (v0', v1', v2', v3'...vm'(v0), v1, v2, v3...vn)となる
    // vn == v0'だと, srcをそのままselfの末尾に追加 (v0, v1, v2, v3...vn(v0'), v1', v2', v3'...vm')となる
    // selfとsrcの最初と最後のポイントを見て, どっちの方向に結合するかを決める
    static bool TryMergePointsToLineString(URnWay* Self, URnWay* Src, float PointDistanceTolerance);


    /*
     * 自身の浅いコピーを返す
     */
    static TRnRef_T<URnWay> ShallowCloneOrDefault(URnWay* Self);
};
