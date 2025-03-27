// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnRoadGroup.h"

#include "Algo/AllOf.h"
#include "RoadNetwork/Structure/RnRoad.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Structure/RnLane.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "RoadNetwork/Structure/RnLineString.h"
#include "RoadNetwork/Structure/RnPoint.h"
#include "RoadNetwork/Util/PLATEAURnDebugEx.h"
#include "RoadNetwork/Util/PLATEAURnEx.h"
#include "RoadNetwork/Util/PLATEAURnLinq.h"

URnRoadGroup::URnRoadGroup(TRnRef_T<URnIntersection> InPrevIntersection,
                           TRnRef_T<URnIntersection> InNextIntersection,
                           const TArray<TRnRef_T<URnRoad>>& InRoads)
{
    Init(InPrevIntersection, InNextIntersection, InRoads);
}

void URnRoadGroup::Init(TRnRef_T<URnIntersection> InPrevIntersection, TRnRef_T<URnIntersection> InNextIntersection,
    const TArray<TRnRef_T<URnRoad>>& InRoads)
{
    PrevIntersection = InPrevIntersection;
    NextIntersection = InNextIntersection;
    Roads = InRoads;
    Align();
}

bool URnRoadGroup::IsValid() const
{
    if (Roads.IsEmpty())
        return false;
    // すべてのRoadが有効かどうか
    for (auto&& Road : Roads) {
        if (!Road->IsValid())
            return false;
    }
    return true;
}

bool URnRoadGroup::IsAllLaneValid() const
{
    for(auto Road : Roads)
    {
        if (!Road->IsAllLaneValid())
            return false;
    }
    return true;
}

int32 URnRoadGroup::GetLeftLaneCount() const {
    Align();
    if (Roads.Num() == 0) return 0;

    auto Ret = INT_MAX;
    for (auto& Road : Roads) {
        Ret = FMath::Min(Ret, Road->GetLeftLaneCount());
    }
    return Ret;
}

int32 URnRoadGroup::GetRightLaneCount() const {
    Align();
    if (Roads.Num() == 0) return 0;

    auto Ret = INT_MAX;
    for (auto& Road : Roads) {
        Ret = FMath::Min(Ret, Road->GetRightLaneCount());
    }
    return Ret;
}

TArray<TRnRef_T<URnLane>> URnRoadGroup::GetRightLanes() const {
    Align();
    TArray<TRnRef_T<URnLane>> Result;
    for (const auto& Road : Roads) {
        Result.Append(Road->GetRightLanes());
    }
    return Result;
}

TArray<TRnRef_T<URnLane>> URnRoadGroup::GetLeftLanes() const {
    Align();
    TArray<TRnRef_T<URnLane>> Result;
    for (const auto& Road : Roads) {
        Result.Append(Road->GetLeftLanes());
    }
    return Result;
}

TArray<TRnRef_T<URnLane>> URnRoadGroup::GetLanes(EPLATEAURnDir Dir) const {
    switch (Dir) {
    case EPLATEAURnDir::Left:
        return GetLeftLanes();
    case EPLATEAURnDir::Right:
        return GetRightLanes();
    default:
        checkf(false, TEXT("Invalid direction in GetLanes"));
        return TArray<TRnRef_T<URnLane>>();
    }
}

void URnRoadGroup::GetMedians(TArray<TRnRef_T<URnWay>>& OutLeft, TArray<TRnRef_T<URnWay>>& OutRight) const {
    OutLeft.Empty(Roads.Num());
    OutRight.Empty(Roads.Num());

    for (const auto& Road : Roads) {
        if (Road->GetLeftLaneCount() > 0) {
            auto CenterLeft = Road->GetLeftLanes().Last();
            OutLeft.Add(CenterLeft->GetRightWay());
        }
        if (Road->GetRightLaneCount() > 0) {
            auto CenterRight = Road->GetRightLanes()[0];
            OutRight.Add(CenterRight->GetRightWay());
        }
    }
}

