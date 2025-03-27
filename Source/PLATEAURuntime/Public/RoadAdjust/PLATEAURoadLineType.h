// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "PLATEAURoadLineType.generated.h"
class FPLATEAURoadLineParam;

UENUM(BlueprintType)
enum class EPLATEAURoadLineType :uint8 {
    None UMETA(DisplayName = "None"),
    WhiteLine UMETA(DisplayName = "WhiteLine"),
    YellowLine UMETA(DisplayName = "YellowLine"),
    DashedWhilteLine UMETA(DisplayName = "DashedWhilteLine"),
    StopLine UMETA(DisplayName = "StopLine"),
    Crossing UMETA(DisplayName = "Crossing"),
};

/**
* @brief EPLATEAURoadLineTypeの拡張機能を提供するクラス
*/
class PLATEAURUNTIME_API PLATEAURoadLineTypeExtension {
public:
    /**
    * @brief 指定されたRoadLineTypeに対応するRoadLineParamを生成します。
    */
    static FPLATEAURoadLineParam ToRoadLineParam(EPLATEAURoadLineType RoadLineType, UStaticMesh* LineMesh, UStaticMesh* TileMesh);
};
