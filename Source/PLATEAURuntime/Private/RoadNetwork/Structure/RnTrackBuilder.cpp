// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/RnTrackBuilder.h"
#include "RoadNetwork/Structure/RnIntersection.h"

bool FBuildTrackOption::IsBuildTarget(URnIntersection* Intersection, URnIntersectionEdge* From, URnIntersectionEdge* To) const {
    // 交差点が無効な場合は対象外
    if (Intersection == nullptr) {
        return false;
    }


    return true;
}

FBuildTrackOption FBuildTrackOption::Default() {
    return FBuildTrackOption();
}

FBuildTrackOption FBuildTrackOption::UnBuiltTracks() {
    FBuildTrackOption Ret;
    Ret.ClearTracks = false;
    Ret.UnCreatedTrackOnly = true;
    return Ret;
}

FBuildTrackOption FBuildTrackOption::WithBorder(const TArray<URnLineString*>& Borders) {
    FBuildTrackOption Ret;
    Ret.ClearTracks = false;
    for (URnLineString* Border : Borders) {
        if (Border != nullptr) {
            Ret.TargetBorderLineStrings.Add(Border);
        }
    }
    return Ret;
}


// コンストラクタ
FRnTracksBuilder::FRnTracksBuilder() {
}

void FRnTracksBuilder::BuildTracks(URnIntersection* Intersection, const FBuildTrackOption& Option) {
    // Option が指定されていない場合は FBuildTrackOption::Default() を利用（呼び出し側で補完してもよい）
    FBuildTrackOption Op = Option;

    if (Op.ClearTracks) {
        Intersection->ClearTracks();
    }

    // borderEdgeGroups : Intersection 内のエッジグループから IsBorder が true のものを抽出
    auto BorderEdgeGroups = FRnIntersectionEx::CreateEdgeGroup(Intersection);
    // Filter : IsBorder == true
    BorderEdgeGroups = BorderEdgeGroups.FilterByPredicate([](const FRnIntersectionEx::FEdgeGroup& Eg) {
        return Eg.IsBorder();
        });

    // ループ処理
    const int32 NumBorderGroups = BorderEdgeGroups.Num();
    for (int32 StartEdgeIndex = 0; StartEdgeIndex < NumBorderGroups; ++StartEdgeIndex) 
    {
        auto& FromEg = BorderEdgeGroups[StartEdgeIndex];

        // inBoundsLeft2Right = InBoundEdges を反転順とする
        TArray<URnIntersectionEdge*> InBoundsLeft2Right = FromEg.GetInBoundEdges();
        Algo::Reverse(InBoundsLeft2Right);

        if (InBoundsLeft2Right.Num() == 0) {
            continue;
        }
        if (!FromEg.IsValid()) {
            continue;
        }

        // 出口候補リスト
        // fromEg -> toEgのTurnTypeとtoEgよりも左側に同じTurnTypeがいくつあるか
        // fromEgから出ていくTrackのTurnTypeをTrack数分の配列で事前に作成
        // 左折2レーン, 直進2レーン, 右折1レーンの場合以下のようになる
        // [Left, Left, Straight, Straight, Right]
        TArray<FOutBound> OutBoundsLeft2Rights;

        // 出口候補数は AllowSelfTrack に応じて決定
        int32 Size = Op.AllowSelfTrack ? BorderEdgeGroups.Num() : BorderEdgeGroups.Num() - 1;
        for (int32 i = 0; i < Size; i++) {
            // 輪番インデックス計算
            int32 Index = (StartEdgeIndex + i + 1) % BorderEdgeGroups.Num();
            auto& ToEg = BorderEdgeGroups[Index];
            if (!ToEg.IsValid()) {
                continue;
            }

            auto ToEgOutBounds = ToEg.GetOutBoundEdges();
            if (ToEgOutBounds.Num() == 0) {
                continue;
            }
            ERnTurnType TurnType = GetTurnType(-FromEg.GetNormal(), ToEg.GetNormal());
            for (URnIntersectionEdge* Out : ToEgOutBounds) {
                OutBoundsLeft2Rights.Add(FOutBound(TurnType, &ToEg, Out));
            }
        }

        // 出口候補が無い場合はスキップ
        if (OutBoundsLeft2Rights.Num() == 0) {
            continue;
        }
        if (InBoundsLeft2Right.Num() > OutBoundsLeft2Rights.Num()) {
            // 余る場合、最も右側に余分を割り当てる
            for (int32 i = 0; i < InBoundsLeft2Right.Num(); ++i) {
                int32 OutBoundIndex = FMath::Clamp(i, 0, OutBoundsLeft2Rights.Num() - 1);
                URnIntersectionEdge* FromNeighbor = InBoundsLeft2Right[i];
                FOutBound& OutBound = OutBoundsLeft2Rights[OutBoundIndex];
                URnTrack* Track = MakeTrack(Intersection, FromNeighbor, Op, &FromEg, OutBound);
                Intersection->TryAddOrUpdateTrack(Track);
            }
        }
        else {
            int32 InBoundIndex = 0;
            for (int32 i = 0; i < OutBoundsLeft2Rights.Num(); ++i) {
                // 必要に応じて InBoundIndex を進める
                if (i > 0 && InBoundIndex < InBoundsLeft2Right.Num() - 1) 
                {

                    // 残りの流出先の数と流入先の数が同じ場合は残りは1:1対応なので進める
                    if ((InBoundsLeft2Right.Num() - InBoundIndex) > (OutBoundsLeft2Rights.Num() - i)) {
                        ++InBoundIndex;
                    }
                    else
                    {
                        // 出力候補が直進の場合、角度がより小さいほうを採用する
                        FOutBound& ToCandidate = OutBoundsLeft2Rights[i];
                        auto ToPos = FRnIntersectionEx::GetEdgeCenter2D(ToCandidate.To);
                        if (ToCandidate.TurnType == ERnTurnType::Straight) 
                        {
                            auto Now = InBoundsLeft2Right[InBoundIndex];
                            auto Next = InBoundsLeft2Right[InBoundIndex + 1];
                            auto NowPos = FRnIntersectionEx::GetEdgeCenter2D(Now);
                            auto NextPos = FRnIntersectionEx::GetEdgeCenter2D(Next);

                            auto NowDir = -FRnIntersectionEx::GetEdgeNormal2D(Now);
                            auto NextDir = -FRnIntersectionEx::GetEdgeNormal2D(Next);

                            float NowAngle = FPLATEAUVector2DEx::Angle(NowDir, ToPos - NowPos);
                            float NextAngle = FPLATEAUVector2DEx::Angle(NextDir, ToPos - NextPos);

                            if (NowAngle > NextAngle) {
                                ++InBoundIndex;
                            }
                        }
                    }
                }

                URnIntersectionEdge* FromNeighbor = InBoundsLeft2Right[InBoundIndex];
                FOutBound& OutBound = OutBoundsLeft2Rights[i];
                URnTrack* Track = MakeTrack(Intersection, FromNeighbor, Op, &FromEg, OutBound);
                Intersection->TryAddOrUpdateTrack(Track);
            }
        }
    }
}

