// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "Reconstruct/PLATEAUMeshLoaderForClassification.h"

#include "PLATEAUCachedMaterialArray.h"
#include "PLATEAUMeshLoader.h"
#include "Component/PLATEAUCityObjectGroup.h"

FPLATEAUMeshLoaderForClassification::FPLATEAUMeshLoaderForClassification(const FPLATEAUCachedMaterialArray& Mats) : FPLATEAUMeshLoaderForReconstruct(Mats) {
    bAutomationTest = false;
}

FPLATEAUMeshLoaderForClassification::FPLATEAUMeshLoaderForClassification(const FPLATEAUCachedMaterialArray& Mats, const bool InbAutomationTest) : FPLATEAUMeshLoaderForReconstruct(Mats) {
    bAutomationTest = InbAutomationTest;
}

UMaterialInterface* FPLATEAUMeshLoaderForClassification::GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component,
    const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) {

    UE_LOG(LogTemp, Log, TEXT("GetMaterialForSubMesh: %d"), SubMeshValue.GameMaterialID);

    if (SubMeshValue.GameMaterialID > -1 && BeforeConvertCachedMaterials.Num() > 0 && SubMeshValue.GameMaterialID < BeforeConvertCachedMaterials.Num()) {
        const auto MatPtr = BeforeConvertCachedMaterials.Get(SubMeshValue.GameMaterialID);
        if (MatPtr != nullptr) {
            const auto& Mat = MatPtr;
            if (Mat != nullptr)
            {
                // ここで UMaterialInstanceDynamic::Create(Mat, Component); としてはいけません。
                // なぜなら、マテリアル分けを複数回実行したときに、キー指定対象外の部分でマテリアルが剥がれるためです。
                return Mat;
            }
                
        } 
    }

    //Defaultマテリアル設定時
    const auto& DefaultMaterial = BeforeConvertCachedMaterials.GetDefaultMaterial();
    if (DefaultMaterial != nullptr) {
        return DefaultMaterial;
    }

    return FPLATEAUMeshLoader::GetMaterialForSubMesh(SubMeshValue, Component, LoadInputData, Texture, NodeHier, Outer);
}



