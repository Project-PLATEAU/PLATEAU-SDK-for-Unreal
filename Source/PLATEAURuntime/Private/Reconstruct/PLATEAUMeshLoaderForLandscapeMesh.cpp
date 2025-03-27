// Copyright 2023 Ministry of Land, Infrastructure and Transport


#include "Reconstruct/PLATEAUMeshLoaderForLandscapeMesh.h"
#include "PLATEAUCityModelLoader.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "Util/PLATEAUComponentUtil.h"
#include "plateau/polygon_mesh/mesh_extractor.h"
#include <plateau/height_map_generator/heightmap_generator.h>
#include "MeshDescription.h"
#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"
#include "plateau/height_map_generator/heightmap_mesh_generator.h"
#include "MathUtil.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

FPLATEAUMeshLoaderForLandscapeMesh::FPLATEAUMeshLoaderForLandscapeMesh(){}

FPLATEAUMeshLoaderForLandscapeMesh::FPLATEAUMeshLoaderForLandscapeMesh(const bool InbAutomationTest) {
    bAutomationTest = InbAutomationTest;
}

void FPLATEAUMeshLoaderForLandscapeMesh::CreateMeshFromHeightMap(AActor& Actor, const int32 SizeX, const int32 SizeY, 
    const TVec3d Min, const TVec3d Max, 
    const TVec2f MinUV, const TVec2f MaxUV, 
    uint16_t* HeightRawData, const FString NodeName) {
    double ActualHeight = abs(Max.z - Min.z);
    float HeightScale = ActualHeight;
    plateau::heightMapGenerator::HeightmapMeshGenerator gen;
    plateau::polygonMesh::Mesh mesh;
    gen.generateMeshFromHeightmap(mesh, SizeX, SizeY, HeightScale, HeightRawData,
        plateau::geometry::CoordinateSystem::ESU, Min, Max, MinUV, MaxUV, false);

    auto ParentComponent = Actor.GetRootComponent();
    const auto BaseComponents = FPLATEAUComponentUtil::FindComponentsByName(&Actor, NodeName);
    if (BaseComponents.Num() > 0) {
        const auto BaseComponent = BaseComponents[0];
        if (BaseComponent->IsA<UStaticMeshComponent>()) {

            const auto BaseStaticMeshComponent = (UStaticMeshComponent*)BaseComponent;
            ReplaceMaterial = BaseStaticMeshComponent->GetMaterial(0);
        }

        TArray<USceneComponent*> Parents;
        BaseComponent->GetParentComponents(Parents);
        if (Parents.Num() > 0)
            ParentComponent = Parents[0];
    }

    plateau::polygonMesh::MeshExtractOptions MeshExtractOptions{};
    FLoadInputData LoadInputData
    {
        MeshExtractOptions,
        std::vector<plateau::geometry::Extent>{},
        FString(),
        false,
        nullptr
    };

    FString ComponentName = FString::Format(*FString(TEXT("Mesh_{0}")), { NodeName });
    UStaticMeshComponent* Component = CreateStaticMeshComponent(Actor, *ParentComponent, mesh, LoadInputData, nullptr, FNodeHierarchy(ComponentName));

    Component->Mobility = EComponentMobility::Movable;
    Actor.AddInstanceComponent(Component);
    Component->RegisterComponent();
    Component->AttachToComponent(ParentComponent, FAttachmentTransformRules::KeepWorldTransform);

    // メッシュをワールド内にビルド
    const auto CopiedStaticMeshes = StaticMeshes;

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [CopiedStaticMeshes]() {
            UStaticMesh::BatchBuild(CopiedStaticMeshes, true, [](UStaticMesh* mesh) {
                return true;
                });
        }, TStatId(), nullptr, ENamedThreads::GameThread)->Wait();
        StaticMeshes.Reset();
}

bool FPLATEAUMeshLoaderForLandscapeMesh::OverwriteTexture() {
    return false;
}

bool FPLATEAUMeshLoaderForLandscapeMesh::InvertMeshNormal() {
    return false;
}

bool FPLATEAUMeshLoaderForLandscapeMesh::MergeTriangles() {
    return true;
}

UStaticMeshComponent* FPLATEAUMeshLoaderForLandscapeMesh::GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
    const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData,
    const std::shared_ptr <const citygml::CityModel> CityModel) {

    const FString NodeName = NodeHier.NodeName;
    const auto& PLATEAUCityObjectGroup = NewObject<UPLATEAUCityObjectGroup>(&Actor, NAME_None);

    // Originalコンポーネントの属性をそのまま利用
    const FString ReplacedName = NodeName.Replace(*FString("Mesh_"), *FString());
    const auto& OriginalComponent = FPLATEAUComponentUtil::GetCityObjectGroupByName(&Actor, ReplacedName);
    if (OriginalComponent) {
        PLATEAUCityObjectGroup->SerializedCityObjects = OriginalComponent->SerializedCityObjects;
        PLATEAUCityObjectGroup->OutsideChildren = OriginalComponent->OutsideChildren;
        PLATEAUCityObjectGroup->OutsideParent = OriginalComponent->OutsideParent;
        PLATEAUCityObjectGroup->MeshGranularityIntValue = OriginalComponent->MeshGranularityIntValue;
    }
    return PLATEAUCityObjectGroup;
}

UMaterialInterface* FPLATEAUMeshLoaderForLandscapeMesh::GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component,
    const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) {
    if (ReplaceMaterial != nullptr) {
        //Dynamic Material 再生成・Texture再設定 (レベル保存時に消えてしまうため）
        auto DynMaterial = UMaterialInstanceDynamic::Create(ReplaceMaterial->GetMaterial(), Outer);
        UTexture* ReferencedTexture = nullptr;
        ReplaceMaterial->GetTextureParameterValue(TEXT("Texture"), ReferencedTexture);
        DynMaterial->SetTextureParameterValue("Texture", ReferencedTexture);
        return DynMaterial;
    }
    return FPLATEAUMeshLoader::GetMaterialForSubMesh(SubMeshValue, Component, LoadInputData, Texture, NodeHier, Outer);
}

void FPLATEAUMeshLoaderForLandscapeMesh::ModifyMeshDescription(FMeshDescription& MeshDescription) {

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
