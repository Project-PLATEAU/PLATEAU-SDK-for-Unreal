// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnModel.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Algo/Count.h"
#include "RoadNetwork/Structure/RnRoad.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Structure/RnLane.h"
#include "RoadNetwork/Structure/RnRoadGroup.h"
#include "RoadNetwork/Structure/RnWay.h"

const FString& URnModel::GetFactoryVersion() const
{
    return FactoryVersion;
}

void URnModel::SetFactoryVersion(const FString& InFactoryVersion)
{
    this->FactoryVersion = InFactoryVersion;
}

URnModel::URnModel() {
}

void URnModel::Init()
{
    Roads.Reset();
    Intersections.Reset();
    SideWalks.Reset();
}

void URnModel::AddRoadBase(const TRnRef_T<URnRoadBase>& RoadBase)
{
    if (!RoadBase) 
        return;
    if (auto Road = RoadBase->CastToRoad()) {
        AddRoad(Road);
    }
    else if (auto Intersection = RoadBase->CastToIntersection()) {
        AddIntersection(Intersection);
    }
}

void URnModel::AddRoad(const TRnRef_T<URnRoad>& Road) {
    if (!Road) return;
    Road->SetParentModel(TRnRef_T<URnModel>(this));
    Roads.AddUnique(Road);
}

void URnModel::RemoveRoad(const TRnRef_T<URnRoad>& Road) {
    if (!Road) return;
    Road->SetParentModel(nullptr);
    Roads.Remove(Road);
}

void URnModel::AddIntersection(const TRnRef_T<URnIntersection>& Intersection) {
    if (!Intersection) return;
    Intersection->SetParentModel(TRnRef_T<URnModel>(this));
    Intersections.AddUnique(Intersection);
}

void URnModel::RemoveIntersection(const TRnRef_T<URnIntersection>& Intersection) {
    if (!Intersection) return;
    Intersection->SetParentModel(nullptr);
    Intersections.Remove(Intersection);
}

void URnModel::AddSideWalk(const TRnRef_T<URnSideWalk>& SideWalk) {
    if (!SideWalk) return;
    SideWalks.AddUnique(SideWalk);
}

void URnModel::RemoveSideWalk(const TRnRef_T<URnSideWalk>& SideWalk) {
    if (!SideWalk) return;
    SideWalks.Remove(SideWalk);
}

const TArray<TRnRef_T<URnRoad>>& URnModel::GetRoads() const {
    return Roads;
}

const TArray<TRnRef_T<URnIntersection>>& URnModel::GetIntersections() const {
    return Intersections;
}

const TArray<TRnRef_T<URnSideWalk>>& URnModel::GetSideWalks() const {
    return SideWalks;
}

TRnRef_T<URnRoad> URnModel::GetRoadBy(UPLATEAUCityObjectGroup* TargetTran) const {
    if (!TargetTran) return nullptr;

    for (const auto& Road : Roads) {
        if (Road->GetTargetTrans().Contains(TargetTran)) {
            return Road;
        }
    }
    return nullptr;
}

TRnRef_T<URnIntersection> URnModel::GetIntersectionBy(UPLATEAUCityObjectGroup* TargetTran) const {
    if (!TargetTran) return nullptr;

    for (const auto& Intersection : Intersections) {
        if (Intersection->GetTargetTrans().Contains(TargetTran)) {
            return Intersection;
        }
    }
    return nullptr;
}

TRnRef_T<URnSideWalk> URnModel::GetSideWalkBy(UPLATEAUCityObjectGroup* TargetTran) const {
    if (!TargetTran) return nullptr;

    for (const auto& SideWalk : SideWalks) {
        if (SideWalk->GetParentRoad() && SideWalk->GetParentRoad()->GetTargetTrans().Contains(TargetTran)) {
            return SideWalk;
        }
    }
    return nullptr;
}

TRnRef_T<URnRoadBase> URnModel::GetRoadBaseBy(UPLATEAUCityObjectGroup* TargetTran) const {
    auto Road = GetRoadBy(TargetTran);
    if (Road) return Road;
    return GetIntersectionBy(TargetTran);
}

TArray<TRnRef_T<URnRoadBase>> URnModel::GetNeighborRoadBases(const TRnRef_T<URnRoadBase>& RoadBase) const {
    if (!RoadBase) return TArray<TRnRef_T<URnRoadBase>>();
    return RoadBase->GetNeighborRoads();
}

