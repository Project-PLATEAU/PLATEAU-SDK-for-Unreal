// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/RGraph/RGraphFactory.h"

#include "Component/PLATEAUCityObjectGroup.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "RoadNetwork/CityObject/SubDividedCityObject.h"
#include "RoadNetwork/RGraph/RGraph.h"
#include "RoadNetwork/RGraph/RGraphEx.h"

namespace
{
    struct FEdgeKey {
        RGraphRef_t<URVertex> V0;
        RGraphRef_t<URVertex> V1;

        FEdgeKey(RGraphRef_t<URVertex> InV0, RGraphRef_t<URVertex> InV1)
            : V0(InV0), V1(InV1)
        {
            if (V0 > V1) {
                Swap(V0, V1);
            }
        }

        bool operator==(const FEdgeKey& Other) const {
            return (V0 == Other.V0 && V1 == Other.V1) || (V0 == Other.V1 && V1 == Other.V0);
        }

        friend uint32 GetTypeHash(const FEdgeKey& Key) {
            return HashCombine(GetTypeHash(Key.V0), GetTypeHash(Key.V1));
        }
    };
}

RGraphRef_t<URGraph> FRGraphFactoryEx::CreateGraph(const FRGraphFactory& Factory,
    const TArray<FSubDividedCityObject>& CityObjects)
{
    auto Graph = RGraphNew<URGraph>();

    TMap<FVector, RGraphRef_t<URVertex>> VertexMap;
    TMap<FEdgeKey, RGraphRef_t<UREdge>> EdgeMap;
    auto OrigVertexCount = 0;
    auto OrigEdgeCount = 0;
    for (auto& CityObject : CityObjects) {
        if (CityObject.CityObjectGroup == nullptr) {
            continue;
        }

        auto&& LODLevel = CityObject.CityObjectGroup->MinLOD;
        auto&& RoadType = CityObject.GetRoadType(true);
        // transformを適用する
        auto&& tr = CityObject.CityObjectGroup->GetComponentTransform();
        for (auto&& mesh : CityObject.Meshes) {
            auto&& face = RGraphNew<URFace>(Graph, CityObject.CityObjectGroup.Get(), RoadType, LODLevel);

            TArray<RGraphRef_t<URVertex>> vertices;
            for (auto&& LocalPos : mesh.Vertices) {
                auto WorldPos = tr.TransformPosition(LocalPos);

                if (VertexMap.Contains(WorldPos) == false) {
                    VertexMap.Add(WorldPos, RGraphNew<URVertex>(WorldPos));
                }

                vertices.Add(VertexMap[WorldPos]);
            }
            OrigVertexCount += mesh.Vertices.Num();
            for (auto&& s : mesh.SubMeshes) {
                auto AddEdge = [&EdgeMap, &face, &OrigEdgeCount](RGraphRef_t<URVertex> V0, RGraphRef_t<URVertex> V1) {

                    auto key = FEdgeKey(V0, V1);
                    if (EdgeMap.Contains(key) == false) {
                        auto Found = false;
                        for(auto& Pair : EdgeMap)
                        {
                            auto K = Pair.Key;

                            if((K.V0 == V0 && K.V1 == V1)
                                || (K.V0 == V1 && K.V1 == V0))
                            {
                                Found = true;
                                break;
                            }
                        }

                        if(Found)
                        {
                            auto Tmp = 0;
                        }
                        EdgeMap.Add(key, RGraphNew<UREdge>(key.V0, key.V1));
                    }
                    face->AddEdge(EdgeMap[key]);
                    OrigEdgeCount++;
                    };

                if (Factory.bUseCityObjectOutline) {
                    auto&& indexTable = s.CreateOutlineIndices();
                    for (auto&& indices : indexTable) {
                        for (auto&& i = 0; i < indices.Num(); i++) {
                            AddEdge(vertices[indices[i]], vertices[indices[(i + 1) % indices.Num()]]);
                        }
                    }
                }
                else {
                    for (auto&& i = 0; i < s.Triangles.Num(); i += 3) {
                        AddEdge(vertices[s.Triangles[i + 0]], vertices[s.Triangles[i + 1]]);
                        AddEdge(vertices[s.Triangles[i + 1]], vertices[s.Triangles[i + 2]]);
                        AddEdge(vertices[s.Triangles[i + 2]], vertices[s.Triangles[i]]);
                    }
                }

            }
            Graph->AddFace(face);
        }
    }
#if false
    auto CheckVertices = [&]() {
        auto Vertices = Graph->GetAllVertices().Array();
        for (auto i = 0; i < Vertices.Num() - 1; ++i) {
            auto V1 = Vertices[i];
            for (auto j = i + 1; j < Vertices.Num(); ++j) {
                auto V2 = Vertices[j];
                auto Dist = FVector::Distance(V1->GetPosition(), V2->GetPosition());
                if (Dist < 1e-3f) {
                    auto Tmp = 0;
                }

            }
        }
    };
    auto CheckEdge = [&]() {
        auto Edges = Graph->GetAllEdges().Array();
        for (auto i = 0; i < Edges.Num() - 1; ++i) {
            auto E1 = Edges[i];
            for (auto j = i + 1; j < Edges.Num(); ++j) {
                auto E2 = Edges[j];
                if (E1->IsSameVertex(E2)) {
                    auto Tmp = 0;
                }
            }
        }
    };
#endif
    if (Factory.bOptAdjustSmallLodHeight) {
        FRGraphEx::AdjustSmallLodHeight(Graph, Factory.MergeCellSize, Factory.MergeCellLength, Factory.RemoveMidPointTolerance);
    }
    if (Factory.bOptEdgeReduction) {
        FRGraphEx::EdgeReduction(Graph);
    }
    if (Factory.bOptVertexReduction) {
        FRGraphEx::VertexReduction(Graph, Factory.MergeCellSize, Factory.MergeCellLength, Factory.RemoveMidPointTolerance);
    }
    if (Factory.bOptRemoveIsolatedEdgeFromFace) {
        FRGraphEx::RemoveIsolatedEdgeFromFace(Graph);
    }
    if (Factory.bOptEdgeReduction) {
        FRGraphEx::EdgeReduction(Graph);
    }
    if (Factory.bOptInsertVertexInNearEdge) {
        FRGraphEx::InsertVertexInNearEdge(Graph, Factory.RemoveMidPointTolerance);
    }
    if (Factory.bOptEdgeReduction) {
        FRGraphEx::EdgeReduction(Graph);
    }
    if (Factory.bOptSeparateFaces) {
        FRGraphEx::SeparateFaces(Graph);
    }

    if (Factory.bOptRemoveIsolatedEdgeFromFace) {
        FRGraphEx::RemoveIsolatedEdgeFromFace(Graph);
    }

    if (Factory.bOptModifySideWalkShape) {
        FRGraphEx::ModifySideWalkShape(Graph);
    }

    if(Factory.bFaceReduction) {
        FRGraphEx::FaceReduction(Graph);
    }
    return Graph;
}
