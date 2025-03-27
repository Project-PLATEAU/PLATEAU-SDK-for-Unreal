// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include <citygml/material.h>
#include "MeshTypes.h"
#include "MeshDescription.h"
#include <plateau/polygon_mesh/mesh.h>
#include "StaticMeshAttributes.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

struct FPLATEAUCityObject;
struct FLoadInputData;
class UPLATEAUCityObjectGroup;
class FStaticMeshAttributes;

namespace citygml {
    class CityModel;
}

namespace plateau::polygonMesh {
    enum class MeshGranularity;
    class Model;
    class Node;
    class SubMesh;
    class Mesh;
}

// SubMesh情報保持
struct FSubMeshMaterialSet {
public:
    bool hasMaterial;
    FVector3f Diffuse;
    FVector3f Specular;
    FVector3f Emissive;
    float Shininess;
    float Transparency;
    float Ambient;
    bool isSmooth;
    FString TexturePath;
    FPolygonGroupID PolygonGroupID = 0;
    FString MaterialSlot = FString("");
    int GameMaterialID = 0;

    FSubMeshMaterialSet();
    FSubMeshMaterialSet(std::shared_ptr<const citygml::Material> mat, FString texPath, int matId);
    bool operator==(const FSubMeshMaterialSet& Other) const;
    bool Equals(const FSubMeshMaterialSet& Other) const;
private:
};
FORCEINLINE uint32 GetTypeHash(const FSubMeshMaterialSet& Value);

// Nodeから、Node名、Nodeパスを取得し保持
struct FNodeHierarchy {
public:
    FString NodeName;
    FString NodePath;
    FString RootNodeName;
    FNodeHierarchy();
    FNodeHierarchy(FString Name);
    FNodeHierarchy(const plateau::polygonMesh::Node& InNode);
    std::string GetNameAsStandardString();
};

class PLATEAURUNTIME_API FPLATEAUMeshLoader {
    using FPathToTexture = TMap<FString, UTexture2D*>;
public:
    virtual ~FPLATEAUMeshLoader() = default;

    FPLATEAUMeshLoader() 
    {
        bAutomationTest = false;
    }
    FPLATEAUMeshLoader(const bool InbAutomationTest)
    {
        bAutomationTest = InbAutomationTest;
    }

    void LoadModel(
        AActor* ModelActor,
        USceneComponent* ParentComponent,
        std::shared_ptr<plateau::polygonMesh::Model> Model,
        const FLoadInputData& LoadInputData,
        const std::shared_ptr<const citygml::CityModel> CityModel,
        TAtomic<bool>* bCanceled);

    //前回のロードで作成されたComponentのリストを返します
    TArray<USceneComponent*> GetLastCreatedComponents();

protected:
    // SubMesh情報等に応じてMaterialを作成します。
    virtual UMaterialInterface* GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer);
    // Loaderのタイプに応じて異なるStaticMeshComponentを作成
    virtual UStaticMeshComponent* GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
        const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData, 
        const std::shared_ptr <const citygml::CityModel> CityModel);

    virtual bool UseCachedMaterial(); //マテリアルキャッシュ有効・無効
    virtual UMaterialInterface* GetPreCachedMaterial(int32 MaterialId); //事前にキャッシュされたマテリアル使用
    virtual bool InvertMeshNormal(); //Mesh反転有無
    virtual bool MergeTriangles(); //Triangle生成時にVertexIDを結合・分割

protected:
    bool bAutomationTest;
    TArray<UStaticMesh*> StaticMeshes;
    TMap<FSubMeshMaterialSet, UMaterialInterface*> CachedMaterials;

    /// 何度も同じテクスチャをロードすると重いので使い回せるように覚えておきます
     FPathToTexture PathToTexture;

    // 前回のLoadModel, ReloadComponentFromNode実行時に作成されたComponentを保持しておきます
    TArray<USceneComponent*> LastCreatedComponents;

    virtual UStaticMeshComponent* CreateStaticMeshComponent(
        AActor& Actor,
        USceneComponent& ParentComponent,
        const plateau::polygonMesh::Mesh& InMesh,
        const FLoadInputData& LoadInputData,
        const std::shared_ptr<const citygml::CityModel> CityModel,
        FNodeHierarchy NodeHier);
    USceneComponent* LoadNode(
        USceneComponent* ParentComponent,
        const plateau::polygonMesh::Node& Node,
        const FLoadInputData& LoadInputData,
        const std::shared_ptr<const citygml::CityModel> CityModel,
        AActor& Actor);
    void LoadNodeRecursive(
        USceneComponent* InParentComponent,
        const plateau::polygonMesh::Node& InNode,
        const FLoadInputData& InLoadInputData,
        const std::shared_ptr<const citygml::CityModel> InCityModel,
        AActor& InActor);

    //MeshDescriptionの上書き
    virtual void ModifyMeshDescription(FMeshDescription& MeshDescription);
    //既存のTextureを上書きするか
    virtual bool OverwriteTexture();

    virtual void ComputeNormals(FStaticMeshAttributes& Attributes, bool InvertNormal);
    virtual bool ConvertMesh(const plateau::polygonMesh::Mesh& InMesh, FMeshDescription& OutMeshDescription,
        TArray<FSubMeshMaterialSet>& SubMeshMaterialSets, bool InvertNormal, bool MergeTriangles);
    virtual UStaticMesh* CreateStaticMesh(const plateau::polygonMesh::Mesh& InMesh, UObject* InOuter, FName Name);

};
