// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/LineSegment3D.h"

#include "RoadNetwork/GeoGraph/GeoGraphEx.h"

FLineSegment3D::FLineSegment3D(const FVector& InStart, const FVector& InEnd)
    : Start(InStart)
    , End(InEnd) {
    UpdateDirectionAndMagnitude();
}

void FLineSegment3D::UpdateDirectionAndMagnitude() {
    Magnitude = (End - Start).Size();
    Direction = Magnitude <= 0.0f ? FVector::ZeroVector : (End - Start) / Magnitude;
}

void FLineSegment3D::SetStart(const FVector& InStart) {
    Start = InStart;
    UpdateDirectionAndMagnitude();
}

void FLineSegment3D::SetEnd(const FVector& InEnd) {
    End = InEnd;
    UpdateDirectionAndMagnitude();
}

FVector FLineSegment3D::GetNearestPoint(const FVector& Point) const {
    float Unused;
    return GetNearestPoint(Point, Unused);
}

FVector FLineSegment3D::GetNearestPoint(const FVector& Point, float& OutDistanceFromStart) const {
    OutDistanceFromStart = FVector::DotProduct(Direction, Point - Start);
    OutDistanceFromStart = FMath::Clamp(OutDistanceFromStart, 0.0f, Magnitude);
    return Start + OutDistanceFromStart * Direction;
}

bool FLineSegment3D::TryLineIntersectionBy2D(const FVector& Origin, const FVector& Dir, EAxisPlane Plane,
    float NormalTolerance, FVector& OutIntersection, float& OutLineOffset, float& OutSegmentT) const
{
    auto Self2D = To2D(Plane);
    FVector2D Inter2D;
    if (!Self2D.TryLineIntersection(
        FAxisPlaneEx::ToVector2D(Origin, Plane)
        , FAxisPlaneEx::ToVector2D(Dir, Plane)
        , Inter2D, OutLineOffset, OutSegmentT)) {
        return false;
    }

    const float Y1 = 
        FMath::Lerp(FAxisPlaneEx::GetNormal(Start, Plane),
        FAxisPlaneEx::GetNormal(End, Plane), OutSegmentT);
    const float Y2 = FAxisPlaneEx::GetNormal(Origin, Plane) + FAxisPlaneEx::GetNormal(Dir, Plane) * OutLineOffset;
    if (FMath::Abs(Y2 - Y1) > NormalTolerance && NormalTolerance >= 0.0f) {
        return false;
    }

    OutIntersection = FAxisPlaneEx::ToVector3D(Inter2D, Plane, (Y1 + Y2) * 0.5f);
    return true;
}

FVector FLineSegment3D::GetPoint(float Distance) const {
    return Start + Direction * Distance;
}

FVector FLineSegment3D::Lerp(float T) const {
    return FMath::Lerp(Start, End, T);
}

FLineSegment3D FLineSegment3D::Reversed() const {
    return FLineSegment3D(End, Start);
}

FLineSegment2D FLineSegment3D::To2D(EAxisPlane Plane) const {
    return FLineSegment2D(
        FAxisPlaneEx::GetTangent(Start, Plane),
        FAxisPlaneEx::GetTangent(End, Plane)
    );
}

FLineSegment2D FLineSegment3D::To2D(TFunction<FVector2D(const FVector&)> ToVec2) const {
    return FLineSegment2D(ToVec2(Start), ToVec2(End));
}

bool FLineSegment3D::TrySegmentIntersectionBy2D(const FLineSegment3D& Other, EAxisPlane Plane,
    float NormalTolerance, FVector& OutIntersection, float& OutT1, float& OutT2) const {
    auto Self2D = To2D(Plane);
    auto Other2D = Other.To2D(Plane);

    FVector2D Inter2D;
    if (!Self2D.TrySegmentIntersection(Other2D, Inter2D, OutT1, OutT2)) {
        return false;
    }

    const float Y1 = FMath::Lerp(FAxisPlaneEx::GetNormal(Start, Plane),
        FAxisPlaneEx::GetNormal(End, Plane), OutT1);
    const float Y2 = FMath::Lerp(FAxisPlaneEx::GetNormal(Other.Start, Plane),
        FAxisPlaneEx::GetNormal(Other.End, Plane), OutT1);

    if (FMath::Abs(Y2 - Y1) > NormalTolerance && NormalTolerance >= 0.0f) {
        return false;
    }

    OutIntersection = FAxisPlaneEx::ToVector3D(Inter2D, Plane, (Y1 + Y2) * 0.5f);
    return true;
}
