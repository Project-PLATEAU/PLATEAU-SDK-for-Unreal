// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnSideWalk.h"
#include "RoadNetwork/Structure/RnWay.h"
#include "RoadNetwork/Structure/RnRoadBase.h"
#include "RoadNetwork/Structure/RnIntersection.h"
#include "RoadNetwork/Structure/RnPoint.h"
#include "RoadNetwork/Structure/RnRoad.h"
#include "RoadNetwork/Util/PLATEAUVector2DEx.h"
#include "RoadNetwork/Util/PLATEAUVectorEx.h"

URnSideWalk::URnSideWalk()
    : LaneType(EPLATEAURnSideWalkLaneType::Undefined) {
}

void URnSideWalk::Init()
{}

void URnSideWalk::Init(const TRnRef_T<URnRoadBase>& Parent, const TRnRef_T<URnWay>& InOutsideWay,
    const TRnRef_T<URnWay>& InInsideWay, const TRnRef_T<URnWay>& InStartEdgeWay, const TRnRef_T<URnWay>& InEndEdgeWay,
    EPLATEAURnSideWalkLaneType InLaneType)
{
    this->ParentRoad = Parent;
    this->OutsideWay = InOutsideWay;
    this->InsideWay = InInsideWay;
    this->StartEdgeWay = InStartEdgeWay;
    this->EndEdgeWay = InEndEdgeWay;
    this->LaneType = InLaneType;
    this->Align();
}

TRnRef_T<URnRoadBase> URnSideWalk::GetParentRoad() const
{ return RnFrom(ParentRoad); }

TArray<TRnRef_T<URnWay>> URnSideWalk::GetSideWays() const {
    TArray<TRnRef_T<URnWay>> Ways;
    if (OutsideWay) Ways.Add(OutsideWay);
    if (InsideWay) Ways.Add(InsideWay);
    return Ways;
}

TArray<TRnRef_T<URnWay>> URnSideWalk::GetEdgeWays() const {
    TArray<TRnRef_T<URnWay>> Ways;
    if (StartEdgeWay) Ways.Add(StartEdgeWay);
    if (EndEdgeWay) Ways.Add(EndEdgeWay);
    return Ways;
}

TArray<TRnRef_T<URnWay>> URnSideWalk::GetAllWays() const {
    TArray<TRnRef_T<URnWay>> Ways = GetSideWays();
    Ways.Append(GetEdgeWays());
    return Ways;
}

bool URnSideWalk::IsValid() const {
    return InsideWay && InsideWay->IsValid() && OutsideWay && OutsideWay->IsValid();
}

EPLATEAURnSideWalkWayTypeMask URnSideWalk::GetValidWayTypeMask() const {
    EPLATEAURnSideWalkWayTypeMask Mask = EPLATEAURnSideWalkWayTypeMask::None;
    if (OutsideWay) Mask |= EPLATEAURnSideWalkWayTypeMask::Outside;
    if (InsideWay) Mask |= EPLATEAURnSideWalkWayTypeMask::Inside;
    if (StartEdgeWay) Mask |= EPLATEAURnSideWalkWayTypeMask::StartEdge;
    if (EndEdgeWay) Mask |= EPLATEAURnSideWalkWayTypeMask::EndEdge;
    return Mask;
}

void URnSideWalk::SetParent(const TRnRef_T<URnRoadBase>& InParent) {
    ParentRoad = InParent;
}

void URnSideWalk::SetSideWays(const TRnRef_T<URnWay>& InOutsideWay, const TRnRef_T<URnWay>& InInsideWay) {
    OutsideWay = InOutsideWay;
    InsideWay = InInsideWay;
    Align();
}

void URnSideWalk::SetEdgeWays(const TRnRef_T<URnWay>& StartWay, const TRnRef_T<URnWay>& EndWay) {
    StartEdgeWay = StartWay;
    EndEdgeWay = EndWay;
    Align();
}

void URnSideWalk::SetStartEdgeWay(const TRnRef_T<URnWay>& StartWay) {
    StartEdgeWay = StartWay;
}

void URnSideWalk::SetEndEdgeWay(const TRnRef_T<URnWay>& EndWay) {
    EndEdgeWay = EndWay;
}

void URnSideWalk::ReverseLaneType() {
    if (LaneType == EPLATEAURnSideWalkLaneType::LeftLane)
        LaneType = EPLATEAURnSideWalkLaneType::RightLane;
    else if (LaneType == EPLATEAURnSideWalkLaneType::RightLane)
        LaneType = EPLATEAURnSideWalkLaneType::LeftLane;
}