bool URnRoadGroup::HasMedians() const {
    return Roads.ContainsByPredicate([](const TRnRef_T<URnRoad>& Road) { return Road->MedianLane != nullptr; });
}

bool URnRoadGroup::Align() const
{
    return const_cast<URnRoadGroup*>(this)->Align();
}

bool URnRoadGroup::CreateMedianOrSkip(float MedianWidth, float MaxMedianLaneRate) {
    if (HasMedian()) return false;

    if (GetLeftLaneCount() == 0 || GetRightLaneCount() == 0) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot create median without both left and right lanes"));
        return false;
    }

    MedianWidth = FMath::Max(1.0f, MedianWidth);
    float Width = FLT_MAX;
    for (const auto& Road : Roads) {
        float RoadWidth = 0.0f;
        for (const auto& Lane : Road->GetAllLanesWithMedian()) {
            RoadWidth += Lane->CalcWidth();
        }
        Width = FMath::Min(Width, RoadWidth);
    }

    float MedianRate = FMath::Min(MaxMedianLaneRate, MedianWidth / Width);
    SetLaneCountWithMedian(GetLeftLaneCount(), GetRightLaneCount(), MedianRate);
    return true;
}

bool URnRoadGroup::HasMedian() const {
    return std::all_of(Roads.begin(), Roads.end(), [](const TRnRef_T<URnRoad>& Road) { return Road->MedianLane != nullptr; });
}

bool URnRoadGroup::IsAligned() const {
    if (Roads.Num() <= 1) return true;

    for (int32 i = 0; i < Roads.Num(); ++i) {
        if (i < Roads.Num() - 1 && (Roads)[i]->Next != (Roads)[i + 1])
            return false;

        if (i > 0 && (Roads)[i]->Prev != (Roads)[i - 1])
            return false;
    }

    return true;
}

bool URnRoadGroup::IsDeepAligned() const {
    if (!IsAligned()) return false;
    if (Roads.Num() <= 1) return true;

    for (int32 i = 1; i < Roads.Num(); ++i) {
        auto PrevRoad = (Roads)[i - 1];
        auto NowRoad = (Roads)[i];
        auto NowLanes = NowRoad->GetAllLanesWithMedian();
        auto PrevLanes = PrevRoad->GetAllLanesWithMedian();

        if (NowLanes.Num() != PrevLanes.Num()) {
            UE_LOG(LogTemp, Error, TEXT("Lane counts is mismatch"));
            return false;
        }

        for (int32 j = 0; j < FMath::Min(NowLanes.Num(), PrevLanes.Num()); ++j) {
            auto NowLane = NowLanes[j];
            auto PrevLane = PrevLanes[j];

            if (!PrevLane->GetIsReversed()) {
                if (!NowLane->GetPrevBorder()->IsSameLineReference(PrevLane->GetNextBorder())) {
                    UE_LOG(LogTemp, Error, TEXT("Invalid Direction Lane[%d]"), j);
                    return false;
                }
            }
            else {
                if (!PrevLane->GetPrevBorder()->IsSameLineReference(NowLane->GetNextBorder())) {
                    UE_LOG(LogTemp, Error, TEXT("Invalid Direction Lane[%d]"), j);
                    return false;
                }
            }
        }
    }

    return true;
}

bool URnRoadGroup::Align() {
    if (IsAligned()) return true;

    // Roads.Count <= 1の場合はIsAligned=trueなのでここでは
    // インデックス範囲外チェックはしなくてよい
    // 0番目が逆かどうかチェック
    if ((Roads)[0]->Next != (Roads)[1]) {
        (Roads)[0]->Reverse();
    }

    // 1番目以降が逆かどうかチェック
    for (int32 i = 1; i < Roads.Num(); ++i) {
        if ((Roads)[i]->Prev != (Roads)[i - 1]) {
            (Roads)[i]->Reverse();
        }
    }

    // 境界線の向きもそろえる
    for (const auto& Road : Roads) {
        Road->AlignLaneBorder();
    }

    if ((Roads)[0]->Prev != PrevIntersection) {
        Swap(PrevIntersection, NextIntersection);
    }

    return IsDeepAligned();
}

