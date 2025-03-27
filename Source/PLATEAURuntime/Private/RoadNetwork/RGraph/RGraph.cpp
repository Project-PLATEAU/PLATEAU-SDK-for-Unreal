// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/RGraph/RGraph.h"
#include "Component/PLATEAUCityObjectGroup.h"

URVertex::URVertex(const FVector& InPosition)
{
    Init(InPosition);
}

URVertex::~URVertex()
{
   // DisConnect();
}

void URVertex::Init(const FVector& InPosition)
{
    Position = InPosition;
}

TArray<RGraphRef_t<URFace>> URVertex::GetFaces()
{
    TSet<RGraphRef_t<URFace>> FaceSet;
    for (auto& Edge : Edges) 
    {
        if (!Edge)
            continue;

        for (const auto& Face : Edge->GetFaces()) {
            FaceSet.Add(Face);
        }
    }
    return FaceSet.Array();
}

void URVertex::AddEdge(RGraphRef_t<UREdge> Edge) {
    if (Edge) {
        Edges.Add(Edge);
    }
}

void URVertex::RemoveEdge(RGraphRef_t<UREdge> Edge) {
    if (Edge) {
        Edges.Remove(Edge);
    }
}

void URVertex::DisConnect(bool RemoveEdge)
{
    TArray<RGraphRef_t<UREdge>> EdgeArray = Edges.Array();
    for (auto Edge : EdgeArray) {
        if (!Edge)
            continue;

        Edge->RemoveVertex(this);
        if (RemoveEdge) {
            Edge->DisConnect();
        }

    }
    Edges.Empty();
}

void URVertex::DisConnectWithKeepLink() {
    auto EdgeArray = Edges.Array();
    for (auto Edge : EdgeArray) {
        if(Edge)
            Edge->RemoveVertex(RGraphRef_t<URVertex>(this));
    }
    Edges.Empty();
}

TArray<RGraphRef_t<URVertex>> URVertex::GetNeighborVertices() {
    TArray<RGraphRef_t<URVertex>> Result;
    for (auto Edge : GetEdges()) {
        if (!Edge)
            continue;
        Result.Add(Edge->GetOppositeVertex(RGraphRef_t<URVertex>(this)));
    }
    return Result;
}

bool URVertex::IsNeighbor(RGraphRef_t<URVertex> Other) {
    if (!Other)
        return false;

    for (auto Edge : GetEdges()) 
    {
        if (!Edge)
            continue;
        if (Edge->GetOppositeVertex(RGraphRef_t<URVertex>(this)) == Other) {
            return true;
        }
    }
    return false;
}

void URVertex::MergeTo(RGraphRef_t<URVertex> Dst, bool CheckEdgeMerge) {
    if (!Dst)
        return;

    TArray<RGraphRef_t<UREdge>> EdgeArray = Edges.Array();
    for (auto Edge : EdgeArray) 
    {
        Edge->ChangeVertex(RGraphRef_t<URVertex>(this), Dst);
        if (CheckEdgeMerge) {
            for (auto DstEdge : Dst->GetEdges()) {
                if (Edge != DstEdge && Edge->IsSameVertex(DstEdge)) {
                    Edge->MergeTo(DstEdge);
                    break;
                }
            }
        }
    }
}

ERRoadTypeMask URVertex::GetRoadType(TFunction<bool(RGraphRef_t<URFace>)> FaceSelector) {
    ERRoadTypeMask Result = ERRoadTypeMask::Empty;
    for (auto Edge : Edges) {
        for (auto Face : Edge->GetFaces()) {
            if (!FaceSelector || FaceSelector(Face)) {
                Result = static_cast<ERRoadTypeMask>(static_cast<uint8>(Result) | static_cast<uint8>(Face->GetRoadTypes()));
            }
        }
    }
    return Result;
}

int32 URVertex::GetMaxLodLevel(TFunction<bool(RGraphRef_t<URFace>)> FaceSelector) {
    int32 MaxLevel = -1;
    for (auto Edge : Edges) {
        for (auto Face : Edge->GetFaces()) {
            if (!FaceSelector || FaceSelector(Face)) {
                MaxLevel = FMath::Max(MaxLevel, Face->GetLodLevel());
            }
        }
    }
    return MaxLevel;
}

