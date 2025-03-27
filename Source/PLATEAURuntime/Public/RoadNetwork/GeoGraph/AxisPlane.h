// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Math/Vector.h"
#include "Containers/Array.h"
#include "AxisPlane.generated.h"

UENUM(BlueprintType)
enum class EAxisPlane : uint8 {
    Xy UMETA(DisplayName = "Xy Plane"),
    Xz UMETA(DisplayName = "Xz Plane"),
    Yz UMETA(DisplayName = "YZ Plane")
};

USTRUCT()
struct PLATEAURUNTIME_API FAxisPlaneEx {
    GENERATED_BODY()

public:
    // Get tangent vector from 3D vector on specified plane
    static FVector2D GetTangent(const FVector& Vector, EAxisPlane Plane);

    // Convert 3D vector to 2D vector on specified plane
    static FVector2D ToVector2D(const FVector& Vector, EAxisPlane Plane);

    // Convert 2D vector to 3D vector on specified plane
    static FVector ToVector3D(const FVector2D& Vector, EAxisPlane Plane, float A = 0.0f);

    // Get normal component from 3D vector on specified plane
    static float GetNormal(const FVector& Vector, EAxisPlane Plane);

    // Put normal component to 3D vector on specified plane
    static FVector PutNormal(const FVector& Vector, EAxisPlane Plane, float N);

    // Put 2D vector components to 3D vector on specified plane
    static FVector Put(const FVector& Vector, EAxisPlane Plane, const FVector2D& A);

    // Make 3D vector from 2D vector and normal component on specified plane
    static FVector Make(EAxisPlane Plane, const FVector2D& V, float Normal);

    // Get normal vector of specified plane
    static FVector NormalVector(EAxisPlane Plane);
};