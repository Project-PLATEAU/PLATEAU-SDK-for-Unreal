// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnRoad.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "RoadNetwork/Structure/RnLane.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "RoadNetwork/Structure/RnPoint.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Util/PLATEAUVectorEx.h"
#include <algorithm>

#include "RoadNetwork/Structure/RnTrackBuilder.h"

void URnRoad::SetNext(TRnRef_T<URnRoadBase> InNext)
{
    this->Next = InNext;
}

void URnRoad::SetPrev(TRnRef_T<URnRoadBase> InPrev)
{
    this->Prev = InPrev;
}

void URnRoad::SetMedianLane1(TRnRef_T<URnLane> InMedianLane)
{
    this->MedianLane = InMedianLane;
}

TRnRef_T<URnRoadBase> URnRoad::GetNext() const
{
    return RnFrom(Next);
}

TRnRef_T<URnRoadBase> URnRoad::GetPrev() const
{
    return RnFrom(Prev);
}

TRnRef_T<URnRoadBase> URnRoad::GetNeighborRoad(EPLATEAURnLaneBorderType BorderType) const {
    if (BorderType == EPLATEAURnLaneBorderType::Prev) {
        return GetPrev();
    }
    if (BorderType == EPLATEAURnLaneBorderType::Next) {
        return GetNext();
    }
    return nullptr;
}

TRnRef_T<URnLane> URnRoad::GetMedianLane() const
{
    return RnFrom(MedianLane);
}

URnRoad::URnRoad()
{
}

URnRoad::URnRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran)
{
    Init(TargetTran);
}

URnRoad::URnRoad(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& InTargetTrans)
{
    Init(InTargetTrans);
}

void URnRoad::Init(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran)
{
    MainLanes.Reset();
    if (TargetTran.IsValid()) {
        GetTargetTrans().Add(TargetTran);
    }
}

void URnRoad::Init(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& InTargetTrans)
{
    MainLanes.Reset();
    for (auto&& Trans : InTargetTrans) {
        if (Trans.IsValid()) {
            GetTargetTrans().Add(Trans);
        }
    }
}

TArray<TRnRef_T<URnLane>> URnRoad::GetAllLanesWithMedian() const
{
    TArray<TRnRef_T<URnLane>> Lanes;

    for(auto i = 0; i < MainLanes.Num(); ++i)
    {
        auto Lane = MainLanes[i];
        // 右車線になったら切り替え
        if (IsLeftLane(Lane) == false)
            break;
        Lanes.Add(Lane);
    }

    const auto LeftLaneCount = Lanes.Num();
    // 左/右切り替え時には中央分離帯を入れる
    if (MedianLane)
        Lanes.Add(MedianLane);

    for (auto i = LeftLaneCount; i < MainLanes.Num(); ++i)
        Lanes.Add(MainLanes[i]);

    return Lanes;
}

bool URnRoad::IsValid() const {
    return MainLanes.Num() > 0 && Algo::AllOf( MainLanes,[](TObjectPtr<URnLane> Lane) { return Lane && Lane->HasBothBorder(); });
}

bool URnRoad::IsAllBothConnectedLane() const {
    return std::all_of(MainLanes.begin(), MainLanes.end(), [](const TRnRef_T<URnLane>& Lane) { return Lane->IsBothConnectedLane(); });
}

bool URnRoad::IsAllLaneValid() const {
    return std::all_of(MainLanes.begin(), MainLanes.end(), [](const TRnRef_T<URnLane>& Lane) { return Lane->IsValidWay(); });
}

bool URnRoad::HasBothLane() const {
    return GetLeftLaneCount() > 0 && GetRightLaneCount() > 0;
}

bool URnRoad::IsEmptyRoad() const {
    return std::all_of(MainLanes.begin(), MainLanes.end(), [](const TRnRef_T<URnLane>& Lane) { return Lane->IsEmptyLane(); });
}

TArray<TRnRef_T<URnLane>> URnRoad::GetLanes(EPLATEAURnDir Dir) const {
    return Dir == EPLATEAURnDir::Left ? GetLeftLanes() : GetRightLanes();
}

