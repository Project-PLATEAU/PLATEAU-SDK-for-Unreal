// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/LineSegment2D.h"

#include "RoadNetwork/GeoGraph/LineUtil.h"
#include "RoadNetwork/Util/PLATEAUVector2DEx.h"

FLineSegment2D::FLineSegment2D(const FVector2D& InStart, const FVector2D& InEnd)
    : Start(InStart)
    , End(InEnd) {
    UpdateDirectionAndMagnitude();
}

void FLineSegment2D::SetStart(const FVector2D& InStart) {
    Start = InStart;
    UpdateDirectionAndMagnitude();
}

void FLineSegment2D::SetEnd(const FVector2D& InEnd) {
    End = InEnd;
    UpdateDirectionAndMagnitude();
}

void FLineSegment2D::UpdateDirectionAndMagnitude() {
    Magnitude = (End - Start).Size();
    Direction = (End - Start) / Magnitude;
}

FVector2D FLineSegment2D::GetNearestPoint(const FVector2D& Point) const {
    float Unused;
    return GetNearestPoint(Point, Unused);
}

FVector2D FLineSegment2D::GetNearestPoint(const FVector2D& Point, float& OutDistanceFromStart) const {
    OutDistanceFromStart = FVector2D::DotProduct(Direction, Point - Start);
    OutDistanceFromStart = FMath::Clamp(OutDistanceFromStart, 0.0f, Magnitude);
    return Start + OutDistanceFromStart * Direction;
}

FVector2D FLineSegment2D::GetNearestPoint(const FVector2D& Point, float& OutDistanceFromStart, float& OutT) const {
    OutDistanceFromStart = FVector2D::DotProduct(Direction, Point - Start);
    OutT = OutDistanceFromStart / Magnitude;
    OutDistanceFromStart = FMath::Clamp(OutDistanceFromStart, 0.0f, Magnitude);
    return Start + OutDistanceFromStart * Direction;
}

FVector2D FLineSegment2D::GetPoint(float Distance) const {
    return Start + Direction * Distance;
}

FLineSegment2D FLineSegment2D::Reversed() const {
    return FLineSegment2D(End, Start);
}

bool FLineSegment2D::TrySegmentIntersection(const FVector2D& V0, const FVector2D& V1,
    FVector2D& OutIntersection, float& OutT1, float& OutT2) const {
    return FLineUtil::SegmentIntersection(Start, End, V0, V1, OutIntersection, OutT1, OutT2);
}

bool FLineSegment2D::TrySegmentIntersection(const FLineSegment2D& Other, FVector2D& OutIntersection, float& OutT1,
    float& OutT2) const
{
    return FLineUtil::SegmentIntersection(Start, End, Other.Start, Other.End, OutIntersection, OutT1, OutT2);
}

bool FLineSegment2D::TrySegmentIntersection(const FLineSegment2D& Other, FVector2D& OutIntersection) const
{
    float T1, T2;
    return TrySegmentIntersection(Other, OutIntersection, T1, T2);
}

bool FLineSegment2D::TrySegmentIntersection(const FLineSegment2D& Other) const
{
    FVector2D Intersection;
    return TrySegmentIntersection(Other, Intersection);
}

bool FLineSegment2D::TryHalfLineIntersection(const FVector2D& Origin, const FVector2D& Dir, FVector2D& OutIntersection,
    float& OutHalfLineOffset, float& OutSegmentT) const
{
    return FLineUtil::HalfLineSegmentIntersection(FRay2D(Origin, Dir), Start, End, OutIntersection, OutHalfLineOffset, OutSegmentT);
}

bool FLineSegment2D::TryLineIntersection(const FVector2D& Origin, const FVector2D& Dir,
    FVector2D& OutIntersection, float& OutLineOffset, float& OutSegmentT) const
{
    return FLineUtil::LineSegmentIntersection(FRay2D(Origin, Dir), Start, End, OutIntersection, OutLineOffset, OutSegmentT);
}

float FLineSegment2D::GetDistance(const FLineSegment2D& Other) const {
    FVector2D Intersection;
    float T1, T2;

    if (TrySegmentIntersection(Other, Intersection, T1, T2)) {
        return 0.0f;
    }

    const float D1 = (GetNearestPoint(Other.Start) - Other.Start).Size();
    const float D2 = (GetNearestPoint(Other.End) - Other.End).Size();
    const float D3 = (Other.GetNearestPoint(Start) - Start).Size();
    const float D4 = (Other.GetNearestPoint(End) - End).Size();

    return FMath::Min(D1, FMath::Min(D2, FMath::Min(D3, D4)));
}

int32 FLineSegment2D::Sign(const FVector2D& Point) const {
    const float Cross = FPLATEAUVector2DEx::Cross(Direction, Point - Start);
    return FMath::Sign(Cross);
}

bool FLineSegment2D::IsPointOnLeftSide(const FVector2D& Point) const
{
    return FPLATEAUVector2DEx::IsPointOnLeftSide(Direction, Point - Start);
}
