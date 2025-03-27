// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include <optional>
#include <plateau/polygon_mesh/model.h>

#include "CityGML/PLATEAUCityObject.h"
#include "RoadNetwork/RGraph/RGraphDef.h"
#include "SubDividedCityObject.generated.h"
class UPLATEAUCityObjectGroup;
class URnLineString;
class URnPoint;
struct FAttributeDataHelper;

USTRUCT(BlueprintType)
struct FSubDividedCityObjectSubMesh {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<int32> Triangles;

    TArray<FSubDividedCityObjectSubMesh> Separate() const;
    TArray<TArray<int32>> CreateOutlineIndices() const;
    FSubDividedCityObjectSubMesh DeepCopy() const;
};

USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FSubDividedCityObjectMesh {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<FVector> Vertices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<FSubDividedCityObjectSubMesh> SubMeshes;

    void VertexReduction();
    void Separate();
    FSubDividedCityObjectMesh DeepCopy() const;
};

USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FSubDividedCityObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FString SerializedCityObjects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FPLATEAUCityObject CityObject;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<FSubDividedCityObjectMesh> Meshes;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<FSubDividedCityObject> Children;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;

    // 自身の道路タイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    ERRoadTypeMask SelfRoadType;

    // 親の道路タイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    ERRoadTypeMask ParentRoadType;

    ERRoadTypeMask GetRoadType(bool ContainsParent) const;
    TArray<const FSubDividedCityObject*> GetAllChildren() const;
    void SetCityObjectGroup(UPLATEAUCityObjectGroup* Group);
    TSharedPtr<FSubDividedCityObject> DeepCopy();

    static ERRoadTypeMask GetRoadTypeFromCityObject(const FPLATEAUCityObject& CityObject);

    FSubDividedCityObject()
    : CityObject()
    , SelfRoadType()
    , ParentRoadType()
    {}

    FSubDividedCityObject(UPLATEAUCityObjectGroup* Co, const plateau::polygonMesh::Model& PlateauModel, TMap<FString, FPLATEAUCityObject>& CityObj);
    FSubDividedCityObject(UPLATEAUCityObjectGroup* Co, const plateau::polygonMesh::Node& PlateauNode, TMap<FString, FPLATEAUCityObject>& CityObj, ERRoadTypeMask ParentTypeMask);
};
