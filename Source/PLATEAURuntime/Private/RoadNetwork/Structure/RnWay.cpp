// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnWay.h"
#include "Algo/Reverse.h"


URnWay::URnWay()
    : IsReversed(false)
    , IsReverseNormal(false) {
}

URnWay::URnWay(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed, bool bInIsReverseNormal)
{
    Init(InLineString, bInIsReversed, bInIsReverseNormal);
}

void URnWay::Init()
{
}

void URnWay::Init(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed, bool bInIsReverseNormal)
{
    LineString = InLineString;
    IsReversed = bInIsReversed;
    IsReverseNormal = bInIsReverseNormal;
}

int32 URnWay::Count() const
{ return LineString ? LineString->GetPoints().Num() : 0; }

bool URnWay::IsValid() const
{ return LineString ? LineString->IsValid() : false; }

TRnRef_T<URnPoint> URnWay::GetPoint(int32 Index) const {
    int32 RawIndex = ToRawIndex(Index, true);
    return LineString->GetPoint(RawIndex);
}

TRnRef_T<URnPoint> URnWay::SetPoint(int32 Index, const TRnRef_T<URnPoint>& Point) {
    int32 RawIndex = ToRawIndex(Index, true);
    auto Ret = LineString->GetPoint(RawIndex);
    LineString->SetPoint(RawIndex, Point);
    return Ret;
}

void URnWay::SetPoints(const TArray<TRnRef_T<URnPoint>>& Points)
{
    if(IsReversed)
    {
        TArray<TRnRef_T<URnPoint>> ReversedPoints;
        for (int i = Points.Num() - 1; i >= 0; --i)
        {
            ReversedPoints.Add(Points[i]);
        }
        LineString->Init(ReversedPoints);   
    }else
    {
        LineString->Init(Points);
    }
}

FVector URnWay::GetVertex(int32 Index) const
{
    return GetPoint(Index)->Vertex;
}

FVector URnWay::operator[](int32 Index) const {
    return GetVertex(Index);
}

TRnRef_T<URnWay> URnWay::ReversedWay() const {
    return RnNew<URnWay>(LineString, !IsReversed, !IsReverseNormal);
}

void URnWay::Reverse(bool KeepNormalDir) {
    IsReversed = !IsReversed;
    if (KeepNormalDir) {
        IsReverseNormal = !IsReverseNormal;
    }
}

int32 URnWay::ToRawIndex(int32 Index, bool AllowMinus) const {
    if (AllowMinus && Index < 0) {
        Index = Count() + Index;
    }
    return IsReversed ? Count() - 1 - Index : Index;
}

int32 URnWay::SwitchIndex(int32 Index) const {
    return IsReversed ? Count() - 1 - Index : Index;
}

float URnWay::SwitchIndex(float Index) const {
    return IsReversed ? Count() - 1 - Index : Index;
}

float FRnWayEx::CalcLengthOrDefault(const URnWay* Self)
{
    if (!Self)
        return 0.f;
    return Self->CalcLength();
}

bool FRnWayEx::IsValidWayOrDefault(const URnWay* Self)
{
    return Self && Self->IsValid();
}

// Add implementations:
TArray<FLineSegment2D> URnWay::GetEdges2D() const {
    TArray<FLineSegment2D> Edges;
    for (int32 i = 0; i < Count() - 1; i++) {
        Edges.Add(FLineSegment3D(GetPoint(i)->Vertex, GetPoint(i+1)->Vertex).To2D(FPLATEAURnDef::Plane));
    }
    return Edges;
}

int32 URnWay::FindPoint(const TRnRef_T<URnPoint>& Point) const {
    return LineString->GetPoints().IndexOfByPredicate([&](const TRnRef_T<URnPoint>& P) { return P == Point; });
}

int32 URnWay::FindPointIndex(const TRnRef_T<URnPoint>& Point) const {
    int32 Index = LineString->GetPoints().IndexOfByKey(Point);
    return Index < 0 ? Index : SwitchIndex(Index);
}

void URnWay::GetNearestPoint(const FVector& Pos, FVector& OutNearest, float& OutPointIndex, float& OutDistance) const {
    OutNearest = FVector::ZeroVector;
    LineString->GetNearestPoint(Pos, OutNearest, OutPointIndex, OutDistance);
    OutPointIndex = SwitchIndex(OutPointIndex);
}

