// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUMeshLoader.h"
#include "PLATEAUMeshLoaderForReconstruct.h"

/**
* @brief 元のコンポーネントから属性、マテリアル, 分割をコピーして利用
* 元のコンポーネントと同一階層にコピーを生成(親階層は既存のものをそのまま利用）
*/
class PLATEAURUNTIME_API FPLATEAUMeshLoaderCloneComponent : public FPLATEAUMeshLoaderForReconstruct {

public:
    FPLATEAUMeshLoaderCloneComponent(const FPLATEAUCachedMaterialArray& CachedMaterials);
    FPLATEAUMeshLoaderCloneComponent(const bool InbAutomationTest, const FPLATEAUCachedMaterialArray& CachedMaterials);

    void ReloadComponentFromModel(
        std::shared_ptr<plateau::polygonMesh::Model> Model,
        TMap<FString, UPLATEAUCityObjectGroup*> Components,
        AActor& InActor);     

    void ReloadComponentFromNode(
        const plateau::polygonMesh::Node& InNode,
        TMap<FString, UPLATEAUCityObjectGroup*> Components,
        AActor& InActor);

    USceneComponent* ReloadNode(
        USceneComponent* ParentComponent,
        const plateau::polygonMesh::Node& Node,
        ConvertGranularity Granularity,
        AActor& Actor) override;

    //Meshを結合しSmoothnessを有効にします
    void SetSmoothing(bool bSmooth);

protected:

    virtual UStaticMeshComponent* GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
        const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData,
        const std::shared_ptr <const citygml::CityModel> CityModel) override;

    virtual UMaterialInterface* GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) override;

    UPLATEAUCityObjectGroup* GetOriginalComponent(FString NodePathString);

    virtual bool UseCachedMaterial() override;
    virtual bool MergeTriangles() override;
    virtual void ModifyMeshDescription(FMeshDescription& MeshDescription) override;

private:

    //元のコンポーネント情報を保持　
    TMap<FString, UPLATEAUCityObjectGroup*> ComponentsMap;

    bool IsSmooth = false;
};

