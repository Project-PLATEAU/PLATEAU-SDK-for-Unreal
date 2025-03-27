// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include <optional>

#include "PLATEAURnLinq.h"
#include "RoadNetwork/GeoGraph/GeoGraph2d.h"
#include "RoadNetwork/GeoGraph/LineSegment3D.h"
#include "RoadNetwork/GeoGraph/LineUtil.h"
#include "Engine/EngineTypes.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

class URnRoad;
class UPLATEAUCityObjectGroup;
class URnIntersection;
class URnLineString;
class URnWay;
class URnPoint;
class URnLane;




struct FPLATEAURnEx
{
    class FLineCrossPointResult {
    public:
        class FTargetLineInfo {
        public:
            TRnRef_T<URnLineString> LineString;
            TArray<TTuple<float, FVector>> Intersections;
        };

        TArray<FTargetLineInfo> TargetLines;
        FLineSegment3D LineSegment;
    };


public:

    static TArray<TRnRef_T<UPLATEAUCityObjectGroup>> GetSceneSelectedCityObjectGroups();

    template<typename T>
    static void Replace(TArray<T>& Self, T Before, T After);

    static void ReplaceLane(TArray<TRnRef_T<URnLane>>& Self, TRnRef_T<URnLane> Before, TRnRef_T<URnLane> After);

    static TRnRef_T<URnLineString> CreateInnerLerpLineString(
        const TArray<FVector>& LeftVertices,
        const TArray<FVector>& RightVertices,
        TRnRef_T<URnPoint> Start,
        TRnRef_T<URnPoint> End,
        TRnRef_T<URnWay> StartBorder,
        TRnRef_T<URnWay> EndBorder,
        float T,
        float PointSkipDistance = 1e-3f);

    static FLineCrossPointResult GetLineIntersections(
        const FLineSegment3D& LineSegment,
        const TArray<TRnRef_T<URnWay>>& Ways);

    struct FBorderEdgesResult {
        bool bSuccess;
        TArray<FVector2D> ReducedVertices;
        TArray<int32> ReducedBorderVertexIndices;
        TArray<int32> BorderVertexIndices;
        TArray<FVector2D> SrcVertices;

        TArray<FVector2D> GetReducedBorderVertices() const;
        TArray<FVector2D> GetBorderVertices() const;
    };

    static FVector2D GetEdgeNormal(const FVector2D& Start, const FVector2D& End) {
        const FVector2D Delta = End - Start;
        return FVector2D(-Delta.Y, Delta.X).GetSafeNormal();
    }

    static FVector GetEdgeNormal(const FVector& Start, const FVector& End) {
        const FVector Delta = End - Start;
        return (-FVector::CrossProduct(FVector::UpVector, Delta)).GetSafeNormal();
    }

    static TArray<int32> FindCollinearRange(
        int32 EdgeBaseIndex,
        const TArray<FLineSegment2D>& Edges,
        float ToleranceAngleDegForMidEdge = 20.f);

    template<typename T>
    static TArray<T> Reduction(const TArray<T>& SrcVertices,
        TFunction<FVector2D(const T&)> ToVec2,
        TFunction<T(const FVector2D&)> Creator,
        TFunction<bool(const TArray<T>&, int32)> CheckStop,
        int32 Nest = 0);

    static FBorderEdgesResult FindBorderEdges(const TArray<FVector2D>& Vertices,
                                              float ToleranceAngleDegForMidEdge = 20.f,
                                              float SkipAngleDeg = 20.f);

    static FLineCrossPointResult GetLaneCrossPoints(TRnRef_T<URnRoad> Road,  const FLineSegment3D& LineSegment);

public:
    // ParentにChildを追加する
    // ActorにAddInstanceComponentをして, ChildをParentにアタッチする
    static void AddChildInstanceComponent(AActor* Actor, USceneComponent* Parent, USceneComponent* Child, FAttachmentTransformRules TransformRule = FAttachmentTransformRules::KeepRelativeTransform);

