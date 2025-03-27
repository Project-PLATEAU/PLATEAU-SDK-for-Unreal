// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Util/PLATEAUVector2DEx.h"
#include <cmath>

FVector2D FPLATEAUVector2DEx::Abs(const FVector2D& Vec) {
    return FVector2D(FMath::Abs(Vec.X), FMath::Abs(Vec.Y));
}

FVector2D FPLATEAUVector2DEx::Clamp(const FVector2D& Vec, float Min, float Max) {
    return FVector2D(
        FMath::Clamp(Vec.X, Min, Max),
        FMath::Clamp(Vec.Y, Min, Max)
    );
}

FVector2D FPLATEAUVector2DEx::Clamp(const FVector2D& Vec, const FVector2D& Min, const FVector2D& Max) {
    return FVector2D(
        FMath::Clamp(Vec.X, Min.X, Max.X),
        FMath::Clamp(Vec.Y, Min.Y, Max.Y)
    );
}

float FPLATEAUVector2DEx::Angle360(const FVector2D& From, const FVector2D& To) {
    const float Ang = FMath::RadiansToDegrees(FMath::Acos(FVector2D::DotProduct(From.GetSafeNormal(), To.GetSafeNormal())));
    return Ang * FMath::Sign(Cross(From, To));
}

float FPLATEAUVector2DEx::Angle(const FVector2D& From, const FVector2D& To)
{
    float Num = (float)FMath::Sqrt((double)From.SquaredLength() * (double)To.SquaredLength());
    return (double)Num < 1.0000000116861E-07 ? 0.0f : UKismetMathLibrary::DegAcos(FVector2D::DotProduct(From, To) / Num);
}

float FPLATEAUVector2DEx::SignedAngle(const FVector2D& From, const FVector2D& To)
{
    float Ang = Angle(From, To);
    // UEのXy座標は, UnityのXz座標に対してX軸が反転しているので符号も反転
    return -Ang * FMath::Sign((float)((double)From.X * (double)To.Y - (double)From.Y * (double)To.X));
}

float FPLATEAUVector2DEx::Cross(const FVector2D& A, const FVector2D& B) {
    return FVector2D::CrossProduct(A, B);
}

bool FPLATEAUVector2DEx::IsPointOnLeftSide(const FVector2D& From, const FVector2D& To)
{
    return Cross(From, To) > 0;
}

FVector2D FPLATEAUVector2DEx::RotateTo(const FVector2D& From, const FVector2D& To, float MaxRad) {
    float Ang = Angle(From, To);

    if (Ang < MaxRad)
        return To;

    float Cross = From.X * To.Y - From.Y * To.X;
    float Rad = Cross > 0 ? MaxRad : -MaxRad;
    float S = FMath::Sin(Rad);
    float C = FMath::Cos(Rad);

    return FVector2D(From.X * C - From.Y * S, From.X * S + From.Y * C);
}

FVector2D FPLATEAUVector2DEx::Rotate(const FVector2D& Vec, float Degree) {
    float Rad = FMath::DegreesToRadians(Degree);
    float S = FMath::Sin(Rad);
    float C = FMath::Cos(Rad);

    return FVector2D(
        Vec.X * C - Vec.Y * S,
        Vec.Y * C + Vec.X * S
    );
}

FVector2D FPLATEAUVector2DEx::PutX(const FVector2D& Vec, float X) {
    return FVector2D(X, Vec.Y);
}

FVector2D FPLATEAUVector2DEx::PutY(const FVector2D& Vec, float Y) {
    return FVector2D(Vec.X, Y);
}

FVector FPLATEAUVector2DEx::ToXAY(const FVector2D& Vec, float Y) {
    return FVector(Vec.X, Y, Vec.Y);
}

FVector FPLATEAUVector2DEx::ToXYA(const FVector2D& Vec, float Z) {
    return FVector(Vec.X, Vec.Y, Z);
}

float FPLATEAUVector2DEx::Min(const FVector2D& Vec) {
    return FMath::Min(Vec.X, Vec.Y);
}

float FPLATEAUVector2DEx::Max(const FVector2D& Vec) {
    return FMath::Max(Vec.X, Vec.Y);
}

float FPLATEAUVector2DEx::Sum(const FVector2D& Vec) {
    return Vec.X + Vec.Y;
}

float FPLATEAUVector2DEx::Average(const FVector2D& Vec) {
    return (Vec.X + Vec.Y) * 0.5f;
}

float FPLATEAUVector2DEx::Sin(const FVector2D& Vec) {
    return Vec.Y / Vec.Size();
}

float FPLATEAUVector2DEx::Cos(const FVector2D& Vec) {
    return Vec.X / Vec.Size();
}

float FPLATEAUVector2DEx::Tan(const FVector2D& Vec) {
    return Vec.Y / Vec.X;
}

FVector2D FPLATEAUVector2DEx::Scaled(const FVector2D& Vec, const FVector2D& Mul) {
    return FVector2D(Vec.X * Mul.X, Vec.Y * Mul.Y);
}

FVector2D FPLATEAUVector2DEx::RevScaled(const FVector2D& Vec, const FVector2D& Div) {
    return FVector2D(Vec.X / Div.X, Vec.Y / Div.Y);
}

FVector2D FPLATEAUVector2DEx::RevScale(const FVector2D& A, const FVector2D& B) {
    return FVector2D(A.X / B.X, A.Y / B.Y);
}

FVector2D FPLATEAUVector2DEx::DegreeToVector(float Degree) {
    return RadianToVector(FMath::DegreesToRadians(Degree));
}

FVector2D FPLATEAUVector2DEx::RadianToVector(float Radian) {
    return FVector2D(FMath::Cos(Radian), FMath::Sin(Radian));
}

FVector2D FPLATEAUVector2DEx::PolarToCart(float Degree, float Radius) {
    float Rad = FMath::DegreesToRadians(Degree);
    return FVector2D(FMath::Cos(Rad), FMath::Sin(Rad)) * Radius;
}

bool FPLATEAUVector2DEx::IsNaNOrInfinity(const FVector2D& Vec) {
    return FMath::IsNaN(Vec.X) || FMath::IsNaN(Vec.Y) || std::isinf(Vec.X) || std::isinf(Vec.Y);
}

FIntPoint FPLATEAUVector2DEx::ToIntPoint(const FVector2D& Vec) {
    return FIntPoint(FMath::TruncToInt(Vec.X), FMath::TruncToInt(Vec.Y));
}

FIntPoint FPLATEAUVector2DEx::CeilToInt(const FVector2D& Vec) {
    return FIntPoint(FMath::CeilToInt(Vec.X), FMath::CeilToInt(Vec.Y));
}

FIntPoint FPLATEAUVector2DEx::FloorToInt(const FVector2D& Vec) {
    return FIntPoint(FMath::FloorToInt(Vec.X), FMath::FloorToInt(Vec.Y));
}

FVector2D FPLATEAUVector2DEx::Centroid(const TArray<FVector2D>& Points) {
    if (Points.Num() == 0)
        return FVector2D::ZeroVector;

    FVector2D Sum = FVector2D::ZeroVector;
    for (const FVector2D& Point : Points) {
        Sum += Point;
    }

    return Sum / Points.Num();
}