bool URnRoad::TryGetLanes(TOptional<EPLATEAURnDir> Dir, TArray<TRnRef_T<URnLane>>& OutLanes) const
{
    if (Dir.IsSet() == false) {
        OutLanes = MainLanes;
        return true;
    }

    if (EPLATEAURnDir::Left == *Dir) {
        OutLanes = GetLeftLanes();
        return true;
    }

    if (EPLATEAURnDir::Right == *Dir) {
        OutLanes = GetRightLanes();
        return true;
    }
    return false;
}


bool URnRoad::IsLeftLane(const TRnRef_T<URnLane>& Lane) const {
    return Lane && !Lane->GetIsReversed();
}

bool URnRoad::IsRightLane(const TRnRef_T<URnLane>& Lane) const {
    return Lane && Lane->GetIsReversed();
}

EPLATEAURnDir URnRoad::GetLaneDir(const TRnRef_T<URnLane>& Lane) const {
    return IsLeftLane(Lane) ? EPLATEAURnDir::Left : EPLATEAURnDir::Right;
}

TArray<TRnRef_T<URnWay>> URnRoad::GetBorders() const
{
    TArray<TRnRef_T<URnWay>> Borders;
    for (const auto& Lane : GetAllLanesWithMedian()) {
        if (Lane->GetPrevBorder()) {
            Borders.Add(Lane->GetPrevBorder());
        }
        if (Lane->GetNextBorder()) {
            Borders.Add(Lane->GetNextBorder());
        }
    }
    return Borders;
}

TArray<TRnRef_T<URnLane>> URnRoad::GetLeftLanes() const {
    return MainLanes.FilterByPredicate([this](const TRnRef_T<URnLane>& Lane) { return IsLeftLane(Lane); });
}

TArray<TRnRef_T<URnLane>> URnRoad::GetRightLanes() const {
    return MainLanes.FilterByPredicate([this](const TRnRef_T<URnLane>& Lane) { return IsRightLane(Lane); });
}

int32 URnRoad::GetLeftLaneCount() const {
    return GetLeftLanes().Num();
}

int32 URnRoad::GetRightLaneCount() const {
    return GetRightLanes().Num();
}

float URnRoad::GetMedianWidth() const {
    return MedianLane ? MedianLane->CalcWidth() : 0.0f;
}

void URnRoad::SetMedianLane(const TRnRef_T<URnLane>& Lane) {
    MedianLane = Lane;
    if (MedianLane) {
        MedianLane->SetParent(RnFrom(this));
    }
}

bool URnRoad::IsMedianLane(const TRnRef_T<const URnLane>& Lane) const {
    return MedianLane == Lane;
}

TArray<TRnRef_T<URnRoadBase>> URnRoad::GetNeighborRoads() const {
    TArray<TRnRef_T<URnRoadBase>> Roads;
    if (GetPrev()) Roads.Add(GetPrev());
    if (GetNext()) Roads.Add(GetNext());
    return Roads;
}

bool URnRoad::AddMainLane(TRnRef_T<URnLane> Lane)
{
    if (MainLanes.Contains(Lane))
        return false;
    OnAddLane(Lane);
    MainLanes.Add(Lane);
    return true;
}

bool URnRoad::AddMainLane(TRnRef_T<URnLane> Lane, int32 Index)
{
    if (MainLanes.Contains(Lane))
        return false;
    OnAddLane(Lane);
    MainLanes.Insert(Lane, Index);
    return true;
}

bool URnRoad::RemoveMainLane(TRnRef_T<URnLane> Lane)
{
    if(MainLanes.Contains(Lane))
    {
        MainLanes.Remove(Lane);
        OnRemoveLane(Lane);
        return true;
    }
    return false;
}

TRnRef_T<URnWay> URnRoad::GetMergedBorder(EPLATEAURnLaneBorderType BorderType, TOptional<EPLATEAURnDir> Dir) const
{
    TArray<TRnRef_T<URnLane>> Lanes;
    if (TryGetLanes(Dir, Lanes) == false)
        return nullptr;
    if (Lanes.Num() == 0)
        return nullptr;

    auto Ls = RnNew<URnLineString>();
    for(auto&& Lane : Lanes)
    {
        auto Way = GetBorderWay(Lane, BorderType, EPLATEAURnLaneBorderDir::Left2Right);
        if (!Way)
            continue;
        for (auto&& Point : Way->GetPoints()) {
            Ls->AddPointOrSkip(Point);
        }
    }
    return RnNew<URnWay>(Ls);
}

