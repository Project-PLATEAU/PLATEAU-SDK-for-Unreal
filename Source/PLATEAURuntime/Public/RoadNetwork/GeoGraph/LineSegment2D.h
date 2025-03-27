// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "LineUtil.h"
#include "Math/Vector2D.h"

struct PLATEAURUNTIME_API FLineSegment2D {
    FLineSegment2D(const FVector2D& InStart, const FVector2D& InEnd);

    FVector2D GetStart() const { return Start; }
    void SetStart(const FVector2D& InStart);

    FVector2D GetEnd() const { return End; }
    void SetEnd(const FVector2D& InEnd);

    FVector2D GetDirection() const { return Direction; }
    float GetMagnitude() const { return Magnitude; }

    FVector2D GetNearestPoint(const FVector2D& Point) const;
    FVector2D GetNearestPoint(const FVector2D& Point, float& OutDistanceFromStart) const;
    FVector2D GetNearestPoint(const FVector2D& Point, float& OutDistanceFromStart, float& OutT) const;

    FVector2D GetPoint(float Distance) const;
    FLineSegment2D Reversed() const;

    bool TrySegmentIntersection(const FVector2D& V0, const FVector2D& V1, FVector2D& OutIntersection, float& OutT1, float& OutT2) const;
    bool TrySegmentIntersection(const FLineSegment2D& Other, FVector2D& OutIntersection, float& OutT1, float& OutT2) const;

    bool TrySegmentIntersection(const FLineSegment2D& Other, FVector2D& OutIntersection) const;
    bool TrySegmentIntersection(const FLineSegment2D& Other) const;

    bool TryHalfLineIntersection(const FVector2D& Origin, const FVector2D& Dir, FVector2D& OutIntersection, float& OutHalfLineOffset, float& OutSegmentT) const;
    bool TryLineIntersection(
        const FVector2D& Origin,
        const FVector2D& Direction,
        FVector2D& OutIntersection,
        float& OutLineOffset,
        float& OutSegmentT) const;
    float GetDistance(const FLineSegment2D& Other) const;

    // Pointが線分の左側にあれば1, 右側にあれば-1, 線分上にあれば0を返す
    int32 Sign(const FVector2D& Point) const;

    // Pointが線分の左側にあるかどうか
    bool IsPointOnLeftSide(const FVector2D& Point) const;

    FRay2D Ray() const
    {
        return FRay2D(Start, Direction);    
    }
private:
    FVector2D Start;
    FVector2D End;
    FVector2D Direction;
    float Magnitude;

    void UpdateDirectionAndMagnitude();
};
