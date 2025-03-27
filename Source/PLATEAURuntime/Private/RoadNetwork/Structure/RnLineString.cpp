// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnLineString.h"

#include <optional>

#include "RoadNetwork/GeoGraph/GeoGraph2d.h"
#include "RoadNetwork/GeoGraph/GeoGraphEx.h"
#include "RoadNetwork/Util/PLATEAUVector2DEx.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/Factory/RoadNetworkFactory.h"
#include "RoadNetwork/Util/PLATEAURnLinq.h"

URnLineString::URnLineString() {
}

URnLineString::URnLineString(int32 InitialSize)
{
    Points.SetNum(InitialSize);
}

URnLineString::URnLineString(const TArray<TRnRef_T<URnPoint>>& InPoints)
{
    Points = InPoints;
}

void URnLineString::Init()
{
    
}

void URnLineString::Init(int32 InitialSize)
{
    Points.SetNum(InitialSize);
}

void URnLineString::Init(const TArray<TRnRef_T<URnPoint>>& InPoints)
{
    Points = InPoints;
}

int32 URnLineString::Count() const
{ return Points.Num(); }

bool URnLineString::IsValid() const
{ return Points.Num() >= 2; }


namespace
{
    bool IsNearlyEqual(URnPoint* A, URnPoint* B, float DistanceEpsilon)
    {
        return URnPoint::Equals(A, B, DistanceEpsilon < 0 ? -1.f : DistanceEpsilon * DistanceEpsilon);
    }

    bool IsCollinear(URnPoint* A, URnPoint* B, URnPoint* C, float DegEpsilon, float MidPointTolerance)
    {
        return FGeoGraphEx::IsCollinear(A->GetVertex(), B->GetVertex(), C->GetVertex(), DegEpsilon, MidPointTolerance);
    }
}

void URnLineString::AddPointOrSkip(TRnRef_T<URnPoint> Point, float DistanceEpsilon, float DegEpsilon, float MidPointTolerance) {
    if (!Point) 
        return;

    if (Points.Num() > 0 && ::IsNearlyEqual(Points.Last(), Point, DistanceEpsilon))
        return;

    if (Points.Num() > 1 && ::IsCollinear(Points[Points.Num() - 2], Points[Points.Num() - 1], Point, DegEpsilon, MidPointTolerance)) {
        Points.RemoveAt(Points.Num() - 1);
    }

    Points.Add(Point);
}

void URnLineString::AddPointFrontOrSkip(TRnRef_T<URnPoint> Point, float DistanceEpsilon, float DegEpsilon, float MidPointTolerance) {
    if (!Point) return;

    if (Points.Num() > 0 && ::IsNearlyEqual(Points[0], Point, DistanceEpsilon))
        return;

    if (Points.Num() > 1 && ::IsCollinear(Points[0], Points[1], Point, DegEpsilon, MidPointTolerance)) {
        Points.RemoveAt(1);
    }

    Points.Insert(Point, 0);
}
FVector URnLineString::GetEdgeNormal(int32 StartVertexIndex) const {
    const FVector P0 = (*this)[StartVertexIndex];
    const FVector P1 = (*this)[StartVertexIndex + 1];
    return FVector::CrossProduct(FVector::UpVector, P1 - P0).GetSafeNormal() * -1.0f;
}

TRnRef_T<URnLineString> URnLineString::Clone(bool CloneVertex) const {
    auto NewLineString = RnNew<URnLineString>();

    if (CloneVertex) {
        for (const TRnRef_T<URnPoint> Point : Points) {
            NewLineString->Points.Add(Point->Clone());
        }
    }
    else {
        NewLineString->Points = Points;
    }

    return NewLineString;
}

float URnLineString::CalcLength() const {
    if (!IsValid()) return 0.0f;

    float Length = 0.0f;
    for (int32 i = 0; i < Points.Num() - 1; ++i) {
        Length += (Points[i + 1]->Vertex - Points[i]->Vertex).Size();
    }
    return Length;
}