URnTrack* FRnTracksBuilder::MakeTrack(URnIntersection* Intersection, URnIntersectionEdge* From, const FBuildTrackOption& Option,
    FRnIntersectionEx::FEdgeGroup* FromEg
    , const FOutBound& OutBound) {
    // UEの仕様ではスプラインはいらないので無視
    return RnNew<URnTrack>(From->GetBorder(), OutBound.To->GetBorder(), nullptr, OutBound.TurnType);}


ERnTurnType FRnTracksBuilder::GetTurnType(const FVector& From, const FVector& To) {
    // Project the 3D vectors onto a 2D plane.
    FVector2D VFrom = FPLATEAURnDef::To2D(From);
    FVector2D VTo = FPLATEAURnDef::To2D(To);
    return GetTurnType(VFrom, VTo);
}

ERnTurnType FRnTracksBuilder::GetTurnType(const FVector2D& From, const FVector2D& To) {
    float ang = -FPLATEAUVector2DEx::SignedAngle(From, To) + 180.f;
    // Use angle ranges to determine the turn type.
    if (ang > 10.f && ang < 67.5f) {
        return ERnTurnType::LeftBack;
    }
    else if (ang >= 67.5f && ang < 112.5f) {
        return ERnTurnType::LeftTurn;
    }
    else if (ang >= 112.5f && ang < 157.5f) {
        return ERnTurnType::LeftFront;
    }
    else if (ang >= 157.5f && ang < 202.5f) {
        return ERnTurnType::Straight;
    }
    else if (ang >= 202.5f && ang < 247.5f) {
        return ERnTurnType::RightFront;
    }
    else if (ang >= 247.5f && ang < 292.5f) {
        return ERnTurnType::RightTurn;
    }
    else if (ang >= 292.5f && ang < 337.5f) {
        return ERnTurnType::RightBack;
    }
    else {
        return ERnTurnType::UTurn;
    }
}