TRnRef_T<URnWay> URnRoad::GetMergedSideWay(EPLATEAURnDir Dir) const {
    TRnRef_T<URnWay> LeftWay, RightWay;
    if (!TryGetMergedSideWay(Dir, LeftWay, RightWay)) {
        return nullptr;
    }
    return Dir == EPLATEAURnDir::Left ? LeftWay : RightWay;
}

bool URnRoad::TryGetMergedSideWay(TOptional<EPLATEAURnDir>  Dir, TRnRef_T<URnWay>& OutLeftWay, TRnRef_T<URnWay>& OutRightWay) const
{
    OutLeftWay = OutRightWay = nullptr;
    if (IsValid() == false)
        return false;

    TArray<TRnRef_T<URnLane>> TargetLanes;
    for (auto&& Lane : MainLanes) 
    {
        if (Dir.IsSet() == false || GetLaneDir(Lane) == *Dir)
            TargetLanes.Add(Lane);
    }
    if (TargetLanes.IsEmpty())
        return false;

    auto LeftLane = TargetLanes[0];
    if(LeftLane)
    {
        if (IsLeftLane(LeftLane))
            OutLeftWay = LeftLane->GetLeftWay();
        else if (LeftLane->GetRightWay())
            OutLeftWay = LeftLane->GetRightWay()->ReversedWay();
    }

    auto RightLane = TargetLanes[TargetLanes.Num() - 1];
    if(RightLane)
    {
        if(IsLeftLane(RightLane))
        {
            OutRightWay = RightLane->GetRightWay();
        }
        else if (RightLane->GetLeftWay()) {
            OutRightWay = RightLane->GetLeftWay()->ReversedWay();
        }
    }

    return true;
}

bool URnRoad::TryGetNearestDistanceToSideWays(const TRnRef_T<URnLineString>& LineString, float& OutDistance) const {
    if (!LineString || !LineString->IsValid()) return false;

    TRnRef_T<URnWay> LeftWay, RightWay;
    if (!TryGetMergedSideWay(NullOpt, LeftWay, RightWay)) return false;

    float MinDistance = MAX_FLT;
    for (int32 i = 0; i < LineString->Count(); ++i) {
        FVector Point = LineString->GetPoint(i)->Vertex;
        FVector Nearest;
        float PointIndex, Distance;

        LeftWay->GetNearestPoint(Point, Nearest, PointIndex, Distance);
        MinDistance = FMath::Min(MinDistance, Distance);

        RightWay->GetNearestPoint(Point, Nearest, PointIndex, Distance);
        MinDistance = FMath::Min(MinDistance, Distance);
    }

    OutDistance = MinDistance;
    return true;
}

void URnRoad::AlignLaneBorder(EPLATEAURnLaneBorderDir BorderDir)
{
    for (const auto& Lane : GetAllLanesWithMedian()) {
        Lane->AlignBorder(BorderDir);
    }
}

bool URnRoad::TryGetAdjustBorderSegment(EPLATEAURnLaneBorderType BorderType, FLineSegment3D& OutSegment) const {
    TRnRef_T<URnWay> LeftWay, RightWay;
    if (!TryGetMergedSideWay(NullOpt, LeftWay, RightWay)) return false;

    int32 Index = BorderType == EPLATEAURnLaneBorderType::Prev ? 0 : -1;
    OutSegment = FLineSegment3D(LeftWay->GetPoint(Index)->Vertex, RightWay->GetPoint(Index)->Vertex);
    return true;
}

TRnRef_T<URnWay> URnRoad::GetBorderWay(const TRnRef_T<URnLane>& Lane, EPLATEAURnLaneBorderType BorderType, EPLATEAURnLaneBorderDir BorderDir) const {
    if (!Lane) return nullptr;

    if(IsLeftLane(Lane) == false)
    {
        BorderType = FPLATEAURnLaneBorderTypeEx::GetOpposite(BorderType);
        BorderDir = FPLATEAURnLaneBorderDirEx::GetOpposite(BorderDir);
    }

    auto Border = Lane->GetBorder(BorderType);
    if (!Border) 
        return nullptr;

    if (Lane->GetBorderDir(BorderType) != BorderDir)
        Border = Border->ReversedWay();

    return Border;
}

