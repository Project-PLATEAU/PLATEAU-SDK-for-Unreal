// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RGraph.h"
#include <memory>
#include <functional>

#include "RoadNetwork/Util/PLATEAURnEx.h"


class FSubDividedCityObject;
class UPLATEAUCityObjectGroup;
class FRGraphEx {
public:
    static void RemoveInnerVertex(RGraphRef_t<URFace> Face);
    static void RemoveInnerVertex(RGraphRef_t<URGraph> Graph);
    static TSet<RGraphRef_t<URVertex>> AdjustSmallLodHeight(RGraphRef_t<URGraph> Graph, float MergeCellSizeMeter, int32 MergeCellLength, float HeightToleranceMeter);
    static void VertexReduction(RGraphRef_t<URGraph> Graph, float MergeCellSizeMeter, int32 MergeCellLength, float MidPointToleranceMeter);
    static void EdgeReduction(RGraphRef_t<URGraph> Graph);
    static void MergeIsolatedVertices(RGraphRef_t<URGraph> Graph);
    static void MergeIsolatedVertex(RGraphRef_t<URFace> Face);
    static TArray<RGraphRef_t<URFaceGroup>> GroupBy(RGraphRef_t<URGraph> Graph, TFunction<bool(RGraphRef_t<URFace>, RGraphRef_t<URFace>)> IsMatch);
    static void InsertVertexInNearEdge(RGraphRef_t<URGraph> Graph, float ToleranceMeter);
    static void InsertVerticesInEdgeIntersection(RGraphRef_t<URGraph> Graph, float HeightToleranceMeter);
    static TArray<RGraphRef_t<UREdge>> InsertVertices(RGraphRef_t<UREdge> Edge, TArray<RGraphRef_t<URVertex>> Vertices);
    static void SeparateFaces(RGraphRef_t<URGraph> Graph);
    static void SeparateFace(RGraphRef_t<URFace> Face);
    static TArray<RGraphRef_t<URVertex>> ComputeOutlineVertices(const TArray<RGraphRef_t<URFace>>& Faces);
    static TArray<RGraphRef_t<URVertex>> ComputeOutlineVertices(RGraphRef_t<URFace> Face);
    static TArray<RGraphRef_t<URVertex>> ComputeOutlineVertices(RGraphRef_t<URFaceGroup> FaceGroup, TFunction<bool(RGraphRef_t<URFace>)> Predicate);
    static TArray<RGraphRef_t<URVertex>> ComputeOutlineVerticesByCityObjectGroup(RGraphRef_t<URGraph> Graph, UPLATEAUCityObjectGroup* CityObjectGroup, ERRoadTypeMask RoadTypes, ERRoadTypeMask RemoveRoadTypes);
    static TArray<RGraphRef_t<URVertex>> ComputeConvexHullVertices(RGraphRef_t<URFace> Face);
    static bool IsShareEdge(RGraphRef_t<URFace> A, RGraphRef_t<URFace> B);
    static TSet<RGraphRef_t<URVertex>> CreateVertexSet(RGraphRef_t<URFace> Face);
    static void RemoveIsolatedEdgeFromFace(RGraphRef_t<URGraph> Self);
    static TSet<RGraphRef_t<UREdge>> RemoveIsolatedEdge(RGraphRef_t<URFace> Self);

    // Edgesで表現された線分を頂点配列に分解
    // OutIsLoopはEdgesがループしているかどうかを返す
    static bool SegmentEdge2VertexArray(const TArray<RGraphRef_t<UREdge>>& Edges, TArray<RGraphRef_t<URVertex>>& OutVertices, bool& OutIsLoop);

    // Verticesで表されるポリゴン情報をEdge表現に変換
    // Verticesの隣接情報に基づくので生成できない場合もある
    static bool OutlineVertex2Edge(const TArray<RGraphRef_t<URVertex>>& Vertices, TArray<RGraphRef_t<UREdge>>& OutlineEdges);

    /*
     * 歩道情報を作成
     */
    static bool CreateSideWalk(
        RGraphRef_t<URFace> Face
        , TArray<RGraphRef_t<UREdge>>& OutsideEdges
        , TArray<RGraphRef_t<UREdge>>& InsideEdges
        , TArray<RGraphRef_t<UREdge>>& StartEdges
        , TArray<RGraphRef_t<UREdge>>& EndEdges
    );

    /*
     * 歩道情報を作成
     */
    static bool CreateSideWalk(
        RGraphRef_t<URFaceGroup> Face
        , TArray<RGraphRef_t<UREdge>>& OutsideEdges
        , TArray<RGraphRef_t<UREdge>>& InsideEdges
        , TArray<RGraphRef_t<UREdge>>& StartEdges
        , TArray<RGraphRef_t<UREdge>>& EndEdges
        , TSet<UPLATEAUCityObjectGroup*> NeighborCityObjectGroupsFilter
    );

    /*
     * 歩道のうちOutSideがない歩道に対して微小な辺を追加する
     */
    static void ModifySideWalkShape(RGraphRef_t<URGraph> Self);

    /*
     * 歩道のうちOutSideがない歩道に対して微小な辺を追加する
     */
    static void ModifySideWalkShape(RGraphRef_t<URFace> Self);

    // EdgesをKeyでグループ化したもの
    template<typename TKey>
    using FOutlineBorderGroup = FPLATEAURnEx::FKeyEdgeGroup<TKey, RGraphRef_t<UREdge>>;

    // OutlineEdgesで表現される多角形の各辺をKeySelectorでグループ化
    // ただし, 飛び飛びの辺はグループ化されない
    // 例: OutlineEdges = {A, A, B, B, A, A, C, C, A}
    //   => {B, (2,3)}, {A, (4,5)}, {C, (6,7)}, {A, (8,0, 1)}のようにグループ化される
    template<typename T>
    static TArray<FOutlineBorderGroup<T>> CreateOutlineBorderGroup(
        const TArray<RGraphRef_t<UREdge>>& OutlineEdges
        , TFunction<T(RGraphRef_t<UREdge>)> KeySelector)
    {
        return FPLATEAURnEx::GroupByOutlineEdges<T, RGraphRef_t<UREdge>>(OutlineEdges, KeySelector);
    }

    /**
     * FaceReduction
     *
     * URGraph に含まれる Face 同士のうち、Edge 数ごとにグループ化し、
     * 同じ Edge 群を持つ Face (すなわち、f1 のすべての Edge が f2 に含まれる場合)
     * を f2 -> f1 の順にマージします。マージに成功した場合は対象 Face をグループから除去し、
     * マージされた Face 数を返します。
     *
     * @param Graph マージ対象の URGraph オブジェクト
     * @return マージにより削除された Face の数
     */
    static int32 FaceReduction(URGraph* Graph);
};
