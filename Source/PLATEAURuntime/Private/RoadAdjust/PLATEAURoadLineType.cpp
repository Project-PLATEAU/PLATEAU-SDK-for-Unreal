// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/PLATEAURoadLineType.h"
#include "RoadAdjust/PLATEAUReproducedRoad.h"
#include "RoadAdjust/RoadMarking/LineGeneratorComponent.h"
#include "Materials/MaterialInterface.h"

FPLATEAURoadLineParam PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType RoadLineType, UStaticMesh* LineMesh, UStaticMesh* TileMesh) {
    FPLATEAURoadLineParam Param;

    const TCHAR* WhiteMaterialPath = TEXT("/PLATEAU-SDK-for-Unreal/RoadNetwork/Materials/WhiteLineMaterial");
    const auto WhiteMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, WhiteMaterialPath));
    const TCHAR* YellowMaterialPath = TEXT("/PLATEAU-SDK-for-Unreal/RoadNetwork/Materials/YellowLineMaterial");
    const auto YellowMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, YellowMaterialPath));
    
    switch (RoadLineType) {
        case EPLATEAURoadLineType::WhiteLine:
            Param.Type = EPLATEAURoadLineType::WhiteLine;
            Param.LineMesh = LineMesh;
            Param.LineMaterial = WhiteMaterial;
            Param.LineGap = 0.0f;
            Param.LineXScale = 0.3f;
            Param.LineLength = 0.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = true;
            break;
        case EPLATEAURoadLineType::YellowLine:
            Param.Type = EPLATEAURoadLineType::YellowLine;
            Param.LineMesh = LineMesh;
            Param.LineMaterial = YellowMaterial;
            Param.LineGap = 0.0f;
            Param.LineXScale = 0.3f;
            Param.LineLength = 0.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = true;
            break;
        case EPLATEAURoadLineType::DashedWhilteLine:
            Param.Type = EPLATEAURoadLineType::DashedWhilteLine;
            Param.LineMesh = LineMesh;
            Param.LineMaterial = WhiteMaterial;
            Param.LineGap = 100.0f;
            Param.LineXScale = 0.3f;
            Param.LineLength = 90.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = false;
            break;
        case EPLATEAURoadLineType::StopLine:
            Param.Type = EPLATEAURoadLineType::StopLine;
            Param.LineMesh = LineMesh;
            Param.LineMaterial = WhiteMaterial;
            Param.LineGap = 0.0f;
            Param.LineXScale = 1.2f;
            Param.LineLength = 0.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = true;
            break;
        case EPLATEAURoadLineType::Crossing:
            Param.Type = EPLATEAURoadLineType::Crossing;
            Param.LineMesh = TileMesh;
            Param.LineMaterial = WhiteMaterial;
            Param.LineGap = 80.0f;
            Param.LineXScale = 1.5f;
            Param.LineLength = 50.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = false;
            break;
        default:
            // Noneの場合はWhiteLineと同じパラメータを返す
            Param.Type = EPLATEAURoadLineType::WhiteLine;
            Param.LineMesh = LineMesh;
            Param.LineMaterial = WhiteMaterial;
            Param.LineGap = 0.0f;
            Param.LineXScale = 0.3f;
            Param.LineLength = 0.0f;
            Param.SplinePointType = ESplinePointType::Linear;
            Param.FillEnd = true;
            break;
    }
    
    return Param;
}
