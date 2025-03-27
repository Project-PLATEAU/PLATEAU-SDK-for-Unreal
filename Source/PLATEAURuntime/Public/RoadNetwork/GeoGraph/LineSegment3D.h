// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "AxisPlane.h"
#include "LineSegment2D.h"

struct PLATEAURUNTIME_API FLineSegment3D {
public:
    FLineSegment3D(){}
    FLineSegment3D(const FVector& InStart, const FVector& InEnd);

    FVector GetStart() const { return Start; }
    void SetStart(const FVector& InStart);

    FVector GetEnd() const { return End; }
    void SetEnd(const FVector& InEnd);

    FVector GetDirection() const { return Direction; }
    float GetMagnitude() const { return Magnitude; }

    FVector GetNearestPoint(const FVector& Point) const;
    FVector GetNearestPoint(const FVector& Point, float& OutDistanceFromStart) const;

    bool TryLineIntersectionBy2D(const FVector& Origin, const FVector& Dir, EAxisPlane Plane,
        float NormalTolerance, FVector& OutIntersection, float& OutLineOffset, float& OutSegmentT) const;

    bool TryHalfLineIntersectionBy2D(const FVector& Origin, const FVector& Dir, EAxisPlane Plane,
        float NormalTolerance, FVector& OutIntersection, float& OutT1, float& OutT2) const;

    bool TrySegmentIntersectionBy2D(const FLineSegment3D& Other, EAxisPlane Plane,
        float NormalTolerance, FVector& OutIntersection, float& OutT1, float& OutT2) const;

    bool TrySegmentIntersectionBy2D(const FLineSegment3D& Other, EAxisPlane Plane,
        float NormalTolerance, FVector& OutIntersection) const;

    FVector GetPoint(float Distance) const;
    FVector Lerp(float T) const;
    FLineSegment3D Reversed() const;

    FLineSegment2D To2D(EAxisPlane Plane) const;
    FLineSegment2D To2D(TFunction<FVector2D(const FVector&)> ToVec2) const;

private:
    FVector Start;
    FVector End;
    FVector Direction;
    float Magnitude;

    void UpdateDirectionAndMagnitude();
};
