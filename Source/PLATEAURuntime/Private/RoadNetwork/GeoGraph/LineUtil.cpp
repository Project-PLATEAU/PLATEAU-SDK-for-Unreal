// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/LineUtil.h"

#include "RoadNetwork/Util/PLATEAUVector2DEx.h"

bool FRay2D::CalcIntersection(const FRay2D& other, FVector2D& intersection, float& t1, float& t2) const
{
    auto ret = FLineUtil::LineIntersection(*this, other, intersection,  t1, t2);
    return ret && t1 >= 0.f && t2 >= 0.f;
}

bool FLineUtil::LineIntersection(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D,
                                 FVector2D& OutIntersection, float& OutAbT, float& OutCdT) {
    OutAbT = OutCdT = 0.0f;
    OutIntersection = FVector2D::ZeroVector;

    const auto Deno = FPLATEAUVector2DEx::Cross(B - A, D - C);
    if (FMath::Abs(Deno) < Epsilon) {
        return false;
    }

    OutAbT = FPLATEAUVector2DEx::Cross(C - A, D - C) / Deno;
    OutCdT = FPLATEAUVector2DEx::Cross(B - A, A - C) / Deno;
    OutIntersection = FMath::Lerp(A, B, OutAbT);
    return true;
}

bool FLineUtil::SegmentIntersection(const FVector2D& S1St, const FVector2D& S1En,
    const FVector2D& S2St, const FVector2D& S2En,
    FVector2D& OutIntersection, float& OutT1, float& OutT2) {
    const bool Result = LineIntersection(S1St, S1En, S2St, S2En, OutIntersection, OutT1, OutT2);
    return Result && OutT1 >= 0.0f && OutT1 <= 1.0f && OutT2 >= 0.0f && OutT2 <= 1.0f;
}

bool FLineUtil::LineIntersection(const FRay2D& rayA, const FRay2D& rayB, FVector2D& OutIntersection, float& OutRayAOffset,
    float& OutRayBOffset)
{
    return LineIntersection(rayA.Origin, rayA.Origin + rayA.Direction, rayB.Origin, rayB.Origin + rayB.Direction,
        OutIntersection, OutRayAOffset, OutRayBOffset);
}

bool FLineUtil::HalfLineSegmentIntersection(const FRay2D& HalfLine, const FVector2D& P1, const FVector2D& P2,
    FVector2D& OutIntersection, float& OutHalfLineOffset, float& OutSegmentT)
{
    auto ret = LineIntersection(HalfLine.Origin, HalfLine.Origin + HalfLine.Direction, P1, P2, OutIntersection, OutHalfLineOffset,
       OutSegmentT);
    // halfLineは半直線なので後ろになければOK
    // p1,p2は線分なので0~1の範囲内ならOK
    return ret && OutHalfLineOffset >= 0.f && OutSegmentT >= 0.f && OutSegmentT <= 1.f;
}

bool FLineUtil::LineSegmentIntersection(const FRay2D& line, FVector2D p1, FVector2D p2, FVector2D& Intersection,
    float& OutLineLength, float& OutSegmentT)
{
    auto ret = LineIntersection(line.Origin, line.Origin + line.Direction, p1, p2, Intersection, OutLineLength,
        OutSegmentT);
    // p1,p2は線分なので0~1の範囲内ならOK
    return ret && OutSegmentT >= 0.f && OutSegmentT <= 1.f;
}

float FLineUtil::GetLineSegmentLength(const TArray<FVector>& Vertices) {
    float Length = 0.0f;
    for (int32 i = 0; i < Vertices.Num() - 1; ++i) {
        Length += (Vertices[i + 1] - Vertices[i]).Size();
    }
    return Length;
}

FVector2D FLineUtil::GetNearestPoint(const FRay2D& Self, const FVector2D& P, float& OutT) {
    const FVector2D D = Self.Direction;
    OutT = FVector2D::DotProduct(Self.Direction, P - Self.Origin);
    return Self.Origin + OutT * D;
}

