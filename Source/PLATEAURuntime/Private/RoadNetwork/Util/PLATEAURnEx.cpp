// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Util/PLATEAURnEx.h"

#include "RoadNetwork/GeoGraph/GeoGraph2d.h"
#include "RoadNetwork/GeoGraph/GeoGraphEx.h"
#include "RoadNetwork/Structure/RnLineString.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "RoadNetwork/Structure/RnPoint.h"
#include "RoadNetwork/Structure/RnLane.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Util/PLATEAURnLinq.h"

void FPLATEAURnEx::ReplaceLane(TArray<TRnRef_T<URnLane>>& Self, TRnRef_T<URnLane> Before, TRnRef_T<URnLane> After) {
    Replace(Self, Before, After);
}


TRnRef_T<URnLineString> FPLATEAURnEx::CreateInnerLerpLineString(
    const TArray<FVector>& LeftVertices,
    const TArray<FVector>& RightVertices,
    TRnRef_T<URnPoint> Start,
    TRnRef_T<URnPoint> End,
    TRnRef_T<URnWay> StartBorder,
    TRnRef_T<URnWay> EndBorder,
    float T,
    float PointSkipDistance) {
    // 左右がどちらも直線もしくは点以下の場合 -> start/endを直接つなぐ
    if (LeftVertices.Num() <= 2 && RightVertices.Num() <= 2) {
        auto Points = TArray<TRnRef_T<URnPoint>>();
        Points.Add(Start);
        Points.Add(End);
        return RnNew<URnLineString>(Points);
    }

    auto Plane = FPLATEAURnDef::Plane;
    auto Line = RnNew<URnLineString>();

    auto AddPoint = [&](TRnRef_T<URnPoint> P) {
        if (!P) return;
        Line->AddPointOrSkip(P, PointSkipDistance);
        };

    AddPoint(Start);
    auto Segments = FGeoGraphEx::GetInnerLerpSegments(LeftVertices, RightVertices, Plane, T);

    // 1つ目の点はボーダーと重複するのでスキップ
    for (int32 i = 1; i < Segments.Num(); ++i) {
        AddPoint(RnNew<URnPoint>(Segments[i]));
    }

    AddPoint(End);

    // 自己交差があれば削除する
    FGeoGraph2D::RemoveSelfCrossing<TRnRef_T<URnPoint>>(
        Line->GetPoints(),
        [Plane](TRnRef_T<URnPoint> T) { return FAxisPlaneEx::ToVector2D(T->Vertex, Plane); },
        [](TRnRef_T<URnPoint> P1, TRnRef_T<URnPoint> P2, TRnRef_T<URnPoint> P3, TRnRef_T<URnPoint> P4,
            const FVector2D& Inter, float F1, float F2) {
                return RnNew<URnPoint>(FMath::Lerp(P1->Vertex, P2->Vertex, F1));
        });

    return Line;
}

FPLATEAURnEx::FLineCrossPointResult FPLATEAURnEx::GetLineIntersections(
    const FLineSegment3D& LineSegment,
    const TArray<TRnRef_T<URnWay>>& Ways)
{
    FLineCrossPointResult Result;
    Result.LineSegment = LineSegment;

    // 全てのwayのLineStringを取得
    TSet<TRnRef_T<URnLineString>> TargetLines;
    for (const auto& Way : Ways) {
        TargetLines.Add(Way->LineString);
    }

    for (const auto& Way : TargetLines) {
        FLineCrossPointResult::FTargetLineInfo Elem;
        Elem.LineString = Way;

        auto Intersections = Way->GetIntersectionBy2D(LineSegment, FPLATEAURnDef::Plane);
        for (const auto& R : Intersections) {
            Elem.Intersections.Add(MakeTuple(R.Key, R.Value));
        }

        Result.TargetLines.Add(Elem);
    }

    return Result;
}


FPLATEAURnEx::FBorderEdgesResult FPLATEAURnEx::FindBorderEdges(const TArray<FVector2D>& Vertices,
    float ToleranceAngleDegForMidEdge, float SkipAngleDeg) {
    TArray<FVector2D> Verts = Vertices;
    const bool bIsClockwise = FGeoGraph2D::IsClockwise(Verts);

    FBorderEdgesResult Result;
    Result.SrcVertices = Verts;

    auto AfterVerts = Reduction<FVector2D>(
        Verts,
        [](const FVector2D& V) { return V; },
        [](const FVector2D& V) { return V; },
        [&](const TArray<FVector2D>& List, int32 i) {
            if (List.Num() <= 4) return true;
            float Area1 = FGeoGraph2D::CalcPolygonArea(List);
            float Area2 = FGeoGraph2D::CalcPolygonArea(Verts);
            return Area1 / Area2 < 0.6f;
        }
    );

    Result.ReducedVertices = AfterVerts;
    Result.ReducedBorderVertexIndices = FGeoGraph2D::FindMidEdge(AfterVerts, ToleranceAngleDegForMidEdge, SkipAngleDeg);

    const int32 X = (Result.ReducedBorderVertexIndices.Num() - 1) / 2;
    const int32 Ind0 = Result.ReducedBorderVertexIndices[X];
    const int32 Ind1 = Result.ReducedBorderVertexIndices[X + 1];

    const FVector2D St = AfterVerts[Ind0];
    const FVector2D En = AfterVerts[Ind1];

    FVector2D N = GetEdgeNormal(St, En);
    if (!bIsClockwise) {
        N *= -1.f;
    }

    const FVector2D Mid = (St + En) * 0.5f;
    const FRay2D Ray(Mid, N);

    float MinLen = MAX_FLT;
    int32 MinIndex = -1;

    TArray<FLineSegment2D> Edges =
        FGeoGraphEx::GetEdgeSegments(Verts, false);

    for (int32 i = 0; i < Edges.Num(); ++i) {
        const FLineSegment2D& Seg = Edges[i];
        FVector2D Inter;
        float T1, T2;

        if (Seg.TryHalfLineIntersection(Ray.Origin, Ray.Direction, Inter, T1, T2)) {
            const float Len = (Mid - Inter).SizeSquared();
            if (Len < MinLen) {
                MinLen = Len;
                MinIndex = i;
            }
        }
    }

    if (MinIndex < 0) {
        Result.bSuccess = false;
        Result.BorderVertexIndices = FGeoGraph2D::FindMidEdge(Vertices, ToleranceAngleDegForMidEdge, SkipAngleDeg);
        return Result;
    }

    Result.bSuccess = true;
    auto CollinearRange = FindCollinearRange(MinIndex, Edges, ToleranceAngleDegForMidEdge);
    CollinearRange.Add(CollinearRange.Last() + 1);
    Result.BorderVertexIndices = CollinearRange;

    return Result;
}