TArray<TRnRef_T<URnRoad>> URnModel::GetNeighborRoads(const TRnRef_T<URnRoadBase>& RoadBase) const {
    TArray<TRnRef_T<URnRoad>> Result;
    for (const auto& Neighbor : GetNeighborRoadBases(RoadBase)) {
        if (auto R = Neighbor->CastToRoad()) {
            Result.Add(R);
        }
    }
    return Result;
}

TArray<TRnRef_T<URnIntersection>> URnModel::GetNeighborIntersections(const TRnRef_T<URnRoadBase>& RoadBase) const {
    TArray<TRnRef_T<URnIntersection>> Result;
    for (const auto& Neighbor : GetNeighborRoadBases(RoadBase)) {
        if (auto Intersection = Neighbor->CastToIntersection()) {
            Result.Add(Intersection);
        }
    }
    return Result;
}

TArray<TRnRef_T<URnSideWalk>> URnModel::GetNeighborSideWalks(const TRnRef_T<URnRoadBase>& RoadBase) const {
    if (!RoadBase )
        return TArray<TRnRef_T<URnSideWalk>>();
    return RoadBase->GetSideWalks();
}

void URnModel::CalibrateIntersectionBorderForAllRoad(const FRnModelCalibrateIntersectionBorderOption& Option)
{
    TSet<URnRoad*> Prevs;
    TSet<URnRoad*> Nexts;

    TArray<URnRoad*> RnRoads = GetRoads();
    for (URnRoad* Road : RnRoads) {
        URnRoad* Prev = nullptr;
        URnRoad* Center = nullptr;
        URnRoad* Next = nullptr;
        TrySliceRoadHorizontalNearByBorder(Road, Option, Prev, Center, Next);
        // 失敗してもPrev or Nextどっちかは成功しているかもしれないので結果は見ない
        if (Prev) {
            Prevs.Add(Prev);
        }
        if (Next) {
            Nexts.Add(Next);
        }
    }

    if(Option.SkipMergeRoads == false)
    {
        // Merge with neighboring intersections
        for (URnRoad* Prev : Prevs) {
            Prev->TryMerge2NeighborIntersection(EPLATEAURnLaneBorderType::Prev);
        }

        for (URnRoad* Next : Nexts) {
            Next->TryMerge2NeighborIntersection(EPLATEAURnLaneBorderType::Next);
        }        
    }
}

bool URnModel::TrySliceRoadHorizontalNearByBorder(
    URnRoad* Road,
    const FRnModelCalibrateIntersectionBorderOption& Option,
    URnRoad*& OutPrevSideRoad,
    URnRoad*& OutCenterSideRoad,
    URnRoad*& OutNextSideRoad) {
    OutPrevSideRoad = nullptr;
    OutNextSideRoad = nullptr;
    OutCenterSideRoad = Road;

    if (!Road->IsValid()) {
        return false;
    }

    if (!Road->IsAllLaneValid()) {
        return false;
    }

    URnWay* LeftWay = nullptr;
    URnWay* RightWay = nullptr;
    Road->TryGetMergedSideWay(NullOpt, LeftWay, RightWay);

    const float MinLength = FMath::Min(LeftWay->CalcLength(), RightWay->CalcLength());
    const float MaxOffset = Option.MaxOffsetMeter * FPLATEAURnDef::Meter2Unit;
    const float NeedRoadLengthMeter = Option.NeedRoadLengthMeter * FPLATEAURnDef::Meter2Unit;
    if (MinLength < MaxOffset) {
        return false;
    }
    struct FNeighborInfo {
        URnIntersection* Intersection;
        EPLATEAURnLaneBorderType BorderType;
    };
  
    struct SideInfo {
        URnRoad* FarSide = nullptr;
        URnRoad* NearSide = nullptr;
    };

    TArray<FNeighborInfo> Neighbors;
    for (EPLATEAURnLaneBorderType BorderType : {EPLATEAURnLaneBorderType::Prev, EPLATEAURnLaneBorderType::Next}) {
        if (URnIntersection* Intersection = Cast<URnIntersection>(Road->GetNeighborRoad(BorderType))) {
            Neighbors.Add({ Intersection, BorderType });
        }
    }

    if (Neighbors.Num() == 0) {
        return false;
    }

    const float OffsetLength = FMath::Max(1.0f,
        FMath::Min(MaxOffset, (MinLength - NeedRoadLengthMeter) / Neighbors.Num()));

    auto Success = true;
    if (URnIntersection* NextIntersection = Cast<URnIntersection>(Road->GetNext())) {
        FLineSegment3D Segment;

        if (Road->TryGetVerticalSliceSegment(EPLATEAURnLaneBorderType::Next, OffsetLength, Segment)) {
            FSliceRoadHorizontalResult Result = SliceRoadHorizontal(Road, Segment);
            if (Result.Result == ERoadCutResult::Success) {
                OutCenterSideRoad = Result.PrevRoad;
                OutNextSideRoad = Result.NextRoad;
                Road = Result.PrevRoad;
            }
            else {
                Success = false;
            }
        }
        else {
            Success = false;
        }
    }

    if (URnIntersection* PrevIntersection = Cast<URnIntersection>(Road->GetPrev())) {
        FLineSegment3D Segment;
        if (Road->TryGetVerticalSliceSegment(EPLATEAURnLaneBorderType::Prev, OffsetLength, Segment)) {
            FSliceRoadHorizontalResult Result = SliceRoadHorizontal(Road, Segment);
            if (Result.Result == ERoadCutResult::Success) {
                OutCenterSideRoad = Result.NextRoad;
                OutPrevSideRoad = Result.PrevRoad;
            }
            else {
                Success = false;
            }
        }
        else {
            Success = false;
        }
    }
    
    return Success;
}