TArray<TRnRef_T<URnLineString>> URnLineString::Split(int32 Num, bool InsertNewPoint, TFunction<float(int32)> RateSelector) {
    TArray<TRnRef_T<URnLineString>> Result;
    if (!IsValid() || Num <= 0) return Result;

    if (!RateSelector)
        RateSelector = [Num](int32) { return 1.f / Num; };

    TArray<TRnRef_T<URnLineString>> Ret;

    const float TotalLength = CalcLength();
    float len = 0.0f;
    TArray<TRnRef_T<URnPoint>> SubVertices = { Points[0]};

    auto GetLength = [&](int32 Index) -> float {
        return TotalLength * RateSelector(Index);
        };
    for (int32 i = 1; i < Points.Num(); i++) 
    {
        auto p0 = SubVertices.Last();
        auto p1 = Points[i];
        auto l = (p1->Vertex - p0->Vertex).Length();
        len += l;

        auto length = GetLength(Ret.Num());
        // lenがlengthを超えたら分割線分を追加
        // ただし、最後の線は全部追加する
        while (len >= length && l >= FGeoGraph2D::Eps && Ret.Num() < (Num - 1)) {
            // #TODO : マジックナンバー
            //       : 分割点が隣り合う点とこれ以下の場合は新規で作らず使いまわす
            auto mergeLength = FMath::Min( FPLATEAURnDef::Meter2Unit * 0.1f, length * 0.5f);
            auto threshold = mergeLength * mergeLength;
            auto f = 1.f - (len - length) / l;
            auto end = RnNew<URnPoint>(FMath::Lerp(p0->Vertex, p1->Vertex, f));

            // もし,p0/p1とほぼ同じ点ならそっちを使う
            // ただし、その結果subVerticesが線分にならない場合は無視する
            if (SubVertices.Num() > 1) {
                if ((p1->Vertex - end->Vertex).SquaredLength() < threshold) {
                    end = p1;
                }
                else if ((p0->Vertex - end->Vertex).SquaredLength() < threshold) {
                    end = p0;
                }
            }

            // 同一頂点が複数あった場合は無視する
            if (f >= FLT_EPSILON) {
                SubVertices.Add(end);
                // 自分自身にも追加する場合
                if (InsertNewPoint && p1 != end && p0 != end) {
                    Points.Insert(end, i);
                    i += 1;
                }
            }

            Ret.Add( RnNew<URnLineString>(SubVertices));
            SubVertices = { end };
            len -= length;
            // 次の長さを更新
            length = GetLength(Ret.Num());
        }
        if (SubVertices.IsEmpty() == false && SubVertices.Last() != p1)
            SubVertices.Add(p1);
    }

    // 最後の要素は無条件で返す
    if (Ret.Num() < Num && SubVertices.IsEmpty() == false) {
        if (SubVertices.Last() != Points.Last())
            SubVertices.Add(Points.Last());
        if (SubVertices.Num() > 1)
            Ret.Add(RnNew<URnLineString>(SubVertices));
    }
    return Ret;
}

bool URnLineString::SplitByIndex(float Index, URnLineString*& OutFront, URnLineString*& OutBack, TFunction<URnPoint*(FVector)> CreatePoint) const
{
    auto IsInt = FMath::Abs(Index - FMath::RoundToFloat(Index)) < KINDA_SMALL_NUMBER;

    auto I = IsInt ? FMath::RoundToFloat(Index) : (int)Index;

    // points = [v0, v1, v2, v3]の時
    // index = 1で区切るときは, front = [v0, v1], back = [v1, v2, v3]
    // -> i = 1, frontはTake(1), backはskip(2)にして、v1をお互いに追加
    // index = 1.5で区切るときは,front = [v0, v1, v1.5], back = [v1.5, v2, v3]
    // -> i = 1, frontはTake(2), backはskip(2)にして、v1.5を追加

    TArray<URnPoint*> FrontPoints;
    TArray<URnPoint*> BackPoints;
    for(auto i = 0; i <= FMath::Min(I, Count() - 1); ++i)
        FrontPoints.Add(Points[i]);

    // 整数の時はfrontの最後をbackの最初に追加
    if (IsInt) {
        BackPoints.Add(Points[I]);
    }
    else
    {
        // 少数の時は中間点をfontの最後とbackの最初に追加
        auto V = FMath::Lerp(Points[I]->Vertex, Points[I + 1]->Vertex, Index - I);
        auto MidPoint = CreatePoint(V);
        FrontPoints.Add(MidPoint);
        BackPoints.Add(MidPoint);
    }

    for (auto i = I + 1; i < Count(); ++i)
        BackPoints.Add(Points[i]);


    TArray<TRnRef_T<URnLineString>> Result;
    OutFront = RnNew<URnLineString>(FrontPoints);
    OutBack = RnNew<URnLineString>(BackPoints);
    return true;
}
void URnLineString::AddFrontPoint(TRnRef_T<URnPoint> Point) {
    if (Point) {
        Points.Insert(Point, 0);
    }
}

