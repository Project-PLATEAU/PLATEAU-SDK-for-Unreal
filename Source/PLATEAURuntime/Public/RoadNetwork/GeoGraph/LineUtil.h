// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "LineUtil.generated.h"

USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FRay2D {
    GENERATED_BODY()

    UPROPERTY()
    FVector2D Origin;

    UPROPERTY()
    FVector2D Direction;

    FRay2D() : Origin(FVector::Zero()), Direction(FVector::Zero()) {}
    FRay2D(const FVector2D& InOrigin, const FVector2D& InDirection)
        : Origin(InOrigin), Direction(InDirection.GetSafeNormal()) {
    }

    // 半直線selfと半直線otherの交点を求める.
    bool CalcIntersection(const FRay2D& other, FVector2D& intersection, float& t1, float& t2) const;
};

struct PLATEAURUNTIME_API FLineUtil {

public:
    static constexpr float Epsilon = 1e-3f;

   
    UFUNCTION(BlueprintCallable)
    static bool LineIntersection(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D,
        FVector2D& Intersection, float& T1, float& T2);

    UFUNCTION(BlueprintCallable)
    static bool LineIntersection(const FRay2D& rayA, const FRay2D& rayB, FVector2D& OutIntersection, float& OutRayAOffset, float& OutRayBOffset);

    UFUNCTION(BlueprintCallable)
    static bool HalfLineSegmentIntersection(const FRay2D& HalfLine, const FVector2D& P1, const FVector2D& P2,
        FVector2D& OutIntersection, float& OutHalfLineOffset, float& OutSegmentT);

    static bool LineSegmentIntersection(const FRay2D& line, FVector2D p1, FVector2D p2, FVector2D& Intersection, float& OutLineLength, float& OutSegmentT);
    static bool SegmentIntersection(const FVector2D& S1St, const FVector2D& S1En,
                                    const FVector2D& S2St, const FVector2D& S2En,
                                    FVector2D& OutIntersection, float& OutT1, float& OutT2);

    static float GetLineSegmentLength(const TArray<FVector>& Vertices);

    static FVector2D GetNearestPoint(const FRay2D& Self, const FVector2D& P, float& OutT);
};