bool URnRoadGroup::MergeRoads() {
    if (!Align()) return false;
    if (Roads.Num() <= 1) return true;

    // マージ先の道路
    auto DstRoad = (Roads)[0];
    auto DstLanes = DstRoad->GetAllLanesWithMedian();
    // 配列はコピーして持つ
    for (int32 i = 1; i < Roads.Num(); i++) {
        // マージ元の道路. DstRoadに統合されて消える
        auto SrcRoad = (Roads)[i];
        auto SrcLanes = SrcRoad->GetAllLanesWithMedian();

        for (int32 j = 0; j < SrcLanes.Num(); ++j) {
            auto SrcLane = SrcLanes[j];
            auto DstLane = DstLanes[j];

            // 順方向(左車線)
            if (SrcRoad->IsLeftLane(SrcLane)) {

                auto MergedLeftWay = URnWay::CreateMergedWay(DstLane->GetLeftWay(), SrcLane->GetLeftWay());
                auto MergedRightWay = URnWay::CreateMergedWay(DstLane->GetRightWay(), SrcLane->GetRightWay());
                DstLane->SetSideWay(EPLATEAURnDir::Left, MergedLeftWay);
                DstLane->SetSideWay(EPLATEAURnDir::Right, MergedRightWay);
                DstLane->SetBorder(EPLATEAURnLaneBorderType::Next, SrcLane->GetNextBorder());
            }
            // 逆方向(右車線)
            else {

                auto MergedLeftWay = URnWay::CreateMergedWay(SrcLane->GetLeftWay(), DstLane->GetLeftWay());
                auto MergedRightWay = URnWay::CreateMergedWay(SrcLane->GetRightWay(),DstLane->GetRightWay());
                DstLane->SetSideWay(EPLATEAURnDir::Left, MergedLeftWay);
                DstLane->SetSideWay(EPLATEAURnDir::Right, MergedRightWay);               
                DstLane->SetBorder(EPLATEAURnLaneBorderType::Prev, SrcLane->GetPrevBorder());
            }
        }
        
        auto SrcSideWalks = SrcRoad->GetSideWalks();
        for (const auto& SrcSw : SrcSideWalks) {
            DstRoad->AddSideWalk(SrcSw);
        }
        DstRoad->MergeSamePointLineStrings();

        DstRoad->AddTargetTrans(SrcRoad->GetTargetTrans());
        SrcRoad->DisConnect(true);
    }

    if (NextIntersection) 
    {
        NextIntersection->RemoveEdges([&](const TRnRef_T<URnIntersectionEdge>& Edge) { return Edge->GetRoad() == (Roads)[Roads.Num() - 1]; });
        for (const auto& Lane : DstLanes) 
        {
            auto Border = DstRoad->GetBorderWay(Lane, EPLATEAURnLaneBorderType::Next, EPLATEAURnLaneBorderDir::Left2Right);
            auto LinkedNum = NextIntersection->ReplaceEdgeLink(Border, DstRoad);
            if(LinkedNum == 0)
                UE_LOG(LogTemp, Error, TEXT("Failed to link edge"));
        }
    }
    DstRoad->MergeSamePointLineStrings();
    DstRoad->SetPrevNext(PrevIntersection, NextIntersection);

    Roads.Reset();
    Roads.Add(DstRoad);
    return true;
}

TRnRef_T<URnRoadGroup> URnRoadGroup::CreateRoadGroupOrDefault(TRnRef_T<URnIntersection> PrevIntersection, TRnRef_T<URnIntersection> NextIntersection) {
    if (!PrevIntersection || !NextIntersection) return nullptr;

    for (const auto& Road : PrevIntersection->GetNeighborRoads()) {
        if (auto RoadPtr = Road->CastToRoad()) {
            TArray<TRnRef_T<URnRoad>> Roads = { RoadPtr };
            auto Ret = RnNew<URnRoadGroup>(PrevIntersection, NextIntersection, Roads);
            bool HasPrev = Ret->PrevIntersection == PrevIntersection || Ret->NextIntersection == PrevIntersection;
            bool HasNext = Ret->PrevIntersection == NextIntersection || Ret->NextIntersection == NextIntersection;
            if (HasPrev && HasNext) {
                return Ret;
            }
        }
    }

    return nullptr;
}

