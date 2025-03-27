// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/RoadMarking/PLATEAUDirectionalArrowComposer.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "Engine/StaticMesh.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "RoadNetwork/Structure/RnRoad.h"
#include "RoadNetwork/Structure/RnLane.h"


FPLATEAUDirectionalArrowComposer::FPLATEAUDirectionalArrowComposer(TObjectPtr<URnModel> TargetNetwork,
                                                                   TObjectPtr<APLATEAUReproducedRoad> ReproducedRoad) :
    TargetNetwork(TargetNetwork), ReproducedRoad(ReproducedRoad)
{
    // 矢印メッシュを読み込みます
    const TCHAR* MeshPathLeft = TEXT("StaticMesh'/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/RoadMarkArrowLeft.RoadMarkArrowLeft'");
    const TCHAR* MeshPathRight = TEXT("StaticMesh'/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/RoadMarkArrowRight.RoadMarkArrowRight'");
    const TCHAR* MeshPathStraight = TEXT("StaticMesh'/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/RoadMarkArrowStraight.RoadMarkArrowStraight'");
    const TCHAR* MeshPathStraightLeft = TEXT("StaticMesh'/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/RoadMarkArrowStraightLeft.RoadMarkArrowStraightLeft'");
    const TCHAR* MeshPathStraightRight = TEXT("StaticMesh'/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/RoadMarkArrowStraightRight.RoadMarkArrowStraightRight'");

    MeshLeft = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, MeshPathLeft));
    MeshRight = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, MeshPathRight));
    MeshStraight = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, MeshPathStraight));
    MeshStraightLeft = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, MeshPathStraightLeft));
    MeshStraightRight = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, MeshPathStraightRight));
}

TArray<TObjectPtr<UStaticMeshComponent>> FPLATEAUDirectionalArrowComposer::Compose()
{
    TArray<TObjectPtr<UStaticMeshComponent>> Result;
    RoadArrowIndex = 0;

    const auto Roads = TargetNetwork->GetRoads();
    for (const auto& Road : Roads)
    {
        auto* NextIntersection = Cast<URnIntersection>(Road->Next);
        auto* PrevIntersection = Cast<URnIntersection>(Road->Prev);

        for (const auto& Lane : Road->MainLanes)
        {
            if (!Lane || !Lane->IsValidWay())
            {
                UE_LOG(LogTemp, Log, TEXT("Skipping invalid lane."));
                continue;
            }

            if (Lane->GetNextBorder())
            {
                auto* Inter = Lane->GetIsReversed() ? PrevIntersection : NextIntersection;
                bool bIsSucceedPosition = false;
                bool bIsSucceedAngle = false;
                const auto Position = ArrowPosition(Lane, true, bIsSucceedPosition);
                const auto Angle = ArrowAngle(Lane, true, bIsSucceedAngle);

                if (bIsSucceedPosition && bIsSucceedAngle)
                {
                    auto Arrow = GenerateArrow(Lane->GetNextBorder(), Inter, Position, Angle);
                    if (Arrow != nullptr)
                    {
                        Result.Add(Arrow);
                    }
                }
            }

            if (Lane->GetPrevBorder())
            {
                auto* Inter = Lane->GetIsReversed() ? NextIntersection : PrevIntersection;
                bool bIsSucceedPosition = false;
                bool bIsSucceedAngle = false;
                const auto Position = ArrowPosition(Lane, false, bIsSucceedPosition);
                const auto Angle = ArrowAngle(Lane, false, bIsSucceedAngle);

                if (bIsSucceedPosition && bIsSucceedAngle)
                {
                    auto Arrow = GenerateArrow(Lane->GetPrevBorder(), Inter, Position, Angle);
                    if (Arrow != nullptr)
                    {
                        Result.Add(Arrow);
                    }
                }
            }
        }
    }

    return Result;
}

TObjectPtr<UStaticMeshComponent> FPLATEAUDirectionalArrowComposer::GenerateArrow(
    const URnWay* LaneBorder, const URnIntersection* Intersection, const FVector& Position, float Rotation)
{
    if (!Intersection || !ReproducedRoad) return nullptr;

    const auto Type = ArrowType(LaneBorder, Intersection);
    auto StaticMesh = ToStaticMesh(Type);
    if (StaticMesh == nullptr)
    {
        return nullptr;
    }

    // 矢印メッシュは、ArrowMeshesという名前のコンポーネントの下に配置します。
    // ArrowMeshesを取得します。
    USceneComponent* ArrowMeshesParent = nullptr;
    TArray<USceneComponent*> Components;
    ReproducedRoad->GetRootComponent()->GetChildrenComponents(false, Components);
    for (auto* Component : Components)
    {
        if (Component && Component->GetName() == TEXT("ArrowMeshes"))
        {
            ArrowMeshesParent = Component;
            break;
        }
    }

    // ArrowMeshesが存在しない場合は新規作成します。
    if (!ArrowMeshesParent)
    {
        ArrowMeshesParent = NewObject<USceneComponent>(ReproducedRoad, TEXT("ArrowMeshes"));
        ArrowMeshesParent->SetupAttachment(ReproducedRoad->GetRootComponent());
        ReproducedRoad->AddInstanceComponent(ArrowMeshesParent);
        ArrowMeshesParent->RegisterComponent();
    }

    // 矢印コンポーネントを作成し、ArrowMeshesの子として設定します。
    const FString ComponentName = FString::Printf(TEXT("RoadArrow_%d"), RoadArrowIndex++);
    auto* ArrowComponent = NewObject<UStaticMeshComponent>(ReproducedRoad, *ComponentName);
    ArrowComponent->SetStaticMesh(StaticMesh);
    ArrowComponent->SetupAttachment(ArrowMeshesParent);
    ReproducedRoad->AddInstanceComponent(ArrowComponent);
    ArrowComponent->RegisterComponent();

    // Set transform
    const FRotator NewRotation(0.f, Rotation, 0.f);
    const FVector NewPosition = Position + FVector(0.f, 0.f, ArrowMeshHeightOffset);
    ArrowComponent->SetWorldLocationAndRotation(NewPosition, NewRotation);

    return ArrowComponent;
}

