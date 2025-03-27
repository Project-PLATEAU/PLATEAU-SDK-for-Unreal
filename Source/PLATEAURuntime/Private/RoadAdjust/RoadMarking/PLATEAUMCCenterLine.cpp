// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/RoadMarking/PLATEAUMCCenterLine.h"
#include "RoadAdjust/RoadNetworkToMesh/PLATEAURrTarget.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "RoadNetwork/Structure/RnLineString.h"
#include "RoadNetwork/Structure/RnPoint.h"
#include "RoadAdjust/RoadMarking/PLATEAUMarkedWay.h"

// FPLATEAUIntersectionDistCalc Implementation
FPLATEAUIntersectionDistCalc::FPLATEAUIntersectionDistCalc(URnRoad* Road)
    : PrevLength(0.0f)
    , NextLength(0.0f)
    , LengthBetweenIntersections(0.0f)
{
    float PrevLen = 0.0f;
    auto Prev = Road->GetPrev();
    while (Prev != nullptr)
    {
        PrevLen += RoadLength(Prev);
        auto PrevRoad = Prev->CastToRoad();
        if(PrevRoad == nullptr) break;
        Prev = PrevRoad->GetPrev();
    }

    float NextLen = 0.0f;
    auto Next = Road->GetNext();
    while (Next != nullptr)
    {
        NextLen += RoadLength(Next);
        auto NextRoad = Next->CastToRoad();
        if(NextRoad == nullptr) break;
        Next = NextRoad->GetNext();
    }

    PrevLength = Prev != nullptr && Prev->IsA<URnIntersection>() ? PrevLen : TNumericLimits<float>::Max();
    NextLength = Next != nullptr && Next->IsA<URnIntersection>() ? NextLen : TNumericLimits<float>::Max();
    LengthBetweenIntersections = FMath::Min(PrevLen + NextLen + RoadLength(Road), TNumericLimits<float>::Max());
}

float FPLATEAUIntersectionDistCalc::NearestDistFromIntersection(const URnWay* Way, int32 WayIndexOrig) const
{
    if (Way->Count() <= 1)
        return TNumericLimits<float>::Max();

    const auto& Points = Way->GetVertices().ToArray();
    const bool bIsReversed = Way->IsReversed;
    const int32 WayIndex = bIsReversed ? Way->Count() - 1 - WayIndexOrig : WayIndexOrig;

    float PrevLen = 0.0f;
    for (int32 i = 1; i <= WayIndex && i < Points.Num(); i++)
    {
        PrevLen += (Points[i] - Points[i - 1]).Size();
    }

    float NextLen = 0.0f;
    for (int32 i = WayIndex; i < Points.Num() - 1; i++)
    {
        NextLen += (Points[i + 1] - Points[i]).Size();
    }

    const float PrevSum = PrevLen + PrevLength;
    const float NextSum = NextLen + NextLength;
    return FMath::Min(PrevSum, NextSum);
}


float FPLATEAUIntersectionDistCalc::RoadLength(URnRoadBase* RoadBase) const
{
    if(RoadBase == nullptr) return 0.0f;
    auto Road = RoadBase->CastToRoad();
    if(Road == nullptr) return 0.0f;
    if (Road->GetMainLanes().Num() > 0 && Road->GetMainLanes()[0]->GetRightWay() != nullptr)
    {
        return Road->GetMainLanes()[0]->GetRightWay()->CalcLength();
    }
    return 0.0f;
}

// UPLATEAUMCCenterLine Implementation
bool UPLATEAUMCCenterLine::IsCenterLineYellow(float DistFromIntersection, float LengthBetweenIntersections) const
{
    const bool bIsNearIntersection = DistFromIntersection < YellowIntersectionThreshold;
    const bool bIsLong = LengthBetweenIntersections > YellowRoadLengthThreshold;
    return bIsNearIntersection && bIsLong;
}

EPLATEAUMarkedWayType UPLATEAUMCCenterLine::GetCenterLineTypeOfWidth(const URnRoad* Road) const
{
    const auto& CarLanes = Road->GetMainLanes();
    
    // 片側の道路幅からタイプを判定します
    float ReverseWidth = 0.0f;
    float ForwardWidth = 0.0f;
    
    for (const auto& Lane : CarLanes)
    {
        if (Lane->GetIsReversed())
            ReverseWidth += Lane->CalcWidth();
        else
            ForwardWidth += Lane->CalcWidth();
    }
    
    const bool bIsOver6M = ReverseWidth > WidthThreshold || ForwardWidth > WidthThreshold;
    return bIsOver6M ? EPLATEAUMarkedWayType::CenterLineOver6MWidth : EPLATEAUMarkedWayType::CenterLineUnder6MWidth;
}