void URnSideWalk::Align() {
    auto AlignWay = [this](const TRnRef_T<URnWay>& Way) {
        if (!Way || !Way->IsValid()) return;

        if (StartEdgeWay && StartEdgeWay->IsValid()) {
            auto St = Way->GetPoint(0);
            if (StartEdgeWay->LineString->GetPoints().ContainsByPredicate(
                [&St](const TRnRef_T<URnPoint>& P) { return P->IsSamePoint(St); })) {
                return;
            }

            auto En = Way->GetPoint(-1);
            if (StartEdgeWay->LineString->GetPoints().ContainsByPredicate(
                [&En](const TRnRef_T<URnPoint>& P) { return P->IsSamePoint(En); })) {
                Way->Reverse(true);
                return;
            }
        }

        if (EndEdgeWay && EndEdgeWay->IsValid()) {
            auto En = Way->GetPoint(-1);
            if (EndEdgeWay->LineString->GetPoints().ContainsByPredicate(
                [&En](const TRnRef_T<URnPoint>& P) { return P->IsSamePoint(En); })) {
                return;
            }

            auto St = Way->GetPoint(0);
            if (EndEdgeWay->LineString->GetPoints().ContainsByPredicate(
                [&St](const TRnRef_T<URnPoint>& P) { return P->IsSamePoint(St); })) {
                Way->Reverse(true);
                return;
            }
        }
        };

    AlignWay(InsideWay);
    AlignWay(OutsideWay);
}

TRnRef_T<URnSideWalk> URnSideWalk::Create(
    const TRnRef_T<URnRoadBase>& Parent,
    const TRnRef_T<URnWay>& OutsideWay,
    const TRnRef_T<URnWay>& InsideWay,
    const TRnRef_T<URnWay>& StartEdgeWay,
    const TRnRef_T<URnWay>& EndEdgeWay,
    EPLATEAURnSideWalkLaneType LaneType,
    bool AddToParent) {
    auto SideWalk = RnNew<URnSideWalk>(Parent, OutsideWay, InsideWay, StartEdgeWay, EndEdgeWay, LaneType);
    if (AddToParent && Parent) {
        Parent->AddSideWalk(SideWalk);
    }

    return SideWalk;
}

FVector URnSideWalk::GetCentralVertex() const {
    if (!this) return FVector::ZeroVector;

    TArray<FVector> Points;
    for (const auto& Way : GetSideWays()) {
        Points.Add(Way->GetLerpPoint(0.5f));
    }
    return FPLATEAUVectorEx::Centroid(Points);
}

bool URnSideWalk::IsNeighboring(const TRnRef_T<URnSideWalk>& Other) const {
    return GetAllWays().ContainsByPredicate([&Other](const TRnRef_T<URnWay>& X) {
        return Other->GetAllWays().ContainsByPredicate([&X](const TRnRef_T<URnWay>& B) {
            return X->IsSameLineReference(B);
            });
        });
}

float URnSideWalk::CalcRoadProximityScore(const TRnRef_T<URnRoadBase>& Other) const {
    if (!Other || !this) return -1.0f;

    TArray<TRnRef_T<URnWay>> TargetWays;
    if (auto Road = Other->CastToRoad()) {
        TargetWays = Road->GetMergedSideWays();
    }
    else if (auto Intersection = Other->CastToIntersection()) {
        for (const auto& Edge : Intersection->GetEdges()) {
            TargetWays.Add(Edge->GetBorder());
        }
    }

    if (TargetWays.Num() == 0) return -1.0f;

    TArray<float> Distances;
    float SumDistance = 0.0f;
    int Num = 0;
    for (const auto& Point : InsideWay->GetVertices()) {
        float MinDist = MAX_FLT;
        for (const auto& Way : TargetWays) {
            FVector Nearest;
            float PointIndex, Distance;
            Way->GetNearestPoint(Point, Nearest, PointIndex, Distance);
            MinDist = FMath::Min(MinDist, Distance);
        }
        Distances.Add(MinDist);
        SumDistance += MinDist;
        Num++;
    }
    if (Num > 0) {
        return SumDistance / Num;
    }

    return -1.f;
}

// Reverse
// StartEdgeWay と EndEdgeWay を入れ替え、Align を呼び出す
void URnSideWalk::Reverse() {
    Swap(StartEdgeWay, EndEdgeWay);
    Align();
}


// ReversedSideWalk
// 現在の情報をもとに新しい URnSideWalk インスタンスを生成し、Reverse を実行して返す
URnSideWalk* URnSideWalk::ReversedSideWalk() {
    // 親への追加は行わないようにする
    auto Copy = [](URnWay* W) {
        return W ? W->ShallowClone() : nullptr;
        };
    URnSideWalk* NewSideWalk = Create(ParentRoad.Get()
        , Copy(OutsideWay)
        , Copy(InsideWay)
        , Copy(StartEdgeWay)
        , Copy(EndEdgeWay)
        , LaneType
        , false);
    // Align 済みの状態で、反転処理を行う
    NewSideWalk->Reverse();
    return NewSideWalk;
}

