// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Math/Vector.h"
#include "Math/IntVector.h"
#include "Kismet/KismetMathLibrary.h"

class PLATEAURUNTIME_API FPLATEAUIntVector2Ex {
public:
    // Absolute value of vector components
    static FIntVector2 Abs(const FIntVector2& Vec);

    // Clamp vector components between min and max values
    static FIntVector2 Clamp(const FIntVector2& Vec, float Min, float Max);
    static FIntVector2 Clamp(const FIntVector2& Vec, const FIntVector2& Min, const FIntVector2& Max);

    // Cross product of 2D vectors
    static float Cross(const FIntVector2& A, const FIntVector2& B);

    // Set X or Y component
    static FIntVector2 PutX(const FIntVector2& Vec, float X);
    static FIntVector2 PutY(const FIntVector2& Vec, float Y);

    // Convert to FIntVector (3D)
    static FIntVector ToXAY(const FIntVector2& Vec, float Y = 0.0f);
    static FIntVector ToXYA(const FIntVector2& Vec, float Z = 0.0f);

    // Min/Max/Sum/Average of components
    static float Min(const FIntVector2& Vec);
    static float Max(const FIntVector2& Vec);
    static float Sum(const FIntVector2& Vec);
    static float Average(const FIntVector2& Vec);

    // Scale operations
    static FIntVector2 Scaled(const FIntVector2& Vec, const FIntVector2& Mul);
    static FIntVector2 RevScaled(const FIntVector2& Vec, const FIntVector2& Div);
    static FIntVector2 RevScale(const FIntVector2& A, const FIntVector2& B);

    // Angle to vector conversions
    static FIntVector2 DegreeToVector(float Degree);
    static FIntVector2 RadianToVector(float Radian);
    static FIntVector2 PolarToCart(float Degree, float Radius = 1.0f);
};