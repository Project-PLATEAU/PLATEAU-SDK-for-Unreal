// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RnIntersection.h"
#include "RnWay.h"
#include "GameFramework/Actor.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "PLATEAURnModelDrawerDebug.generated.h"

class URnModel;

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERnPartsTypeMask : uint8 {
    None = 0,
    Point = 1 << 0,
    LineString = 1 << 1,
    Way = 1 << 2,
    Lane = 1 << 3,
    Road = 1 << 4,
    Intersection = 1 << 5,
    Neighbor = 1 << 6,
    SideWalk = 1 << 7
};
ENUM_CLASS_FLAGS(ERnPartsTypeMask);

UENUM(Meta = (Bitflags))
enum class ERnModelDrawerVisibleType : uint8 {
    Empty = 0,
    NonSelected = 1 << 0,
    SceneSelected = 1 << 1,
    GuiSelected = 1 << 2,
    Valid = 1 << 3,
    Invalid = 1 << 4,
    All = ~0
};
ENUM_CLASS_FLAGS(ERnModelDrawerVisibleType);


USTRUCT()
struct FRnModelDrawOption
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bVisible = true;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum = "/Script/PLATEAURuntime.ERnModelDrawerVisibleType"))
    ERnModelDrawerVisibleType VisibleType = ERnModelDrawerVisibleType::All;

};

USTRUCT()
struct FRnModelDrawWayOption : public FRnModelDrawOption
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowVertexNormal = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowEdgeNormal = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bChangeArrowColor = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor NormalWayArrowColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor ReverseWayArrowColor = FLinearColor::Blue;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float ArrowSize = 50.f;
};

// Add these structures to the header file
USTRUCT()
struct FRnModelDrawLaneOption : public FRnModelDrawOption{
    GENERATED_BODY()
public:
    FRnModelDrawLaneOption();
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float BothConnectedLaneAlpha = 1.0f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float ValidWayAlpha = 0.75f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float InvalidWayAlpha = 0.3f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float ReverseWayAlpha = 1.0f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowLeftWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowRightWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowPrevBorder;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowNextBorder;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowCenterWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowNextRoad = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowPrevRoad = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowCenter2Next = false;

};


USTRUCT()
struct FRnModelDrawRoadMergeOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:
    FRnModelDrawRoadMergeOption()
    {
        bVisible = false;
    }
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowMergedBorderNoDir = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    EPLATEAURnDir ShowMergedBorderDir = EPLATEAURnDir::Left;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    int32 SplitBorderNum = 1;
};
USTRUCT()
struct FRnModelDrawRoadNormalOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowNextConnection = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowPrevConnection = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    int ShowLaneIndex = -1;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor GroupColor = FLinearColor::Green;
};
USTRUCT()
struct FRnModelDrawRoadGroupOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:
    FRnModelDrawRoadGroupOption() {
        bVisible = false;
    }
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowMergedBorderNoDir = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    EPLATEAURnDir ShowMergedBorderDir = EPLATEAURnDir::Left;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    int32 SplitBorderNum = 1;
};

USTRUCT()
struct FRnModelDrawRoadOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawRoadMergeOption MergeDrawer;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawRoadNormalOption NormalDrawer;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawRoadGroupOption GroupDrawer;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bSliceHorizontal = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bMerge2Intersection = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bMergeRoadGroup = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bCheckSliceHorizontal = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    EPLATEAURnLaneBorderType CheckSliceHorizontalDir = EPLATEAURnLaneBorderType::Next;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float CheckSliceHorizontalOffset = 200.f;
};

USTRUCT()
struct FRnModelDrawIntersectionOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:
    FRnModelDrawIntersectionOption();

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowNonBorderEdge;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowBorderEdge;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowMedianBorderEdge;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowEdgeIndex = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowEdgeGroup = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowNoTrackBorder = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowEdgeNormal = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowTrack = false;


    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    TMap<ERnTurnType, FLinearColor> showTrackColor;
};

USTRUCT()
struct FRnModelDrawSideWalkOption : public FRnModelDrawOption {
    GENERATED_BODY()
public:

    FRnModelDrawSideWalkOption();
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowOutsideWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowInsideWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowStartEdgeWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawWayOption ShowEndEdgeWay;


    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bCheck;
};

USTRUCT()
struct PLATEAURUNTIME_API FPLATEAURnModelDrawerDebug {
    GENERATED_BODY()

public:
    FPLATEAURnModelDrawerDebug();
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bVisible = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowInsideNormalMidPoint = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowVertexIndex = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowVertexPos = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    int32 ShowVertexFontSize = 20;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float EdgeOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float YScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawIntersectionOption IntersectionOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawRoadOption RoadOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawLaneOption LaneOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawLaneOption MedianLaneOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRnModelDrawSideWalkOption SideWalkOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum = "/Script/PLATEAURuntime.ERnPartsTypeMask"))
    int32 ShowPartsType = 0;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowOnlyTargets = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    TArray<FString> ShowTargetNames;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowOnlyTargetTrans = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    TArray<FString> ShowTargetTranNames;

    void Draw(URnModel* Model);
};
