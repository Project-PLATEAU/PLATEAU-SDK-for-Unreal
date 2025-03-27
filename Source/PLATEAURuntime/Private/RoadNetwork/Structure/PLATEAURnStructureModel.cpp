// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/Structure/PLATEAURnStructureModel.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"

APLATEAURnStructureModel::APLATEAURnStructureModel()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<UPLATEAUSceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());
}

void APLATEAURnStructureModel::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (Debug.bVisible) {
        if(!Model)
            Model = GetComponentByClass<URnModel>();
        Debug.Draw(Model);
    }
}

UE::Tasks::TTask<APLATEAURnStructureModel*> APLATEAURnStructureModel::CreateRnModelAsync(APLATEAUInstancedCityModel* TargetActor)
{
#if true
    // 別スレッドで呼び出すとSubDivided周りでアクター生成時にエラーになるので一旦同期タスクにする
    FRoadNetworkFactoryEx::CreateRnModel(Factory, TargetActor, this);
    //終了イベント通知
    OnCreateRnModelFinished.Broadcast();
    UE::Tasks::TTask<APLATEAURnStructureModel*> CreateRnModelTask = UE::Tasks::Launch(
        TEXT("CreateRnModelTask")
        , [this, TargetActor]() {           
            return this;
        });
    return CreateRnModelTask;
#else
    UE::Tasks::TTask<APLATEAURnStructureModel*> CreateRnModelTask = UE::Tasks::Launch(
        TEXT("CreateRnModelTask")
        , [this, TargetActor]() {
            FRoadNetworkFactoryEx::CreateRnModel(Factory, TargetActor, this);
            FFunctionGraphTask::CreateAndDispatchWhenReady([&]() {
                //終了イベント通知
                OnCreateRnModelFinished.Broadcast();
                }, TStatId(), NULL, ENamedThreads::GameThread);
            return this;
        });
    return CreateRnModelTask;
#endif
}