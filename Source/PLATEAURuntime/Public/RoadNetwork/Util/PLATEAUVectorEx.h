// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
#include "CoreMinimal.h"

class PLATEAURUNTIME_API FPLATEAUVectorEx
{
public:
    static FVector Clamp(const FVector& Vec, float Min, float Max);
    static FVector Clamp(const FVector& Vec, const FVector& Min, const FVector& Max);
    static FVector PutX(const FVector& Vec, float X);
    static FVector PutXY(const FVector& Vec, float X, float Y);
    static FVector PutXZ(const FVector& Vec, float X, float Z);
    static FVector PutY(const FVector& Vec, float Y);
    static FVector PutYZ(const FVector& Vec, float Y, float Z);
    static FVector PutZ(const FVector& Vec, float Z);
    static FVector PutXY(const FVector& Vec, const FVector2D& XY);
    static FVector2D XY(const FVector& Vec);
    static FVector2D XZ(const FVector& Vec);
    static FVector2D YZ(const FVector& Vec);
    static float Min(const FVector& Vec);
    static float Max(const FVector& Vec);
    static float Sum(const FVector& Vec);
    static float Average(const FVector& Vec);
    static FVector Abs(const FVector& Vec);
    static FVector Scaled(const FVector& Vec, const FVector& Mul);
    static FVector RevScaled(const FVector& Vec, const FVector& Div);
    static FVector RevScale(const FVector& A, const FVector& B);
    static FIntVector ToIntVector(const FVector& Vec);
    static FIntVector CeilToInt(const FVector& Vec);
    static FIntVector FloorToInt(const FVector& Vec);
    static bool Between(const FVector& Vec, const FVector& Min, const FVector& Max);
    static FVector4 XYZA(const FVector& Vec, float A);
    static FVector Centroid(const TArray<FVector>& Points);
    static FVector AxisSymmetric(const FVector& Vec, const FVector& Axis);
};
