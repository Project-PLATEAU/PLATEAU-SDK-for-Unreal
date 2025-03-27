// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
// ☀
#pragma once

#include "CoreMinimal.h"
#include "RGraphDef.h"
#include <memory>

#include "RoadNetwork/PLATEAURnDef.h"
#include "RGraph.generated.h"

class UPLATEAUCityObjectGroup;

class URVertex;
class UREdge;
class URFace;
class URGraph;

UCLASS()
class URVertex : public UObject {
    GENERATED_BODY()
public:
    URVertex() = default;
    URVertex(const FVector& InPosition);
    virtual ~URVertex() override;

    void Init(const FVector& InPosition);

    FVector Position;
    const FVector& GetPosition() const { return Position; }
    const auto& GetEdges() const { return Edges; }
    TArray<RGraphRef_t<URFace>> GetFaces();
    void AddEdge(RGraphRef_t<UREdge> Edge);
    void RemoveEdge(RGraphRef_t<UREdge> Edge);
    void DisConnect(bool RemoveEdge = false);
    void DisConnectWithKeepLink();
    TArray<RGraphRef_t<URVertex>> GetNeighborVertices();
    bool IsNeighbor(RGraphRef_t<URVertex> Other);
    void MergeTo(RGraphRef_t<URVertex> Dst, bool CheckEdgeMerge = true);

    ERRoadTypeMask GetRoadType(TFunction<bool(RGraphRef_t<URFace>)> FaceSelector = nullptr);
    int32 GetMaxLodLevel(TFunction<bool(RGraphRef_t<URFace>)> FaceSelector = nullptr);

    // 頂点を参照する面のタイプマスクをorで取得
    ERRoadTypeMask GetAnyFaceTypeMask() const;

    // 頂点を参照する面のタイプマスクをandで取得
    ERRoadTypeMask GetAllFaceTypeMask() const;

    // 頂点を参照する面のタイプマスクを取得
    // AnyFaceType = true   GetAnyFaceTypeMask
    // AnyFaceType = false  GetAllFaceTypeMask
    ERRoadTypeMask GetTypeMaskOrDefault(bool AnyFaceType = false) const;
private:
    UPROPERTY()
    TSet<TObjectPtr<UREdge>> Edges;
};

UCLASS()
class UREdge : public UObject {
    GENERATED_BODY()
public:
    enum class EVertexType : uint8 {
        V0,
        V1
    };
    UREdge() = default;
    UREdge(RGraphRef_t<URVertex> InV0, RGraphRef_t<URVertex> InV1);
    virtual ~UREdge() override;

    void Init(RGraphRef_t<URVertex> InV0, RGraphRef_t<URVertex> InV1);
    const auto& GetFaces() const { return Faces; }
    RGraphRef_t<URVertex> GetV0() const { return GetVertex(EVertexType::V0); }
    RGraphRef_t<URVertex> GetV1() const { return GetVertex(EVertexType::V1); }

    RGraphRef_t<URVertex> GetVertex(EVertexType Type) const
    {
        return Vertices[static_cast<int32>(Type)];    
    }
    const auto& GetVertices() const { return Vertices; }
    bool IsValid() const;
    TArray<RGraphRef_t<UREdge>> GetNeighborEdges();
    RGraphRef_t<URVertex> GetVertex(EVertexType Type);
    void SetVertex(EVertexType Type, RGraphRef_t<URVertex> Vertex);
    void ChangeVertex(RGraphRef_t<URVertex> From, RGraphRef_t<URVertex> To);
    void AddFace(RGraphRef_t<URFace> Face);
    void RemoveFace(RGraphRef_t<URFace> Face);
    void RemoveVertex(RGraphRef_t<URVertex> Vertex);
    void DisConnect();
    RGraphRef_t<UREdge> SplitEdge(RGraphRef_t<URVertex> V);
    bool IsSameVertex(RGraphRef_t<URVertex> V0, RGraphRef_t<URVertex> V1);
    bool IsSameVertex(RGraphRef_t<UREdge> Other);
    bool IsShareAnyVertex(RGraphRef_t<UREdge> Other);
    bool IsShareAnyVertex(RGraphRef_t<UREdge> Other, RGraphRef_t<URVertex>& SharedVertex);
    bool TryGetOppositeVertex(RGraphRef_t<URVertex> Vertex, RGraphRef_t<URVertex>& Opposite);
    RGraphRef_t<URVertex> GetOppositeVertex(RGraphRef_t<URVertex> Vertex);
    void MergeTo(RGraphRef_t<UREdge> Dst, bool CheckFaceMerge = true);


