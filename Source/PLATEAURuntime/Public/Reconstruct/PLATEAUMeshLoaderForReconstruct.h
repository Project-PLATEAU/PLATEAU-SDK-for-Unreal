// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include <plateau/granularity_convert/granularity_converter.h>
#include "CityGML/PLATEAUCityObject.h"
#include "PLATEAUCachedMaterialArray.h"
#include "CoreMinimal.h"
#include "PLATEAUMeshLoader.h"
#include "Util/PLATEAUReconstructUtil.h"

//結合分離処理用MeshLoader
class PLATEAURUNTIME_API FPLATEAUMeshLoaderForReconstruct : public FPLATEAUMeshLoader {

public:
    FPLATEAUMeshLoaderForReconstruct(const FPLATEAUCachedMaterialArray& CachedMaterials);
    FPLATEAUMeshLoaderForReconstruct(const bool InbAutomationTest, const FPLATEAUCachedMaterialArray& CachedMaterials);

    void ReloadComponentFromModel(
        std::shared_ptr<plateau::polygonMesh::Model> Model,
        ConvertGranularity Granularity,
        TMap<FString, FPLATEAUCityObject> CityObj,
        AActor& InActor);

    void ReloadComponentFromNode(
        USceneComponent* InParentComponent,
        const plateau::polygonMesh::Node& InNode,
        ConvertGranularity Granularity,
        TMap<FString, FPLATEAUCityObject> CityObj,
        AActor& InActor);

protected:

    virtual void ReloadNodeRecursive(
        USceneComponent* InParentComponent,
        const plateau::polygonMesh::Node& InNode,
        ConvertGranularity Granularity,
        AActor& InActor);
    virtual USceneComponent* ReloadNode(
        USceneComponent* ParentComponent,
        const plateau::polygonMesh::Node& Node,
        ConvertGranularity Granularity,
        AActor& Actor);

    UMaterialInterface* GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) override;

    UStaticMeshComponent* GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
        const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData, 
        const std::shared_ptr <const citygml::CityModel> CityModel) override;

    UMaterialInterface* GetPreCachedMaterial(int32 MaterialId) override;
    bool InvertMeshNormal() override;
    bool OverwriteTexture() override;

    //分割・結合時に属性情報を保持　
    TMap<FString, FPLATEAUCityObject> CityObjMap;

    ConvertGranularity ConvGranularity;

protected:
    const FPLATEAUCachedMaterialArray& BeforeConvertCachedMaterials;

};