float URnLineString::CalcLength(float StartPointIndex, float EndPointIndex) const
{
    // Determine the starting and ending indices, clamped to valid range.
    int32 stI = FMath::Max(0, FMath::FloorToInt(StartPointIndex));
    int32 enI = FMath::Min(Count() - 1, FMath::FloorToInt(EndPointIndex));

    // If the starting index is the last, there's no segment.
    if (stI >= Count() - 1) {
        return 0.f;
    }

    float t = StartPointIndex - stI;
    // Linearly interpolate between the two points.
    FVector last = FMath::Lerp((*this)[stI], (*this)[stI + 1], t);
    float ret = 0.f;
    // Sum lengths for full segments between stI+1 and enI.
    for (int32 i = stI + 1; i <= enI; ++i) {
        ret += ((*this)[i] - last).Size();
        last = (*this)[i];
    }
    // If the end index is not the last point, add the partial segment.
    if (enI < Count() - 1) {
        float t2 = EndPointIndex - enI;
        ret += (FMath::Lerp((*this)[enI], (*this)[enI + 1], t2) - last).Size();
    }
    return ret;
}

float URnLineString::CalcTotalAngle2D() const {
    if (Points.Num() < 3) return 0.0f;

    float TotalAngle = 0.0f;
    std::optional<FVector2D> Last = std::nullopt;
    const auto Edges = FGeoGraphEx::GetEdges(Points, false);
    for (auto It = Edges.begin(); It != Edges.end(); ++It)
    {
        const auto e = *It;
        auto Dir = FLineSegment3D(e.P0->Vertex, e.P1->Vertex).To2D(FPLATEAURnDef::Plane).GetDirection();
        if(Last.has_value())
        {
            TotalAngle += FPLATEAUVector2DEx::Angle((*Last), Dir);
        }
        Last = Dir;
    }
    return TotalAngle;
}

TArray<FLineSegment2D> URnLineString::GetEdges2D(EAxisPlane axis) const
{
    TArray<FLineSegment2D> Ret;
    const auto Edges = FGeoGraphEx::GetEdges(Points, false);
    for (auto It = Edges.begin(); It != Edges.end(); ++It) 
    {
        const auto e = *It;
        Ret.Add(FLineSegment3D(e.P0->Vertex, e.P0->Vertex).To2D(axis));
    }
    return Ret;
}


TArray<FLineSegment3D> URnLineString::GetEdges() const
{
    TArray<FLineSegment3D> Ret;
    const auto Edges = FGeoGraphEx::GetEdges(Points, false);
    for (auto It = Edges.begin(); It != Edges.end(); ++It) {
        const auto e = *It;
        Ret.Add(FLineSegment3D(e.P0->Vertex, e.P1->Vertex));
    }
    return Ret;
}

bool URnLineString::Contains(TRnRef_T<URnPoint> Point) const {
    return Points.Contains(Point);
}

void URnLineString::GetNearestPoint(const FVector& Pos, FVector& OutNearest, float& OutPointIndex, float& OutDistance) const {
    OutDistance = MAX_FLT;
    OutPointIndex = 0;

    for (int32 i = 0; i < Points.Num() - 1; ++i) {
        const FVector Start = GetVertex(i);
        const FVector End = GetVertex(i+1);
        const FVector ProjectedPoint = FMath::ClosestPointOnSegment(Pos, Start, End);


        auto Diff = (End - Start).Size();
        if (Diff < 1e-8f)
            continue;

        const float Distance = (Pos - ProjectedPoint).Size();
        if (Distance < OutDistance) {

            OutDistance = Distance;
            OutNearest = ProjectedPoint;
            auto Offset = (ProjectedPoint - Start);
            OutPointIndex = i + Offset.Size() / Diff;
        }
    }
}

