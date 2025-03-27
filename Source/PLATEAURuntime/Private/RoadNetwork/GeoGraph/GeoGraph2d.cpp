// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/GeoGraph2d.h"
#include "RoadNetwork/GeoGraph/AxisPlane.h"
#include "RoadNetwork/GeoGraph/GeoGraphEx.h"
#include "Math/UnrealMathUtility.h"
#include "MathUtil.h"

bool FGeoGraph2D::FVector2DEquitable::Equals(const FVector2D& X, const FVector2D& Y) const {
    return (X - Y).SizeSquared() < Tolerance;
}

TArray<int> FGeoGraph2D::FindMidEdge(const TArray<FVector2D>& vertices, float toleranceAngleDegForMidEdge,
    float skipAngleDeg)
{
    auto edges = FGeoGraphEx::GetEdgeSegments(vertices, false);

    // 開始線分が中心線扱いにならないようにskipAngleDegを使ってスキップする
    auto startIndex = 0;
    auto endIndex =  edges.Num() - 1;

    if (skipAngleDeg > 0.f) {
        while (startIndex < edges.Num() - 1 && FMath::Abs(FPLATEAUVector2DEx::Angle(edges[startIndex].GetDirection(), edges[startIndex + 1].GetDirection())) < skipAngleDeg)
            startIndex++;
        while (endIndex > 0 && FMathf::Abs(FPLATEAUVector2DEx::Angle(edges[endIndex].GetDirection(), edges[endIndex - 1].GetDirection())) < skipAngleDeg)
            endIndex--;
    }

    auto leftIndex = startIndex;
    auto rightIndex = endIndex;

    struct Point {
        FRay2D ray;
        bool isLeft;
    };

    struct Info
    {
        FVector2D inter;
        float tCenterRay;
        float tRay;
        bool isLeft;
        FVector2D origin;
    };
    // left ~ rightの間にエッジが1つしかなくなるまで続ける
    //   -> その残った一つがエッジ
    while (leftIndex < rightIndex - 2) {
        auto l = edges[leftIndex];
        // 0 ~ edge.Countまでつながっているような線なので逆順の線分の方向を逆にする
        auto r = edges[rightIndex].Reversed();
        // 中心線を計算
        auto centerRay = FGeoGraph2D::LerpRay(l.Ray(), r.Ray(), 0.5f);
        auto dirL = FVector2D(l.GetDirection().Y, -l.GetDirection().X);
        auto dirR = FVector2D(r.GetDirection().Y, -r.GetDirection().X);

        TArray < Info> points;
        for(const auto& x : {
            Point{ FRay2D(l.GetStart(), dirL), true},
            Point{ FRay2D(l.GetEnd(), dirL), true},
            Point{ FRay2D(r.GetStart(), dirR), false},
            Point{ FRay2D(r.GetEnd(), dirR), false}
            })
        {
            FVector2D inter;
            float t1, t2;
            auto hit = FLineUtil::LineIntersection(centerRay, x.ray, inter, t1, t2);
            if (hit) {
                points.Add(Info
                    {
                        inter,
                        t1,
                        t2,
                        x.isLeft,
                        x.ray.Origin
                    });
            }
        }
        if(points.IsEmpty())
        {
            leftIndex++;
            continue;
        }
        points.Sort([](const Info& a, const Info& b) { return a.tCenterRay < b.tCenterRay; });

        // より遠いのが左の場合右の線分を進める
        if (points.Last().isLeft) {
            rightIndex--;
        }
        else {
            leftIndex++;
        }
    }

    // ここに来る段階でleftIndex == rightIndex - 2のはず
    auto edgeBaseIndex = (leftIndex + rightIndex) / 2;
    auto ret = TArray<int>{ edgeBaseIndex };
    auto stop = TArray<bool>{ false, false };
    while (stop.Contains(false) && ret.Num() < edges.Num() - 1) {
        // 0 : left用
        // 1 : right用
        struct Del
        {
            int now; int d;        
        };
        auto infos = TArray<Del>
            {
                {ret[0], -1},
                {ret.Last() , +1 }
            };

        struct EdgeInfo
        {
            int32 i;
            int32 index;
            float ang;
        };
        TArray<EdgeInfo> es;
        // 差が小さいほうから見る
        for(auto i = 0; i < 2; ++i)
        {
            auto& info = infos[i];
            const auto Index = info.now + info.d;
            if (stop[i] || Index < startIndex || endIndex < Index)
                continue;
            auto e0 = edges[edgeBaseIndex];
            auto e1 = edges[Index];
            es.Add({ i, Index, FPLATEAUVector2DEx::Angle(e0.GetDirection(), e1.GetDirection()) });
        }
        if (es.IsEmpty())
            break;
        es.Sort([](const EdgeInfo& a, const EdgeInfo& b)
        {
                return a.ang < b.ang;
        });

        for(auto& e : es) 
        {
            if (e.ang > toleranceAngleDegForMidEdge) {
                stop[e.i] = true;
                continue;
            }

            if (e.i == 0) {
                ret.Insert(e.index, 0);
            }
            else {
                ret.Add(e.index);
            }
        }
    }
    // edge -> 頂点の配列に戻すために最後のインデックスを足す
    ret.Add(ret.Last() + 1);
    return ret;
}