bool URnWay::IsValidOrDefault() const {
    return IsValid();
}

float URnWay::CalcLength() const {
    return LineString ? LineString->CalcLength() : 0.0f;
}

float URnWay::CalcLength(float StartIndex, float EndIndex) const {
    if (IsReversed) {
        return LineString->CalcLength(SwitchIndex(EndIndex), SwitchIndex(StartIndex));
    }
    return LineString->CalcLength(StartIndex, EndIndex);
}

void URnWay::AppendBack2LineString(const TRnRef_T<URnWay>& Back) {

    if (!Back)
        return;
    // 自己代入は禁止
    if (IsSameLineReference(Back))
        return;

    if (IsReversed) {
        for (const auto& P : Back->GetPoints()) {
            LineString->AddPointFrontOrSkip(P, 0.f, 0.f, 0.f);
        }
    }
    else {
        for (const auto& P : Back->GetPoints()) {
            LineString->AddPointOrSkip(P, 0.f, 0.f, 0.f);
        }
    }
}

void URnWay::AppendFront2LineString(const TRnRef_T<URnWay>& Front) {
    if (!Front) 
        return;

    // 自己代入は禁止
    if (IsSameLineReference(Front))
        return;
    if (IsReversed) {
        for (int32 i = 0; i < Front->Count(); ++i) {
            LineString->AddPointOrSkip(Front->GetPoint(Front->Count() - 1 - i), 0.f, 0.f, 0.f);
        }
    }
    else {
        for (int32 i = 0; i < Front->Count(); ++i) {
            LineString->AddPointFrontOrSkip(Front->GetPoint(Front->Count() - 1 - i), 0.f, 0.f, 0.f);
        }
    }
}

bool URnWay::IsOutSide(const FVector& V, FVector& OutNearest, float& OutDistance) const {
    float PointIndex;
    GetNearestPoint(V, OutNearest, PointIndex, OutDistance);

    if (!IsValidOrDefault()) return false;

    int32 Start = FMath::Clamp(static_cast<int32>(PointIndex), 0, Count() - 2);
    int32 End = FMath::Clamp(FMath::CeilToInt(PointIndex - 1), 0, Count() - 2);

    TSet<int32> IndexSet = { Start, End };
    FVector Delta = V - OutNearest;
    for(auto I : IndexSet)
    {
        if (FVector2D::DotProduct( FPLATEAURnDef::To2D(GetEdgeNormal(I)), FPLATEAURnDef::To2D(Delta)) >= 0.0f)
            return true;
    }
    return false;
}

void URnWay::MoveLerpAlongNormal(const FVector& StartOffset, const FVector& EndOffset) {
    if (!IsValid()) return;

    if (Count() == 2) {
        GetPoint(0)->Vertex += StartOffset;
        GetPoint(1)->Vertex += EndOffset;
        return;
    }

    TArray<FVector> PointOffsets;
    PointOffsets.SetNum(Count());
    PointOffsets[0] = StartOffset;
    PointOffsets.Last() = EndOffset;

    float TotalLength = CalcLength();
    float CurrentLength = 0.0f;

    for (int32 i = 1; i < Count() - 1; ++i) {
        CurrentLength += ((*this)[i] - (*this)[i - 1]).Size();
        float T = CurrentLength / TotalLength;

        FVector Normal = GetVertexNormal(i);
        float StartLen = StartOffset.Size();
        float EndLen = EndOffset.Size();
        float LerpLen = FMath::Lerp(StartLen, EndLen, T);

        PointOffsets[i] = Normal * LerpLen;
    }

    for (int32 i = 0; i < Count(); ++i) {
        GetPoint(i)->Vertex += PointOffsets[i];
    }
}

FVector URnWay::GetAdvancedPointFromFront(float Offset, int32& OutStartIndex, int32& OutEndIndex) const {
    return GetAdvancedPoint(Offset, false, OutStartIndex, OutEndIndex);
}

FVector URnWay::GetAdvancedPointFromBack(float Offset, int32& OutStartIndex, int32& OutEndIndex) const {
    return GetAdvancedPoint(Offset, true, OutStartIndex, OutEndIndex);
}