TRnRef_T<URnModel> URnModel::Create() {
    return RnNew<URnModel>();
}

TArray<TRnRef_T<URnRoadBase>> URnModel::GetConnectedRoadBases(const TRnRef_T<URnRoadBase>& RoadBase) const {
    return GetNeighborRoadBases(RoadBase);
}

TArray<TRnRef_T<URnRoad>> URnModel::GetConnectedRoads(const TRnRef_T<URnRoadBase>& RoadBase) const {
    return GetNeighborRoads(RoadBase);
}

TArray<TRnRef_T<URnIntersection>> URnModel::GetConnectedIntersections(const TRnRef_T<URnRoadBase>& RoadBase) const {
    return GetNeighborIntersections(RoadBase);
}

TArray<TRnRef_T<URnSideWalk>> URnModel::GetConnectedSideWalks(const TRnRef_T<URnRoadBase>& RoadBase) const {
    return GetNeighborSideWalks(RoadBase);
}

TArray<TRnRef_T<URnRoadBase>> URnModel::GetConnectedRoadBasesRecursive(const TRnRef_T<URnRoadBase>& RoadBase) const {
    TArray<TRnRef_T<URnRoadBase>> Result;
    TSet<TRnRef_T<URnRoadBase>> Visited;

    if (!RoadBase) return Result;

    TArray<TRnRef_T<URnRoadBase>> Stack;
    Stack.Push(RoadBase);
    Visited.Add(RoadBase);

    while (Stack.Num() > 0) {
        auto Current = Stack.Pop();
        Result.Add(Current);

        for (const auto& Connected : GetConnectedRoadBases(Current)) {
            if (!Visited.Contains(Connected)) {
                Stack.Push(Connected);
                Visited.Add(Connected);
            }
        }
    }

    return Result;
}

void URnModel::MergeRoadGroup()
{
    TSet<TRnRef_T<URnRoad>> visitedRoads;
    auto CopiedRoads = Roads;
    for(auto& road : CopiedRoads)
    {
        if (visitedRoads.Contains(road))
            continue;

        auto roadGroup = URnRoadGroup::CreateRoadGroupOrDefault(road);
        for(auto Road : roadGroup->Roads)
        {
            visitedRoads.Add(Road);
        }
        roadGroup->MergeRoads();
    }
}