bool FGeoGraph2D::CalcLerpPointInLine(const FRay2D& RayA, const FRay2D& RayB, float P, FVector2D& OutPos)
{
    P = FMath::Clamp(P, 0.0f, 1.0f);
    const float P2 = P * P;
    const float DotDab = FVector2D::DotProduct(RayA.Direction, RayB.Direction);
    const FVector2D DOg = RayA.Origin - RayB.Origin;
    const float DotDao = FVector2D::DotProduct(RayA.Direction, DOg);
    const float DotDbo = FVector2D::DotProduct(RayB.Direction, DOg);

    const float A = 2.0f * P - 1.0f - P2 * DotDab * DotDab;
    const float B = 2.0f * P2 * (DotDao - DotDab * DotDbo);
    const float C = P2 * (DOg.SizeSquared() - DotDbo * DotDbo);
    float D = B * B - 4.0f * A * C;

    // Handle numerical precision for D near zero
    if (D < 0.0f && FMath::Abs(D) < Eps) {
        D = 0.0f;
    }

    OutPos = FVector2D::ZeroVector;
    float T;

    // Linear equation case
    if (FMath::Abs(A) < Eps) {
        if (FMath::IsNearlyZero(B)) {
            return false;
        }
        T = -C / B;
    }
    // Quadratic equation case 
    else {
        if (D < 0.0f) {
            return false;
        }

        const float T1 = (-B + FMath::Sqrt(D)) / (2.0f * A);
        const float T2 = (-B - FMath::Sqrt(D)) / (2.0f * A);

        T = T1;
        if (FMath::Abs(T1) > FMath::Abs(T2)) {
            T = T2;
        }
    }

    OutPos = RayA.Direction * T + RayA.Origin;
    return true;
}
FRay2D FGeoGraph2D::LerpRay(const FRay2D& rayA, const FRay2D& rayB, float p)
{

    // 2線が平行の時は交点が無いので特別処理
    FVector2D intersection;
    float t1;
    float t2;
    if (FLineUtil::LineIntersection(rayA, rayB, intersection, t1, t2) == false) 
    {
        auto aPos = FVector2D::DotProduct(rayB.Origin - rayA.Origin, rayA.Direction) * rayA.Direction + rayA.Origin;
        auto origin = FMath::Lerp(aPos, rayB.Origin, p);
        return FRay2D(origin, rayA.Direction);
    }

    auto dirA = rayA.Direction;
    auto dirB = rayB.Direction;

    auto radX = FMath::DegreesToRadians(FPLATEAUVector2DEx::Angle(dirA, dirB));
    auto siX = FMath::Sin(radX);
    auto coX = FMath::Cos(radX);
    // a-b間の角度をx
    // a-l間の角度A
    // l-b間の角度(x - A)
    // sin(A) : sin(B) = p : (1-p)
    // B = X - A
    // sin(A) : sin(X - A) = p : (1-p)
    // Sin(A) : sin(X)cos(A) - cos(X)sin(A) = p : (1-p)
    // (1-p)Sin(A) = p ( sin(X)cos(A) - cos(X)sin(A))
    // ((1-p) + p * cos(X))sin(A) = p*sin(X)cos(A)
    // tan(A) = p*sin(X) / ((1-p) + p * cos(X))
    auto radA = FMath::Atan2(p * siX, 1 - p + p * coX);
    auto dir = FPLATEAUVector2DEx::RotateTo(dirA, dirB, radA);

    // a,bが平行に近いとintersectionが遠点となりfloat誤差が発生するため, a,bのStartからdirへの射影をして見つかった位置をoriginにする
    auto inters = TArray<FVector2D>();
    inters.Reserve(2);
    // rayAの法線上の点posにおいて, len(rayA.origin - pos) : distance(pos - rayB) = p : 1-pとなる点は, 答えのray上にある
    FVector2D pos;
    if (CalcLerpPointInLine(FRay2D(rayA.Origin, FPLATEAUVector2DEx::Rotate(rayA.Direction, 90)), rayB, p,  pos)) {
        inters.Add(pos);
    }
    FVector2D pos2;
    if (CalcLerpPointInLine(FRay2D(rayB.Origin, FPLATEAUVector2DEx::Rotate(rayB.Direction, 90)), rayA, p,pos2)) 
    {
        inters.Add(pos2);
    }

    if (inters.Num() == 0)
        return FRay2D(intersection, dir);

    if (inters.Num() == 1)
        return FRay2D(inters[0], dir);

    if (FVector2D::DotProduct(dir, inters[1] - inters[0]) > 0)
        return FRay2D(inters[0], dir);
    return FRay2D(inters[1], dir);
}

