// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include <memory>
#include "../../Component/PLATEAUSceneComponent.h"
#include "RGraph.h"
#include "RoadNetwork/Util/PLATEAURnDebugEx.h"
#include "PLATEAURGraph.generated.h"

struct FActorComponentTickFunction;

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERPartsFlag : uint8 {
    None = 0,
    Vertex = 1 << 0,
    Edge = 1 << 1,
    Face = 1 << 2,
    All = Vertex | Edge | Face
};
ENUM_CLASS_FLAGS(ERPartsFlag);

USTRUCT()
struct FRGraphDrawFaceOption : public FPLATEAURnDrawOption {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowOutline = true;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowConvexVolume = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    ERRoadTypeMask ShowOutlineMask = ERRoadTypeMask::Road;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    ERRoadTypeMask ShowOutlineRemoveMask = ERRoadTypeMask::Empty;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowCityObjectOutline = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowOutlineLoop = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowOnlyTargets = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    TArray<FString> ShowTargetNames;
};

USTRUCT()
struct FRGraphDrawEdgeOption : public FPLATEAURnDrawOption {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bUseAnyFaceVertexColor = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowNeighborCount = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum = "/Script/PLATEAURuntime.ERRoadTypeMask"))
    int32 ShowTypeMask = 0;
};

USTRUCT()
struct FRGraphDrawVertexOption : public FPLATEAURnDrawOption {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    int32 Size = 10;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnDrawOption NeighborOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowPos = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowEdgeCount = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bUseAnyFaceVertexColor = false;
};
USTRUCT()
struct FRGraphDrawSideWalkOption : public FPLATEAURnDrawOption {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnDrawOption OutsideWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnDrawOption InsideWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnDrawOption StartEdgeWay;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnDrawOption EndEdgeWay;

};
UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API UPLATEAURGraph : public UPLATEAUSceneComponent {
    GENERATED_BODY()
public:

    UPLATEAURGraph();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

    // --------------------
    // start:フィールド
    // --------------------
    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bVisibility = false;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    bool bShowNormal;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum = "/Script/PLATEAURuntime.ERRoadTypeMask"))
    int32 ShowFaceType;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum= "/Script/PLATEAURuntime.ERRoadTypeMask"))
    int32 RemoveFaceType;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug", Meta = (Bitmask, BitmaskEnum = "/Script/PLATEAURuntime.ERPartsFlag"))
    ERPartsFlag ShowId;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRGraphDrawFaceOption FaceOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRGraphDrawEdgeOption EdgeOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRGraphDrawVertexOption VertexOption;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor RoadColor;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor SideWalkColor;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor MedianColor;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor HighWayColor;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    float FontScale = 1.f;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FLinearColor UndefinedColor;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FRGraphDrawSideWalkOption SideWalkOption;

    UPROPERTY(VisibleAnywhere, Category="PLATEAU")
    URGraph* RGraph;

private:
    FLinearColor GetColor(ERRoadTypeMask RoadType);
    void Draw(const FRGraphDrawVertexOption& Op, RGraphRef_t<URVertex> Vertex, struct FDrawWork& Work);
    void Draw(const FRGraphDrawEdgeOption& Op, RGraphRef_t<UREdge> Edge, struct FDrawWork& Work);
    void Draw(const FRGraphDrawFaceOption& Op, RGraphRef_t<URFace> Face, struct FDrawWork& Work);
    void DrawSideWalk(URGraph* Graph, FDrawWork& Work);
    void DrawNormal(URGraph* Graph, FDrawWork& Work);
};
