// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/GeoGraph/GeoGraphEx.h"

#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/Util/PLATEAUIntVectorEx.h"
#include "RoadNetwork/Util/PLATEAURnLinq.h"
#include "RoadNetwork/Util/PLATEAUVector2DEx.h"
#include "Containers/Queue.h"

TArray<FVector> FGeoGraphEx::GetInnerLerpSegments(
    const TArray<FVector>& LeftVertices,
    const TArray<FVector>& RightVertices,
    EAxisPlane Plane,
    float P,
    float CheckMeter
) {
    P = FMath::Clamp(P, 0.f, 1.f);

    // 線分になっていない場合は無視する
    if (LeftVertices.Num() < 2 || RightVertices.Num() < 2)
        return TArray<FVector>();

    // それぞれ直線の場合は高速化の特別処理入れる
    if (LeftVertices.Num() == 2 && RightVertices.Num() == 2) {
        return TArray<FVector> {
            FMath::Lerp(LeftVertices[0], RightVertices[0], P),
                FMath::Lerp(LeftVertices[1], RightVertices[1], P)
        };
    }

    auto leftEdges = GetEdgeSegments(LeftVertices, false);
    auto rightEdges = GetEdgeSegments(RightVertices, false);

    TArray<float> indices;

    auto CheckLength = FMath::Max(CheckMeter, 1.f) * FPLATEAURnDef::Meter2Unit;

    // 左の線分の頂点をイベントポイントとして登録
    // ただし、線分がCheckLength以上の場合はCheckLength間隔でイベントポイントを追加する
    for (auto i = 0; i < LeftVertices.Num(); i++) {
        indices.Add(i);
        // 最後の頂点はチェックしない
        if (i == LeftVertices.Num() - 1)
            break;
        const auto& p0 = LeftVertices[i];
        const auto& p1 = LeftVertices[i + 1];
        auto sqrLen = (p1 - p0).SizeSquared();
        if (sqrLen > CheckLength * CheckLength) {
            auto len = FMath::Sqrt(sqrLen);
            auto num = len / CheckLength;
            for (auto j = 0; j < num - 1; ++j)
                indices.Add(i + (j + 1.f) / num);
        }
    }

    auto IsInInnerSide = [&](
        TOptional<FLineSegment3D> BaseEdge
        , const FLineSegment3D& TargetSeg
        , const bool InSideIsLeftSide
        , const bool bIsPrev) -> bool {
        if (!BaseEdge)
            return true;
        const auto BaseEdge2D = BaseEdge->To2D(Plane);
        const auto TargetSeg2D = TargetSeg.To2D(Plane);
        const auto Sign = BaseEdge2D.Sign(TargetSeg2D.GetEnd());
        if (Sign == 1 && InSideIsLeftSide)
            return true;

        if (Sign == -1 && !InSideIsLeftSide)
            return true;

        // 同一線分上にある場合
        // 同じ方向を向いているか逆方向を向いているかで判定する
        if (Sign == 0) 
        {
            auto Dot = BaseEdge2D.GetDirection().Dot(TargetSeg2D.GetDirection());
            if (bIsPrev)
                Dot = -Dot;
            return Dot < 0.f;
        }

        return true;
        };

    auto CheckCollision = [&](FVector a, FVector b, const TArray<FLineSegment3D>& edges, float indexF) -> bool {
        auto a2 = FAxisPlaneEx::ToVector2D(a, Plane);
        auto b2 = FAxisPlaneEx::ToVector2D(b, Plane);
        auto index = (int)indexF;
        auto f = indexF - index;
        auto prevIndex = f > 0 ? index : index - 1;
        for (auto i = 0; i < edges.Num(); ++i) {
            if (i == index || i == prevIndex)
                continue;
            const auto& e = edges[i];
            auto e2 = e.To2D(Plane);
            FVector2D inter;
            if (e2.TrySegmentIntersection( FLineSegment2D(a2, b2), inter))
                return true;
        }

        return false;
        };

    for (auto i = 0; i < RightVertices.Num(); ++i) {
        const auto& pos = RightVertices[i];

        TOptional<FLineSegment3D> prevEdge = i > 0 ? rightEdges[i - 1] : (TOptional<FLineSegment3D>)NullOpt;
        TOptional<FLineSegment3D> nextEdge = i < rightEdges.Num() ? rightEdges[i] : (TOptional<FLineSegment3D>)NullOpt;

        float minIndexF = -1;
        float minDist = FLT_MAX;
        for (auto edgeIndex = 0; edgeIndex < leftEdges.Num(); ++edgeIndex) {
            const auto& e = leftEdges[edgeIndex];
            float distanceFromStart;
            auto nearPos = e.GetNearestPoint(pos, distanceFromStart);
            auto d = nearPos - pos;

            auto dist = d.Length();
            if (dist >= minDist)
                continue;
            auto Seg = FLineSegment3D(pos, nearPos);
            if (IsInInnerSide(prevEdge, Seg, true, true) == false)
                continue;
            if (IsInInnerSide(nextEdge, Seg, true, false) == false)
                continue;
            if (CheckCollision(pos, nearPos, rightEdges, i))
                continue;
            minDist = dist;
            minIndexF = edgeIndex + distanceFromStart / e.GetMagnitude();
        }

        if (minIndexF < 0)
            continue;
        indices.Add(minIndexF);
    }

    indices.Sort();

    auto searchRightIndex = 0;
    TArray<FVector> ret;
    ret.Reserve(indices.Num());
    for(auto indexF : indices) {
        auto i = FMath::Clamp((int)indexF, 0, leftEdges.Num() - 1);
        const auto& e1 = leftEdges[i];
        auto f = FMath::Clamp(indexF - i, 0.f, 1.f);
        auto pos = FMath::Lerp(e1.GetStart(), e1.GetEnd(), f);

        TOptional<FLineSegment3D> prevEdge = NullOpt;
        TOptional<FLineSegment3D> nextEdge = NullOpt;
        if (f > 0.f && f < 1.f) {
            prevEdge = FLineSegment3D(e1.GetStart(), pos);
            nextEdge = FLineSegment3D(pos, e1.GetEnd());
        }
        else {
            if (i > 0)
                prevEdge = leftEdges[i - 1];
            if (i < leftEdges.Num())
                nextEdge = leftEdges[i];
        }

        float minIndexF = -1;
        float minDist = FLT_MAX;
        FVector minPos = FVector::Zero();
        for (auto edgeIndex = searchRightIndex; edgeIndex < rightEdges.Num(); ++edgeIndex) {
            const auto& e2 = rightEdges[edgeIndex];
            float t;
            auto nearPos = e2.GetNearestPoint(pos, t);
            auto d = nearPos - pos;
            auto dist = d.Length();

            if (dist >= minDist)
                continue;
            auto Seg = FLineSegment3D(pos, nearPos);
            if (IsInInnerSide(prevEdge, Seg, false, true) == false)
                continue;
            if (IsInInnerSide(nextEdge, Seg, false, false) == false)
                continue;
            if (CheckCollision(pos, nearPos, leftEdges, indexF))
                continue;
            minDist = dist;
            minIndexF = edgeIndex + t;
            minPos = nearPos;
        }

        if (minIndexF < 0)
            continue;
        // #TODO : やってみたらおかしくなったので毎回最初から探す
        // 高速化のため. 戻ることは無いはずなので見つかったindexから探索でよいはず
        //searchRightIndex = (int)minIndexF;

        ret.Add(FMath::Lerp(pos, minPos, P));
    }

    return ret;
}