void URnModel::SplitLaneByWidth(float RoadWidthMeter, bool rebuildTrack, TArray<FString>& failedRoads, TFunction<bool(URnRoadGroup*)> IsLaneSplitTarget)
{
    failedRoads.Reset();
    TSet<TRnRef_T<URnRoad>> visitedRoads;
    // メートルをユニットに変換
    const auto RoadWidth = FPLATEAURnDef::Meter2Unit * RoadWidthMeter;
    for(auto&& Road : Roads) 
    {
        if (visitedRoads.Contains(Road))
            continue;

        try {
            auto&& RoadGroup = URnRoadGroup::CreateRoadGroupOrDefault(Road);
            for(auto&& l : RoadGroup->Roads)
                visitedRoads.Add(l);

            RoadGroup->Align();
            if (RoadGroup->IsValid() == false)
                continue;

            if (RoadGroup->IsAllLaneValid() == false)
                continue;

            if (RoadGroup->Roads.ContainsByPredicate([](TRnRef_T<URnRoad> l) { return l->MainLanes[0]->HasBothBorder() == false; }))
                continue;

            if (IsLaneSplitTarget(RoadGroup) == false)
                continue;

            auto&& leftCount = RoadGroup->GetLeftLaneCount();
            auto&& rightCount = RoadGroup->GetRightLaneCount();
            // すでにレーンが分かれている場合、左右で独立して分割を行う
            auto GetWidth = [&](TOptional<EPLATEAURnDir> Dir)
            {
                auto Width = FLT_MAX;
                for (auto&& Road : RoadGroup->Roads) {
                    auto&& WidthSum = 0.f;
                    TArray<TRnRef_T<URnLane>> OutLanes;
                    Road->TryGetLanes(Dir, OutLanes);
                    for (auto&& l : OutLanes)
                        WidthSum += l->CalcWidth();
                    Width = FMath::Min(Width, WidthSum);
                }
                return Width;
            };
            if (leftCount > 0 && rightCount > 0) 
            {
                for(auto&& dir : { EPLATEAURnDir::Left, EPLATEAURnDir::Right }) 
                {
                    auto Width = GetWidth(dir);
                    auto&& Num = (int)(Width / RoadWidth);                   
                    RoadGroup->SetLaneCount(dir, Num, rebuildTrack);
                }
            }
            // 
            else {
                auto&& Width = GetWidth(NullOpt);
                auto&& Num = (int)(Width / RoadWidth);
                if (Num <= 1)
                    continue;

                auto&& LeftLaneCount = (Num + 1) / 2;
                auto&& RightLaneCount = Num - LeftLaneCount;
                RoadGroup->SetLaneCount(LeftLaneCount, RightLaneCount, rebuildTrack);
            }

        }
        catch (std::exception e) {
            failedRoads.Add(Road->GetName());
        }
    }
}

bool URnModel::Check() const
{
    for (auto&& Road : Roads) {
        if (Road->Check() == false)
            return false;
    }
    for (auto&& Intersection : Intersections) {
        if (Intersection->Check() == false)
            return false;
    }
    for (auto&& SideWalk : SideWalks) {
        if (SideWalk->Check() == false)
            return false;
    }
    return true;
}

namespace
{

    /// <summary>
    /// 切断時の端点判定の時の許容誤差
    /// </summary>
    constexpr float CutIndexTolerance = 1e-5f;
    
    /// <summary>
    /// wayのlineStringだけ差し替えて他同じ物を返す
    /// </summary>
    /// <param name="lineString"></param>
    /// <param name="way"></param>
    /// <returns></returns>
    URnWay* CopyWay(URnLineString* lineString, URnWay* way) {
        if (!way || !lineString)
            return nullptr;
        return RnNew<URnWay>(lineString, way->IsReversed, way->IsReverseNormal);
    }


    // key: Original LineString, value: SliceRoadMapValue after split
    struct SliceRoadMapValue {
        URnLineString* Prev;
        URnLineString* Next;
        URnPoint* MidPoint;
    };