bool URnRoadGroup::IsSameRoadGroup(TRnRef_T<URnRoadGroup> A, TRnRef_T<URnRoadGroup> B) {
    if (!A && !B) return false;

    // 同じ交差点を含むか（Next,Prevは問わない）
    bool IsSameIntersection =
        (A->PrevIntersection == B->PrevIntersection && A->NextIntersection == B->NextIntersection) ||
        (A->PrevIntersection == B->NextIntersection && A->NextIntersection == B->PrevIntersection);

    // 同じ道路を含むか
    bool IsContainSameRoads = true;
    for (const auto& Road : A->Roads) {
        if (!B->Roads.Contains(Road)) {
            IsContainSameRoads = false;
            break;
        }
    }

    return IsSameIntersection && IsContainSameRoads;
}

void URnRoadGroup::AdjustBorder() {
    Align();

    auto Adjust = [](TRnRef_T<URnRoad> Road, EPLATEAURnLaneBorderType BorderType, TRnRef_T<URnIntersection> Inter) {
        if (!Inter) return;

        FLineSegment3D BorderLeft2Right;
        if (!Road->TryGetAdjustBorderSegment(BorderType, BorderLeft2Right))
            return;

        auto LeftWay = Road->GetMergedSideWay(EPLATEAURnDir::Left);
        auto RightWay = Road->GetMergedSideWay(EPLATEAURnDir::Right);

        auto DuplicatePoints = [&Inter,&Road](TRnRef_T<URnPoint> P, const FVector& CheckVertex) {
            if ((P->Vertex - CheckVertex).SizeSquared() < 1e-6f)
                return;

            for (const auto& Edge : Inter->GetEdges()) {
                if (Edge->GetRoad() != Road && Edge->GetBorder()->LineString->Contains(P)) {
                    int32 I = Edge->GetBorder()->LineString->GetPoints().IndexOfByKey(P);
                    if (I == 0) {
                        Edge->GetBorder()->LineString->GetPoints().Insert(RnNew<URnPoint>(P->Vertex), 1);
                    }
                    else if (I == Edge->GetBorder()->LineString->GetPoints().Num() - 1) {
                        Edge->GetBorder()->LineString->GetPoints().Insert(RnNew<URnPoint>(P->Vertex), Edge->GetBorder()->LineString->GetPoints().Num() - 1);
                    }
                }
            }
            };

        int32 LastPointIndex = BorderType == EPLATEAURnLaneBorderType::Prev ? 0 : -1;
        DuplicatePoints(LeftWay->GetPoint(LastPointIndex), BorderLeft2Right.GetStart());
        DuplicatePoints(RightWay->GetPoint(LastPointIndex), BorderLeft2Right.GetEnd());

        auto AdjustWay = [&](TRnRef_T<URnLane> Lane, TRnRef_T<URnWay> Way) {
            if (Lane->GetIsReversed())
                Way = Way->ReversedWay();
            auto P = Way->GetPoint(LastPointIndex);
            auto N = BorderLeft2Right.GetNearestPoint(P->Vertex);
            P->Vertex = N;
            };

        auto Lanes = Road->GetAllLanesWithMedian();
        TArray<TRnRef_T<URnWay>> NewBorders;
        NewBorders.Reserve(Lanes.Num());

        for (int32 i = 0; i < Lanes.Num(); ++i) {
            auto Lane = Lanes[i];
            AdjustWay(Lane, Lane->GetLeftWay());
            if (i == Lanes.Num() - 1)
                AdjustWay(Lane, Lane->GetRightWay());

            auto L = Lane->GetLeftWay();
            auto R = Lane->GetRightWay();
            if (!Road->IsLeftLane(Lane)) {
                L = L->ReversedWay();
                R = R->ReversedWay();
            }

            auto Points = TArray<TRnRef_T<URnPoint>>();
            Points.Add(L->GetPoint(LastPointIndex));
            Points.Add(R->GetPoint(LastPointIndex));

            auto Ls = RnNew<URnLineString>();
            Ls->SetPoints(Points);
            NewBorders.Add(RnNew<URnWay>(Ls));
        }

        Inter->ReplaceEdges(Road, BorderType, NewBorders);
        for (int32 i = 0; i < Lanes.Num(); ++i) {
            auto B = NewBorders[i];
            auto Lane = Lanes[i];
            if (Road->IsLeftLane(Lane)) {
                Lane->SetBorder(BorderType, B);
            }
            else {
                Lane->SetBorder(  FPLATEAURnLaneBorderTypeEx::GetOpposite(BorderType), B->ReversedWay());
            }
        }
        };

    Adjust((Roads)[Roads.Num() - 1], EPLATEAURnLaneBorderType::Next, NextIntersection);
    Adjust((Roads)[0], EPLATEAURnLaneBorderType::Prev, PrevIntersection);
}
void URnRoadGroup::SetLaneCountImpl(int32 Count, EPLATEAURnDir Dir, bool RebuildTrack)
{
    if (IsValid() == false)
        return;

    // 既に指定の数になっている場合は何もしない
    if (std::all_of(Roads.begin(), Roads.end(), [Count, Dir](const TRnRef_T<URnRoad> R) {
        return  R->GetLanes(Dir).Num() == Count;
        })) {
        return;
    }

    const auto AfterLanes = SplitLane(Count, Dir);

    for(auto I = 0; I < Roads.Num(); ++I)
    {
        auto Road = (Roads)[I];
        auto Lanes =AfterLanes[Road];

        auto BeforeLanes = Road->GetLanes(Dir);
        if (I == Roads.Num() - 1 && NextIntersection) 
        {
            for(auto l : BeforeLanes)
                NextIntersection->RemoveEdge(Road, l);
            for(auto l : Lanes) 
            {
                NextIntersection->AddEdge(Road, l->GetNextBorder());
            }
        }
        if (I == 0 && PrevIntersection) 
        {
            for(auto l : BeforeLanes)
                PrevIntersection->RemoveEdge(Road, l);
            for(auto l : Lanes) {
                PrevIntersection->AddEdge(Road, l->GetPrevBorder());
            }
        }

        // 右車線の場合は反対にする
        // #NOTE : 隣接情報変更後に反転させる
        if (Dir == EPLATEAURnDir::Right) 
        {
            for(auto l : Lanes)
                l->Reverse();
        }

        (Roads)[I]->ReplaceLanes(Lanes, Dir);
    }
}

