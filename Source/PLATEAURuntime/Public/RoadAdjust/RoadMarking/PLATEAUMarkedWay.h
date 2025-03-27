// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadAdjust/PLATEAUReproducedRoad.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "PLATEAUMarkedWay.generated.h"

UENUM()
enum class EPLATEAUMarkedWayType : uint8
{
    CenterLineOver6MWidth      UMETA(DisplayName = "CenterLineOver6MWidth"),
    CenterLineUnder6MWidth     UMETA(DisplayName = "CenterLineUnder6MWidth"),
    CenterLineNearIntersection UMETA(DisplayName = "CenterLineNearIntersection"),
    LaneLine                   UMETA(DisplayName = "LaneLine"),
    ShoulderLine              UMETA(DisplayName = "ShoulderLine"),
    StopLine                  UMETA(DisplayName = "StopLine"),
    Crosswalk              UMETA(DisplayName = "Crosswalk"),
    None                      UMETA(DisplayName = "None")
};

/**
 * 路面標示の線の座標列を表現するクラスです。
 * MWはMarkedWayの略です。
 */
USTRUCT()
struct PLATEAURUNTIME_API FPLATEAUMWLine
{
    GENERATED_BODY()

public:
    FPLATEAUMWLine() = default;
    explicit FPLATEAUMWLine(const URnWay::VertexEnumerator& InPoints);
    explicit FPLATEAUMWLine(const TArray<FVector>& InPoints) : Points(InPoints) { }

    const TArray<FVector>& GetPoints() const { return Points; }
    void SetPoints(const TArray<FVector>& InPoints) { Points = InPoints; }

    FVector operator[](int32 Index) const { return Points[Index]; }
    int32 Num() const { return Points.Num(); }

    float SumDistance() const;

private:
    UPROPERTY()
    TArray<FVector> Points;
};

/**
 * 路面標示の線1つを表現するクラスです。
 */
USTRUCT()
struct PLATEAURUNTIME_API FPLATEAUMarkedWay
{
    GENERATED_BODY()

public:
    FPLATEAUMarkedWay() = default;
    FPLATEAUMarkedWay(const FPLATEAUMWLine& InLine, EPLATEAUMarkedWayType InType, bool bInIsReversed);

    const FPLATEAUMWLine& GetLine() const { return Line; }
    EPLATEAUMarkedWayType GetMarkedWayType() const { return Type; }
    EPLATEAURoadLineType GetRoadLineType() const;
    bool IsReversed() const { return bIsReversed; }

    void Translate(const FVector& Diff);

private:
    UPROPERTY()
    FPLATEAUMWLine Line;

    UPROPERTY()
    EPLATEAUMarkedWayType Type = EPLATEAUMarkedWayType::None;

    UPROPERTY()
    bool bIsReversed = false;
};

/**
 * MarkedWayのリストを管理するクラスです。
 */
USTRUCT()
struct PLATEAURUNTIME_API FPLATEAUMarkedWayList
{
    GENERATED_BODY()

public:
    FPLATEAUMarkedWayList() = default;
    explicit FPLATEAUMarkedWayList(const TArray<FPLATEAUMarkedWay>& InWays);

    const TArray<FPLATEAUMarkedWay>& GetMarkedWays() const { return Ways; }
    void Add(const FPLATEAUMarkedWay& Way);
    void AddRange(const FPLATEAUMarkedWayList& WayList);
    void Translate(const FVector& Diff);
    int32 Num() const { return Ways.Num(); }

private:
    UPROPERTY()
    TArray<FPLATEAUMarkedWay> Ways;
};
