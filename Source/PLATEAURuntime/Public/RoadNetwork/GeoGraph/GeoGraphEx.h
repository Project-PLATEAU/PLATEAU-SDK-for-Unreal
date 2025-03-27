// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "AxisPlane.h"
#include "LineSegment3D.h"
#include "Math/Vector.h"
#include "Containers/Array.h"

class PLATEAURUNTIME_API FGeoGraphEx
{
public:

    template<class T>
    struct Edge
    {
    public:        
        Edge(const T& P0, const T& P1)
            : P0(P0)
            , P1(P1) {
        }
        const T& P0;
        const T& P1;    
    };

    /*
     * 頂点を表す配列Arrに対して
     * (V[0], V[1]), (V[1], V[2])...(V[N-2], V[N-1])のペアを返すイテレータ.
     * isLoop=trueの時は最後に(V[N-1], V[0])も返る
     */
    template<class T>
    class EdgeEnumerator
    {
        using InnerValueType = T;
    public:
        
        class Iterator
        {
        public:
            Iterator(const TArray<T>& InVertices, const int InIndex)
            : Vertices(InVertices)
            , Index(InIndex)
            { }

            // 前置++
            Iterator& operator++()
            {
                // 最後まで行ったらループする
                if (Index == Vertices.Num())
                    Index = 0;
                else
                    ++Index;

                return *this;
            }

            // 後置++
            Iterator operator++(int) {
                Iterator Temp = *this;
                this->operator++();
                return Temp;
            }


            Edge<InnerValueType> operator*() const {
                // インデックスが最後ならループさせる処理
                const auto I0 = Index % Vertices.Num();
                const auto I1 = (Index + 1) % Vertices.Num();
                return Edge<InnerValueType>(Vertices[I0], Vertices[I1]);
            }

            bool operator==(const Iterator& Other) const {
                return Index == Other.Index;
            }

            bool operator!=(const Iterator& Other) const {
                return !(*this == Other);
            }
        private:
            const TArray<T>& Vertices;
            // 0, 1, 2...N-1, N, 0でループする(ループ判定のために最後はNにはなる)
            int32 Index;
        };

    public:
        EdgeEnumerator(const TArray<T>& InVertices, bool InIsLoop)
        : IsLoop(InIsLoop)
        , Vertices(InVertices)
        {}

        Iterator begin() const
        {
            // 要素が0の場合空のイテレータを返す
            if (IsEmpty())
                return EmptyIterator();

            return Iterator(Vertices, 0);
        }

        Iterator end() const
        {
            // 要素が0の場合空のイテレータを返す
            if (IsEmpty())
                return EmptyIterator();

            if(IsLoop)
                return Iterator(Vertices, Vertices.Num());

            return Iterator(Vertices, Vertices.Num() - 1);
        }

        /*
         * 配列に変換する
         */
        TArray<Edge<T>> ToArray() const {
            TArray<Edge<T>> Result;
            for(auto It = begin(); It != end(); ++It)
            {
                Result.Add(*It);                
            }
            return Result;
        }

    private:
        // Edgeが作れない
        bool IsEmpty() const
        {
            return Vertices.Num() <= 1;
        }

        Iterator EmptyIterator() const
        {
            return Iterator(Vertices, Vertices.Num());
        }

        const bool IsLoop;
        const TArray<T>& Vertices;
    };
public:
    // T型の頂点配列Verticesで構成される線分リストに対し
    // (Vertices[0], Vertices[1]), (Vertices[1], Vertices[2]), ... (Vertices[N-1], Vertices[0])となる線分リストを取得
    // bIsLoopがtrueの場合、最後の線分は(Vertices[N-1], Vertices[0])となる
    template<typename T>
    static EdgeEnumerator<T> GetEdges(const TArray<T>& Vertices, bool bIsLoop)
    {
        return EdgeEnumerator<T>(Vertices, bIsLoop);
    }

    static TArray<FLineSegment3D> GetEdgeSegments(const TArray<FVector>& Vertices, bool bIsLoop)
    {
        TArray<FLineSegment3D> Segments;
        for (auto Edge : GetEdges(Vertices, bIsLoop)) {
            Segments.Add(FLineSegment3D(Edge.P0, Edge.P1));
        }
        return Segments;    
    }

    static TArray<FLineSegment2D> GetEdgeSegments(const TArray<FVector2D>& Vertices, bool bIsLoop) {
        TArray<FLineSegment2D> Segments = TArray<FLineSegment2D>();
        for (auto Edge : GetEdges(Vertices, bIsLoop)) {
            Segments.Add(FLineSegment2D(Edge.P0, Edge.P1));
        }
        return Segments;
    }

   /* template<typename T>
    static TArray<TTuple<T, T>> GetEdges(const TArray<T>& Vertices, bool bIsLoop);*/

    static TArray<FVector> GetInnerLerpSegments(
        const TArray<FVector>& LeftVertices,
        const TArray<FVector>& RightVertices,
        EAxisPlane Plane,
        float P,
        float CheckMeter = 3.f);


    static TArray<FIntVector> GetNeighborDistance3D(int32 D);
    static TArray<FIntPoint> GetNeighborDistance2D(int32 D);

    static  bool IsCollinear(const FVector& A, const FVector& B, const FVector& C, float DegEpsilon, float MidPointTolerance);
    static TMap<FVector, FVector> MergeVertices(const TArray<FVector>& Vertices, float CellSize, int32 MergeCellLength);


private:
    static bool IsInInnerSide(const TOptional<FLineSegment3D>& Edge, const FVector& Direction, bool bReverse, bool bIsPrev);
    static bool CheckCollision(const FVector& A, const FVector& B, const TArray<FLineSegment3D>& Edges, float IndexF);
};

