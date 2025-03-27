// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "Components/SplineComponent.h"
#include "PLATEAUReproducedRoad.generated.h"

enum class EPLATEAURoadLineType : uint8;
/**
* @brief 道路の両端に配置される道路標示（横断歩道）において、道路のどちら側に配置されたかを示します。
* どちらでもない道路標示はNoneとなります。
*/
UENUM(BlueprintType)
enum class EPLATEAUReproducedRoadDirection : uint8 {
    None UMETA(DisplayName = "None"),
    Prev UMETA(DisplayName = "Prev"),
    Next UMETA(DisplayName = "Next"),
};

/**
* @brief Road Line 生成用パラメータ
*
*/
USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FPLATEAURoadLineParam {
    GENERATED_BODY()

    FPLATEAURoadLineParam() : Type(), LineMesh(), LineMaterial(), SplinePointType(), FillEnd(), LineGap(), LineXScale(), LineLength(){
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    EPLATEAURoadLineType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    UStaticMesh* LineMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    UMaterialInterface* LineMaterial;

    //Splineタイプ (CurveCustomTangent/Linearなど）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    TEnumAsByte<ESplinePointType::Type> SplinePointType;

    //SplineMeshType長さ単位の場合、終点位置までMesh生成
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    bool FillEnd;

    //Mesh間の隙間
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    float LineGap;

    //元メッシュの幅に対する倍率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    float LineXScale;

    //この値が0の場合アセットMeshのBoundsのサイズを利用
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    float LineLength;
};

/// 道路ネットワークを元に道路の見た目を生成します。
UCLASS()
class PLATEAURUNTIME_API APLATEAUReproducedRoad : public AActor {
    GENERATED_BODY()
public:
    APLATEAUReproducedRoad();

    /// 道路標示を生成します。
    UFUNCTION(BlueprintCallable, meta = (Category = "PLATEAU|RoadAdjust"))
    void CreateRoadMarks(APLATEAURnStructureModel* Model, FString CrosswalkFrequency);


protected:
    // Called when the game starts or when spawneds
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

private:

    int32 NumComponents;
    TMap<EPLATEAURoadLineType, FPLATEAURoadLineParam> LineTypeMap;

    void CreateLineTypeMap();
    void CreateLineComponentByType(EPLATEAURoadLineType Type, const TArray<FVector>& LinePoints, FVector2D Offset = FVector2D::Zero());
};