void URnRoadGroup::SetLaneCountWithoutMedian(int32 LeftCount, int32 RightCount, bool RebuildTrack) {

    if (IsValid() == false)
        return;

    // 既に指定の数になっている場合は何もしない
    if(Algo::AllOf(Roads, [LeftCount, RightCount](const TRnRef_T<URnRoad>& Road) {
        return Road->GetLeftLaneCount() == LeftCount && Road->GetRightLaneCount() == RightCount;
        }))
    {
        return;
    }

    // 向きをそろえる
    Align();

    auto Num = LeftCount + RightCount;
    auto AfterLanes = SplitLane(Num, NullOpt);
    if (AfterLanes.IsEmpty())
        return;

    TArray<TRnRef_T<URnLineString>> NewNextBorders;
    TArray<TRnRef_T<URnLineString>> NewPrevBorders;

    for (int32 i = 0; i < Roads.Num(); ++i) {
        auto Road = (Roads)[i];
        auto& Lanes = AfterLanes[Road];

        if (i == Roads.Num() - 1) {
            if(NextIntersection)
                NextIntersection->ReplaceEdges(Road, EPLATEAURnLaneBorderType::Next, FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetNextBorder(); }));
            NewNextBorders.Append(FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetNextBorder()->LineString; }));
        }

        if (i == 0) {
            if(PrevIntersection)
                PrevIntersection->ReplaceEdges(Road, EPLATEAURnLaneBorderType::Prev, FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetPrevBorder(); }));
            NewPrevBorders.Append(FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetPrevBorder()->LineString; }));
        }

        for (int32 j = LeftCount; j < Lanes.Num(); ++j) {
            Lanes[j]->Reverse();
        }

        Road->ReplaceLanes(Lanes);
    }

    // 中央分離帯を削除する
    for (const auto& Road : Roads) {
        Road->SetMedianLane(nullptr);
    }
}