// TryMergeNeighborSideWalk
// 隣接する歩道との統合を試み、統合できた場合は true を返す
bool URnSideWalk::TryMergeNeighborSideWalk(URnSideWalk* SrcSideWalk) {

    if (SrcSideWalk == nullptr) {
        return false;
    }
    // 両方の歩道について、方向合わせの処理
    Align();
    SrcSideWalk->Align();

    // IsMatch: 2 つの URnWay が同一の線分を参照しているか
    auto IsMatch = [](URnWay* A, URnWay* B) -> bool {
        return (A != nullptr && B != nullptr && A->IsSameLineReference(B));
        };

    // MergeSideWays: src 側の SideWay と dst 側の SideWay を統合するラムダ
    auto MergeSideWays = [](URnSideWalk* SrcSw, URnSideWalk* DstSw) {
        // #TODO : 現状の使い方だと問題ないが, 今後OutsideWay同士, InsideWay同士が繋がっているかの保証が無いのでポイント単位でチェックすべき
        if (SrcSw && DstSw) {
            // URnWay::CreateMergedWay は、2 つの URnWay を統合して新たな URnWay を生成する静的メソッドと想定
            DstSw->SetSideWays(
                URnWay::CreateMergedWay(SrcSw->GetOutsideWay(), DstSw->GetOutsideWay(), false),
                URnWay::CreateMergedWay(SrcSw->GetInsideWay(), DstSw->GetInsideWay(), false)
            );
            // 始点側の Edge は、src 側のものに切り替える
            DstSw->SetStartEdgeWay(SrcSw->GetStartEdgeWay());
        }
        };

    // 以下、各ケースについて、隣接部分が一致する場合の処理

    // ケース1: 自身の StartEdgeWay と SrcSideWalk の StartEdgeWay が一致
    if (IsMatch(GetStartEdgeWay(), SrcSideWalk->GetStartEdgeWay())) {
        // SrcSideWalk 側を反転して統合する
        URnSideWalk* Rev = SrcSideWalk->ReversedSideWalk();
        MergeSideWays(Rev, this);
        return true;
    }
    // ケース2: 自身の StartEdgeWay と SrcSideWalk の EndEdgeWay が一致
    else if (IsMatch(GetStartEdgeWay(), SrcSideWalk->GetEndEdgeWay())) {
        MergeSideWays(SrcSideWalk, this);
        return true;
    }
    // ケース3: 自身の EndEdgeWay と SrcSideWalk の EndEdgeWay が一致
    else if (IsMatch(GetEndEdgeWay(), SrcSideWalk->GetEndEdgeWay())) {
        // 自身を一旦反転してから統合、統合後再反転
        Reverse();
        MergeSideWays(SrcSideWalk, this);
        Reverse();
        return true;
    }
    // ケース4: 自身の EndEdgeWay と SrcSideWalk の StartEdgeWay が一致
    else if (IsMatch(GetEndEdgeWay(), SrcSideWalk->GetStartEdgeWay())) {
        Reverse();
        URnSideWalk* Rev = SrcSideWalk->ReversedSideWalk();
        MergeSideWays(Rev, this);
        Reverse();
        return true;
    }

    return false;
}

bool URnSideWalk::Check() const
{
    auto IsContinuous = [this](URnWay* A, URnWay* B) {
        if (A == nullptr || B == nullptr)
            return true;
        auto Av0 = A->GetPoint(0);
        auto Av1 = A->GetPoint(-1);

        auto Bv0 = B->GetPoint(0);
        auto Bv1 = B->GetPoint(-1);
        return (Av0 == Bv0 || Av0 == Bv1) || (Av1 == Bv0 || Av1 == Bv1);
        };
    if (IsContinuous(GetInsideWay(), GetStartEdgeWay()) == false) {
        UE_LOG(LogTemp, Error, TEXT("InsideWayとStartEdgeWayが端点で繋がっていません. %s"), *GetName());
        return false;
    }
    if (IsContinuous(GetInsideWay(), GetEndEdgeWay()) == false) {
        UE_LOG(LogTemp, Error, TEXT("InsideWayとEndEdgeWayが端点で繋がっていません. %s"), *GetName());
        return false;

    }
    if (IsContinuous(GetOutsideWay(), GetStartEdgeWay()) == false) {
        UE_LOG(LogTemp, Error, TEXT("OutsideWayとStartEdgeWayが端点で繋がっていません. %s"), *GetName());
        return false;
    }
    if (IsContinuous(GetOutsideWay(), GetEndEdgeWay()) == false) {
        UE_LOG(LogTemp, Error, TEXT("OutsideWayとEndEdgeWayが端点で繋がっていません. %s"), *GetName());
        return false;

    }
    return true;
}