TArray<URnWay*> URnRoad::GetBorderWays(EPLATEAURnLaneBorderType BorderType) const
{
    TArray<URnWay*> Result;

    // Left -> Median -> Right order
    for (URnLane* Lane : GetAllLanesWithMedian()) {
        EPLATEAURnLaneBorderType LaneBorderType = BorderType;
        EPLATEAURnLaneBorderDir LaneBorderDir = EPLATEAURnLaneBorderDir::Left2Right;

        if (!IsLeftLane(Lane)) {
            LaneBorderType = FPLATEAURnLaneBorderTypeEx::GetOpposite(LaneBorderType);
            LaneBorderDir = FPLATEAURnLaneBorderDirEx::GetOpposite(LaneBorderDir);
        }

        URnWay* Way = Lane->GetBorder(LaneBorderType);
        if (!Way) {
            continue;
        }

        if (Lane->GetBorderDir(LaneBorderType) != LaneBorderDir) {
            Way = Way->ReversedWay();
        }

        Result.Add(Way);
    }
    return Result;
}

void URnRoad::ReplaceLanes(const TArray<TRnRef_T<URnLane>>& NewLanes, EPLATEAURnDir Dir) {
    auto OldLanes = GetLanes(Dir);
    for (auto Lane : OldLanes)
        RemoveMainLane(Lane);

    // Leftは先頭に追加
    if(Dir == EPLATEAURnDir::Left)
    {
        auto i = 0;
        for(auto L : NewLanes)
        {
            AddMainLane(L, i);
            i++;
        }
    }
    else
    {
        for (auto L : NewLanes)
            AddMainLane(L);
    }
}

void URnRoad::ReplaceLanes(const TArray<TRnRef_T<URnLane>>& NewLanes) {
    MainLanes.Reset();
    for (const auto& Lane : NewLanes) {
        Lane->SetParent(RnFrom(this));
        MainLanes.Add(Lane);
    }
}

void URnRoad::SetPrevNext(const TRnRef_T<URnRoadBase>& PrevRoad, const TRnRef_T<URnRoadBase>& NextRoad) {
    Prev = PrevRoad;
    Next = NextRoad;
}

void URnRoad::Reverse(bool KeepOneLaneIsLeft)
{
    Swap(Prev, Next);

    // 親の向きが変わるので,レーンの親に対する向きも変わる
    // 各レーンのWayの向きは変えずにIsRevereだけ変える
    // 左車線/右車線の関係が変わるので配列の並びも逆にする
    for (auto& Lane : GetAllLanesWithMedian()) {
        Lane->SetIsReversed(!Lane->GetIsReversed());
    }
    Algo::Reverse(MainLanes);
    for (auto& Sw : GetSideWalks())
        Sw->ReverseLaneType();

    // １車線道路でかつその道路が右車線扱いになったら, 左車線になるようにレーンを反転させる
    if (KeepOneLaneIsLeft) {
        auto AllLaneWithMedian = GetAllLanesWithMedian();
        if (AllLaneWithMedian.Num() == 1 && GetLeftLaneCount() == 0) {
            for(auto Lane : AllLaneWithMedian)                
                Lane->Reverse();

            for(auto Sw : GetSideWalks())
                Sw->ReverseLaneType();
        }
    }
}

TArray<URnLane*> URnRoad::GetConnectedLanes(URnWay* border)
{
    if (border == nullptr)
        return TArray<URnLane*>();
    TArray<URnLane*> lanes; 
    for(auto lane : MainLanes) 
    {
        // Borderと同じ線上にあるレーンを返す
        auto borders = lane->GetAllBorders();
        if ( Algo::AnyOf(borders, [&](URnWay* W){ return W->IsSameLineReference(border);}))
            lanes.Add(lane);
    }
    return lanes;
}

FVector URnRoad::GetCentralVertex() const {
    if (!IsValid()) return FVector::ZeroVector;

    TArray<FVector> Points;
    TRnRef_T<URnWay> LeftWay, RightWay;
    if (TryGetMergedSideWay(NullOpt, LeftWay, RightWay)) {
        if(LeftWay)
            Points.Add(LeftWay->GetLerpPoint(0.5f));
        if (RightWay)
            Points.Add(RightWay->GetLerpPoint(0.5f));
    }
    return FPLATEAUVectorEx::Centroid(Points);
}

