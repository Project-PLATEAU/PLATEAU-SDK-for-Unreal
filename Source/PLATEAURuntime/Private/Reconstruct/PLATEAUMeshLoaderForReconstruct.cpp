// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "Reconstruct/PLATEAUMeshLoaderForReconstruct.h"
#include "PLATEAUCityModelLoader.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Materials/MaterialInstance.h"

FPLATEAUMeshLoaderForReconstruct::FPLATEAUMeshLoaderForReconstruct(const FPLATEAUCachedMaterialArray& CachedMaterials) : BeforeConvertCachedMaterials(CachedMaterials) {
    bAutomationTest = false;
}

FPLATEAUMeshLoaderForReconstruct::FPLATEAUMeshLoaderForReconstruct(const bool InbAutomationTest, const FPLATEAUCachedMaterialArray& CachedMaterials) : BeforeConvertCachedMaterials(CachedMaterials) {
    bAutomationTest = InbAutomationTest;
}

void FPLATEAUMeshLoaderForReconstruct::ReloadComponentFromModel(
    std::shared_ptr<plateau::polygonMesh::Model> Model,
    ConvertGranularity Granularity,
    TMap<FString, FPLATEAUCityObject> CityObj,
    AActor& InActor) {
    CityObjMap = CityObj;
    ConvGranularity = Granularity;
    LastCreatedComponents.Empty();

    for (int i = 0; i < Model->getRootNodeCount(); i++) {
        ReloadComponentFromNode(InActor.GetRootComponent(), Model->getRootNodeAt(i), ConvGranularity, CityObjMap, InActor);
    }
}

void FPLATEAUMeshLoaderForReconstruct::ReloadComponentFromNode(
    USceneComponent* InParentComponent,
    const plateau::polygonMesh::Node& InNode,
    ConvertGranularity Granularity,
    TMap<FString, FPLATEAUCityObject> CityObj,
    AActor& InActor) {
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

void FPLATEAUMeshLoaderForReconstruct::ReloadNodeRecursive(
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

USceneComponent* FPLATEAUMeshLoaderForReconstruct::ReloadNode(USceneComponent* ParentComponent,
    const plateau::polygonMesh::Node& Node,
    ConvertGranularity Granularity,
    AActor& Actor) {
    FNodeHierarchy NodeHier(Node);
    if (Node.getMesh() == nullptr || Node.getMesh()->getVertices().size() == 0) {
        USceneComponent* Comp = nullptr;
        UClass* StaticClass;      
        const FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&, NodeHier] {

            // すでに同名、同階層のComponentが存在する場合は再利用
            if (const auto ExistComponent = FPLATEAUComponentUtil::FindChildComponentWithOriginalName(ParentComponent, NodeHier.NodeName)) {
                Comp = ExistComponent;
                return;
            }

            StaticClass = UPLATEAUCityObjectGroup::StaticClass();
            const auto& PLATEAUCityObjectGroup = NewObject<UPLATEAUCityObjectGroup>(&Actor, NAME_None);
            Comp = PLATEAUCityObjectGroup;

            auto cityObjRef = CityObjMap.Find(NodeHier.NodeName);
            if (cityObjRef != nullptr) {
                const FPLATEAUCityObject cityObj = *cityObjRef;
                PLATEAUCityObjectGroup->SerializeCityObject(Node, cityObj, Granularity);
            }

            const FString NewUniqueName = FPLATEAUComponentUtil::MakeUniqueGmlObjectName(&Actor, StaticClass, NodeHier.NodeName);
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
    MeshExtractOptions.mesh_granularity = FPLATEAUReconstructUtil::ConvertGranularityToMeshGranularity(Granularity);
    FLoadInputData LoadInputData
    {
        MeshExtractOptions,
        std::vector<plateau::geometry::Extent>{},
        FString(),
        false,
        nullptr
    };
    return CreateStaticMeshComponent(Actor, *ParentComponent, *Node.getMesh(), LoadInputData, nullptr,
        NodeHier);
}

UMaterialInterface* FPLATEAUMeshLoaderForReconstruct::GetMaterialForSubMesh(const FSubMeshMaterialSet& SubMeshValue, UStaticMeshComponent* Component,
    const FLoadInputData& LoadInputData, UTexture2D* Texture, FNodeHierarchy NodeHier, UObject* Outer) {

    FString TexturePath = SubMeshValue.TexturePath;
    //分割・結合時のFallback Material取得
    if (!TexturePath.IsEmpty()) {
        FString Path, FileName;
        TexturePath.Split("/", &Path, &FileName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        FString FallbackName = UPLATEAUImportSettings::GetFallbackMaterialNameFromDiffuseTextureName(FileName);
        if (!FallbackName.IsEmpty()) {
            FString SourcePath = "/PLATEAU-SDK-for-Unreal/Materials/Fallback/" / FallbackName;
            UMaterialInstance* FallbackMat = Cast<UMaterialInstance>(
                StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
            return Cast<UMaterialInterface>(FallbackMat);
        }
    }
    return FPLATEAUMeshLoader::GetMaterialForSubMesh(SubMeshValue, Component, LoadInputData, Texture, NodeHier, Outer);
}

UStaticMeshComponent* FPLATEAUMeshLoaderForReconstruct::GetStaticMeshComponentForCondition(AActor& Actor, EName Name, FNodeHierarchy NodeHier,
    const plateau::polygonMesh::Mesh& InMesh, const FLoadInputData& LoadInputData,
    const std::shared_ptr <const citygml::CityModel> CityModel) {

    //　分割・結合時は、処理前に保存したCityObjMapからFPLATEAUCityObjectを取得して利用する
    const FString NodeName = NodeHier.NodeName;
    const auto& PLATEAUCityObjectGroup = NewObject<UPLATEAUCityObjectGroup>(&Actor, NAME_None);
    PLATEAUCityObjectGroup->SerializeCityObject(NodeName, InMesh, ConvGranularity, CityObjMap);
    return PLATEAUCityObjectGroup;
}

UMaterialInterface* FPLATEAUMeshLoaderForReconstruct::GetPreCachedMaterial(int32 MaterialId) {
    if (UseCachedMaterial() && 
        MaterialId >= 0 && BeforeConvertCachedMaterials.Num() > 0 &&
        MaterialId < BeforeConvertCachedMaterials.Num())
    {
        return BeforeConvertCachedMaterials.Get(MaterialId);
    }
    return nullptr;
}


bool FPLATEAUMeshLoaderForReconstruct::InvertMeshNormal() {
    return true;
}

bool FPLATEAUMeshLoaderForReconstruct::OverwriteTexture() {
    return false;
}
