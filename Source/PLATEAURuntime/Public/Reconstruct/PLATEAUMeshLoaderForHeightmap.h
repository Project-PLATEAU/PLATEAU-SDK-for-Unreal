// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUMeshLoader.h"
#include "PLATEAUGeometry.h"
#include "PLATEAUCityModelLoader.h"
#include "Landscape.h"
#include "PLATEAUMeshLoaderForHeightmap.generated.h"

UENUM(BlueprintType)
enum class EPLATEAULandscapeHeightmapImageOutput : uint8 {
    None = 0 UMETA(DisplayName = "None"),
    PNG = 1 UMETA(DisplayName = "PNG"),
    RAW = 2 UMETA(DisplayName = "RAW"),
    PNG_RAW = 3 UMETA(DisplayName = "PNG & RAW")
};

//地形平滑化入力パラメータ
USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FPLATEAULandscapeParam {
    GENERATED_BODY()

    /*
    Recommended Landscape Sizes
    Overall size(vertices)	Quads / section	Sections / Component	Landscape Component size	Total Landscape Components
    8129 x 8129	127	4 (2x2)	254x254	1024 (32x32)
    4033 x 4033	63	4 (2x2)	126x126	1024 (32x32)
    2017 x 2017	63	4 (2x2)	126x126	256 (16x16)
    1009 x 1009	63	4 (2x2)	126x126	64 (8x8)
    1009 x 1009	63	1	63x63	256 (16x16)
    505 x 505	63	4 (2x2)	126x126	16 (4x4)
    505 x 505	63	1	63x63	64 (8x8)
    253 x 253	63	4 (2x2)	126x126	4 (2x2)
    253 x 253	63	1	63x63	16 (4x4)
    127 x 127	63	4 (2x2)	126x126	1
    127 x 127	63	1	63x63	4 (2x2)
    */

    FPLATEAULandscapeParam() :
        TextureWidth(505), //
        TextureHeight(505),
        NumSubsections(2), //1 , 2
        SubsectionSizeQuads(63), //7, 15, 31, 63, 127, 255
        ComponentCountX(126),
        ComponentCountY(126),
        Offset(0,0),
        ConvertTerrain(true),
        ConvertToLandscape(false),
        ApplyBlurFilter(true),
        FillEdges(true),
        AlignLand(true),
        InvertRoadLod3(true),
        HeightmapImageOutput(EPLATEAULandscapeHeightmapImageOutput::None){}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 TextureWidth;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 TextureHeight;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 NumSubsections; //1 , 2
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 SubsectionSizeQuads;  //7, 15, 31, 63, 127, 255
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 ComponentCountX;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        int32 ComponentCountY;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        FVector2D Offset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool ConvertTerrain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool ConvertToLandscape;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool ApplyBlurFilter;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool FillEdges;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool AlignLand;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        bool InvertRoadLod3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|BPLibraries|Landscape")
        EPLATEAULandscapeHeightmapImageOutput HeightmapImageOutput;
};

//ハイトマップ生成Resultデータ
struct  HeightmapCreationResult {
    FString NodeName;
    TSharedPtr<std::vector<uint16_t>> Data;
    TVec3d Min;
    TVec3d Max;
    TVec2f MinUV;
    TVec2f MaxUV;
    FString TexturePath;
};

class PLATEAURUNTIME_API FPLATEAUMeshLoaderForHeightmap : public FPLATEAUMeshLoader {

public:
    FPLATEAUMeshLoaderForHeightmap();
    FPLATEAUMeshLoaderForHeightmap(const bool InbAutomationTest);

    TArray<HeightmapCreationResult> CreateHeightMap(
        AActor* ModelActor,
        const std::shared_ptr<plateau::polygonMesh::Model> Model, FPLATEAULandscapeParam Param);

    //LandscapeのReference Componentを元のDemの階層に生成します
    void CreateReference(ALandscape* Landscape, AActor* Actor, const FString NodeName);

protected:

    void LoadNodeRecursiveForHeightMap(
        const plateau::polygonMesh::Node& InNode,
        AActor& InActor, FPLATEAULandscapeParam Param, TArray<HeightmapCreationResult> &Results);
    void LoadNodeForHeightMap(
        const plateau::polygonMesh::Node& Node,
        AActor& Actor, FPLATEAULandscapeParam Param, TArray<HeightmapCreationResult> &Results);
    HeightmapCreationResult CreateHeightMapFromMesh(
        const plateau::polygonMesh::Mesh& InMesh,
        const FString NodeName,
        AActor& Actor, FPLATEAULandscapeParam Param);

    virtual bool UseCachedMaterial() override;


private:

};