TArray<TRnRef_T<URnWay>> URnRoad::GetAllWays() const {
    TArray<TRnRef_T<URnWay>> Ways = Super::GetAllWays();
    for (const auto& Lane : GetAllLanesWithMedian()) {
        for (const auto& Way : Lane->GetAllWays()) {
            Ways.Add(Way);
        }
    }
    return Ways;
}

void URnRoad::DisConnect(bool RemoveFromModel) {
    Super::DisConnect(RemoveFromModel);
    if (RemoveFromModel && GetParentModel()) {
        GetParentModel()->RemoveRoad(RnFrom(this));
    }
    if (GetPrev()) GetPrev()->UnLink(RnFrom(this));
    if (GetNext()) GetNext()->UnLink(RnFrom(this));
    Prev = nullptr;
    Next = nullptr;
}

void URnRoad::ReplaceNeighbor(const TRnRef_T<URnRoadBase>& From, const TRnRef_T<URnRoadBase>& To) {
    if (Prev == From) Prev = To;
    if (Next == From) Next = To;
}

void URnRoad::ReplaceNeighbor(URnWay* BorderWay, URnRoadBase* To)
{
    if (BorderWay == nullptr) {
        return;
    }

    for(auto Lane : GetAllLanesWithMedian())
    {
        auto PrevBorder = GetBorderWay(Lane, EPLATEAURnLaneBorderType::Prev, EPLATEAURnLaneBorderDir::Left2Right);
        if(PrevBorder && PrevBorder->IsSameLineReference(BorderWay))
        {
            SetPrev(To);
            // BorderのWayにかぶりはないはずなので高速化の為即リターン
            return;
        }

        auto NextBorder = GetBorderWay(Lane, EPLATEAURnLaneBorderType::Next, EPLATEAURnLaneBorderDir::Left2Right);
        if(NextBorder && NextBorder->IsSameLineReference(BorderWay))
        {
            SetNext(To);
            return;
        }
    }
}

bool URnRoad::Check()
{
    for (auto BorderType : { EPLATEAURnLaneBorderType::Prev, EPLATEAURnLaneBorderType::Next }) {
        if (!FRnRoadEx::IsValidBorderAdjacentNeighbor(this, BorderType, true))
            return false;
    }

    for (auto Lane : GetAllLanesWithMedian())
    {
        if (Lane->Check() == false)
            return false;
    }

    return Super::Check();
}