TArray<FIntVector> FGeoGraphEx::GetNeighborDistance3D(int32 D) {
    const int32 W = 2 * D + 1;
    const int32 Size = W * W * W;
    const int32 Half = D;
    const int32 W2 = W * W;

    TArray<FIntVector> Result;
    Result.SetNum(Size);

    for (int32 i = 0; i < Size; i++) {
        const int32 DX = i % W - Half;
        const int32 DY = (i / W) % W - Half;
        const int32 DZ = (i / W2) - Half;
        Result[i] = FIntVector(DX, DY, DZ);
    }

    return Result;
}

TArray<FIntPoint> FGeoGraphEx::GetNeighborDistance2D(int32 D) {
    const int32 W = 2 * D + 1;
    const int32 Size = W * W;
    const int32 Half = D;

    TArray<FIntPoint> Result;
    Result.SetNum(Size);

    for (int32 i = 0; i < Size; i++) {
        const int32 DX = i % W - Half;
        const int32 DY = (i / W) % W - Half;
        Result[i] = FIntPoint(DX, DY);
    }

    return Result;
}

bool FGeoGraphEx::IsCollinear(const FVector& A, const FVector& B, const FVector& C, float DegEpsilon, float MidPointTolerance) {
    if (DegEpsilon >= 0.0f) {
        const float Deg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct((A - B).GetSafeNormal(), (C - B).GetSafeNormal())));
        if (FMath::Abs(180.0f - Deg) <= DegEpsilon)
            return true;
    }

    if (MidPointTolerance >= 0.0f) {
        const FVector Dir = (C - A).GetSafeNormal();
        const FVector Pos = A + Dir * FVector::DotProduct(B - A, Dir);
        return MidPointTolerance > 0.0f && (B - Pos).SizeSquared() <= MidPointTolerance * MidPointTolerance;
    }

    return false;
}