FPLATEAURnEx::FLineCrossPointResult FPLATEAURnEx::GetLaneCrossPoints(TRnRef_T<URnRoad> Road,
    const FLineSegment3D& LineSegment)
{
    TArray<URnWay*> TargetLines;
    // Get all lane ways
    for (URnLane* Lane : Road->GetAllLanesWithMedian()) {
        for (URnWay* Way : Lane->GetBothWays()) {
            TargetLines.Add(Way);
        }
    }

    // Get all sidewalk ways
    for (URnSideWalk* SideWalk : Road->GetSideWalks()) {
        for (URnWay* Way : SideWalk->GetSideWays()) {
            TargetLines.Add(Way);
        }
    }

    return GetLineIntersections(LineSegment, TargetLines);
}

TArray<FVector2D> FPLATEAURnEx::FBorderEdgesResult::GetReducedBorderVertices() const {
    return FPLATEAURnLinq::Select(ReducedBorderVertexIndices, [this](int32 i) {
        return ReducedVertices[i];
        });
}

TArray<FVector2D> FPLATEAURnEx::FBorderEdgesResult::GetBorderVertices() const
{
    TArray<FVector2D> Result;
    for (int32 Index : BorderVertexIndices) {
        Result.Add(SrcVertices[Index]);
    }
    return Result;
}

TArray<int32> FPLATEAURnEx::FindCollinearRange(int32 EdgeBaseIndex, const TArray<FLineSegment2D>& Edges,
                                               float ToleranceAngleDegForMidEdge)
{
    TArray<int32> Result;
    Result.Add(EdgeBaseIndex);

    TArray<bool> Stop;
    Stop.Add(false);
    Stop.Add(false);
    struct FAngleInfo {
        int32 DirectionIndex;
        int32 EdgeIndex;
        float Angle;
    };
    while (!Stop.Contains(true) && Result.Num() < Edges.Num() - 1) {
        struct FDirectionInfo {
            int32 Now;
            int32 Direction;
        };

        TArray<FDirectionInfo> Infos;
        Infos.Add({ Result[0], -1 });
        Infos.Add({ Result.Last(), 1 });

        TArray<FAngleInfo> EdgeAngles;
        for (int32 i = 0; i < 2; ++i) {
            if (Stop[i]) continue;

            const FDirectionInfo& Info = Infos[i];
            const int32 NextIndex = Info.Now + Info.Direction;

            if (NextIndex < 0 || NextIndex >= Edges.Num()) continue;

            const FLineSegment2D& E0 = Edges[EdgeBaseIndex];
            const FLineSegment2D& E1 = Edges[NextIndex];

            const float Angle = FMath::RadiansToDegrees(
                FMath::Acos(FVector2D::DotProduct(E0.GetDirection(), E1.GetDirection()))
            );

            EdgeAngles.Add({ i, NextIndex, Angle });
        }

        if (EdgeAngles.Num() == 0) break;

        EdgeAngles.Sort([](const FAngleInfo& A, const FAngleInfo& B) {
            return A.Angle < B.Angle;
        });

        for (const FAngleInfo& Edge : EdgeAngles) {
            if (Edge.Angle > ToleranceAngleDegForMidEdge) {
                Stop[Edge.DirectionIndex] = true;
                continue;
            }

            if (Edge.DirectionIndex == 0) {
                Result.Insert(Edge.EdgeIndex, 0);
            }
            else {
                Result.Add(Edge.EdgeIndex);
            }
        }
    }

    return Result;
}

void FPLATEAURnEx::AddChildInstanceComponent(AActor* Actor, USceneComponent* Parent, USceneComponent* Child,
                                             FAttachmentTransformRules TransformRule)
{
    if (!Parent || !Child || !Actor)
        return;
    Actor->AddInstanceComponent(Child);
    Child->SetupAttachment(Parent);
    Child->RegisterComponent();
    Child->AttachToComponent(Parent, TransformRule);
#if WITH_EDITOR
    Actor->RerunConstructionScripts();
#endif
}
