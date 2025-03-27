// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnLane.h"

#include "Algo/AnyOf.h"
#include "RoadNetwork/Util/PLATEAURnEx.h"
#include "RoadNetwork/Util/PLATEAUVectorEx.h"

URnLane::URnLane()
    : bIsReversed(false) {
}

URnLane::URnLane(const TRnRef_T<URnWay>& InLeftWay, const TRnRef_T<URnWay>& InRightWay,
    const TRnRef_T<URnWay>& InPrevBorder, const TRnRef_T<URnWay>& InNextBorder)
    : bIsReversed(false) {
    Init(InLeftWay, InRightWay, InPrevBorder, InNextBorder);

}

void URnLane::Init()
{}

void URnLane::Init(const TRnRef_T<URnWay>& InLeftWay, const TRnRef_T<URnWay>& InRightWay,
                   const TRnRef_T<URnWay>& InPrevBorder, const TRnRef_T<URnWay>& InNextBorder)
{
    this->LeftWay    = InLeftWay;
    this->RightWay   = InRightWay;
    this->PrevBorder = InPrevBorder;
    this->NextBorder = InNextBorder;
}

TArray<TRnRef_T<URnWay>> URnLane::GetBothWays() const {
    TArray<TRnRef_T<URnWay>> Ways;
    if (LeftWay) Ways.Add(LeftWay);
    if (RightWay) Ways.Add(RightWay);
    return Ways;
}

TArray<TRnRef_T<URnWay>> URnLane::GetAllBorders() const {
    TArray<TRnRef_T<URnWay>> Borders;
    if (PrevBorder) Borders.Add(PrevBorder);
    if (NextBorder) Borders.Add(NextBorder);
    return Borders;
}

TArray<TRnRef_T<URnWay>> URnLane::GetAllWays() const {
    TArray<TRnRef_T<URnWay>> Ways = GetBothWays();
    Ways.Append(GetAllBorders());
    return Ways;
}

TRnRef_T<URnRoad> URnLane::GetParent() const
{
    return RnFrom(Parent);
}

void URnLane::SetParent(TRnRef_T<URnRoad> InParent)
{
    Parent = InParent;
}

TRnRef_T<URnWay> URnLane::GetLeftWay() const
{ return LeftWay; }

TRnRef_T<URnWay> URnLane::GetRightWay() const
{ return RightWay; }

TRnRef_T<URnWay> URnLane::GetPrevBorder() const
{ return PrevBorder; }

TRnRef_T<URnWay> URnLane::GetNextBorder() const
{ return NextBorder; }

bool URnLane::IsValidWay() const {
    return LeftWay && RightWay && LeftWay->IsValid() && RightWay->IsValid();
}

bool URnLane::IsBothConnectedLane() const {
    return GetPrevBorder() && GetNextBorder();
}

bool URnLane::HasBothBorder() const {
    return GetPrevBorder() && GetNextBorder();
}

bool URnLane::IsEmptyLane() const {
    return !GetLeftWay() && !GetRightWay() && HasBothBorder();
}

bool URnLane::IsMedianLane() const {
    return GetParent() ? GetParent()->IsMedianLane(RnFrom(this)) : false;
}
TOptional<EPLATEAURnLaneBorderDir> URnLane::GetBorderDir(EPLATEAURnLaneBorderType BorderType) const
{
    const auto Border = GetBorder(BorderType);
    if (!Border)
        return NullOpt;
    if (!IsValidWay())
        return NullOpt;

    if (Border->IsValid() == false)
        return NullOpt;

    // とりあえず重なるかで判定
    // LeftWay/RightWayの端点とボーダーの開始点が一致するかで判定
    const auto Start = Border->GetPoint(0);
    for(const auto Ind : { 0, -1 })
    {
        if (Start == LeftWay->GetPoint(Ind))
            return EPLATEAURnLaneBorderDir::Left2Right;

        if (Start == RightWay->GetPoint(Ind))
            return EPLATEAURnLaneBorderDir::Right2Left;
    }

    // 一致しない場合はベクトルの向きで大体で判定
    const auto D = Border->GetPoint(1)->Vertex - Border->GetPoint(0)->Vertex;
    const auto Index = BorderType == EPLATEAURnLaneBorderType::Prev ? 0 : -1;
    // 左->右の方向ベクトルとボーダーの方向ベクトルの内積が正ならLeft2Right
    auto D2 = RightWay->GetPoint(Index)->Vertex - LeftWay->GetPoint(Index)->Vertex;
    if (FVector2d::DotProduct(FPLATEAURnDef::To2D(D), FPLATEAURnDef::To2D(D2)) > 0.f)
        return EPLATEAURnLaneBorderDir::Left2Right;

    return EPLATEAURnLaneBorderDir::Right2Left;
}

