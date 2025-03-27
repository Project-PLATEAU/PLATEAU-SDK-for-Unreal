// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
// Plugins/PLATEAU-SDK-for-Unreal/Source/PLATEAURuntime/Private/RoadNetwork/Util/Vector2Ex.h

#pragma once

#include "CoreMinimal.h"

struct FRay2D;

class PLATEAURUNTIME_API FPLATEAURay2DEx
{
public:

    // pointがselfの左側にあるかどうか
    static bool IsPointOnLeftSide(const FRay2D& Self, FVector2D Point);
};
