// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include <memory>

#include "RGraph.h"
#include "RGraphFactory.generated.h"
struct FSubDividedCityObject;
class UPLATEAUCityObjectGroup;
class URnModel;
class URGraph;

USTRUCT(BlueprintType)
struct FRGraphFactory
{
    GENERATED_BODY();
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory")
    bool bUseCityObjectOutline = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    float MergeCellSize = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    int32 MergeCellLength = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    float RemoveMidPointTolerance = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    float Lod1HeightTolerance = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptAdjustSmallLodHeight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptEdgeReduction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptVertexReduction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptRemoveIsolatedEdgeFromFace = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptInsertVertexInNearEdge = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptSeparateFaces = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bOptModifySideWalkShape = true;

    // 全く同じ辺を持つFaceを統合する
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Factory|Optimize")
    bool bFaceReduction = true;
};

struct FRGraphFactoryEx
{
    static RGraphRef_t<URGraph> CreateGraph(const FRGraphFactory& Factory, const TArray<FSubDividedCityObject>& CityObjects);
};
