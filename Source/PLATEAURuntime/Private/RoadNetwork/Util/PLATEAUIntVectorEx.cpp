// Copyright 2023 Ministry of Land, Infrastructure and Transport

// Plugins/PLATEAU-SDK-for-Unreal/Source/PLATEAURuntime/Private/RoadNetwork/Util/Vector2Ex.cpp
#include "RoadNetwork/Util/PLATEAUIntVectorEx.h"

FIntVector FPLATEAUIntVectorEx::Clamp(const FIntVector& Vec, float Min, float Max) {
    return FIntVector(FMath::Clamp(Vec.X, Min, Max), FMath::Clamp(Vec.Y, Min, Max), FMath::Clamp(Vec.Z, Min, Max));
}

FIntVector FPLATEAUIntVectorEx::Clamp(const FIntVector& Vec, const FIntVector& Min, const FIntVector& Max) {
    return FIntVector(FMath::Clamp(Vec.X, Min.X, Max.X), FMath::Clamp(Vec.Y, Min.Y, Max.Y), FMath::Clamp(Vec.Z, Min.Z, Max.Z));
}

FIntVector FPLATEAUIntVectorEx::PutX(const FIntVector& Vec, float X) {
    return FIntVector(X, Vec.Y, Vec.Z);
}

FIntVector FPLATEAUIntVectorEx::PutXY(const FIntVector& Vec, float X, float Y) {
    return FIntVector(X, Y, Vec.Z);
}

FIntVector FPLATEAUIntVectorEx::PutXZ(const FIntVector& Vec, float X, float Z) {
    return FIntVector(X, Vec.Y, Z);
}

FIntVector FPLATEAUIntVectorEx::PutY(const FIntVector& Vec, float Y) {
    return FIntVector(Vec.X, Y, Vec.Z);
}

FIntVector FPLATEAUIntVectorEx::PutYZ(const FIntVector& Vec, float Y, float Z) {
    return FIntVector(Vec.X, Y, Z);
}

FIntVector FPLATEAUIntVectorEx::PutZ(const FIntVector& Vec, float Z) {
    return FIntVector(Vec.X, Vec.Y, Z);
}

FIntVector FPLATEAUIntVectorEx::PutXY(const FIntVector& Vec, const FIntVector2& XY) {
    return FIntVector(XY.X, XY.Y, Vec.Z);
}

FIntVector2 FPLATEAUIntVectorEx::XY(const FIntVector& Vec) {
    return FIntVector2(Vec.X, Vec.Y);
}

FIntVector2 FPLATEAUIntVectorEx::XZ(const FIntVector& Vec) {
    return FIntVector2(Vec.X, Vec.Z);
}

FIntVector2 FPLATEAUIntVectorEx::YZ(const FIntVector& Vec) {
    return FIntVector2(Vec.Y, Vec.Z);
}

float FPLATEAUIntVectorEx::Min(const FIntVector& Vec) {
    return FMath::Min(Vec.Z, FMath::Min(Vec.X, Vec.Y));
}

float FPLATEAUIntVectorEx::Max(const FIntVector& Vec) {
    return FMath::Max(Vec.Z, FMath::Max(Vec.X, Vec.Y));
}

float FPLATEAUIntVectorEx::Sum(const FIntVector& Vec) {
    return Vec.X + Vec.Y + Vec.Z;
}

float FPLATEAUIntVectorEx::Average(const FIntVector& Vec) {
    return (Vec.X + Vec.Y + Vec.Z) / 3.0f;
}

FIntVector FPLATEAUIntVectorEx::Abs(const FIntVector& Vec) {
    return FIntVector(FMath::Abs(Vec.X), FMath::Abs(Vec.Y), FMath::Abs(Vec.Z));
}

FIntVector FPLATEAUIntVectorEx::Scaled(const FIntVector& Vec, const FIntVector& Mul) {
    return FIntVector(Vec.X * Mul.X, Vec.Y * Mul.Y, Vec.Z * Mul.Z);
}

FIntVector FPLATEAUIntVectorEx::RevScaled(const FIntVector& Vec, const FIntVector& Div) {
    return FIntVector(Vec.X / Div.X, Vec.Y / Div.Y, Vec.Z / Div.Z);
}

FIntVector FPLATEAUIntVectorEx::RevScale(const FIntVector& A, const FIntVector& B) {
    return FIntVector(A.X / B.X, A.Y / B.Y, A.Z / B.Z);
}

bool FPLATEAUIntVectorEx::Between(const FIntVector& Vec, const FIntVector& Min, const FIntVector& Max) {
    return Min.X <= Vec.X && Vec.X <= Max.X &&
        Min.Y <= Vec.Y && Vec.Y <= Max.Y &&
        Min.Z <= Vec.Z && Vec.Z <= Max.Z;
}

FIntVector4 FPLATEAUIntVectorEx::XYZA(const FIntVector& Vec, float A) {
    return FIntVector4(Vec.X, Vec.Y, Vec.Z, A);
}