bool URnRoad::TryGetVerticalSliceSegment(EPLATEAURnLaneBorderType BorderSide, float BorderOffset,
    FLineSegment3D& OutSegment)
{
    OutSegment = FLineSegment3D();

    URnWay* LeftWay = nullptr;
    URnWay* RightWay = nullptr;
    if (!TryGetMergedSideWay(NullOpt, LeftWay, RightWay)) {
        UE_LOG(LogTemp, Warning, TEXT("TryGetMergedSideWayに失敗(%s)"), *GetTargetTransName());
        return false;
    }

    URnWay* PrevBorder = GetMergedBorder(EPLATEAURnLaneBorderType::Prev);
    URnWay* NextBorder = GetMergedBorder(EPLATEAURnLaneBorderType::Next);

    FVector Start = PrevBorder->GetLerpPoint(0.5f);
    FVector End = NextBorder->GetLerpPoint(0.5f);


    auto Vertices = FPLATEAURnEx::CreateInnerLerpLineString(
        LeftWay->GetVertices().ToArray(),
        RightWay->GetVertices().ToArray(),
        RnNew<URnPoint>(Start),
        RnNew<URnPoint>(End),
        PrevBorder,
        NextBorder,
        0.5f);

    URnWay* CenterWay = RnNew<URnWay>(Vertices);
    int32 StartIndex = 0;
    int32 EndIndex = 0;
    FVector Position = FVector::ZeroVector;

    URnWay* Border = (BorderSide == EPLATEAURnLaneBorderType::Prev) ? PrevBorder : NextBorder;

    TArray<FVector> CheckBorderPoints;
    CheckBorderPoints.Add(Border->GetVertex(0));
    CheckBorderPoints.Add(Border->GetVertex(-1));

    auto MaxBorderAdvanceLength = 10 * FPLATEAURnDef::Meter2Unit;
    URnRoadBase* NeighborRoad = GetNeighborRoad(BorderSide);
    if (NeighborRoad) {
        for (URnSideWalk* SideWalk : GetSideWalks()) {
            for (URnWay* Way : SideWalk->GetEdgeWays()) 
            {
                // neighborRoadと繋がっている境界線のみを対象
                if (NeighborRoad->GetSideWalks().ContainsByPredicate(
                    [&](URnSideWalk* Sw) {
                        return Sw->GetAllWays().ContainsByPredicate([&](URnWay* W) {
                            return W->IsSameLineReference(Way);
                            }); }
                        ) == false)
                {
                    continue;
                }

                // borderとの距離が一定以上離れている場合, 全然違うところにある歩道の可能性が高いので無視
                // (交差点に同じ道路をPrev/Next両方でつながっている場合に起こりえる)
                const auto Distance = Border->GetDistance2D(Way);
                if (Distance > MaxBorderAdvanceLength)
                    continue;

                CheckBorderPoints.Add(Way->GetVertex(0));
                CheckBorderPoints.Add(Way->GetVertex(-1));
            }
        }
    }

    for (const FVector& Point : CheckBorderPoints) {
        float Index;
        FVector Nearest;
        float Distance;
        CenterWay->GetNearestPoint(Point, Nearest, Index, Distance);

        float Length;
        if (BorderSide == EPLATEAURnLaneBorderType::Next) {
            Length = CenterWay->CalcLength(Index, CenterWay->Count() - 1);
        }
        else {
            Length = CenterWay->CalcLength(0, Index);
        }

        // 余りにも長くとりすぎる場合は, 境界線関係ない歩道の可能性が高いので無視する
        // 交差点に同じ道路かNext/Prev両方でつながっている場合に起こりえる
        if (Length > BorderOffset + MaxBorderAdvanceLength)
            continue;

        // Add 2.5m margin
        BorderOffset = FMath::Max(BorderOffset, Length + 2.5f * FPLATEAURnDef::Meter2Unit);
    }

    if (BorderSide == EPLATEAURnLaneBorderType::Next) {
        Position = CenterWay->GetAdvancedPointFromBack(BorderOffset, StartIndex, EndIndex);
    }
    else if (BorderSide == EPLATEAURnLaneBorderType::Prev) {
        Position = CenterWay->GetAdvancedPointFromFront(BorderOffset, StartIndex, EndIndex);
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("InValid BorderSide(%s)"), *GetTargetTransName());
        return false;
    }

    FVector Direction = (CenterWay->GetVertex(EndIndex) - CenterWay->GetVertex(StartIndex)).GetSafeNormal();
    Direction = FRotator(0.f, 90.f, 0.f).RotateVector(Direction);

    FRay Ray(Position, Direction);

    TTuple<float, FVector> LeftIntersection, RightIntersection;
    if (!LeftWay->LineString->TryGetNearestIntersectionBy2D(Ray, LeftIntersection)) {
        UE_LOG(LogTemp, Warning, TEXT("LeftWayの切断に失敗(%s)"), *GetTargetTransName());
        return false;
    }

    if (!RightWay->LineString->TryGetNearestIntersectionBy2D(Ray, RightIntersection)) {
        UE_LOG(LogTemp, Warning, TEXT("RightWayの切断に失敗(%s)"), *GetTargetTransName());
        return false;
    }

    FVector DirectionNormalized = (LeftIntersection.Value - Ray.Origin).GetSafeNormal();
    auto Padding = 20.f * FPLATEAURnDef::Meter2Unit;
    OutSegment = FLineSegment3D(
        LeftIntersection.Value + DirectionNormalized * Padding,
        RightIntersection.Value - DirectionNormalized * Padding);

    return true;
}

TRnRef_T<URnRoad> URnRoad::Create(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran) {
    return RnNew<URnRoad>(TargetTran);
}

TRnRef_T<URnRoad> URnRoad::Create(const TArray<TWeakObjectPtr<UPLATEAUCityObjectGroup>>& TargetTrans) {
    return RnNew<URnRoad>(TargetTrans);
}