ERRoadTypeMask URVertex::GetAnyFaceTypeMask() const
{
    ERRoadTypeMask Mask = ERRoadTypeMask::Empty;
    for (const auto& Edge : GetEdges()) 
    {
        for (const auto& Face : Edge->GetFaces()) {
            Mask |= Face->GetRoadTypes();
        }
    }
    return Mask;

}

ERRoadTypeMask URVertex::GetAllFaceTypeMask() const
{
    ERRoadTypeMask Mask = FRRoadTypeMaskEx::All();
    for (const auto& Edge : GetEdges()) {
        for (const auto& Face : Edge->GetFaces()) {
            Mask &= Face->GetRoadTypes();
        }
    }
    return Mask;
}

ERRoadTypeMask URVertex::GetTypeMaskOrDefault(bool AnyFaceType) const
{
    return AnyFaceType ? GetAnyFaceTypeMask() : GetAllFaceTypeMask();
}

UREdge::UREdge(RGraphRef_t<URVertex> InV0, RGraphRef_t<URVertex> InV1)
{
    Init(InV0, InV1);
}

UREdge::~UREdge()
{
    //DisConnect();
}

void UREdge::Init(RGraphRef_t<URVertex> InV0, RGraphRef_t<URVertex> InV1)
{
    Vertices.Add(InV0);
    Vertices.Add(InV1);

    if (InV0) InV0->AddEdge(RGraphRef_t<UREdge>(this));
    if (InV1) InV1->AddEdge(RGraphRef_t<UREdge>(this));
}

bool UREdge::IsValid() const {
    return Vertices[0] != nullptr && Vertices[1] != nullptr;
}

TArray<RGraphRef_t<UREdge>> UREdge::GetNeighborEdges() {
    TSet<RGraphRef_t<UREdge>> Result;
    for (auto Vertex : Vertices) {
        if (Vertex) {
            for (auto& E : Vertex->GetEdges())
                Result.Add(E);
        }
    }
    Result.Remove(this);
    return Result.Array();
}

RGraphRef_t<URVertex> UREdge::GetVertex(EVertexType Type) {
    return Vertices[static_cast<int32>(Type)];
}

void UREdge::SetVertex(EVertexType Type, RGraphRef_t<URVertex> Vertex) {
    auto OldVertex = Vertices[static_cast<int32>(Type)];
    if (OldVertex) {
        OldVertex->RemoveEdge(RGraphRef_t<UREdge>(this));
    }

    Vertices[static_cast<int32>(Type)] = Vertex;
    if (Vertex) {
        Vertex->AddEdge(RGraphRef_t<UREdge>(this));
    }
}

void UREdge::ChangeVertex(RGraphRef_t<URVertex> From, RGraphRef_t<URVertex> To) {
    if (GetV0() == From) {
        // 両方ともToになる場合は接続自体解除
        if (GetV1() == To)
        {
            DisConnect();
        }
        else
        {
            SetVertex(EVertexType::V0, To);            
        }
    }
    else if (GetV1() == From) 
    {
        if (GetV0() == To) {
            DisConnect();
        }
        else {
            SetVertex(EVertexType::V1, To);
        }
    }
}

void UREdge::AddFace(RGraphRef_t<URFace> Face) {
    if (Face) {
        Faces.Add(Face);
    }
}

void UREdge::RemoveFace(RGraphRef_t<URFace> Face) {
    if (Face) {
        Faces.Remove(Face);
    }
}

void UREdge::RemoveVertex(RGraphRef_t<URVertex> Vertex) {
    if (Vertices[0] == Vertex) {
        SetVertex(EVertexType::V0, nullptr);
    }
    else if (Vertices[1] == Vertex) {
        SetVertex(EVertexType::V1, nullptr);
    }
}

void UREdge::DisConnect() {
    TArray<RGraphRef_t<URFace>> FaceArray = Faces.Array();
    for (auto Face : FaceArray) {
        Face->RemoveEdge(RGraphRef_t<UREdge>(this));
    }
    Faces.Empty();

    for (auto Vertex : Vertices) {
        if (Vertex) {
            Vertex->RemoveEdge(RGraphRef_t<UREdge>(this));
        }
    }
}