TMap<FVector, FVector> FGeoGraphEx::MergeVertices(const TArray<FVector>& Vertices, float CellSize, int32 MergeCellLength) {
    const float Len = CellSize;
    const int32 MergeLen = MergeCellLength;

    TMap<FIntVector, TArray<FVector>> Cells;
    FIntVector Min(INT_MAX, INT_MAX, INT_MAX);

    for (const FVector& V : Vertices) {
        FIntVector C = FIntVector(
            FMath::FloorToInt(V.X / Len),
            FMath::FloorToInt(V.Y / Len),
            FMath::FloorToInt(V.Z / Len)
        );

        if (!Cells.Contains(C)) {
            Cells.Add(C, TArray<FVector>());
        }
        Cells[C].Add(V);
        Min.X = FMath::Min(Min.X, C.X);
        Min.Y = FMath::Min(Min.Y, C.Y);
        Min.Z = FMath::Min(Min.Z, C.Z);
    }

    TArray<FIntVector> Keys;
    Cells.GetKeys(Keys);
    Keys.Sort([](const FIntVector& A, const FIntVector& B) {
        auto d = A.X - B.X;
        if (d != 0) return d < 0;
        d = A.Y - B.Y;
        if (d != 0) return d < 0;
        return A.Z < B.Z;
        });
    TMap<FVector, FVector> Result;
    const auto Del1 = GetNeighborDistance3D(1);

    for(auto& K : Keys)
    {
        if (Cells.Contains(K) == false) {
            continue;
        }

        TQueue<FIntVector> Queue;
        Queue.Enqueue(K);
        while(Queue.IsEmpty() == false)
        {
            FIntVector C;
            Queue.Dequeue(C);

            for(auto D : Del1)
            {
                auto N = C + D;
                if (N == K)
                    continue;
                if (Cells.Contains(N) == false) {
                    continue;
                }
                
                if (FPLATEAUIntVectorEx::Sum(FPLATEAUIntVectorEx::Abs(K - N)) > MergeLen)
                    continue;
                for (auto& V : Cells[N])
                    Cells[K].Add(V);
                Cells.Remove(N);
                Queue.Enqueue(N);
            }
        }
    }

    for (auto& Cell : Cells) {
        if (Cell.Value.Num() == 1) continue;

        FVector Center = FVector::ZeroVector;
        for (const auto& V : Cell.Value) {
            Center += V;
        }
        Center /= Cell.Value.Num();
        for (const auto& V : Cell.Value) {
            Result.Add(V, Center);
        }
    }

    return Result;
}

bool FGeoGraphEx::IsInInnerSide(const TOptional<FLineSegment3D>& Edge, const FVector& Direction, bool bReverse, bool bIsPrev) {
    if (!Edge.IsSet()) {
        return true;
    }

    const FVector2D EdgeDir2D = FAxisPlaneEx::GetTangent(Edge.GetValue().GetDirection(), EAxisPlane::Xy);
    const FVector2D Dir2D = FAxisPlaneEx::GetTangent(Direction, EAxisPlane::Xy);

    float Cross = EdgeDir2D.X * Dir2D.Y - EdgeDir2D.Y * Dir2D.X;
    if (!bReverse) {
        Cross = -Cross;
    }

    if (Cross > 0.0f) {
        return false;
    }

    if (FMath::IsNearlyZero(Cross)) {
        float Dot = EdgeDir2D.X * Dir2D.X + EdgeDir2D.Y * Dir2D.Y;
        if (bIsPrev) {
            Dot = -Dot;
        }
        return Dot < 0.0f;
    }

    return true;
}

bool FGeoGraphEx::CheckCollision(const FVector& A, const FVector& B, const TArray<FLineSegment3D>& Edges, float IndexF) {
    const FVector2D A2 = FAxisPlaneEx::GetTangent(A, EAxisPlane::Xy);
    const FVector2D B2 = FAxisPlaneEx::GetTangent(B, EAxisPlane::Xy);

    const int32 Index = FMath::FloorToInt(IndexF);
    const float F = IndexF - Index;
    const int32 PrevIndex = F > 0.0f ? Index : Index - 1;

    for (int32 i = 0; i < Edges.Num(); ++i) {
        if (i == Index || i == PrevIndex) {
            continue;
        }

        const auto& Edge = Edges[i];
        const FVector2D EdgeStart = FAxisPlaneEx::GetTangent(Edge.GetStart(), EAxisPlane::Xy);
        const FVector2D EdgeEnd = FAxisPlaneEx::GetTangent(Edge.GetEnd(), EAxisPlane::Xy);

        // Line segment intersection check
        const FVector2D D = EdgeEnd - EdgeStart;
        const FVector2D R = B2 - A2;
        const float DenominatorU = (D.X * R.Y - D.Y * R.X);

        if (!FMath::IsNearlyZero(DenominatorU)) {
            const FVector2D S = A2 - EdgeStart;
            const float U = (R.X * S.Y - R.Y * S.X) / DenominatorU;
            const float T = (D.X * S.Y - D.Y * S.X) / DenominatorU;

            if (U >= 0.0f && U <= 1.0f && T >= 0.0f && T <= 1.0f) {
                return true;
            }
        }
    }

    return false;
}