    // SelfのT型の子コンポーネントを取得する
    // bIncludeAllDescendants : 子孫も含めるか
    // bIncludeSelf : 自分自身も含めるか
    template<typename T>
    static TArray<T*> GetChildrenComponents(USceneComponent* Self, bool bIncludeAllDescendants = true, bool bIncludeSelf = false)
    {
        TArray<T*> Children;
        if (bIncludeSelf && Cast<T>(Self)) {
            Children.Add(Cast<T>(Self));
        }
        TArray<USceneComponent*> Components;
        Self->GetChildrenComponents(bIncludeAllDescendants, Components);
        for (auto Child : Components) {
            if (auto C = Cast<T>(Child)) {
                Children.Add(C);
            }
        }
        return Children;
    }

    template<typename T>
    static T* GetOrCreateInstanceComponentWithName(AActor* Actor, USceneComponent* Root, const FName& Name)
    {
        if (!Actor) 
            return nullptr;
        auto Component = Actor->GetComponentByClass<T>();

        if (!Component) {
            auto UniqueName = MakeUniqueObjectName(Actor, T::StaticClass(), Name);
            Component = NewObject<T>(Actor, UniqueName);
            AddChildInstanceComponent(Actor, Actor->GetRootComponent(), Component);
        }
        return Cast<T>(Component);
    }


    template<typename TKey, typename TEdge>
    struct FKeyEdgeGroup
    {
        TKey Key;
        TArray<TEdge> Edges;
        FKeyEdgeGroup(){}
        FKeyEdgeGroup(TKey InKey):Key(InKey){}
    };

    // OutlineEdgesで表現される多角形の各辺をKeySelectorでグループ化
   // ただし, 飛び飛びの辺はグループ化されない
   // 例: OutlineEdges = {A, A, B, B, A, A, C, C, A}
   //   => {B, (2,3)}, {A, (4,5)}, {C, (6,7)}, {A, (8,0, 1)}のようにグループ化される
    template<typename TKey, typename TEdge>
    static TArray<FKeyEdgeGroup<TKey, TEdge>> GroupByOutlineEdges(
        const TArray<TEdge>& OutlineEdges
        , TFunction<TKey(TEdge)> KeySelector)
    {
        TArray<FKeyEdgeGroup<TKey, TEdge>> Ret;
        for (auto i = 0; i < OutlineEdges.Num(); ++i) 
        {
            auto&& e = OutlineEdges[i];
            auto key = KeySelector(e);
            if (i == 0 || Ret.Last().Key != key) 
            {
                Ret.Add(FKeyEdgeGroup<TKey, TEdge>(key));
            }
            Ret.Last().Edges.Add(e);
        }

        // Waysの最初と最後が同じKeyの場合は結合
        if (Ret.Num() > 1 && Ret[0].Key == Ret.Last().Key) {
            for (auto&& E : Ret[0].Edges)
                Ret.Last().Edges.Add(E);
            Ret.RemoveAt(0);
        }

        return Ret;
    }
};

template<typename T>
void FPLATEAURnEx::Replace(TArray<T>& Self, T Before, T After) {
    for (int32 i = 0; i < Self.Num(); i++) {
        if (Self[i] == Before) {
            Self[i] = After;
        }
    }
}