RGraphRef_t<UREdge> UREdge::SplitEdge(RGraphRef_t<URVertex> V) {
    if (!V) return nullptr;

    auto NewEdge = RGraphNew<UREdge>(V, Vertices[1]);
    SetVertex(EVertexType::V1, V);

    TArray<RGraphRef_t<URFace>> FaceArray = Faces.Array();
    for (auto Face : FaceArray) {
        Face->AddEdge(NewEdge);
    }

    return NewEdge;
}

bool UREdge::IsSameVertex(RGraphRef_t<URVertex> V0, RGraphRef_t<URVertex> V1) {
    return (Vertices[0] == V0 && Vertices[1] == V1) ||
        (Vertices[0] == V1 && Vertices[1] == V0);
}

bool UREdge::IsSameVertex(RGraphRef_t<UREdge> Other) {
    if (!Other) return false;
    return IsSameVertex(Other->Vertices[0], Other->Vertices[1]);
}

bool UREdge::IsShareAnyVertex(RGraphRef_t<UREdge> Other) {
    if (!Other) return false;
    return Vertices[0] == Other->Vertices[0] ||
        Vertices[0] == Other->Vertices[1] ||
        Vertices[1] == Other->Vertices[0] ||
        Vertices[1] == Other->Vertices[1];
}

bool UREdge::IsShareAnyVertex(RGraphRef_t<UREdge> Other, RGraphRef_t<URVertex>& SharedVertex) {
    if (!Other) return false;

    if (Vertices[0] == Other->Vertices[0] || Vertices[0] == Other->Vertices[1]) {
        SharedVertex = Vertices[0];
        return true;
    }
    if (Vertices[1] == Other->Vertices[0] || Vertices[1] == Other->Vertices[1]) {
        SharedVertex = Vertices[1];
        return true;
    }
    return false;
}

bool UREdge::TryGetOppositeVertex(RGraphRef_t<URVertex> Vertex, RGraphRef_t<URVertex>& Opposite) {
    if (Vertices[0] == Vertex) {
        Opposite = Vertices[1];
        return true;
    }
    if (Vertices[1] == Vertex) {
        Opposite = Vertices[0];
        return true;
    }
    return false;
}

RGraphRef_t<URVertex> UREdge::GetOppositeVertex(RGraphRef_t<URVertex> Vertex) {
    return (Vertices[0] == Vertex) ? Vertices[1] : Vertices[0];
}

void UREdge::MergeTo(RGraphRef_t<UREdge> Dst, bool CheckFaceMerge) {
    if (!Dst) return;

    TArray<RGraphRef_t<URFace>> FaceArray = Faces.Array();
    for (auto Face : FaceArray) {
        Face->ChangeEdge(RGraphRef_t<UREdge>(this), Dst);
        if (CheckFaceMerge) {
            for (auto DstFace : Dst->Faces) {
                if (Face != DstFace && Face->IsSameEdges(DstFace)) {
                    Face->TryMergeTo(DstFace);
                    break;
                }
            }
        }
    }
}

ERRoadTypeMask UREdge::GetAnyFaceTypeMask() const
{
    ERRoadTypeMask Mask = ERRoadTypeMask::Empty;
    for (const auto& Face : GetFaces()) {
        Mask |= Face->GetRoadTypes();
    }
    return Mask;
}

ERRoadTypeMask UREdge::GetAllFaceTypeMask() const
{
    ERRoadTypeMask Mask = FRRoadTypeMaskEx::All();
    for (const auto& Face : GetFaces()) {
        Mask &= Face->GetRoadTypes();
    }
    return Mask;
}

ERRoadTypeMask UREdge::GetTypeMaskOrDefault(const bool AnyFaceType) const
{
    return AnyFaceType ? GetAnyFaceTypeMask() : GetAllFaceTypeMask();
}

// Face implementations
URFace::URFace(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, ERRoadTypeMask InRoadType, int32 InLodLevel)
{
    Init(InGraph, InCityObjectGroup, InRoadType, InLodLevel);
}

URFace::~URFace()
{
    //DisConnect();
}

void URFace::Init(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, ERRoadTypeMask InRoadType,
    int32 InLodLevel)
{
    CityObjectGroup = InCityObjectGroup;
    RoadTypes = InRoadType;
    LodLevel = InLodLevel;
    SetGraph(InGraph);
}

void URFace::AddEdge(RGraphRef_t<UREdge> Edge) {
    if (Edge) {
        Edges.Add(Edge);
        Edge->AddFace(RGraphRef_t<URFace>(this));
    }
}

