// Copyright 2023 Ministry of Land, Infrastructure and Transport


#include "Reconstruct/PLATEAUMeshLoaderCloneComponent.h"
#include "PLATEAUCityModelLoader.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "PLATEAUInstancedCityModel.h"
#include "MeshDescription.h"
#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"
#include "Math/UnrealMathUtility.h"
#include "MathUtil.h"

FPLATEAUMeshLoaderCloneComponent::FPLATEAUMeshLoaderCloneComponent(const FPLATEAUCachedMaterialArray& CachedMaterials) : FPLATEAUMeshLoaderForReconstruct(CachedMaterials) {}

FPLATEAUMeshLoaderCloneComponent::FPLATEAUMeshLoaderCloneComponent(const bool InbAutomationTest, const FPLATEAUCachedMaterialArray& CachedMaterials) : FPLATEAUMeshLoaderForReconstruct(CachedMaterials){
    bAutomationTest = InbAutomationTest;
}

void FPLATEAUMeshLoaderCloneComponent::SetSmoothing(bool bSmooth) {
    IsSmooth = bSmooth;
}

UPLATEAUCityObjectGroup* FPLATEAUMeshLoaderCloneComponent::GetOriginalComponent(FString NodePathString) {

    auto Ptr = ComponentsMap.Find(NodePathString);
    if (Ptr) {
        const auto& OriginalComponent = *Ptr;
        return OriginalComponent;
    }

    UE_LOG(LogTemp, Error, TEXT("Node not found : %s"), *NodePathString);
    return nullptr;
}

void FPLATEAUMeshLoaderCloneComponent::ReloadComponentFromModel(
    std::shared_ptr<plateau::polygonMesh::Model> Model,
    TMap<FString, UPLATEAUCityObjectGroup*> Components,
    AActor& InActor) {

    //Model->assignNodeHierarchy();

    ComponentsMap = Components;
    for (int i = 0; i < Model->getRootNodeCount(); i++) {
        FPLATEAUMeshLoaderForReconstruct::ReloadComponentFromNode(nullptr, Model->getRootNodeAt(i), 
            plateau::granularityConvert::ConvertGranularity::PerPrimaryFeatureObject, TMap<FString, FPLATEAUCityObject>(), InActor);
    }  
}

void FPLATEAUMeshLoaderCloneComponent::ReloadComponentFromNode(
    const plateau::polygonMesh::Node& InNode,
    TMap<FString, UPLATEAUCityObjectGroup*> Components,
    AActor& InActor) {
    ComponentsMap = Components;
    FPLATEAUMeshLoaderForReconstruct::ReloadComponentFromNode(nullptr, InNode,
        plateau::granularityConvert::ConvertGranularity::PerPrimaryFeatureObject, TMap<FString, FPLATEAUCityObject>(), InActor);
}

UStaticMeshComponent* FPLATEAUMeshLoaderCloneComponent::GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
    const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData,
    const std::shared_ptr <const citygml::CityModel> CityModel) {

    const FString NodeName = NodeHier.NodeName;
    const auto& PLATEAUCityObjectGroup = NewObject<UPLATEAUCityObjectGroup>(&Actor, NAME_None);

    // Originalコンポーネントの属性をそのまま利用
    const auto& OriginalComponent = GetOriginalComponent(NodeHier.NodePath);
    if (OriginalComponent) {
        PLATEAUCityObjectGroup->SerializedCityObjects = OriginalComponent->SerializedCityObjects;
        PLATEAUCityObjectGroup->OutsideChildren = OriginalComponent->OutsideChildren;
        PLATEAUCityObjectGroup->OutsideParent = OriginalComponent->OutsideParent;
        PLATEAUCityObjectGroup->MeshGranularityIntValue = OriginalComponent->MeshGranularityIntValue;
    }
    return PLATEAUCityObjectGroup;
}

UMaterialInterface* FPLATEAUMeshLoaderCloneComponent::GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, 
    const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) {

    // Originalコンポーネントのマテリアルをそのまま利用
    const auto& OriginalComponent = GetOriginalComponent(NodeHier.NodePath);
    if (OriginalComponent)
        return OriginalComponent->GetMaterial(0);

    return FPLATEAUMeshLoader::GetMaterialForSubMesh(SubMeshValue, Component, LoadInputData, Texture, NodeHier, Outer);
}

/**
 * @brief Meshとコンポーネントが存在する場合、Originalコンポーネントと同一階層にCloneを配置します。それ以外は処理しません。
 * ParentComponent, MeshGranularityパラメータは無視してコンポーネントの値を利用
 */
USceneComponent* FPLATEAUMeshLoaderCloneComponent::ReloadNode(USceneComponent* ParentComponent,
    const plateau::polygonMesh::Node& Node,
    ConvertGranularity Granularity,
    AActor& Actor) {

    if (Node.getMesh() != nullptr && Node.getMesh()->getVertices().size() > 0) {

        FNodeHierarchy NodeHier(Node);
        const auto& OriginalComponent = GetOriginalComponent(NodeHier.NodePath);

        if (OriginalComponent) {

            const auto& OriginalParentComponent = OriginalComponent->GetAttachParent();
            const auto& OriginalGranularity = StaticCast<plateau::polygonMesh::MeshGranularity>(OriginalComponent->MeshGranularityIntValue);

            plateau::polygonMesh::MeshExtractOptions MeshExtractOptions{};
            MeshExtractOptions.mesh_granularity = OriginalGranularity;
            FLoadInputData LoadInputData
            {
                MeshExtractOptions,
                std::vector<plateau::geometry::Extent>{},
                FString(),
                false,
                nullptr
            };
            return CreateStaticMeshComponent(Actor, *OriginalParentComponent, *Node.getMesh(), LoadInputData, nullptr,
                NodeHier);
        }
    }
    return nullptr;
}

bool FPLATEAUMeshLoaderCloneComponent::UseCachedMaterial() {
    return false;
}

bool FPLATEAUMeshLoaderCloneComponent::MergeTriangles() {
    return IsSmooth;
}

void FPLATEAUMeshLoaderCloneComponent::ModifyMeshDescription(FMeshDescription& MeshDescription) {

    if (!IsSmooth) return;

    FStaticMeshOperations::DetermineEdgeHardnessesFromVertexInstanceNormals(MeshDescription);

    TEdgeAttributesRef<bool> EdgeHardness =
        MeshDescription.EdgeAttributes().GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);
    for (FEdgeID EdgeID : MeshDescription.Edges().GetElementIDs()) {
        EdgeHardness.Set(EdgeID, 0, false);
    }

    FStaticMeshOperations::ComputeTriangleTangentsAndNormals(MeshDescription, FMathf::Epsilon);
    FStaticMeshOperations::RecomputeNormalsAndTangentsIfNeeded(MeshDescription, 
        EComputeNTBsFlags::WeightedNTBs | EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents | EComputeNTBsFlags::BlendOverlappingNormals);
}
