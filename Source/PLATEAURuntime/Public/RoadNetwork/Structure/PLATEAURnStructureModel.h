// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include "PLATEAURnModelDrawerDebug.h"
#include "RoadNetwork/Factory/RoadNetworkFactory.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "PLATEAURnStructureModel.generated.h"

class APLATEAUInstancedCityModel;
class AActor;
class URnModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateRnModelFinishedDelegate);

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API APLATEAURnStructureModel : public AActor {
    GENERATED_BODY()
public:
    APLATEAURnStructureModel();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PLATEAU")
    FRoadNetworkFactory Factory;

    UPROPERTY(VisibleAnywhere, Category = "PLATEAU|Model")
    URnModel* Model;

    UPROPERTY(EditAnywhere, Category = "PLATEAU|Debug")
    FPLATEAURnModelDrawerDebug Debug;

    /**
     * @brief 道路構造生成処理終了イベント
     */
    UPROPERTY(BlueprintAssignable, Category = "PLATEAU|BPLibraries")
    FOnCreateRnModelFinishedDelegate OnCreateRnModelFinished;

    /**
     * @brief 道路構造の生成を行います
     * @param
     */
    UE::Tasks::TTask<APLATEAURnStructureModel*> CreateRnModelAsync(APLATEAUInstancedCityModel* TargetActor);
public:
    virtual void Tick(float DeltaTime) override;
};
