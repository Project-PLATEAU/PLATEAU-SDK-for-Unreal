// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "PLATEAUCachedMaterialArray.h"
#include "PLATEAUMeshLoaderForReconstruct.h"

class PLATEAURUNTIME_API FPLATEAUMeshLoaderForClassification : public FPLATEAUMeshLoaderForReconstruct {

public:
    FPLATEAUMeshLoaderForClassification(const FPLATEAUCachedMaterialArray& Mats);
    FPLATEAUMeshLoaderForClassification(const FPLATEAUCachedMaterialArray& Mats, const bool InbAutomationTest);

protected:
    virtual UMaterialInterface* GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) override;
    
private:
    
    //Material分け時のマテリアルリスト。
    // FPLATEAUCachedMaterialArray CachedMaterials;
};