float URnLineString::GetDistance2D(const TRnRef_T<URnLineString> Other, EAxisPlane Plane) const {
    if (!Other || !IsValid() || !Other->IsValid()) return MAX_FLT;

    float MinDistance = MAX_FLT;
    auto SelfEdges = GetEdges2D(Plane);
    auto OtherEdges = Other->GetEdges2D(Plane);
    for (auto& s1 : SelfEdges) 
    {
        for (auto& s2 : OtherEdges) {
            MinDistance = FMath::Min(MinDistance, s1.GetDistance(s2));
        }
    }

    return MinDistance;
}

FVector URnLineString::GetVertexNormal(int32 VertexIndex) const {
    if (Points.Num() <= 1) return FVector::ZeroVector;

    FVector Normal = FVector::ZeroVector;
    int32 Count = 0;

    if (VertexIndex > 0) {
        Normal += GetEdgeNormal(VertexIndex - 1);
        Count++;
    }

    if (VertexIndex < Points.Num() - 1) {
        Normal += GetEdgeNormal(VertexIndex);
        Count++;
    }

    return (Count > 0) ? Normal / Count : FVector::ZeroVector;
}

int32 URnLineString::ReplacePoint(TRnRef_T<URnPoint> OldPoint, TRnRef_T<URnPoint> NewPoint) {
    int32 ReplaceCount = 0;
    for (int32 i = 0; i < Points.Num(); i++) {
        if (Points[i] == OldPoint) {
            Points[i] = NewPoint;
            ReplaceCount++;
        }
    }
    return ReplaceCount;
}


FVector URnLineString::operator[](int32 Index) const
{ return (Points)[Index]->Vertex; }

FVector URnLineString::GetVertex(int32 Index) const
{
    return (*this)[Index];
}

TRnRef_T<URnPoint> URnLineString::GetPoint(int32 Index) const
{
    return (Points)[Index];
}

FVector URnLineString::GetVertexByFloatIndex(float index) const
{
    const auto I1 = (int)index;
    const auto I2 = I1 + 1;
    if (I2 >= Count()) {
        return GetVertex(Count()-1);
    }
    const auto t = index - I1;
    return FMath::Lerp((*this)[I1], (*this)[I2], t);
}

void URnLineString::SetPoint(int32 Index, const TRnRef_T<URnPoint>& Point)
{
    (Points)[Index] = Point;
}

FVector URnLineString::GetAdvancedPointFromFront(float Offset, int32& OutStartIndex, int32& OutEndIndex) const {
    return GetAdvancedPoint(Offset, false, OutStartIndex, OutEndIndex);
}

FVector URnLineString::GetAdvancedPointFromBack(float Offset, int32& OutStartIndex, int32& OutEndIndex) const {
    return GetAdvancedPoint(Offset, true, OutStartIndex, OutEndIndex);
}

FVector URnLineString::GetAdvancedPoint(float Offset, bool bReverse, int32& OutStartIndex, int32& OutEndIndex) const
{
    if (Count() == 0) {
        OutStartIndex = OutEndIndex = -1;
        return FVector::ZeroVector;
    }

    int32 Delta = bReverse ? -1 : 1;
    int32 BeginIndex = bReverse ? Count() - 1 : 0;

    int32 Index = BeginIndex;
    for (int32 i = 0; i < Count() - 1; ++i) {
        int32 NextIndex = Index + Delta;
        FVector P0 = (*this)[Index];
        FVector P1 = (*this)[NextIndex];
        float Len = (P0 - P1).Size();

        if (Len >= Offset) {
            OutStartIndex = Index;
            OutEndIndex = Index + Delta;
            return P0 + (P1 - P0).GetSafeNormal() * Offset;
        }

        Offset -= Len;
        Index = NextIndex;
    }

    OutStartIndex = OutEndIndex = Count() - 1 - BeginIndex;
    return (*this)[OutEndIndex];
}

TArray<TTuple<float, FVector>> URnLineString::GetIntersectionBy2D(
    const FLineSegment3D& LineSegment,
    EAxisPlane Plane) const {
    TArray<TTuple<float, FVector>> Result;
    auto Edges = GetEdges();
    for(auto i = 0; i < Edges.Num(); ++i)
    {
        auto E = Edges[i];
        FVector P;
        float T1;
        float T2;
        if(E.TrySegmentIntersectionBy2D(LineSegment, Plane, -1.f, P, T1, T2))
        {
            auto V = E.Lerp(T1);
            Result.Add(MakeTuple(i + T1, V));
        }
    }

    return Result;
}

