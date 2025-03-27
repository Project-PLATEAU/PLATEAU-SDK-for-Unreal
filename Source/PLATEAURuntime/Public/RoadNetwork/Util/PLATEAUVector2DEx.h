// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"

class PLATEAURUNTIME_API FPLATEAUVector2DEx {
public:
    // Absolute value of vector components
    static FVector2D Abs(const FVector2D& Vec);

    // Clamp vector components between min and max values
    static FVector2D Clamp(const FVector2D& Vec, float Min, float Max);
    static FVector2D Clamp(const FVector2D& Vec, const FVector2D& Min, const FVector2D& Max);

    // Get signed angle between vectors (-180 to 180)
    static float Angle360(const FVector2D& From, const FVector2D& To);

    // From -> Toの間の角度を取得
    static float Angle(const FVector2D& From, const FVector2D& To);

    // From -> Toの間の角度を取得(符号付)
    static float SignedAngle(const FVector2D& From, const FVector2D& To);

    // Cross product of 2D vectors
    static float Cross(const FVector2D& A, const FVector2D& B);

    // ToがFromの左側にあるかどうか
    // 原点(O)に対してO -> Fromの方向ベクトルと, O -> Toの方向ベクトルの外積でチェック
    static bool IsPointOnLeftSide(const FVector2D& From, const FVector2D& To);

    // Rotate vector towards target with max angle
    static FVector2D RotateTo(const FVector2D& From, const FVector2D& To, float MaxRad);

    // Rotate vector by degree
    static FVector2D Rotate(const FVector2D& Vec, float Degree);

    // Set X or Y component
    static FVector2D PutX(const FVector2D& Vec, float X);
    static FVector2D PutY(const FVector2D& Vec, float Y);

    // Convert to FVector (3D)
    static FVector ToXAY(const FVector2D& Vec, float Y = 0.0f);
    static FVector ToXYA(const FVector2D& Vec, float Z = 0.0f);

    // Min/Max/Sum/Average of components
    static float Min(const FVector2D& Vec);
    static float Max(const FVector2D& Vec);
    static float Sum(const FVector2D& Vec);
    static float Average(const FVector2D& Vec);

    // Trigonometric functions
    static float Sin(const FVector2D& Vec);
    static float Cos(const FVector2D& Vec);
    static float Tan(const FVector2D& Vec);

    // Scale operations
    static FVector2D Scaled(const FVector2D& Vec, const FVector2D& Mul);
    static FVector2D RevScaled(const FVector2D& Vec, const FVector2D& Div);
    static FVector2D RevScale(const FVector2D& A, const FVector2D& B);

    // Angle to vector conversions
    static FVector2D DegreeToVector(float Degree);
    static FVector2D RadianToVector(float Radian);
    static FVector2D PolarToCart(float Degree, float Radius = 1.0f);

    // Check for NaN or Infinity
    static bool IsNaNOrInfinity(const FVector2D& Vec);

    // Integer conversions
    static FIntPoint ToIntPoint(const FVector2D& Vec);
    static FIntPoint CeilToInt(const FVector2D& Vec);
    static FIntPoint FloorToInt(const FVector2D& Vec);

    // Geometric center
    static FVector2D Centroid(const TArray<FVector2D>& Points);
};