void URFace::RemoveGraph(RGraphRef_t<URGraph> G) {
    if (Graph == G) {
        Graph = nullptr;
    }
}

void URFace::SetGraph(RGraphRef_t<URGraph> G) {
    if (Graph != G) {
        if (Graph) {
            Graph->RemoveFace(RGraphRef_t<URFace>(this));
        }
        Graph = G;
        if (Graph) {
            Graph->AddFace(RGraphRef_t<URFace>(this));
        }
    }
}

void URFace::RemoveEdge(RGraphRef_t<UREdge> Edge) {
    if (Edge) {
        Edges.Remove(Edge);
        Edge->RemoveFace(RGraphRef_t<URFace>(this));
    }
}

void URFace::ChangeEdge(RGraphRef_t<UREdge> From, RGraphRef_t<UREdge> To) {
    if (From && To && From != To && Edges.Contains(From)) {
        RemoveEdge(From);
        AddEdge(To);
    }
}

bool URFace::IsSameEdges(RGraphRef_t<URFace> Other) {
    if (!Other || Edges.Num() != Other->Edges.Num()) {
        return false;
    }

    for (auto Edge : Edges) {
        if (!Other->Edges.Contains(Edge)) {
            return false;
        }
    }
    return true;
}

bool URFace::TryMergeTo(RGraphRef_t<URFace> Dst) {
    if (!Dst || !IsSameEdges(Dst)) {
        return false;
    }

    DisConnect();
    return true;
}

void URFace::DisConnect() {
    TArray<RGraphRef_t<UREdge>> EdgeArray = Edges.Array();
    for (auto Edge : EdgeArray) {
        RemoveEdge(Edge);
    }
    if(Graph)
        Graph->RemoveFace(RGraphRef_t<URFace>(this));
    SetGraph(nullptr);
}

URGraph::URGraph()
{
}

TSet<RGraphRef_t<UREdge>> URGraph::GetAllEdges() {
    TSet<RGraphRef_t<UREdge>> Result;
    for (auto Face : Faces) {
        for (auto Edge : Face->GetEdges())
            Result.Add(Edge);
    }
    return Result;
}

TSet<RGraphRef_t<URVertex>> URGraph::GetAllVertices() {
    TSet<RGraphRef_t<URVertex>> Result;
    for (auto Edge : GetAllEdges()) {
        Result.Add(Edge->GetV0());
        Result.Add(Edge->GetV1());
    }
    return Result;
}

void URGraph::AddFace(RGraphRef_t<URFace> Face) {
    if (Face && !Faces.Contains(Face)) {
        Faces.Add(Face);
    }
}

void URGraph::RemoveFace(RGraphRef_t<URFace> Face) {
    if (Face) {
        Faces.Remove(Face);
    }
}

// Face Group implementations
URFaceGroup::URFaceGroup(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup, const TArray<RGraphRef_t<URFace>>& InFaces)
{
    Init(InGraph, InCityObjectGroup, InFaces);
}

void URFaceGroup::Init(RGraphRef_t<URGraph> InGraph, UPLATEAUCityObjectGroup* InCityObjectGroup,
    const TArray<RGraphRef_t<URFace>>& InFaces)
{
    Graph = InGraph;
    CityObjectGroup = InCityObjectGroup;
    Faces.Reset();
    for (auto Face : InFaces) {
        Faces.Add(Face);
    }        
}

ERRoadTypeMask URFaceGroup::GetRoadTypes() const {
    ERRoadTypeMask Result = ERRoadTypeMask::Empty;
    for (auto Face : Faces) {
        Result = static_cast<ERRoadTypeMask>(static_cast<uint8>(Result) | static_cast<uint8>(Face->GetRoadTypes()));
    }
    return Result;
}

RGraphRef_t<URFaceGroup> URFaceGroup::CreateFaceGroup(RGraphRef_t<URGraph> Graph,
    UPLATEAUCityObjectGroup* CityObjectGroup, const TArray<RGraphRef_t<URFace>>& Faces)
{
    return RGraphNew<URFaceGroup>(Graph, CityObjectGroup, Faces);
}

constexpr auto SIZE = sizeof(UObject);
constexpr auto OFFSET = sizeof(FVector);