TRnRef_T<URnWay> URnLane::GetBorder(EPLATEAURnLaneBorderType Type) const {
    return Type == EPLATEAURnLaneBorderType::Prev ? PrevBorder : NextBorder;
}

void URnLane::SetBorder(EPLATEAURnLaneBorderType Type, const TRnRef_T<URnWay>& Border) {
    if (Type == EPLATEAURnLaneBorderType::Prev)
        PrevBorder = Border;
    else
        NextBorder = Border;
}

TRnRef_T<URnWay> URnLane::GetSideWay(EPLATEAURnDir Dir) const {
    return Dir == EPLATEAURnDir::Left ? LeftWay : RightWay;
}

void URnLane::SetSideWay(EPLATEAURnDir Dir, const TRnRef_T<URnWay>& Way) {
    if (Dir == EPLATEAURnDir::Left)
        LeftWay = Way;
    else
        RightWay = Way;
}

float URnLane::CalcWidth() const {
    return FMath::Min(CalcPrevBorderWidth(), CalcNextBorderWidth());
}
float URnLane::CalcPrevBorderWidth() const {
    return FRnWayEx::CalcLengthOrDefault(PrevBorder);
}

/// <summary>
/// NextBorderの長さを返す
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
float URnLane::CalcNextBorderWidth() const {
    return FRnWayEx::CalcLengthOrDefault(NextBorder);
}
float URnLane::CalcMinWidth() const
{
    if (!FRnWayEx::IsValidWayOrDefault(LeftWay))
        return 0.f;
    if (!FRnWayEx::IsValidWayOrDefault(RightWay))
        return 0.f;

    float MinW = MAX_FLT;
    for(auto V : LeftWay->GetVertices())
    {
        FVector OutNearest;
        float OutPointIndex;
        float OutDistance;
        RightWay->GetNearestPoint(V, OutNearest, OutPointIndex, OutDistance);
        MinW = FMath::Min(MinW, OutDistance);
    }
    for (auto V : RightWay->GetVertices()) {
        FVector OutNearest;
        float OutPointIndex;
        float OutDistance;
        LeftWay->GetNearestPoint(V, OutNearest, OutPointIndex, OutDistance);
        MinW = FMath::Min(MinW, OutDistance);
    }
    return MinW;
}

void URnLane::Reverse() {
    bIsReversed = !bIsReversed;
    Swap(PrevBorder, NextBorder);
    Swap(LeftWay, RightWay);
    for (auto Way : GetAllWays())
        Way->Reverse(true);
}

void URnLane::AlignBorder(EPLATEAURnLaneBorderDir borderDir)
{
    AlignBorder(EPLATEAURnLaneBorderType::Prev, borderDir);
    AlignBorder(EPLATEAURnLaneBorderType::Next, borderDir);
}

void URnLane::BuildCenterWay() {
    if (!IsValidWay()) return;



    auto Prev = PrevBorder;
    if (!Prev)
        Prev = RnNew<URnWay>(URnLineString::Create({ LeftWay->GetPoint(0), RightWay->GetPoint(0) }));

    auto Next = NextBorder;
    if(!Next)
        Next = RnNew<URnWay>(URnLineString::Create({ LeftWay->GetPoint(-1), RightWay->GetPoint(-1) }));

    auto St = RnNew<URnPoint>(Prev->GetLerpPoint(0.5f));
    auto En = RnNew<URnPoint>(Next->GetLerpPoint(0.5f));
    auto Vertices = FPLATEAURnEx::CreateInnerLerpLineString(
        LeftWay->GetVertices().ToArray()
        , RightWay->GetVertices().ToArray()
        , St
        , En
        , Prev
        , Next
        , 0.5f
    );

    CenterWay = RnNew<URnWay>(Vertices);
}

TRnRef_T<URnWay> URnLane::GetCenterWay() {
    if (!CenterWay) BuildCenterWay();
    return CenterWay;
}

void URnLane::GetNearestCenterPoint(const FVector& Pos, FVector& OutNearest, float& OutPointIndex, float& OutDistance) const {
    if (!CenterWay) {
        const_cast<URnLane*>(this)->BuildCenterWay();
    }
    CenterWay->GetNearestPoint(Pos, OutNearest, OutPointIndex, OutDistance);
}

float URnLane::GetCenterLength() const {
    return CenterWay ? CenterWay->CalcLength() : 0.0f;
}

float URnLane::GetDistanceFrom(const FVector& Point) const {
    if (!IsValidWay()) return MAX_FLT;

    FVector Nearest;
    float PointIndex, Distance;
    GetNearestCenterPoint(Point, Nearest, PointIndex, Distance);
    return Distance;
}

bool URnLane::IsInside(const FVector& Point) const {
    if (!IsValidWay()) return false;

    FVector Nearest;
    float Distance;
    return !LeftWay->IsOutSide(Point, Nearest, Distance) &&
        !RightWay->IsOutSide(Point, Nearest, Distance);
}


