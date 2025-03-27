// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "RoadNetwork/PLATEAURnDef.h"
#include "PLATEAUInstancedCityModel.h"
#include "SubDividedCityObject.h"

class UPLATEAUCityObjectGroup;
class URnLineString;
class URnPoint;
class PLATEAURUNTIME_API FSubDividedCityObjectFactory 
{
public:
    class FCityObjectInfo {
    public:
        TWeakObjectPtr<USceneComponent> Transform;
        UStaticMesh* UeMesh;
        TWeakObjectPtr<UPLATEAUCityObjectGroup> CityObjectGroup;
        UStaticMesh* OriginalMesh;

        static TSharedPtr<FCityObjectInfo> Create(UPLATEAUCityObjectGroup* Cog, bool UseContourMesh);
    };

    class FConvertCityObjectResult {
    public:
        TArray<TSharedPtr<FSubDividedCityObject>> ConvertedCityObjects;

        FConvertCityObjectResult() {
        }
    };

    TSharedPtr<FConvertCityObjectResult> ConvertCityObjectsAsync(
        APLATEAUInstancedCityModel* Actor,
        const TArray<UPLATEAUCityObjectGroup*>& CityObjectGroups,
        float Epsilon = 0.1f,
        bool UseContourMesh = true);

private:
#if false
    void ReloadComponentFromNode(
        USceneComponent* InParentComponent,
        const plateau::polygonMesh::Node& InNode,
        ConvertGranularity Granularity,
        TMap<FString, FPLATEAUCityObject> CityObj,
        AActor& InActor)
    {

        CityObjMap = CityObj;
        ConvGranularity = Granularity;
        LastCreatedComponents.Empty();

        ReloadNodeRecursive(InParentComponent, InNode, Granularity, InActor);

        // メッシュをワールド内にビルド
        const auto CopiedStaticMeshes = StaticMeshes;
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [CopiedStaticMeshes]() {
                UStaticMesh::BatchBuild(CopiedStaticMeshes, true, [](UStaticMesh* mesh) {return false; });
            }, TStatId(), nullptr, ENamedThreads::GameThread)->Wait();
        StaticMeshes.Reset();
    }

    void ReloadNodeRecursive(
        USceneComponent* InParentComponent,
        const plateau::polygonMesh::Node& InNode,
        ConvertGranularity Granularity,
        AActor& InActor) {
        const auto Component = ReloadNode(InParentComponent, InNode, Granularity, InActor);
        const size_t ChildNodeCount = InNode.getChildCount();
        for (int i = 0; i < ChildNodeCount; i++) {
            const auto& TargetNode = InNode.getChildAt(i);
            ReloadNodeRecursive(Component, TargetNode, Granularity, InActor);
        }
    }

    USceneComponent* ReloadNode(USceneComponent* ParentComponent,
        const plateau::polygonMesh::Node& Node,
        ConvertGranularity Granularity,
        AActor& Actor) {
        if (Node.getMesh() == nullptr || Node.getMesh()->getVertices().size() == 0) {
            USceneComponent* Comp = nullptr;
            UClass* StaticClass;
            const FString DesiredName = FString(UTF8_TO_TCHAR(Node.getName().c_str()));
            const FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&, DesiredName] {

                // すでに同名、同階層のComponentが存在する場合は再利用
                if (const auto ExistComponent = FindChildComponentWithOriginalName(ParentComponent, DesiredName)) {
                    Comp = ExistComponent;
                    return;
                }

                StaticClass = UPLATEAUCityObjectGroup::StaticClass();
                const auto& PLATEAUCityObjectGroup = NewObject<UPLATEAUCityObjectGroup>(&Actor, NAME_None);
                Comp = PLATEAUCityObjectGroup;

                auto cityObjRef = CityObjMap.Find(DesiredName);
                if (cityObjRef != nullptr) {
                    const FPLATEAUCityObject cityObj = *cityObjRef;
                    PLATEAUCityObjectGroup->SerializeCityObject(Node, cityObj, Granularity);
                }

                const FString NewUniqueName = MakeUniqueGmlObjectName(&Actor, StaticClass, DesiredName);
                Comp->Rename(*NewUniqueName, nullptr, REN_DontCreateRedirectors);

                check(Comp != nullptr);
                if (bAutomationTest) {
                    Comp->Mobility = EComponentMobility::Movable;
                }
                else {
                    Comp->Mobility = EComponentMobility::Static;
                }

                Actor.AddInstanceComponent(Comp);
                Comp->RegisterComponent();
                Comp->AttachToComponent(ParentComponent, FAttachmentTransformRules::KeepWorldTransform);
                }, TStatId(), nullptr, ENamedThreads::GameThread);
            Task->Wait();
            return Comp;
        }

        plateau::polygonMesh::MeshExtractOptions MeshExtractOptions{};
        MeshExtractOptions.mesh_granularity = ConvertGranularityToMeshGranularity(Granularity);
        FLoadInputData LoadInputData
        {
            MeshExtractOptions,
            std::vector<plateau::geometry::Extent>{},
            FString(),
            false,
            nullptr
        };
        return CreateStaticMeshComponent(Actor, *ParentComponent, *Node.getMesh(), LoadInputData, nullptr,
            Node.getName());
    }
#endif

};