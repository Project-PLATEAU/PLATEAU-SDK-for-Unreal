// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include <memory>
//#include "CityGML/PLATEAUCityObject.h"
//#include "Serialization/JsonWriter.h"
//#include "Serialization/JsonReader.h"
//#include "Serialization/JsonSerializer.h"
#include "SubDividedCityObject.h"
#include "../../Component/PLATEAUSceneComponent.h"
#include "PLATEAUSubDividedCityObjectGroup.generated.h"

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API UPLATEAUSubDividedCityObject : public UPLATEAUSceneComponent {
    GENERATED_BODY()
public:

    UPLATEAUSubDividedCityObject() {}
    // --------------------
    // start:フィールド
    // --------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    FSubDividedCityObject CityObject;
};

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API UPLATEAUSubDividedCityObjectGroup : public UPLATEAUSceneComponent
{
    GENERATED_BODY()
public:

    UPLATEAUSubDividedCityObjectGroup();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Debug")
    bool bVisibility = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Debug")
    int32 MeshColorNum;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Debug")
    bool bShowVertexIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Debug")
    bool bShowOutline;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|Debug")
    int32 ShowVertexIndexFontSize;

    TSet<FSubDividedCityObject> TargetCityObjects;

    void DrawMesh(const FSubDividedCityObjectMesh& Mesh, const FSubDividedCityObjectSubMesh& SubMesh,
        const FMatrix& Mat, const FLinearColor& Color = FLinearColor::White, float Duration = 0.0f);

    void DrawCityObjects();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    TArray<UPLATEAUSubDividedCityObject*> GetCityObjects();

    // --------------------
    // start:フィールド
    // --------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
    TArray<UPLATEAUSubDividedCityObject*> CityObjects;
};