FVector URnWay::GetAdvancedPoint(float Offset, bool Reverse, int32& OutStartIndex, int32& OutEndIndex) const {
    const auto Ret = LineString->GetAdvancedPoint(Offset, IsReversed != Reverse, OutStartIndex, OutEndIndex);
    OutStartIndex = SwitchIndex(OutStartIndex);
    OutEndIndex = SwitchIndex(OutEndIndex);
    return Ret;
}

FVector URnWay::GetAdvancedPoint(float Offset, bool Reverse) const {
    int32 StartIndex, EndIndex;
    return GetAdvancedPoint(Offset, Reverse, StartIndex, EndIndex);
}

float URnWay::GetDistance2D(const TRnRef_T<URnWay>& Other, EAxisPlane Plane) const {
    return LineString ? LineString->GetDistance2D(Other->LineString, Plane) : MAX_FLT;
}

FVector URnWay::GetEdgeNormal(int32 StartVertexIndex) const {
    int32 Index = ToRawIndex(StartVertexIndex);
    if (IsReversed) {
        Index -= 1;
    }
    FVector Normal = LineString->GetEdgeNormal(Index);
    if (IsReversed != IsReverseNormal) {
        Normal *= -1.0f;
    }
    return Normal;
}

FVector URnWay::GetVertexNormal(int32 VertexIndex) const {
    if (Count() <= 1) {
        return FVector::ZeroVector;
    }

    FVector Normal = LineString->GetVertexNormal(ToRawIndex(VertexIndex));
    if (IsReversed != IsReverseNormal) {
        Normal *= -1.0f;
    }
    return Normal;
}

// Add implementations:
FVector URnWay::GetLerpPoint(float P) const {
    FVector MidPoint;
    GetLerpPoint(P, MidPoint);
    return MidPoint;
}

float URnWay::GetLerpPoint(float P, FVector& OutMidPoint) const {
    float TotalLength = CalcLength();
    float TargetLength = TotalLength * P;
    float CurrentLength = 0.0f;

    for (int32 i = 0; i < Count() - 1; ++i) {
        FVector Start = (*this)[i];
        FVector End = (*this)[i + 1];
        float SegmentLength = (End - Start).Size();

        if (CurrentLength + SegmentLength >= TargetLength) {
            float T = (TargetLength - CurrentLength) / SegmentLength;
            OutMidPoint = FMath::Lerp(Start, End, T);
            return static_cast<float>(i) + T;
        }
        CurrentLength += SegmentLength;
    }

    OutMidPoint = (*this)[Count() - 1];
    return static_cast<float>(Count() - 1);
}

void URnWay::MoveAlongNormal(float Offset) {
    if (!IsValid()) return;

    // Special handling for 2 vertices
    if (Count() == 2) {
        FVector Normal = GetEdgeNormal(0);
        for (auto Point : GetPoints()) {
            Point->Vertex += Normal * Offset;
        }
        return;
    }

    int32 Index = 0;
    TArray<FVector> EdgeNormals = { GetEdgeNormal(0), GetEdgeNormal(FMath::Min(Count() - 1, 1)) };
    TArray<FVector> VertexNormals = { GetVertexNormal(0), GetVertexNormal(1) };
    float Delta = Offset;

    for (int32 i = 0; i < Count(); ++i) {
        FVector En0 = EdgeNormals[Index];
        FVector En1 = EdgeNormals[(Index + 1) & 1];
        FVector Vn = VertexNormals[Index];

        float M = FVector::DotProduct(Vn, En0);
        float D = Delta;
        bool bIsZero = FMath::Abs(M) < 1e-5f;
        if (!bIsZero) {
            D /= M;
        }

        FVector O = Vn * D;
        if (i < Count() - 1) {
            VertexNormals[Index] = GetVertexNormal(i + 1);
            EdgeNormals[Index] = GetEdgeNormal(FMath::Min(Count() - 2, i + 1));
            Index = (Index + 1) & 1;
        }

        GetPoint(i)->Vertex += O;
        Delta = D * FVector::DotProduct(Vn, En1);
    }
}

void URnWay::Move(const FVector& Offset) {
    for (int32 i = 0; i < Count(); ++i) {
        GetPoint(i)->Vertex += Offset;
    }
}

TRnRef_T<URnWay> URnWay::Clone(bool CloneVertex) const
{
    return Create(LineString->Clone(CloneVertex), IsReversed, IsReverseNormal);
}

TRnRef_T<URnWay> URnWay::ShallowClone() const
{
    return Create(LineString, IsReversed, IsReverseNormal);
}