    // 頂点を参照する面のタイプマスクをorで取得
    ERRoadTypeMask GetAnyFaceTypeMask() const;

    // 頂点を参照する面のタイプマスクをandで取得
    ERRoadTypeMask GetAllFaceTypeMask() const;

    // 頂点を参照する面のタイプマスクを取得
    // AnyFaceType = true   GetAnyFaceTypeMask
    // AnyFaceType = false  GetAllFaceTypeMask
    ERRoadTypeMask GetTypeMaskOrDefault(bool AnyFaceType = false) const;
private:
    UPROPERTY()
    TSet<TObjectPtr<URFace>> Faces;

    UPROPERTY()
    TArray<TObjectPtr<URVertex>> Vertices;
};

UCLASS()
class URFace : public UObject {
    GENERATED_BODY()
public:
    URFace() = default;
    URFace(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, ERRoadTypeMask InRoadType, int32 InLodLevel);
    virtual ~URFace() override;

    void Init(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, ERRoadTypeMask InRoadType, int32 InLodLevel);

    const auto& GetEdges() const { return Edges; }
    auto GetGraph() const { return Graph; }
    auto GetGraph() { return Graph; }

    bool IsValid() const { return Edges.Num() > 0; }
    void AddEdge(RGraphRef_t<UREdge> Edge);
    void RemoveGraph(RGraphRef_t<URGraph> G);
    void SetGraph(RGraphRef_t<URGraph> G);
    void RemoveEdge(RGraphRef_t<UREdge> Edge);
    void ChangeEdge(RGraphRef_t<UREdge> From, RGraphRef_t<UREdge> To);
    bool IsSameEdges(RGraphRef_t<URFace> Other);
    bool TryMergeTo(RGraphRef_t<URFace> Dst);
    void DisConnect();
    ERRoadTypeMask GetRoadTypes() const
    {
        return RoadTypes;    
    }
    int32 GetLodLevel() const {
        return LodLevel;
    }
    TWeakObjectPtr<UPLATEAUCityObjectGroup> GetCityObjectGroup() const {
        return CityObjectGroup;
    }
private:
    UPROPERTY()
    TSet<TObjectPtr<UREdge>> Edges;
    UPROPERTY()
    TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;
    UPROPERTY()
    ERRoadTypeMask RoadTypes;
    UPROPERTY()
    int32 LodLevel;
    UPROPERTY()
    TObjectPtr<URGraph> Graph;
};

UCLASS()
class URGraph : public UObject {
    GENERATED_BODY()
public:
    URGraph();
    void Init(){}
    const auto& GetFaces() const { return Faces; }
    TSet<RGraphRef_t<UREdge>> GetAllEdges();
    TSet<RGraphRef_t<URVertex>> GetAllVertices();
    void AddFace(RGraphRef_t<URFace> Face);
    void RemoveFace(RGraphRef_t<URFace> Face);
private:
    UPROPERTY()
    TSet<TObjectPtr<URFace>> Faces;
};

UCLASS()
class URFaceGroup : public UObject {
    GENERATED_BODY()
public:
    URFaceGroup() = default;
    URFaceGroup(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, const TArray<RGraphRef_t<URFace>>& InFaces);

    void Init(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, const TArray<RGraphRef_t<URFace>>& InFaces);


    const auto& GetFaces() const { return Faces; }

    RGraphRef_t<URGraph> Graph;
    TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;

    ERRoadTypeMask GetRoadTypes() const;


    static RGraphRef_t<URFaceGroup> CreateFaceGroup(RGraphRef_t<URGraph> Graph, UPLATEAUCityObjectGroup* CityObjectGroup, const TArray<RGraphRef_t<URFace>>& Faces);
private:
    TSet<RGraphRef_t<URFace>> Faces;
};