void URnRoadGroup::SetLaneCount(int32 LeftCount, int32 RightCount, bool RebuildTrack) {
    if (!IsValid()) return;

    if (LeftCount <= 0 && RightCount <= 0) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot set both lane counts to 0"));
        return;
    }

    Align();

    auto NowLeft = GetLeftLaneCount();
    auto NowRight = GetRightLaneCount();

    if ((NowLeft > 0 || LeftCount == 0) && (NowRight > 0 || RightCount == 0)) {
        SetLeftLaneCount(LeftCount, RebuildTrack);
        SetRightLaneCount(RightCount, RebuildTrack);
        return;
    }

    SetLaneCountWithoutMedian(LeftCount, RightCount, RebuildTrack);
}

void URnRoadGroup::SetLaneCountWithMedian(int32 LeftCount, int32 RightCount, float MedianWidthRate) {
    if (!IsValid()) return;

    if (LeftCount <= 0 && RightCount <= 0) {
        UE_LOG(LogTemp, Warning, TEXT("Cannot set both lane counts to 0"));
        return;
    }

    Align();

    auto Num = LeftCount + RightCount + 1;
    auto LaneRate = (1.0f - MedianWidthRate) / (Num - 1);

    auto AfterLanes = SplitLane(Num, NullOpt, [LeftCount, LaneRate, MedianWidthRate](int32 I) {
        if (I == LeftCount)
            return MedianWidthRate;
        return LaneRate;
        });

    TArray<TRnRef_T<URnLineString>> NewNextBorders;
    TArray<TRnRef_T<URnLineString>> NewPrevBorders;

    for (int32 i = 0; i < Roads.Num(); ++i) {
        auto Road = (Roads)[i];
        auto Lanes = AfterLanes[Road];

        if (i == Roads.Num() - 1 && NextIntersection)
        {
            NextIntersection->ReplaceEdges(Road, EPLATEAURnLaneBorderType::Next, FPLATEAURnLinq::Select( Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetNextBorder(); }));
            NewNextBorders.Append(FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetNextBorder()->LineString; }));
        }

        if (i == 0 && PrevIntersection) {
            PrevIntersection->ReplaceEdges(Road, EPLATEAURnLaneBorderType::Prev, FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetPrevBorder(); }));
            NewPrevBorders.Append(FPLATEAURnLinq::Select(Lanes, [](const TRnRef_T<URnLane>& Lane) { return Lane->GetPrevBorder()->LineString; }));
        }

        for (int32 j = LeftCount + 1; j < Lanes.Num(); ++j) {
            Lanes[j]->Reverse();
        }

        auto Median = Lanes[LeftCount];
        Lanes.RemoveAt(LeftCount);
        Road->ReplaceLanes(Lanes);
        Road->SetMedianLane(Median);
    }
}

void URnRoadGroup::SetLeftLaneCount(int32 Count, bool RebuildTrack) {
    if (GetLeftLaneCount() == Count) return;

    if (GetLeftLaneCount() == 0 || Count == 0) {
        SetLaneCountWithoutMedian(Count, GetRightLaneCount(), RebuildTrack);
    }
    else {
        SetLaneCountImpl(Count, EPLATEAURnDir::Left, RebuildTrack);
    }
}

