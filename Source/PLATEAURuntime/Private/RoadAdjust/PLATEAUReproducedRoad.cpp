// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/PLATEAUReproducedRoad.h"

#include "Kismet/GameplayStatics.h"
#include "RoadAdjust/PLATEAURoadLineType.h"
#include "RoadAdjust/RoadMarking/PLATEAUDirectionalArrowComposer.h"
#include "RoadAdjust/RoadMarking/LineGeneratorComponent.h"
#include "RoadAdjust/RoadMarking/PLATEAUCrosswalkComposer.h"
#include "RoadAdjust/RoadMarking/PLATEAUMarkedWayListComposerMain.h"
#include "RoadAdjust/PLATEAUCrosswalkPlacementRule.h"
#include "RoadAdjust/RoadNetworkToMesh/PLATEAURrTarget.h"
#include "RoadMarking/LineSmoother.h"
#include "RoadNetwork/Structure/RnModel.h"
#include "RoadNetwork/Structure/PLATEAURnStructureModel.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/World.h"

APLATEAUReproducedRoad::APLATEAUReproducedRoad() {
    CreateLineTypeMap();
    auto SceneRootComponent = CreateDefaultSubobject<UPLATEAUSceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());
    SceneRootComponent->SetMobility(EComponentMobility::Static);
    // SceneRootComponent->RegisterComponent();
    this->AddInstanceComponent(SceneRootComponent);
    this->SetRootComponent(SceneRootComponent);
}


void APLATEAUReproducedRoad::CreateLineTypeMap() {
    const TCHAR* LineMeshPath = TEXT("/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/simple_line");
    const auto LineMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, LineMeshPath));
    const TCHAR* TileMeshPath = TEXT("/PLATEAU-SDK-for-Unreal/RoadNetwork/Meshes/simple_tile");
    const auto TileMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TileMeshPath));
    
    LineTypeMap.Add(EPLATEAURoadLineType::WhiteLine, PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType::WhiteLine, LineMesh, TileMesh));
    LineTypeMap.Add(EPLATEAURoadLineType::YellowLine, PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType::YellowLine, LineMesh, TileMesh));
    LineTypeMap.Add(EPLATEAURoadLineType::DashedWhilteLine, PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType::DashedWhilteLine, LineMesh, TileMesh));
    LineTypeMap.Add(EPLATEAURoadLineType::StopLine, PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType::StopLine, LineMesh, TileMesh));
    LineTypeMap.Add(EPLATEAURoadLineType::Crossing, PLATEAURoadLineTypeExtension::ToRoadLineParam(EPLATEAURoadLineType::Crossing, LineMesh, TileMesh));
}

void APLATEAUReproducedRoad::CreateRoadMarks(APLATEAURnStructureModel* Model, FString CrosswalkFrequency) {
    
    // すでに生成済みの道路標示があれば削除します
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(Cast<UObject>(GetWorld()), APLATEAUReproducedRoad::StaticClass(), FoundActors);
    for (AActor* Actor : FoundActors) {
        if (Actor != this) {
            Actor->Destroy();
        }
    }

    auto ProgressDialogue = FScopedSlowTask(10, FText::FromString(TEXT("処理中...")));
    ProgressDialogue.MakeDialog(false);

    auto RnModel = Model->Model;

    // 道路ネットワークのスムージング。
    // 道路ネットワークのコピーは未実装なので、スムージングは破壊的変更になります。
    FString ProgressSmooth = FString(TEXT("道路ネットワークのスムージング中"));
    ProgressDialogue.EnterProgressFrame(1, FText::FromString(ProgressSmooth));
    PLATEAU::RoadAdjust::RoadMarking::FRoadNetworkLineSmoother Smoother;
    Smoother.Smooth(RnModel, PLATEAU::RoadAdjust::RoadMarking::FSmoothingStrategyRespectOriginal());

    // ターゲットとなる道路ネットワークの定義
    auto TargetModel = NewObject<UPLATEAURrTargetModel>(GetTransientPackage(), UPLATEAURrTargetModel::StaticClass());
    TargetModel->Initialize(RnModel);

    // 白線生成の対象と種類を定義
    FString ProgressMarkedWay = FString(TEXT("白線対象を収集中"));
    ProgressDialogue.EnterProgressFrame(1, FText::FromString(ProgressMarkedWay));
    auto WayComposer = NewObject<UPLATEAUMarkedWayListComposerMain>(GetTransientPackage(), UPLATEAUMarkedWayListComposerMain::StaticClass());
    auto MarkedWays = WayComposer->ComposeFrom(TargetModel).GetMarkedWays();

    // 横断歩道
    FString ProgressCrosswalk = FString(TEXT("横断歩道の場所を計算中"));
    ProgressDialogue.EnterProgressFrame(1, FText::FromString(ProgressCrosswalk));
    auto CrossRoads = NewObject<UPLATEAUCrosswalkComposer>()->Compose(*TargetModel, FPLATEAUCrosswalkFrequencyExtensions::StrToFrequency(CrosswalkFrequency));
    MarkedWays.Append(CrossRoads.GetMarkedWays());
    
    // 白線を生成
    for(int i=0; i<MarkedWays.Num(); i++)
    {
        const auto& MarkedWay = MarkedWays[i];
        FString ProgressGenMarkedWay = FString::Printf(TEXT("白線を生成中(%d/%d)"), i, MarkedWays.Num()); 
        ProgressDialogue.EnterProgressFrame(0, FText::FromString(ProgressGenMarkedWay));
        const auto& Points = MarkedWay.GetLine().GetPoints();
        const auto Type = MarkedWay.GetRoadLineType();
        CreateLineComponentByType(Type, Points, FVector2d(0.0f, 0.0f));
    }

    // 車線矢印を生成
    FString ProgressArrow = FString(TEXT("車線の矢印を生成中"));
    ProgressDialogue.EnterProgressFrame(1, FText::FromString(ProgressArrow));
    auto ArrowComposer = FPLATEAUDirectionalArrowComposer(RnModel, this);
    ArrowComposer.Compose();
}

void APLATEAUReproducedRoad::CreateLineComponentByType(EPLATEAURoadLineType Type, const TArray<FVector>& LinePoints, FVector2D Offset) {
    const auto& Param = LineTypeMap[Type];
    const auto& Component = NewObject<ULineGeneratorComponent>(this, FName(TEXT("LineGeneratorComponent_") + StaticEnum<EPLATEAURoadLineType>()->GetDisplayValueAsText(Type).ToString() + TEXT("_") + FString::FromInt(NumComponents)));
    Component->RegisterComponent();
    this->AddInstanceComponent(Component);
    Component->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);  
    
    Component->Init(LinePoints, Param, Offset);
    
    // メッシュの生成
    Component->CreateSplineMeshFromAssets(this, Param.LineMesh, Param.LineMaterial, Param.LineGap, Param.LineXScale, Param.LineLength);
    NumComponents++;
}

void APLATEAUReproducedRoad::BeginPlay() {
    Super::BeginPlay();
}

void APLATEAUReproducedRoad::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}