bool URnWay::IsSameLineReference(const URnWay* Other) const {
    if (!Other) return false;
    return LineString == Other->LineString;
}

bool URnWay::IsSameLineSequence(const URnWay* Other) const {
    if (!Other) return false;
    if (!LineString || !Other->LineString) return false;
    if (Count() != Other->Count()) return false;

    for (int i = 0; i < Count(); ++i) 
    {
        constexpr float Threshold = 0.1f;
        const auto P1 = GetPoint(i)->Vertex;
        const auto P2 = Other->GetPoint(i)->Vertex;
        if (FMath::Abs(P1.X - P2.X) > Threshold) return false;
        if (FMath::Abs(P1.Y - P2.Y) > Threshold) return false;
        if (FMath::Abs(P1.Z - P2.Z) > Threshold) return false;
    }
    return true;
}

FVector URnWay::PositionAtDistance(float Distance, bool EndSide) const
{
    float Len = 0.0f;
    int32 Index = EndSide ? Count() - 1 : 0;
    FVector Pos = GetPoint(Index)->Vertex;

    while (Len < Distance) // オフセットの分だけ線上を動かします。
    {
        Index += EndSide ? -1 : 1;
        if (Index < 0 || Index >= Count()) break;

        FVector NextPos = GetPoint(Index)->Vertex;
        float LenDiff = FVector::Dist(NextPos, Pos);
        if (Len + LenDiff >= Distance)
        {
            float T = (Len + LenDiff - Distance) / LenDiff; // オーバーした割合
            return FMath::Lerp(Pos, NextPos, 1.0f - T);
        }

        Pos = NextPos;
        Len += LenDiff;
    }

    return Pos;
}

TArray<TRnRef_T<URnWay>> URnWay::Split(int32 Num, bool InsertNewPoint, TFunction<float(int32)> RateSelector)
{
    auto Selector = RateSelector;
    if(IsReversed && RateSelector)
        Selector = [RateSelector, Num](int32 Index) { return RateSelector(Num - 1 - Index); };

    auto LineStrings = LineString->Split(Num, InsertNewPoint, Selector);

    TArray<TRnRef_T<URnWay>> Result;
    for (auto& Ls : LineStrings) {
        Result.Add(RnNew<URnWay>(Ls, IsReversed, IsReverseNormal));
    }
    if(IsReversed)
        Algo::Reverse(Result);
    return Result;
}

TRnRef_T<URnWay> URnWay::Create(const TRnRef_T<URnLineString>& InLineString, bool bInIsReversed, bool bInIsReverseNormal)
{
    return RnNew<URnWay>(InLineString, bInIsReversed, bInIsReverseNormal);
}

TRnRef_T<URnWay> URnWay::CreateMergedWay(URnWay* A, URnWay* B, bool RemoveDuplicate)
{
    TArray<URnPoint*> Points;
    if(A)
    {
        for (auto P : A->GetPoints())
            Points.Add(P);
    }

    if(B)
    {
        for (auto P : B->GetPoints())
            Points.Add(P);
    }
    auto Ls = URnLineString::Create(Points, RemoveDuplicate);
    return Create(Ls, false, false);
}

bool FRnWayEx::TryMergePointsToLineString(URnWay* Self, URnWay* Src, float PointDistanceTolerance) {
    if (Self->GetPoint(0)->IsSamePoint(Src->GetPoint(0), PointDistanceTolerance)) {
        Self->AppendFront2LineString(Src->ReversedWay());
    }
    else if (Self->GetPoint(0)->IsSamePoint(Src->GetPoint(-1), PointDistanceTolerance)) {
        Self->AppendFront2LineString(Src);
    }
    else if (Self->GetPoint(-1)->IsSamePoint(Src->GetPoint(0), PointDistanceTolerance)) {
        Self->AppendBack2LineString(Src);
    }
    else if (Self->GetPoint(-1)->IsSamePoint(Src->GetPoint(-1), PointDistanceTolerance)) {
        Self->AppendBack2LineString(Src->ReversedWay());
    }
    else {
        return false;
    }

    return true;
}

TRnRef_T<URnWay> FRnWayEx::ShallowCloneOrDefault(URnWay* Self)
{
    if (!Self)
        return nullptr;
    return Self->ShallowClone();
}
