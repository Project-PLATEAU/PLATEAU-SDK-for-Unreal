// Copyright 2023 Ministry of Land, Infrastructure and Transport


#include "Reconstruct/PLATEAUMeshLoaderForHeightmap.h"
#include "PLATEAUCityModelLoader.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "plateau/polygon_mesh/mesh_extractor.h"
#include <plateau/height_map_generator/heightmap_generator.h>
#include "plateau/height_map_generator/heightmap_mesh_generator.h"
#include "MeshDescription.h"
#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"
#include "Component/PLATEAULandscapeRefComponent.h"
#include "Landscape.h"
#include "Util/PLATEAUReconstructUtil.h"
#include "Util/PLATEAUComponentUtil.h"


FPLATEAUMeshLoaderForHeightmap::FPLATEAUMeshLoaderForHeightmap() {}

FPLATEAUMeshLoaderForHeightmap::FPLATEAUMeshLoaderForHeightmap(const bool InbAutomationTest){
    bAutomationTest = InbAutomationTest;
}

TArray<HeightmapCreationResult> FPLATEAUMeshLoaderForHeightmap::CreateHeightMap(
    AActor* ModelActor,
    const std::shared_ptr<plateau::polygonMesh::Model> Model, FPLATEAULandscapeParam Param) {
    TArray<HeightmapCreationResult> CreationResults;
    for (int i = 0; i < Model->getRootNodeCount(); i++) {
        LoadNodeRecursiveForHeightMap(Model->getRootNodeAt(i), *ModelActor, Param, CreationResults);
    }
    return CreationResults;
}

void FPLATEAUMeshLoaderForHeightmap::LoadNodeRecursiveForHeightMap(
    const plateau::polygonMesh::Node& InNode,
    AActor& InActor, FPLATEAULandscapeParam Param, TArray<HeightmapCreationResult> &Results) {
    LoadNodeForHeightMap(InNode, InActor, Param, Results);
    const size_t ChildNodeCount = InNode.getChildCount();
    for (int i = 0; i < ChildNodeCount; i++) {
        const auto& TargetNode = InNode.getChildAt(i);
        LoadNodeRecursiveForHeightMap( TargetNode, InActor, Param, Results);
    }
}

void FPLATEAUMeshLoaderForHeightmap::LoadNodeForHeightMap(
    const plateau::polygonMesh::Node& InNode,
    AActor& InActor, FPLATEAULandscapeParam Param, TArray<HeightmapCreationResult> &Results) {
    if (InNode.getMesh() == nullptr || InNode.getMesh()->getVertices().size() == 0) {
        const FString DesiredName = FString(UTF8_TO_TCHAR(InNode.getName().c_str()));      
    }
    else {
        auto Result = CreateHeightMapFromMesh(*InNode.getMesh(), FString(UTF8_TO_TCHAR(InNode.getName().c_str())), InActor, Param);
        Results.Add(Result);
    }
}

HeightmapCreationResult FPLATEAUMeshLoaderForHeightmap::CreateHeightMapFromMesh(
    const plateau::polygonMesh::Mesh& InMesh, const FString NodeName, AActor& Actor, FPLATEAULandscapeParam Param) {

    plateau::heightMapGenerator::HeightmapGenerator generator;
    TVec3d ExtMin, ExtMax;
    TVec2f UVMin, UVMax;
    TVec2d Offset(Param.Offset.X, Param.Offset.Y);
    std::vector<uint16_t> heightMapData = generator.generateFromMesh(InMesh, Param.TextureWidth, Param.TextureHeight, Offset, 
        plateau::geometry::CoordinateSystem::ESU, Param.FillEdges, Param.ApplyBlurFilter, ExtMin, ExtMax, UVMin, UVMax);
    
    // Heightmap Image Output 
    FPLATEAUReconstructUtil::SaveHeightmapImage(Param.HeightmapImageOutput, "HM_" + NodeName , Param.TextureWidth, Param.TextureHeight, heightMapData.data());

    //Texture
    FString TexturePath;
    const auto& subMeshes = InMesh.getSubMeshes();
    if (subMeshes.size() > 0) {
        const auto& subMesh = subMeshes.at(0);
        TexturePath = FString(subMesh.getTexturePath().c_str());
    }

    TSharedPtr<std::vector<uint16_t>> sharedData = MakeShared<std::vector<uint16_t>>(heightMapData);
    HeightmapCreationResult Result{ NodeName, sharedData ,ExtMin, ExtMax , UVMin, UVMax, TexturePath };
    return Result;
}


void FPLATEAUMeshLoaderForHeightmap::CreateReference(ALandscape* Landscape, AActor* Actor, const FString NodeName) {
    const FString ReplacedNodeName = NodeName.Replace(*FString("Mesh_"), *FString()); //Mesh Prefix ����
    auto OriginalComponent = FPLATEAUComponentUtil::GetCityObjectGroupByName(Actor, ReplacedNodeName);
    if (OriginalComponent) {
        const auto& OriginalParentComponent = OriginalComponent->GetAttachParent();
        auto RefComponent = (UPLATEAULandscapeRefComponent*)Actor->AddComponentByClass(UPLATEAULandscapeRefComponent::StaticClass(), false, FTransform(), false);

        // Originalコンポーネントの属性をそのまま利用
        if (OriginalComponent) {
            RefComponent->SerializedCityObjects = OriginalComponent->SerializedCityObjects;
            RefComponent->OutsideChildren = OriginalComponent->OutsideChildren;
            RefComponent->OutsideParent = OriginalComponent->OutsideParent;
            RefComponent->MeshGranularityIntValue = OriginalComponent->MeshGranularityIntValue;  
        }

        RefComponent->LandscapeReference = Landscape;
        RefComponent->SetMobility(EComponentMobility::Type::Static);

        auto NewName = "Ref_" + NodeName;
        RefComponent->Rename(*NewName, nullptr, REN_DontCreateRedirectors);      
        Actor->AddInstanceComponent(RefComponent);
        RefComponent->RegisterComponent();
        RefComponent->AttachToComponent(OriginalParentComponent, FAttachmentTransformRules::KeepWorldTransform);

#if WITH_EDITOR
        RefComponent->PostEditChange();
#endif
    }
}

bool FPLATEAUMeshLoaderForHeightmap::UseCachedMaterial() {
    return false;
}