void URnRoadGroup::SetRightLaneCount(int32 Count, bool RebuildTrack) {
    if (GetRightLaneCount() == Count) return;

    if (GetRightLaneCount() == 0 || Count == 0) {
        SetLaneCountWithoutMedian(GetLeftLaneCount(), Count, RebuildTrack);
    }
    else {
        SetLaneCountImpl(Count, EPLATEAURnDir::Right, RebuildTrack);
    }
}

void URnRoadGroup::SetLaneCount(EPLATEAURnDir Dir, int32 Count, bool RebuildTrack) {
    switch (Dir) {
    case EPLATEAURnDir::Left:
        SetLeftLaneCount(Count, RebuildTrack);
        break;
    case EPLATEAURnDir::Right:
        SetRightLaneCount(Count, RebuildTrack);
        break;
    default:
        checkf(false, TEXT("Invalid direction in SetLaneCount"));
        break;
    }
}
TMap<TRnRef_T<URnRoad>, TArray<TRnRef_T<URnLane>>> URnRoadGroup::SplitLane(
    int32 Num,
    TOptional<EPLATEAURnDir> Dir,
    const TFunction<float(int32)>& GetSplitRate) {
    if (Num <= 0) 
        return TMap<TRnRef_T<URnRoad>, TArray<TRnRef_T<URnLane>>>();

    // 各Roadの境界線をLeft->Rightの方向で取得
    TArray<TRnRef_T<URnWay>> MergedBorders = FPLATEAURnLinq::Select(Roads, [Dir](const TRnRef_T<URnRoad>& Road) {
        return Road->GetMergedBorder(EPLATEAURnLaneBorderType::Prev, Dir);
        });
    MergedBorders.Add(Roads.Last()->GetMergedBorder(EPLATEAURnLaneBorderType::Next, Dir));

    TArray<TArray<TRnRef_T<URnWay>>> BorderWays;
    BorderWays.Reserve(Roads.Num() + 1);

    for (const auto& B : MergedBorders) {
        auto Split = B->Split(Num, false, GetSplitRate);
        BorderWays.Add(Split);
    }

    auto Result = TMap<TRnRef_T<URnRoad>, TArray<TRnRef_T<URnLane>>>();
    for (int32 i = 0; i < Roads.Num(); ++i) {
        auto Road = (Roads)[i];
        auto PrevBorders = BorderWays[i];
        auto NextBorders = BorderWays[i + 1];

        TRnRef_T<URnWay> LeftWay, RightWay;
        Road->TryGetMergedSideWay(Dir, LeftWay, RightWay);

        auto LeftVertices = LeftWay->GetVertices().ToArray();
        auto RightVertices = RightWay->GetVertices().ToArray();
        auto Left = LeftWay;
        TArray<TRnRef_T<URnLane>> Lanes;
        Lanes.Reserve(Num);
        float Rate = 0.0f;

        for (int32 N = 0; N < Num; ++N) {
            auto Right = RightWay;
            if (N < Num - 1) {
                Rate += GetSplitRate ? GetSplitRate(N) : (1.0f / Num);
                auto PrevBorder = PrevBorders[N];
                auto NextBorder = NextBorders[N];
                auto Line = FPLATEAURnEx::CreateInnerLerpLineString(
                    LeftVertices,
                    RightVertices,
                    PrevBorder->GetPoint(-1),
                    NextBorder->GetPoint(-1),
                    PrevBorder,
                    NextBorder,
                    Rate);

                Right = RnNew<URnWay>(Line, false, true);
            }

            auto L = RnNew<URnWay>(Left->LineString, Left->IsReversed, false);
            auto R = RnNew<URnWay>(Right->LineString, Right->IsReversed, true);
            auto NewLane = RnNew<URnLane>(L, R, PrevBorders[N], NextBorders[N]);
            Lanes.Add(NewLane);
            Left = Right;
        }

        Result.Add(Road, Lanes);
    }

    return Result;
}