    void SliceSideWalks(
        URnModel* Model,
        const TArray<URnSideWalk*>& SrcSideWalks,
        const FLineSegment2D& LineSegment2D,
        const TMap<URnLineString*, SliceRoadMapValue>& LineTable,
        URnRoadBase* NewNextRoad,
        const TFunction<bool(URnSideWalk*)>& IsPrevSide)
    {
        auto CopySideWalks = SrcSideWalks;
        for (URnSideWalk* SideWalk : CopySideWalks) {
            const SliceRoadMapValue* Inside =
                LineTable.Find(SideWalk->GetInsideWay()->GetLineString());
            const SliceRoadMapValue* Outside =
                LineTable.Find(SideWalk->GetOutsideWay()->GetLineString());

            if (!Inside && !Outside) {
                if (!IsPrevSide(SideWalk)) {
                    NewNextRoad->AddSideWalk(SideWalk);
                }
                continue;
            }

            URnWay* NextInsideWay = CopyWay(Inside ? Inside->Next : nullptr, SideWalk->GetInsideWay());
            URnWay* NextOutsideWay = CopyWay(Outside ? Outside->Next : nullptr, SideWalk->GetOutsideWay());

            URnWay* PrevInsideWay = CopyWay(Inside ? Inside->Prev : nullptr, SideWalk->GetInsideWay());
            URnWay* PrevOutsideWay = CopyWay(Outside ? Outside->Prev : nullptr, SideWalk->GetOutsideWay());

            auto IsPointShared = [](const URnWay* Way, std::initializer_list<const URnWay*> TargetWays)-> bool {
                if (!Way)
                    return false;
                for (auto W : TargetWays) {
                    if (FRnLineStringEx::IsPointShared(Way->LineString, W->LineString))
                        return true;
                }
                return false;
            };

            // 元のSideWay -> Prev側
            // 新しいSideWay -> Next側
            // となるようにする

            auto StartEdgeWay = SideWalk->GetStartEdgeWay();
            auto EndEdgeWay = SideWalk->GetEndEdgeWay();
            // EndEdgeWayがPrev側のSideWayとポイント共有していたら逆
            if (IsPointShared(EndEdgeWay, { PrevInsideWay, PrevOutsideWay })) {
                Swap(StartEdgeWay, EndEdgeWay);
            }
            // StartEdgeWayがNext側のSideWayとポイント共有していたら逆
            else if (IsPointShared(StartEdgeWay, { NextInsideWay, NextOutsideWay })) {
                Swap(StartEdgeWay, EndEdgeWay);
            }
            
            TArray<URnPoint*> points{ Inside->MidPoint, Outside->MidPoint };
            URnWay* MidEdgeWay = URnWay::Create(URnLineString::Create(points));

            URnSideWalk* NewSideWalk = URnSideWalk::Create(
                NewNextRoad, NextOutsideWay, NextInsideWay, MidEdgeWay, EndEdgeWay, SideWalk->GetLaneType());

            SideWalk->SetSideWays(PrevOutsideWay, PrevInsideWay);
            SideWalk->SetEdgeWays(StartEdgeWay, MidEdgeWay);
            Model->AddSideWalk(NewSideWalk);
        }
    }
}

URnModel::ERoadCutResult URnModel::CanSliceRoadHorizontal(URnRoad* Road, const FLineSegment3D& LineSegment,
    FPLATEAURnEx::FLineCrossPointResult& OutResult)
{
    if (!Road || !Road->IsValid() || !Road->IsAllLaneValid()) {
        return URnModel::ERoadCutResult::InvalidRoad;
    }

    OutResult = FPLATEAURnEx::GetLaneCrossPoints(Road, LineSegment);

    // 同じLineStringと２回以上交わってはいけない
    if (Algo::AnyOf(OutResult.TargetLines, [](const FPLATEAURnEx::FLineCrossPointResult::FTargetLineInfo& X) {
        return X.Intersections.Num() > 1;
    })) {
        return URnModel::ERoadCutResult::InvalidCutLine;
    }

    auto& targetLines = OutResult.TargetLines;

    auto IsSliced = [&](URnWay* Way) {
        if (!Way)
            return false;
        return Algo::AnyOf(targetLines, [Way](const FPLATEAURnEx::FLineCrossPointResult::FTargetLineInfo& X) {
            return X.LineString == Way->LineString && X.Intersections.IsEmpty() == false;
        });
    };

    for (auto Lane : Road->GetAllLanesWithMedian()) {
        for (auto Way : Lane->GetBothWays()) {
            if (IsSliced(Way) == false)
                return URnModel::ERoadCutResult::UnSlicedLaneExist;
        }
    }

    for (auto Sw : Road->GetSideWalks()) {
        if (Sw->IsValid() == false)
            return URnModel::ERoadCutResult::InvalidSideWalk;

        // 歩道は角の道だったりすると前後で分かれていたりするので交わらない場合もある
        // ただし、inside/outsideがどっちも交わるかどっちも交わらないかしか許さない
        auto SwSideWays = Sw->GetSideWays();
        auto SliceCount = Algo::CountIf(SwSideWays, [&](URnWay* Way) {
            return IsSliced(Way);
        });
        if (!(SliceCount == 0 || SliceCount == 2)) {
            return URnModel::ERoadCutResult::PartiallySlicedSideWalkExist;
        }
    }

    for (auto& Line : OutResult.TargetLines) {
        if (Line.Intersections.Num() == 0)
            continue;

        if (Line.Intersections[0].Key <= CutIndexTolerance ||
            Line.Intersections[0].Key >= Line.LineString->Count() - 1.f - CutIndexTolerance) {
            return URnModel::ERoadCutResult::TerminateCutLine;
        }
    }

    return URnModel::ERoadCutResult::Success;
}

