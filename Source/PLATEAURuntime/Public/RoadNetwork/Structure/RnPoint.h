// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <memory>

#include "RoadNetwork/PLATEAURnDef.h"
#include "RnPoint.generated.h"

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class URnPoint : public UObject {
    GENERATED_BODY()
public:
    URnPoint();
    URnPoint(const FVector& InVertex);
    void Init();
    void Init(const FVector& InVertex);

    TRnRef_T<URnPoint> Clone() const;

    static bool Equals(const URnPoint& X, const URnPoint& Y, float SqrMagnitudeTolerance = 0.0f);
    static bool Equals(TRnRef_T<const URnPoint> X, TRnRef_T< const URnPoint> Y, float SqrMagnitudeTolerance = 0.0f);
    operator FVector() const { return Vertex; }

    bool IsSamePoint(const URnPoint* Other, float SqrMagnitudeTolerance = 0.0f) const;
    bool IsSamePoint(const TRnRef_T<URnPoint>& Other, float SqrMagnitudeTolerance = 0.0f) const;

    FVector GetVertex() const { return Vertex; }
public:
    UPROPERTY(EditAnywhere, Category = "PLATEAU")
    FVector Vertex;

};
