// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/RoadMarking/PLATEAUMCLaneLine.h"
#include "RoadAdjust/RoadNetworkToMesh/PLATEAURrTarget.h"

FPLATEAUMarkedWayList UPLATEAUMCLaneLine::ComposeFrom(const IPLATEAURrTarget* Target) {
    FPLATEAUMarkedWayList Result;
    
    // if (!Target->IsValid())
        // return Result;

    // 各道路について処理
    const auto& Roads = Target->GetRoads();
    for (const auto& Road : Roads) {
        if (!Road->IsValid())
            continue;

        const auto& CarLanes = Road->GetMainLanes();
        // 車道のうち、端でない（路側帯線でない）もののLeftWayは車線境界線です。
        for (int i = 1; i < CarLanes.Num() - 1; i++) { // 端を除くループ
            const auto& Lane = CarLanes[i];
            if (!Lane->IsValidWay())
                continue;

            Result.Add(FPLATEAUMarkedWay(
                FPLATEAUMWLine(Lane->GetLeftWay()->GetVertices()),
                EPLATEAUMarkedWayType::LaneLine,
                Lane->GetIsReversed()
            ));
        }
    }

    return Result;
}
