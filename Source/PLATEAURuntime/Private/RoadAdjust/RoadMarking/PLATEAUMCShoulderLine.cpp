// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/RoadMarking/PLATEAUMCShoulderLine.h"
#include "RoadAdjust/RoadNetworkToMesh/PLATEAURrTarget.h"

FPLATEAUMarkedWayList UPLATEAUMCShoulderLine::ComposeFrom(const IPLATEAURrTarget* Target) {
    FPLATEAUMarkedWayList Result;

    // 各道路について処理
    const auto& Roads = Target->GetRoads();
    for (const auto& Road : Roads) {
        if (!Road->IsValid())
            continue;

        const auto& CarLanes = Road->GetMainLanes();
        if (CarLanes.Num() == 0)
            continue;

        // 端の車線について、そのLeftWayは歩道と車道の間です
        const auto& FirstLane = CarLanes[0];
        const auto& LastLane = CarLanes.Last();

        // 最初の車線の左側の路側帯線を追加
        if (FirstLane->IsValidWay() && FirstLane->GetLeftWay() != nullptr) {
            Result.Add(FPLATEAUMarkedWay(
                FPLATEAUMWLine(FirstLane->GetLeftWay()->GetVertices()),
                EPLATEAUMarkedWayType::ShoulderLine,
                FirstLane->GetIsReversed()
            ));
        }

        // 最後の車線の左側の路側帯線を追加
        if (LastLane->IsValidWay() && LastLane->GetLeftWay() != nullptr) {
            Result.Add(FPLATEAUMarkedWay(
                FPLATEAUMWLine(LastLane->GetLeftWay()->GetVertices()),
                EPLATEAUMarkedWayType::ShoulderLine,
                LastLane->GetIsReversed()
            ));
        }
    }

    return Result;
}