URnWay* UPLATEAUMCCenterLine::WayWithMiddlePoint(const URnWay* Way) const
{
    const float WayLength = Way->CalcLength();
    const float HalfWayLength = WayLength / 2.0f;
    float Len = 0.0f;
    
    URnLineString* DstLine = NewObject<URnLineString>();
    DstLine->AddPointOrSkip(Way->GetPoint(0));
    
    bool bCenterAdded = false;
    for (int32 j = 1; j < Way->Count(); j++)
    {
        const FVector& PCurrent = Way->GetPoint(j)->Vertex;
        const FVector& PPrev = Way->GetPoint(j - 1)->Vertex;
        const float LenDiff = (PCurrent - PPrev).Size();
        const float PrevLen = Len;
        Len += LenDiff;
        
        if (!bCenterAdded && Len >= HalfWayLength)
        {
            const FVector Pos = FMath::Lerp(PPrev, PCurrent, (HalfWayLength - PrevLen) / LenDiff);
            URnPoint* NewPoint = NewObject<URnPoint>();
            NewPoint->Vertex = Pos;
            DstLine->AddPointOrSkip(NewPoint);
            bCenterAdded = true;
        }
        
        DstLine->AddPointOrSkip(Way->GetPoint(j));
    }

    URnWay* NewWay = NewObject<URnWay>();
    NewWay->Init(DstLine, Way->IsReversed, Way->IsReverseNormal);
    return NewWay;
}

FPLATEAUMarkedWayList UPLATEAUMCCenterLine::ComposeFrom(const IPLATEAURrTarget* Target)
{
    FPLATEAUMarkedWayList Result;

    const auto& Roads = Target->GetRoads();
    for (const auto& Road : Roads)
    {
        if (!Road->IsValid())
            continue;

        const auto& CarLanes = Road->GetMainLanes();
        const auto WidthType = GetCenterLineTypeOfWidth(Road);
        const FPLATEAUIntersectionDistCalc InterDistCalc(Road);
        const bool bMedianLaneExist = Road->GetMedianLane() != nullptr;

        for (int32 i = 0; i < CarLanes.Num(); i++)
        {
            const auto& Lane = CarLanes[i];
            // 隣のレーンと進行方向が異なる場合、Rightwayはセンターラインです
            bool bIsCenterLane = i < CarLanes.Num() - 1 && Lane->GetIsReversed() != CarLanes[i + 1]->GetIsReversed();
            
            // 中央分離帯がある場合、センターラインは2つになるので、隣チェックを両方向で行います
            if (bMedianLaneExist)
            {
                bIsCenterLane |= i >= 1 && Lane->GetIsReversed() != CarLanes[i - 1]->GetIsReversed();
            }

            if (!bIsCenterLane)
                continue;

            // センターラインの場合
            URnWay* SrcWay = WayWithMiddlePoint(Lane->GetRightWay());
            URnLineString* LineString = NewObject<URnLineString>();
            EPLATEAUMarkedWayType PrevInterType = EPLATEAUMarkedWayType::None;

            for (int32 j = 0; j < SrcWay->Count(); j++)
            {
                const float CurrentDist = InterDistCalc.NearestDistFromIntersection(SrcWay, j);
                const auto InterType = IsCenterLineYellow(CurrentDist, InterDistCalc.GetLengthBetweenCenterLine())
                    ? EPLATEAUMarkedWayType::CenterLineNearIntersection
                    : WidthType;

                if (PrevInterType != InterType && PrevInterType != EPLATEAUMarkedWayType::None)
                {
                    // 交差点との距離がしきい値となる点を補間して追加
                    const float PrevDist = InterDistCalc.NearestDistFromIntersection(SrcWay, j - 1);
                    float t;
                    if (FMath::Abs(CurrentDist - PrevDist) < 0.1f)
                    {
                        t = 1.0f;
                    }
                    else
                    {
                        t = (YellowIntersectionThreshold - PrevDist) / (CurrentDist - PrevDist);
                    }

                    const FVector LerpedPoint = FMath::Lerp(
                        SrcWay->GetPoint(j - 1)->Vertex,
                        SrcWay->GetPoint(j)->Vertex,
                        t);

                    URnPoint* NewPoint = NewObject<URnPoint>();
                    NewPoint->Vertex = LerpedPoint;
                    LineString->AddPointOrSkip(NewPoint);

                    // 線を追加
                    TArray<FVector> Points;
                    for (const auto& Point : LineString->GetPoints())
                    {
                        Points.Add(Point->Vertex);
                    }
                    Result.Add(FPLATEAUMarkedWay(
                        FPLATEAUMWLine(Points),
                        PrevInterType,
                        Lane->GetIsReversed()
                    ));

                    // リセット
                    LineString = NewObject<URnLineString>();
                    // 次の始点
                    NewPoint = NewObject<URnPoint>();
                    NewPoint->Vertex = LerpedPoint;
                    LineString->AddPointOrSkip(NewPoint);
                }

                PrevInterType = InterType;
                LineString->AddPointOrSkip(SrcWay->GetPoint(j));
            }

            if (LineString->GetPoints().Num() > 0)
            {
                TArray<FVector> Points;
                for (const auto& Point : LineString->GetPoints())
                {
                    Points.Add(Point->Vertex);
                }
                Result.Add(FPLATEAUMarkedWay(
                    FPLATEAUMWLine(Points),
                    PrevInterType,
                    Lane->GetIsReversed()
                ));
            }

            // センターラインの数は、中央分離帯がなければ最大1個、あれば最大2個です
            if ((Result.Num() == 1 && !bMedianLaneExist) || (Result.Num() == 2 && bMedianLaneExist))
            {
                break;
            }
        }
    }

    return Result;
}