URnModel::FSliceRoadHorizontalResult URnModel::SliceRoadHorizontal(URnRoad* Road, const FLineSegment3D& LineSegment)
{
    FSliceRoadHorizontalResult Result;
    FPLATEAURnEx::FLineCrossPointResult CrossPointResult;
    Result.Result = CanSliceRoadHorizontal(Road, LineSegment, CrossPointResult);
    if (Result.Result != ERoadCutResult::Success) {
        return Result;
    }

    const FLineSegment2D LineSegment2D = LineSegment.To2D(FPLATEAURnDef::Plane);

    TMap<URnLineString*, SliceRoadMapValue> LineTable;

    auto OldPrevBorders = Road->GetBorderWays(EPLATEAURnLaneBorderType::Prev);
    auto OldNextBorders = Road->GetBorderWays(EPLATEAURnLaneBorderType::Next);

    // Points for prev/next side determination
    TSet<URnPoint*> PrevBorderPoints;
    TSet<URnPoint*> NextBorderPoints;

    // Collect border points
    for (URnWay* Way : OldPrevBorders) {
        for (URnPoint* Point : Way->GetPoints()) {
            PrevBorderPoints.Add(Point);
        }
    }
    for (URnWay* Way : OldNextBorders) {
        for (URnPoint* Point : Way->GetPoints()) {
            NextBorderPoints.Add(Point);
        }
    }

    // Undecided lines
    TSet<FPLATEAURnEx::FLineCrossPointResult::FTargetLineInfo*> Undecideds;

    // Share points table to prevent duplicates
    TMap<FVector, URnPoint*> SharePoints;

    // Split processing functions
    auto SplitByIndex = [&](URnLineString* Ls, float Index, URnLineString*& OutFront, URnLineString*& OutBack) {
        Ls->SplitByIndex(Index, OutFront, OutBack, [&](const FVector& V) {
            URnPoint** Found = SharePoints.Find(V);
            if (!Found) {
                URnPoint* NewPoint = RnNew<URnPoint>(V);
                SharePoints.Add(V, NewPoint);
                return NewPoint;
            }
            return *Found;
            });
        };

    auto AddTable = [&](const FPLATEAURnEx::FLineCrossPointResult::FTargetLineInfo& inter, bool IsFrontPrev) {
        const auto& item = inter.Intersections[0];
        URnLineString* Front = nullptr;
        URnLineString* Back = nullptr;
        SplitByIndex(inter.LineString, item.Key, Front, Back);
        auto Mid = Back->GetPoint(0);
        if(IsFrontPrev)
        {
            LineTable.Add(inter.LineString, { Front, Back, Mid });
        }
        else {
            LineTable.Add(inter.LineString, { Back, Front, Mid });
        }
    };

    for (auto& inter : CrossPointResult.TargetLines)
    {
        if (inter.Intersections.Num() == 0)
            continue;

        if(PrevBorderPoints.Contains(inter.LineString->GetPoint(0)))
        {
            AddTable(inter, true);
        }
        else if(NextBorderPoints.Contains(inter.LineString->GetPoint(0)))
        {
            AddTable(inter, false);
        }
        else
        {
            Undecideds.Add(&inter);
        }
    }

    for (auto& inter : Undecideds) 
    {
        auto& item = inter->Intersections[0];
        URnLineString* Front = nullptr;
        URnLineString* Back = nullptr;
        SplitByIndex(inter->LineString, item.Key, Front, Back);

        auto MinPrevScore = FLT_MAX;
        auto MaxPrevScore = FLT_MAX;
        for(auto& L : LineTable)
        {
            auto PrevScore = Front->CalcProximityScore(L.Value.Prev);
            auto NextScore = Front->CalcProximityScore(L.Value.Next);

            if (PrevScore.IsSet() && PrevScore.GetValue() < MinPrevScore) {
                MinPrevScore = PrevScore.GetValue();
            }
            if (NextScore.IsSet() && NextScore.GetValue() < MaxPrevScore) {
                MaxPrevScore = NextScore.GetValue();
            }
        }
        auto Mid = Back->GetPoint(0);
        if (MinPrevScore < MaxPrevScore) {
            LineTable.Add(inter->LineString, { Front, Back, Mid });
        }
        else {
            LineTable.Add(inter->LineString, { Back, Front, Mid });
        }
    }

    // 新しく生成されるRoad(Next側に挿入される)
    auto newNextRoad = RnNew<URnRoad>(Road->GetTargetTrans());

    // roadをprev/next側で分断して, next側をnewRoadにする
    for (auto lane : Road->GetAllLanesWithMedian()) 
    {
        // 必ず存在する前提
        auto left = LineTable[lane->GetLeftWay()->LineString];
        auto right = LineTable[lane->GetRightWay()->LineString];

        auto nextLeftWay = CopyWay(left.Next, lane->GetLeftWay());
        auto nextRightWay = CopyWay(right.Next, lane->GetRightWay());

        auto prevLeftWay = CopyWay(left.Prev, lane->GetLeftWay());
        auto prevRightWay = CopyWay(right.Prev, lane->GetRightWay());

        auto isReverseLane = lane->GetIsReversed();

        // 分割個所の境界線
        auto midBorderWay = RnNew<URnWay>(URnLineString::Create(TArray<URnPoint*> { left.MidPoint, right.MidPoint }));

        // 順方向ならNext/逆方向ならPrevが中間地点になる
        auto laneMidBorderType = isReverseLane ? EPLATEAURnLaneBorderType::Prev : EPLATEAURnLaneBorderType::Next;

        // 以前のボーダーは新しいボーダーに設定する
        auto newLaneBorder = lane->GetBorder(laneMidBorderType);
        lane->SetBorder(laneMidBorderType, midBorderWay);

        auto newLane = RnNew<URnLane>(nextLeftWay, nextRightWay, nullptr, nullptr);
        newLane->SetIsReversed(isReverseLane);
        newLane->SetBorder(laneMidBorderType, newLaneBorder);
        newLane->SetBorder( FPLATEAURnLaneBorderTypeEx::GetOpposite(laneMidBorderType), midBorderWay);
        if (lane->IsMedianLane()) {
            newNextRoad->SetMedianLane(newLane);
        }
        else {
            newNextRoad->AddMainLane(newLane);
        }
        lane->SetSideWay(EPLATEAURnDir::Left, prevLeftWay);
        lane->SetSideWay(EPLATEAURnDir::Right, prevRightWay);
    }

    newNextRoad->SetPrevNext(Road, Road->GetNext());
    Road->SetPrevNext(Road->GetPrev(), newNextRoad);

    // next側は新しく道路か挿入されるので,参照情報もroad -> newNextRoadに置き換え
    // prev側は元の道路のままなので差し替える必要はなし
    if(newNextRoad->GetNext())
    {
        for(auto B : OldNextBorders)
            newNextRoad->GetNext()->ReplaceNeighbor(B, newNextRoad);
    }

    AddRoad(newNextRoad);

    // 歩道周りを処理する

    SliceSideWalks(this, Road->GetSideWalks(), LineSegment2D, LineTable, newNextRoad
        , [&](URnSideWalk* sw) {
            return sw->CalcRoadProximityScore(Road) < sw->CalcRoadProximityScore(newNextRoad);
        });
    Result.PrevRoad = Road;
    Result.NextRoad = newNextRoad;
    return Result;
}

void URnModel::SeparateContinuousBorder()
{
    for(auto Road : Roads) {
        Road->SeparateContinuousBorder();
    }
}