TArray<TTuple<float, FVector>> URnLineString::GetIntersectionBy2D(const FRay& Ray, EAxisPlane Plane) const
{
    TArray<TTuple<float, FVector>> Result;
    auto Edges = GetEdges();
    for (auto i = 0; i < Edges.Num(); ++i) {
        auto E = Edges[i];
        FVector P;
        float LineLength;
        float SegmentT;
        if (E.TryLineIntersectionBy2D(Ray.Origin, Ray.Direction, Plane, -1.f, P, LineLength, SegmentT)) {
            Result.Add(MakeTuple(i + SegmentT, P));
        }
    }

    return Result;
}

bool URnLineString::TryGetNearestIntersectionBy2D(const FRay& Ray, TTuple<float, FVector>& Res,
                                                  EAxisPlane Plane) const {
    auto ret = GetIntersectionBy2D(Ray, Plane);
    if (ret.IsEmpty()) {
        return false;
    }
    return FPLATEAURnLinq::TryFindMinElement<TTuple<float, FVector>>(
        ret,
        [&](const TTuple<float, FVector>& X, const TTuple<float, FVector>& Y) {
            auto A = (X.Value - Ray.Origin).SizeSquared();
            auto B = (Y.Value - Ray.Origin).SizeSquared();
            return A < B;
        }
    , Res);
}

TOptional<float> URnLineString::CalcProximityScore(const URnLineString* Other) const
{
    if (IsValid() == false || !Other || !Other->IsValid())
        return NullOpt;

    return FPLATEAURnLinq::Average<URnPoint*, float>(Points, [&](const TRnRef_T<URnPoint>& V) {
        FVector Inter;
        float Index;
        float Distance;
        Other->GetNearestPoint(V->Vertex, Inter, Index, Distance);
        return Distance;
        });
}

TRnRef_T<URnLineString> URnLineString::Create(const TArray<TRnRef_T<URnPoint>>& Vertices, float DistanceEpsilon,
    float DegEpsilon, float MidPointTolerance)
{
    auto LineString = RnNew<URnLineString>();
    for (auto& P : Vertices)
        LineString->AddPointOrSkip(P, DistanceEpsilon, DegEpsilon, MidPointTolerance);
    return LineString;
}


TRnRef_T<URnLineString> URnLineString::Create(const TArray<TRnRef_T<URnPoint>>& Vertices,
                                              bool RemoveDuplicate) {
    if (RemoveDuplicate)
        return Create(Vertices, DefaultDistanceEpsilon, DefaultDegEpsilon, DefaultMidPointTolerance);

    return Create(Vertices, -1.f, -1.f, -1.f);
}

TRnRef_T<URnLineString> URnLineString::Create(const TArray<FVector>& Vertices, bool RemoveDuplicate) {
    auto Points = TArray<TRnRef_T<URnPoint>>();
    for (const FVector& Vertex : Vertices) {
        auto P = RnNew<URnPoint>(Vertex);
        Points.Add(P);
    }

    return Create(Points, RemoveDuplicate);
}

bool URnLineString::Equals(const TRnRef_T<URnLineString> X, const TRnRef_T<URnLineString> Y) {
    if (X == Y) return true;
    if (!X || !Y) return false;
    if (X->Points.Num() != Y->Points.Num()) return false;

    for (int32 i = 0; i < X->Points.Num(); i++) {
        if (!URnPoint::Equals(X->GetPoint(i), Y->GetPoint(i))) {
            return false;
        }
    }

    return true;
}

bool FRnLineStringEx::IsSequenceEqual(URnLineString* A, URnLineString* B, bool& OutIsReverse)
{
    if (!A || !B)
        return false;
    return FPLATEAULineStringFactoryWork::IsEqual(A->GetPoints(), B->GetPoints(), OutIsReverse);
}

bool FRnLineStringEx::IsPointShared(const URnLineString* A, const URnLineString* B)
{
    if (!A || !B)
        return false;
    for(auto P :  A->GetPoints())
    {
        if (B->GetPoints().Contains(P))
            return true;
    }
    return false;
}