FVector FPLATEAUDirectionalArrowComposer::ArrowPosition(const URnLane* Lane, bool bIsNext, bool& bIsSucceed)
{
    int WayCount = 0;
    FVector PosSum = FVector::ZeroVector;

    const auto* LeftWay = Lane->GetLeftWay();
    if (LeftWay && LeftWay->Count() > 0)
    {
        PosSum += LeftWay->PositionAtDistance(ArrowPositionOffset, bIsNext);
        WayCount++;
    }

    const auto* RightWay = Lane->GetRightWay();
    if (RightWay && RightWay->Count() > 0)
    {
        PosSum += RightWay->PositionAtDistance(ArrowPositionOffset, bIsNext);
        WayCount++;
    }

    if (WayCount == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Skipping because way count is 0."));
        bIsSucceed = false;
        return FVector::ZeroVector;
    }

    bIsSucceed = true;
    return PosSum / WayCount;
}

float FPLATEAUDirectionalArrowComposer::ArrowAngle(const URnLane* Lane, bool bIsNext, bool& bIsSucceed)
{
    int32 WayCount = 0;
    float AngleSum = 0.f;

    const auto* LeftWay = Lane->GetLeftWay();
    if (LeftWay && LeftWay->Count() > 1)
    {
        AngleSum += ArrowAngleOneWay(LeftWay, bIsNext);
        WayCount++;
    }

    const auto* RightWay = Lane->GetRightWay();
    if (RightWay && RightWay->Count() > 1)
    {
        AngleSum += ArrowAngleOneWay(RightWay, bIsNext);
        WayCount++;
    }

    if (WayCount == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Skipping Angle because way count is 0."));
        bIsSucceed = false;
        return 0.f;
    }

    bIsSucceed = true;
    return AngleSum / WayCount;
}

float FPLATEAUDirectionalArrowComposer::ArrowAngleOneWay(const URnWay* Way, bool bIsNext)
{
    // wayの頂点数が2以上であることが前提
    FVector Diff;
    if (bIsNext)
    {
        Diff = Way->GetPoint(Way->Count() - 1)->Vertex - Way->GetPoint(Way->Count() - 2)->Vertex;
    }
    else
    {
        Diff = Way->GetPoint(0)->Vertex - Way->GetPoint(1)->Vertex;
    }

    // UnityのVector2.SignedAngleに相当する処理
    FVector2D Vec2D(Diff.X, Diff.Y);
    FVector2D BaseVec(0.f, -1.f);
    float Angle = FMath::Atan2(Vec2D.Y, Vec2D.X) - FMath::Atan2(BaseVec.Y, BaseVec.X);
    return FMath::RadiansToDegrees(Angle);
}

EPLATEAUDirectionalArrowType FPLATEAUDirectionalArrowComposer::ArrowType(const URnWay* LaneBorder,
                                                           const URnIntersection* Intersection)
{
    if (!Intersection) return EPLATEAUDirectionalArrowType::None;

    bool bContainStraight = false;
    bool bContainLeft = false;
    bool bContainRight = false;

    for (const auto& Track : Intersection->GetTracks())
    {
        if (!Track->FromBorder->IsSameLineSequence(LaneBorder)) continue;
        const auto TurnType = Track->TurnType;
        bContainLeft |= FRnTurnTypeUtil::IsLeft(TurnType);
        bContainRight |= FRnTurnTypeUtil::IsRight(TurnType);
        bContainStraight |= TurnType == ERnTurnType::Straight;
    }

    if (bContainLeft && bContainRight) return EPLATEAUDirectionalArrowType::None;
    if (bContainStraight && bContainLeft) return EPLATEAUDirectionalArrowType::StraightAndLeft;
    if (bContainStraight && bContainRight) return EPLATEAUDirectionalArrowType::StraightAndRight;
    if (bContainStraight) return EPLATEAUDirectionalArrowType::Straight;
    if (bContainRight) return EPLATEAUDirectionalArrowType::Right;
    if (bContainLeft) return EPLATEAUDirectionalArrowType::Left;

    return EPLATEAUDirectionalArrowType::None;
}

TObjectPtr<UStaticMesh> FPLATEAUDirectionalArrowComposer::ToStaticMesh(EPLATEAUDirectionalArrowType Type) const
{
    
    UStaticMesh* SelectedMesh;

    switch(Type)
    {
    case EPLATEAUDirectionalArrowType::None:
        return nullptr;
    case EPLATEAUDirectionalArrowType::Left:
        SelectedMesh = MeshLeft;
        break;
    case EPLATEAUDirectionalArrowType::Right:
        SelectedMesh = MeshRight;
        break;
    case EPLATEAUDirectionalArrowType::Straight:
        SelectedMesh = MeshStraight;
        break;
    case EPLATEAUDirectionalArrowType::StraightAndLeft:
        SelectedMesh = MeshStraightLeft;
        break;
    case EPLATEAUDirectionalArrowType::StraightAndRight:
        SelectedMesh = MeshStraightRight;
        break;
    default:
        UE_LOG(LogTemp, Error, TEXT("Unknown DirectionalArrowType"));
        return nullptr;
    }

    if (!SelectedMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load arrow mesh"));
        return nullptr;
    }

    return SelectedMesh;
}
