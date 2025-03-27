// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Util/PLATEAUIntVector2Ex.h"

FIntVector2 FPLATEAUIntVector2Ex::Abs(const FIntVector2& Vec) {
    return FIntVector2(FMath::Abs(Vec.X), FMath::Abs(Vec.Y));
}

FIntVector2 FPLATEAUIntVector2Ex::Clamp(const FIntVector2& Vec, float Min, float Max) {
    return FIntVector2(
        FMath::Clamp(Vec.X, Min, Max),
        FMath::Clamp(Vec.Y, Min, Max)
    );
}

FIntVector2 FPLATEAUIntVector2Ex::Clamp(const FIntVector2& Vec, const FIntVector2& Min, const FIntVector2& Max) {
    return FIntVector2(
        FMath::Clamp(Vec.X, Min.X, Max.X),
        FMath::Clamp(Vec.Y, Min.Y, Max.Y)
    );
}

float FPLATEAUIntVector2Ex::Cross(const FIntVector2& A, const FIntVector2& B) {
    return A.X * B.Y - A.Y * B.X;
}

FIntVector2 FPLATEAUIntVector2Ex::PutX(const FIntVector2& Vec, float X) {
    return FIntVector2(X, Vec.Y);
}

FIntVector2 FPLATEAUIntVector2Ex::PutY(const FIntVector2& Vec, float Y) {
    return FIntVector2(Vec.X, Y);
}

FIntVector FPLATEAUIntVector2Ex::ToXAY(const FIntVector2& Vec, float Y) {
    return FIntVector(Vec.X, Y, Vec.Y);
}

FIntVector FPLATEAUIntVector2Ex::ToXYA(const FIntVector2& Vec, float Z) {
    return FIntVector(Vec.X, Vec.Y, Z);
}

float FPLATEAUIntVector2Ex::Min(const FIntVector2& Vec) {
    return FMath::Min(Vec.X, Vec.Y);
}

float FPLATEAUIntVector2Ex::Max(const FIntVector2& Vec) {
    return FMath::Max(Vec.X, Vec.Y);
}

float FPLATEAUIntVector2Ex::Sum(const FIntVector2& Vec) {
    return Vec.X + Vec.Y;
}

float FPLATEAUIntVector2Ex::Average(const FIntVector2& Vec) {
    return (Vec.X + Vec.Y) * 0.5f;
}

FIntVector2 FPLATEAUIntVector2Ex::Scaled(const FIntVector2& Vec, const FIntVector2& Mul) {
    return FIntVector2(Vec.X * Mul.X, Vec.Y * Mul.Y);
}

FIntVector2 FPLATEAUIntVector2Ex::RevScaled(const FIntVector2& Vec, const FIntVector2& Div) {
    return FIntVector2(Vec.X / Div.X, Vec.Y / Div.Y);
}

FIntVector2 FPLATEAUIntVector2Ex::RevScale(const FIntVector2& A, const FIntVector2& B) {
    return FIntVector2(A.X / B.X, A.Y / B.Y);
}

FIntVector2 FPLATEAUIntVector2Ex::DegreeToVector(float Degree) {
    return RadianToVector(FMath::DegreesToRadians(Degree));
}

FIntVector2 FPLATEAUIntVector2Ex::RadianToVector(float Radian) {
    return FIntVector2(FMath::Cos(Radian), FMath::Sin(Radian));
}

FIntVector2 FPLATEAUIntVector2Ex::PolarToCart(float Degree, float Radius) {
    float Rad = FMath::DegreesToRadians(Degree);
    return FIntVector2(FMath::Cos(Rad), FMath::Sin(Rad)) * Radius;
}