TRnRef_T<URnRoad> URnRoad::CreateIsolatedRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran, TRnRef_T<URnWay> Way)
{
    const auto Lane = URnLane::CreateOneWayLane(Way);
    auto Ret = RnNew<URnRoad>(TargetTran);
    Ret->AddMainLane(Lane);
    return Ret;
        
}

TRnRef_T<URnRoad> URnRoad::CreateOneLaneRoad(TWeakObjectPtr<UPLATEAUCityObjectGroup> TargetTran, TRnRef_T<URnLane> Lane)
{
    auto Ret = RnNew<URnRoad>(TargetTran);
    Ret->AddMainLane(Lane);
    return Ret;
}

void URnRoad::OnAddLane(const TRnRef_T<URnLane> Lane)
{
    if (!Lane)
        return;
    Lane->SetParent(RnFrom(this));
}

void URnRoad::OnRemoveLane(const TRnRef_T<URnLane> Lane)
{
    if (!Lane)
        return;
    if (Lane->GetParent() == this)
        Lane->SetParent(nullptr);
}

bool FRnRoadEx::IsValidBorderAdjacentNeighbor(const URnRoad* Self, EPLATEAURnLaneBorderType BorderType,
                                              bool NoBorderIsTrue)
{
    if (!Self)
        return false;
    auto Neighbor = Self->GetNeighborRoad(BorderType);
    if (!Neighbor)
        return NoBorderIsTrue;
    auto Ways = Neighbor->GetBorders();

    const auto OppositeBorderType = FPLATEAURnLaneBorderTypeEx::GetOpposite(BorderType);
    for (auto Lane : Self->GetMainLanes()) 
    {
        auto LaneBorder = Self->GetBorderWay(Lane, BorderType, EPLATEAURnLaneBorderDir::Left2Right);        
        if (!LaneBorder)
        {
            if (NoBorderIsTrue)
                continue;
            return false;
        }

        auto OppositeLaneBorder = Self->GetBorderWay(Lane, OppositeBorderType, EPLATEAURnLaneBorderDir::Left2Right);

        // 隣接する道路に共通の境界線を持たない場合はfalse
        if (!Algo::AnyOf(Ways, [LaneBorder](const TRnRef_T<URnWay>& Way) { return Way->IsSameLineReference(LaneBorder); })) 
        {
            // 逆側のボーダーを持っている場合, バグでPrev/Nextが逆になっている
            auto IsRev = Algo::AnyOf(Ways
                , [OppositeLaneBorder](const TRnRef_T<URnWay>& Way) { return Way->IsSameLineReference(OppositeLaneBorder); });
            if(!NoBorderIsTrue || IsRev)
                return false;
        }
    }
    return true;
}

TArray<TRnRef_T<URnWay>> URnRoad::GetMergedSideWays() const {
    TArray<TRnRef_T<URnWay>> Ways;
    TRnRef_T<URnWay> LeftWay, RightWay;

    if (TryGetMergedSideWay(NullOpt, LeftWay, RightWay))
    {
        Ways.Add(LeftWay);
        Ways.Add(RightWay);
    }
    return Ways;
}

