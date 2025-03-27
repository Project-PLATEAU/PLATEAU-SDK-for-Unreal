// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadAdjust/PLATEAUReproducedRoad.h"
#include "RoadNetwork/Structure/RnModel.h"


enum class EPLATEAUDirectionalArrowType
{
    None,
    Straight,
    Left,
    Right,
    StraightAndLeft,
    StraightAndRight
};

/**
 * 交差点前の矢印を生成します
 */
class PLATEAURUNTIME_API FPLATEAUDirectionalArrowComposer
{
public:
    explicit FPLATEAUDirectionalArrowComposer(TObjectPtr<URnModel> TargetNetwork, TObjectPtr<APLATEAUReproducedRoad> ReproducedRoad);

    TArray<TObjectPtr<UStaticMeshComponent>> Compose();

private:
    static constexpr float ArrowMeshHeightOffset = 9.0f; // cm
    static constexpr float ArrowPositionOffset = 450.0f; // cm 

    TObjectPtr<UStaticMeshComponent> GenerateArrow(const URnWay* LaneBorder, const URnIntersection* Intersection,
                                                  const FVector& Position, float Rotation);
    FVector ArrowPosition(const URnLane* Lane, bool bIsNext, bool& bIsSucceed);
    float ArrowAngle(const URnLane* Lane, bool bIsNext, bool& bIsSucceed);
    float ArrowAngleOneWay(const URnWay* Way, bool bIsNext);
    EPLATEAUDirectionalArrowType ArrowType(const URnWay* LaneBorder, const URnIntersection* Intersection);
    TObjectPtr<UStaticMesh> ToStaticMesh(EPLATEAUDirectionalArrowType Type) const;

    TObjectPtr<URnModel> TargetNetwork;
    TObjectPtr<APLATEAUReproducedRoad> ReproducedRoad;

    UStaticMesh* MeshLeft;
    UStaticMesh* MeshRight;
    UStaticMesh* MeshStraight;
    UStaticMesh* MeshStraightLeft;
    UStaticMesh* MeshStraightRight;
    int RoadArrowIndex = 0;
};