bool FGeoGraph2D::IsLastClockwise(const TArray<FVector2D>& List) {
    if (List.Num() <= 2) {
        return true;
    }

    const FVector2D& V1 = List[List.Num() - 1];
    const FVector2D& V2 = List[List.Num() - 2];
    const FVector2D& V3 = List[List.Num() - 3];

    const FVector2D D1 = V1 - V2;
    const FVector2D D2 = V2 - V3;

    // Cross product in 2D
    float Cross = D1.X * D2.Y - D1.Y * D2.X;
    return Cross > 0;
}

TArray<FVector> FGeoGraph2D::ComputeConvexVolume(
    const TArray<FVector>& Vertices,
    TFunction<FVector2D(const FVector&)> ToVec2)
{
    TArray<FVector2D > Points2D;
    for(auto& v : Vertices)
        Points2D.Add(ToVec2(v));

    auto ConvexHull2D = ComputeConvexVolume(Points2D);

    TArray<FVector> Result;
    for (const auto& Point2D : ConvexHull2D) {
        for (const auto& Vertex : Vertices) {
            if (FVector2D::Distance(ToVec2(Vertex), Point2D) < Eps) {
                Result.Add(Vertex);
                break;
            }
        }
    }
    return Result;
}
TArray<FVector2D> FGeoGraph2D::ComputeConvexVolume(const TArray<FVector2D>& Vertices) {
    if (Vertices.Num() <= 3) {
        return Vertices;
    }

    // Find the leftmost bottom point
    int32 StartIndex = FindMostLeftBottom<FVector2D>(Vertices, [](FVector2D V) {return V; });
    TArray<FVector2D> Points = { Vertices[StartIndex] };
    TArray<int32> Stack = { StartIndex };

    // Sort points by angle and distance
    TArray<TTuple<int32, float>> Angles;
    for (int32 i = 0; i < Vertices.Num(); ++i) {
        if (i == StartIndex) continue;

        FVector2D Diff = Vertices[i] - Vertices[StartIndex];
        float Angle = FMath::Atan2(Diff.Y, Diff.X);
        Angles.Add(MakeTuple(i, Angle));
    }

    Angles.Sort([](const TTuple<int32, float>& A, const TTuple<int32, float>& B) {
        return A.Value < B.Value;
        });

    // Graham scan
    for (const auto& IndexAngle : Angles) {
        while (Stack.Num() >= 2) {
            FVector2D P1 = Vertices[Stack[Stack.Num() - 2]];
            FVector2D P2 = Vertices[Stack.Last()];
            FVector2D P3 = Vertices[IndexAngle.Key];

            FVector2D V1 = P2 - P1;
            FVector2D V2 = P3 - P2;

            if (Cross(V1, V2) > 0) {
                break;
            }
            Stack.Pop();
            Points.Pop();
        }

        Stack.Add(IndexAngle.Key);
        Points.Add(Vertices[IndexAngle.Key]);
    }

    return Points;
}

TArray<int32> FGeoGraph2D::GetNearVertexTable(
    const TArray<FVector>& Vertices,
    TFunction<float(const FVector&, const FVector&)> CalcDistance,
    float Epsilon) {
    TArray<int32> Result;
    Result.SetNum(Vertices.Num());

    for (int32 i = 0; i < Vertices.Num(); ++i) {
        Result[i] = i;
        float MinDistance = MAX_flt;
        int32 MinIndex = i;

        for (int32 j = 0; j < i; ++j) {
            float Distance = CalcDistance(Vertices[i], Vertices[j]);
            if (Distance < MinDistance && Distance < Epsilon) {
                MinDistance = Distance;
                MinIndex = j;
            }
        }

        Result[i] = MinIndex;
    }

    return Result;
}