FVector URnLane::GetCentralVertex() const {
    TArray<FVector> Points;
    if (GetLeftWay())
        Points.Add(GetLeftWay()->GetLerpPoint(0.5f));
    if (GetRightWay())
        Points.Add(GetRightWay()->GetLerpPoint(0.5f));
    return FPLATEAUVectorEx::Centroid(Points);
}



TRnRef_T<URnLane> URnLane::Clone() const {
    auto NewLane = RnNew<URnLane>();
    NewLane->LeftWay = LeftWay ? LeftWay->Clone(true) : nullptr;
    NewLane->RightWay = RightWay ? RightWay->Clone(true) : nullptr;
    NewLane->PrevBorder = PrevBorder ? PrevBorder->Clone(true) : nullptr;
    NewLane->NextBorder = NextBorder ? NextBorder->Clone(true) : nullptr;
    NewLane->bIsReversed = bIsReversed;
    if (CenterWay) NewLane->CenterWay = CenterWay->Clone(true);
    return NewLane;
}

TRnRef_T<URnRoadBase> URnLane::GetNextRoad()
{
    auto Ret = GetNextRoads();
    if (Ret.IsEmpty())
        return nullptr;
    return Ret[0];
}

TRnRef_T<URnRoadBase> URnLane::GetPrevRoad()
{
    auto Ret = GetPrevRoads();
    if (Ret.IsEmpty())
        return nullptr;
    return Ret[0];
}

bool URnLane::Check()
{
    // LeftWay, RightWay, PrevBorder, NextBorder が全て有効な場合のみチェック
    if (LeftWay && RightWay && PrevBorder && NextBorder) {
        // 左側 Way の先頭の点が PrevBorder に含まれているかチェック
        if (!PrevBorder->GetLineString()->GetPoints().Contains(LeftWay->GetPoint(0))) {
            UE_LOG(LogTemp, Error, TEXT("PrevBorderにLeftWay[0]が含まれていません. %s"), *Parent->GetTargetTransName());
            return false;
        }
        // 右側 Way の先頭の点が PrevBorder に含まれているかチェック
        if (!PrevBorder->GetLineString()->GetPoints().Contains(RightWay->GetPoint(0))) {
            UE_LOG(LogTemp, Error, TEXT("PrevBorderにRightWay[0]が含まれていません. %s"), *Parent->GetTargetTransName());
            return false;
        }

        // NextBorder には各 Way の末尾の点が含まれているはずです。
        const int32 LeftLastIndex = LeftWay->Count() - 1;
        const int32 RightLastIndex = RightWay->Count() - 1;
        if (!NextBorder->GetLineString()->GetPoints().Contains(LeftWay->GetPoint(LeftLastIndex))) {
            UE_LOG(LogTemp, Error, TEXT("NextBorderにLeftWay[最後]が含まれていません. %s"), *Parent->GetTargetTransName());
            return false;
        }
        if (!NextBorder->GetLineString()->GetPoints().Contains(RightWay->GetPoint(RightLastIndex))) {
            UE_LOG(LogTemp, Error, TEXT("NextBorderにRightWay[最後]が含まれていません. %s"), *Parent->GetTargetTransName());
            return false;
        }
    }

    return true;
}

TRnRef_T<URnLane> URnLane::CreateOneWayLane(TRnRef_T<URnWay> way)
{
    return RnNew<URnLane>(way, nullptr, nullptr, nullptr);
}

TRnRef_T<URnLane> URnLane::CreateEmptyLane(TRnRef_T<URnWay> border, TRnRef_T<URnWay> centerWay)
{
    auto ret = RnNew<URnLane>(nullptr, nullptr, border, border);
    ret->CenterWay = centerWay;
    return ret;
}

void URnLane::AlignBorder(EPLATEAURnLaneBorderType type, EPLATEAURnLaneBorderDir borderDir)
{
    auto border = GetBorder(type);
    if (!border)
        return;
    auto dir = GetBorderDir(type);
    if (dir != borderDir) {
        border->Reverse(true);
    }
}

TArray<TRnRef_T<URnRoadBase>> URnLane::GetConnectedRoads(TRnRef_T<URnWay> Border)
{
    if(!Parent || !Border)
        return TArray<TRnRef_T<URnRoadBase>>();
    TArray<TRnRef_T<URnRoadBase>> Result;
    for(auto B : Parent->GetNeighborRoads())
    {
        if (Algo::AnyOf( B->GetBorders(), [&](TRnRef_T<URnWay> W)
        {
                return W->IsSameLineReference(Border);
            })) {
            Result.Add(B);
        }
    }
    return Result;
}
TArray<TRnRef_T<URnRoadBase>> URnLane::GetNextRoads()
{
    return GetConnectedRoads(NextBorder);
}

TArray<TRnRef_T<URnRoadBase>> URnLane::GetPrevRoads() {
    return GetConnectedRoads(PrevBorder);
}