template <typename T>
TArray<T> FPLATEAURnEx::Reduction(const TArray<T>& SrcVertices, TFunction<FVector2D(const T&)> ToVec2,
    TFunction<T(const FVector2D&)> Creator, TFunction<bool(const TArray<T>&, int32)> CheckStop, int32 Nest)
{
    struct FMinLenInfo {
        float MinLen;
        int32 Index;
        float Offset;
        FVector2D Intersection;
    };

    TArray<T> Vertices = SrcVertices;
    const bool bIsClockwise = FGeoGraph2D::IsClockwise(Vertices);
    const float NormalSign = bIsClockwise ? -1.f : 1.f;

    auto GetPos = [&](int32 X) -> FVector2D {
        return ToVec2(Vertices[(X + Vertices.Num()) % Vertices.Num()]);
    };

    auto GetEdgeNormal = [&](int32 X) -> FVector2D {
        X += Vertices.Num();
        const FVector2D Start = GetPos(X);
        const FVector2D End = GetPos(X + 1);
        const FVector2D Delta = End - Start;
        return NormalSign * FVector2D(-Delta.Y, Delta.X).GetSafeNormal();
    };

    auto GetVertexNormal = [&](int32 X) -> FVector2D {
        return (GetEdgeNormal(X) + GetEdgeNormal(X - 1)).GetSafeNormal();
    };

    auto Move = [&](float Delta) -> TArray<T> {
        TArray<T> Points;
        for (int32 i = 0; i < Vertices.Num(); ++i) {
            const FVector2D E0 = GetEdgeNormal(i);
            const FVector2D E1 = GetEdgeNormal(i - 1);
            const float DotProduct = FVector2D::DotProduct(E0, E1);
            const FVector2D DD = E0 + E1 * (1.f - DotProduct);
            Points.Add(Creator(GetPos(i) + DD * Delta));
        }
        return Points;
    };

    TMap<int32, FMinLenInfo> MinLenMap;

    auto Check = [&](int32 SrcIndex, int32 DstIndex, const FVector2D& Inter) {
        const FVector2D SrcV = GetPos(SrcIndex);
        const FVector2D Dir = Inter - SrcV;
        const float Len = Dir.Size();
        const FVector2D En = GetEdgeNormal(SrcIndex);
        const float Offset = FVector2D::DotProduct(En, Dir);

        if (!MinLenMap.Contains(SrcIndex)) {
            MinLenMap.Add(SrcIndex, { Len, DstIndex, Offset, Inter });
        }
        else if (Len < MinLenMap[SrcIndex].MinLen) {
            MinLenMap[SrcIndex] = { Len, DstIndex, Offset, Inter };
        }
    };

    for (int32 i = 0; i < Vertices.Num() - 2; ++i) {
        const FVector2D Vn1 = GetVertexNormal(i);
        const FRay2D HalfRay1(GetPos(i), Vn1);

        for (int32 j = i + 2; j < Vertices.Num() - 1; ++j) {
            const FVector2D Vn2 = GetVertexNormal(j);
            const FRay2D HalfRay2(GetPos(j), Vn2);

            FVector2D Intersection;
            float T1, T2;
            if (HalfRay2.CalcIntersection(HalfRay1, Intersection, T1, T2)) {
                Check(i, j, Intersection);
                Check(j, i, Intersection);
            }
        }
    }

    // Find minimum element where indices match
    TOptional<TPair<int32, FMinLenInfo>> MinElement;
    float MinOffset = MAX_FLT;

    for (const auto& Entry : MinLenMap) {
        const int32 Key = Entry.Key;
        const int32 Val = Entry.Value.Index;
        if (MinLenMap.Contains(Val) && MinLenMap[Val].Index == Key) {
            const float CurrentOffset = Entry.Value.Offset;
            if (!MinElement.IsSet() || CurrentOffset < MinOffset) {
                MinElement = TPair<int32, FMinLenInfo>(Key, Entry.Value);
                MinOffset = CurrentOffset;
            }
        }
    }

    if (MinElement.IsSet()) {
        auto Moved = Move(MinElement->Value.Offset);
        int32 From = MinElement->Key;
        int32 To = MinElement->Value.Index;

        if (From > To) {
            Swap(From, To);
        }

        TArray<T> Range;
        for (int32 i = From; i < To; ++i) {
            Range.Add(Moved[i]);
        }

        Moved.RemoveAt(From, To - From);

        if (FGeoGraph2D::CalcPolygonArea(Range) > FGeoGraph2D::CalcPolygonArea(Moved)) {
            Moved = Range;
        }

        if (CheckStop(Moved, Nest + 1)) {
            return Vertices;
        }

        return Reduction(Moved, ToVec2, Creator, CheckStop, Nest + 1);
    }

    return Vertices;
}