bool URnRoad::TryMerge2NeighborIntersection(EPLATEAURnLaneBorderType BorderType) {
    URnRoadBase* Neighbor = GetNeighborRoad(BorderType);
    URnIntersection* Intersection = Cast<URnIntersection>(Neighbor);
    if (!Intersection) {
        return false;
    }

    URnWay* LeftWay = nullptr;
    URnWay* RightWay = nullptr;
    TryGetMergedSideWay(NullOpt, LeftWay, RightWay);

    auto EdgeGroups = FRnIntersectionEx::CreateEdgeGroup(Intersection);
    auto EdgeGroup = EdgeGroups.FindByPredicate([&](const FRnIntersectionEx::FEdgeGroup& X) {
        return X.Key == this;
        });
    if (!EdgeGroup) {
        return false;
    }

    TArray<URnWay*> OppositeBorders =  GetBorderWays( FPLATEAURnLaneBorderTypeEx::GetOpposite(BorderType));
    URnRoadBase* OppositeRoadBase = GetNeighborRoad(FPLATEAURnLaneBorderTypeEx::GetOpposite(BorderType));


    // 外形線部分を追加
    Intersection->AddEdge(nullptr, LeftWay);
    Intersection->AddEdge(nullptr, RightWay);

    // 参照情報からthisを削除して新しいものに差し替え
    Intersection->ReplaceEdges(this, BorderType, OppositeBorders);

    // Selfの隣接道路に対して, Selfに対する接続情報を置き換える
    Intersection->ReplaceNeighbor(this, OppositeRoadBase);
    OppositeRoadBase->ReplaceNeighbor(this, Intersection);

    // 外形のEdgeがEdge分かれるので一つにまとめる
    Intersection->MergeContinuousNonBorderEdge();


    // 歩道を一つに統合する
    TArray<URnSideWalk*> SrcSideWalks = GetSideWalks();
    Intersection->MergeSideWalks(SrcSideWalks);

    // 全部終わった後, 共通のポイントを持つLineStringは統合する
    Intersection->MergeSamePointLineStrings();

    // トラックを生成しなおす
    TArray<URnLineString*> RebuildTargetLineStrings;
    for (auto B : OppositeBorders) {
        RebuildTargetLineStrings.Add(B->LineString);
    }
    Intersection->BuildTracks(FBuildTrackOption::WithBorder(RebuildTargetLineStrings));

    Intersection->AddTargetTrans(GetTargetTrans());
    DisConnect(true);

    return true;
}
void URnRoad::SeparateContinuousBorder() {
    auto IsConnected = [](URnWay* A, URnWay* B, URnPoint*& OutNewA, URnPoint*& OutJointPoint, URnPoint*& OutNewB) -> bool {
        OutNewA = nullptr;
        OutNewB = nullptr;
        OutJointPoint = nullptr;

        TArray<int32> D = { 0, -1 };
        for (int32 D1 : D) {
            for (int32 D2 : D) {
                if (A->GetPoint(D1) == B->GetPoint(D2)) {
                    // 1cmずらす
                    const float Offset = 0.01f * FPLATEAURnDef::Meter2Unit;
                    OutNewA = RnNew<URnPoint>(A->GetAdvancedPoint(Offset, D1 == -1));
                    OutNewB = RnNew<URnPoint>(B->GetAdvancedPoint(Offset, D2 == -1));
                    OutJointPoint = A->GetPoint(D1);
                    return true;
                }
            }
        }
        return false;
        };

    auto SeparateLane = [&](URnLane* Lane) {
        if (!Lane->GetPrevBorder() || !Lane->GetNextBorder()) {
            return;
        }

        URnPoint* NewA;
        URnPoint* JointPoint;
        URnPoint* NewB;
        if (!IsConnected(Lane->GetPrevBorder(), Lane->GetNextBorder(), NewA, JointPoint, NewB)) {
            return;
        }

        TArray<URnPoint*> Points = { NewA, JointPoint, NewB };
        URnLineString* NewLs = URnLineString::Create(Points);
        URnRoadBase* PrevRoad = Lane->GetPrevRoad();
        URnRoadBase* NextRoad = Lane->GetNextRoad();

        Lane->GetPrevBorder()->GetLineString()->ReplacePoint(JointPoint, NewA);
        Lane->GetNextBorder()->GetLineString()->ReplacePoint(JointPoint, NewB);

        for (URnLineString* Ls : PrevRoad->GetAllLineStringsDistinct()) {
            Ls->ReplacePoint(JointPoint, NewA);
        }
        for (URnLineString* Ls : NextRoad->GetAllLineStringsDistinct()) {
            Ls->ReplacePoint(JointPoint, NewB);
        }

        if (!Lane->GetLeftWay()) {
            URnWay* NewWay = RnNew<URnWay>(NewLs, false, false);
            Lane->SetSideWay(EPLATEAURnDir::Left, NewWay);
        }
        else if (!Lane->GetRightWay()) {
            URnWay* NewWay = RnNew<URnWay>(NewLs, false, true);
            Lane->SetSideWay(EPLATEAURnDir::Right, NewWay);
        }
        };

    for (URnLane* Lane : GetAllLanesWithMedian()) {
        SeparateLane(Lane);
    }
}
