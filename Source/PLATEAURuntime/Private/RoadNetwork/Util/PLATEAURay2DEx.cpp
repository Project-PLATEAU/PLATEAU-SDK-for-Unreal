// Copyright 2023 Ministry of Land, Infrastructure and Transport

// Plugins/PLATEAU-SDK-for-Unreal/Source/PLATEAURuntime/Private/RoadNetwork/Util/Vector2Ex.cpp
#include "RoadNetwork/Util/PLATEAURay2DEx.h"

#include "RoadNetwork/GeoGraph/LineUtil.h"
#include "RoadNetwork/Util/PLATEAUVector2DEx.h"

struct FRay2D;

bool FPLATEAURay2DEx::IsPointOnLeftSide(const FRay2D& Self, const FVector2D Point) {
    return FPLATEAUVector2DEx::IsPointOnLeftSide(Self.Direction, Point - Self.Origin);
}
