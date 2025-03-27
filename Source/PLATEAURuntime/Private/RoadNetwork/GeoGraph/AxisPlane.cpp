// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/AxisPlane.h"

FVector2D FAxisPlaneEx::GetTangent(const FVector& Vector, EAxisPlane Plane) {
    switch (Plane) {
    case EAxisPlane::Xy:
        return FVector2D(Vector.X, Vector.Y);
    case EAxisPlane::Xz:
        return FVector2D(Vector.X, Vector.Z);
    case EAxisPlane::Yz:
        return FVector2D(Vector.Y, Vector.Z);
    default:
        checkf(false, TEXT("Invalid plane type"));
        return FVector2D::ZeroVector;
    }
}

FVector2D FAxisPlaneEx::ToVector2D(const FVector& Vector, EAxisPlane Plane) {
    return GetTangent(Vector, Plane);
}

FVector FAxisPlaneEx::ToVector3D(const FVector2D& Vector, EAxisPlane Plane, float A) {
    switch (Plane) {
    case EAxisPlane::Xy:
        return FVector(Vector.X, Vector.Y, A);
    case EAxisPlane::Xz:
        return FVector(Vector.X, A, Vector.Y);
    case EAxisPlane::Yz:
        return FVector(A, Vector.X, Vector.Y);
    default:
        checkf(false, TEXT("Invalid plane type"));
        return FVector::ZeroVector;
    }
}

float FAxisPlaneEx::GetNormal(const FVector& Vector, EAxisPlane Plane) {
    switch (Plane) {
    case EAxisPlane::Xy:
        return Vector.Z;
    case EAxisPlane::Xz:
        return Vector.Y;
    case EAxisPlane::Yz:
        return Vector.X;
    default:
        checkf(false, TEXT("Invalid plane type"));
        return 0.0f;
    }
}

FVector FAxisPlaneEx::PutNormal(const FVector& Vector, EAxisPlane Plane, float N) {
    FVector Result = Vector;
    switch (Plane) {
    case EAxisPlane::Xy:
        Result.Z = N;
        break;
    case EAxisPlane::Xz:
        Result.Y = N;
        break;
    case EAxisPlane::Yz:
        Result.X = N;
        break;
    default:
        checkf(false, TEXT("Invalid plane type"));
    }
    return Result;
}

FVector FAxisPlaneEx::Put(const FVector& Vector, EAxisPlane Plane, const FVector2D& A) {
    FVector Result = Vector;
    switch (Plane) {
    case EAxisPlane::Xy:
        Result.X = A.X;
        Result.Y = A.Y;
        break;
    case EAxisPlane::Xz:
        Result.X = A.X;
        Result.Z = A.Y;
        break;
    case EAxisPlane::Yz:
        Result.Y = A.X;
        Result.Z = A.Y;
        break;
    default:
        checkf(false, TEXT("Invalid plane type"));
    }
    return Result;
}

FVector FAxisPlaneEx::Make(EAxisPlane Plane, const FVector2D& V, float Normal) {
    return PutNormal(Put(FVector::ZeroVector, Plane, V), Plane, Normal);
}

FVector FAxisPlaneEx::NormalVector(EAxisPlane Plane) {
    return Make(Plane, FVector2D::ZeroVector, 1.0f);
}
