// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "PLATEAUMeshLoaderForHeightmap.h"

//地形を平滑化されたMeshに変換します
class PLATEAURUNTIME_API FPLATEAUMeshLoaderForLandscapeMesh : public FPLATEAUMeshLoaderForHeightmap {

public:
    FPLATEAUMeshLoaderForLandscapeMesh();
    FPLATEAUMeshLoaderForLandscapeMesh(const bool InbAutomationTest);

    void CreateMeshFromHeightMap(AActor& Actor, const int32 SizeX, const int32 SizeY, 
        const TVec3d Min, const TVec3d Max, 
        const TVec2f MinUV, const TVec2f MaxUV, 
        uint16_t* HeightRawData, 
        const FString NodeName);

protected:
    UStaticMeshComponent* GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
        const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData,
        const std::shared_ptr <const citygml::CityModel> CityModel) override;
    UMaterialInterface* GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component, const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) override;

    bool OverwriteTexture() override;
    bool InvertMeshNormal() override;
    bool MergeTriangles() override;
    void ModifyMeshDescription(FMeshDescription& MeshDescription) override;

private:
    UMaterialInterface* ReplaceMaterial;

};

