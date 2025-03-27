// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Math/IntVector.h"
#include "Kismet/KismetMathLibrary.h"
// Plugins/PLATEAU-SDK-for-Unreal/Source/PLATEAURuntime/Private/RoadNetwork/Util/Vector2Ex.h

#pragma once

#include "CoreMinimal.h"

class PLATEAURUNTIME_API FPLATEAUIntVectorEx
{
public:
    static FIntVector Clamp(const FIntVector& Vec, float Min, float Max);
    static FIntVector Clamp(const FIntVector& Vec, const FIntVector& Min, const FIntVector& Max);
    static FIntVector PutX(const FIntVector& Vec, float X);
    static FIntVector PutXY(const FIntVector& Vec, float X, float Y);
    static FIntVector PutXZ(const FIntVector& Vec, float X, float Z);
    static FIntVector PutY(const FIntVector& Vec, float Y);
    static FIntVector PutYZ(const FIntVector& Vec, float Y, float Z);
    static FIntVector PutZ(const FIntVector& Vec, float Z);
    static FIntVector PutXY(const FIntVector& Vec, const FIntVector2& XY);
    static FIntVector2 XY(const FIntVector& Vec);
    static FIntVector2 XZ(const FIntVector& Vec);
    static FIntVector2 YZ(const FIntVector& Vec);
    static float Min(const FIntVector& Vec);
    static float Max(const FIntVector& Vec);
    static float Sum(const FIntVector& Vec);
    static float Average(const FIntVector& Vec);
    static FIntVector Abs(const FIntVector& Vec);
    static FIntVector Scaled(const FIntVector& Vec, const FIntVector& Mul);
    static FIntVector RevScaled(const FIntVector& Vec, const FIntVector& Div);
    static FIntVector RevScale(const FIntVector& A, const FIntVector& B);
    static bool Between(const FIntVector& Vec, const FIntVector& Min, const FIntVector& Max);
    static FIntVector4 XYZA(const FIntVector& Vec, float A);
};