TArray<FVector> FGeoGraph2D::ComputeMeshOutlineVertices(
    const TArray<FVector>& Vert,
    const TArray<int32>& Triangles,
    TFunction<FVector2D(const FVector&)> ToVec2,
    float Epsilon) {
    // Create edge list from triangles
    TArray<TTuple<int32, int32>> Edges;
    for (int32 i = 0; i < Triangles.Num(); i += 3) {
        Edges.Add(MakeTuple(Triangles[i], Triangles[i + 1]));
        Edges.Add(MakeTuple(Triangles[i + 1], Triangles[i + 2]));
        Edges.Add(MakeTuple(Triangles[i + 2], Triangles[i]));
    }

    // Count edge occurrences
    TMap<TTuple<int32, int32>, int32> EdgeCount;
    for (const auto& Edge : Edges) {
        auto NormalizedEdge = Edge.Get<0>() < Edge.Get<1>() ? Edge : MakeTuple(Edge.Get<1>(), Edge.Get<0>());
        EdgeCount.Add(NormalizedEdge, EdgeCount.FindRef(NormalizedEdge) + 1);
    }

    // Find outline edges (edges that appear only once)
    TArray<TTuple<int32, int32>> OutlineEdges;
    for (const auto& Edge : Edges) {
        auto NormalizedEdge = Edge.Get<0>() < Edge.Get<1>() ? Edge : MakeTuple(Edge.Get<1>(), Edge.Get<0>());
        if (EdgeCount[NormalizedEdge] == 1) {
            OutlineEdges.Add(Edge);
        }
    }

    // Convert to continuous outline
    TArray<FVector> Result;
    if (OutlineEdges.Num() == 0) {
        return Result;
    }

    int32 CurrentVertex = OutlineEdges[0].Get<0>();
    Result.Add(Vert[CurrentVertex]);

    while (OutlineEdges.Num() > 0) {
        bool Found = false;
        for (int32 i = 0; i < OutlineEdges.Num(); ++i) {
            if (OutlineEdges[i].Get<0>() == CurrentVertex) {
                CurrentVertex = OutlineEdges[i].Get<1>();
                Result.Add(Vert[CurrentVertex]);
                OutlineEdges.RemoveAt(i);
                Found = true;
                break;
            }
            if (OutlineEdges[i].Get<1>() == CurrentVertex) {
                CurrentVertex = OutlineEdges[i].Get<0>();
                Result.Add(Vert[CurrentVertex]);
                OutlineEdges.RemoveAt(i);
                Found = true;
                break;
            }
        }

        if (!Found) {
            break;
        }
    }

    return Result;
}

float FGeoGraph2D::CalcTotalAngle(
    const TArray<FVector>& Points,
    TFunction<FVector2D(const FVector&)> ToVec2)
{
    TArray<FVector2D> Points2D;
    Points2D.Reserve(Points.Num());
    for (const auto& Point : Points)
    {
        Points2D.Add(ToVec2(Point));
    }
    return CalcTotalAngle(Points2D);
}

float FGeoGraph2D::CalcTotalAngle(const TArray<FVector2D>& Points)
{
    float TotalAngle = 0.0f;
    for (int32 i = 0; i < Points.Num(); ++i)
    {
        int32 Prev = (i + Points.Num() - 1) % Points.Num();
        int32 Next = (i + 1) % Points.Num();

        FVector2D V1 = Points[i] - Points[Prev];
        FVector2D V2 = Points[Next] - Points[i];

        float Angle = FMath::Atan2(Cross(V1, V2), FVector2D::DotProduct(V1, V2));
        TotalAngle += Angle;
    }
    return TotalAngle;
}

bool FGeoGraph2D::IsConvex(const TArray<FVector2D>& Points)
{
    if (Points.Num() < 3) return true;

    bool IsPositive = Cross(Points[1] - Points[0], Points[2] - Points[1]) > 0;
    
    for (int32 i = 1; i < Points.Num(); ++i)
    {
        int32 Next = (i + 1) % Points.Num();
        int32 NextNext = (i + 2) % Points.Num();
        
        bool CurrentIsPositive = Cross(Points[Next] - Points[i], Points[NextNext] - Points[Next]) > 0;
        if (CurrentIsPositive != IsPositive) return false;
    }
    
    return true;
}

float FGeoGraph2D::Cross(const FVector2D& A, const FVector2D& B)
{
    return A.X * B.Y - A.Y * B.X;
}