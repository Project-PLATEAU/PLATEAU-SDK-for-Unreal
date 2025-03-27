// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUMeshLoaderCloneComponent.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>
#include "MeshDescription.h"

/// <summary>
/// コンポーネントパラメータコピー利用 MeshLoader (FPLATEAUMeshLoaderCloneComponent) Test
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_MeshLoader_CloneComponent, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.MeshLoader.PLATEAUMeshLoaderCloneComponent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/// <summary>
/// FPLATEAUMeshLoaderCloneComponentの単体テスト
/// 主に階層生成テスト
/// </summary>
bool FPLATEAUTest_MeshLoader_CloneComponent::RunTest(const FString& Parameters) {
    InitializeTest("MeshLoader.PLATEAUMeshLoaderCloneComponent");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    plateau::polygonMesh::Mesh Mesh;
    plateau::polygonMesh::CityObjectList CityObjectList;
    PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectList(CityObjectList);
    PLATEAUAutomationTestUtil::Fixtures::CreateMesh(Mesh, CityObjectList);
    std::shared_ptr<plateau::polygonMesh::Model> Model = PLATEAUAutomationTestUtil::Fixtures::CreateModel(Mesh);

    const auto& Actor = PLATEAUAutomationTestUtil::Fixtures::CreateActor(*GetWorld());
    const auto LoadData = PLATEAUAutomationTestUtil::Fixtures::CreateLoadInputData(plateau::polygonMesh::MeshGranularity::PerPrimaryFeatureObject);
    const ConvertGranularity ConvGranularity = ConvertGranularity::PerPrimaryFeatureObject;
    TMap<FString, FPLATEAUCityObject> CityObj = PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectMap();
        
    //CityObjectGroup Item
    auto OriginalItem = Actor->FindComponentByTag<UPLATEAUCityObjectGroup>(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);
    OriginalItem->SerializeCityObject(PLATEAUAutomationTestUtil::Fixtures::GetObjNode(Model), CityObj[PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME], ConvertGranularity::PerPrimaryFeatureObject);
    TMap<FString, UPLATEAUCityObjectGroup*> CompMap = FPLATEAUComponentUtil::CreateComponentsMapWithNodePath(TArray<UPLATEAUCityObjectGroup*>{OriginalItem});
    OriginalItem->SetStaticMesh(PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(Actor, FName(TEXT("TestStaticMesh"))));
    PLATEAUAutomationTestUtil::Fixtures::SetMaterial(OriginalItem);

    TAtomic<bool> bCanceled;
    bCanceled.Store(false);
    auto MeshLoader = FPLATEAUMeshLoaderCloneComponent(FPLATEAUCachedMaterialArray());

    MeshLoader.ReloadComponentFromModel(Model, CompMap, *Actor);

    AddInfo(FString(Model->debugString().c_str()));

    GEngine->BroadcastLevelActorListChanged();

    //Assertions
    const auto DefaultSceneRootComp = Actor->GetRootComponent();
    const auto RootComp = DefaultSceneRootComp->GetAttachChildren()[0];
    const auto LodComp = RootComp->GetAttachChildren()[0];
    const auto ObjComps = LodComp->GetAttachChildren();

    ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f));

    TestEqual("Obj should be 2", ObjComps.Num() , 2 );
    for (auto Obj : ObjComps) {
        TestEqual("Original Compoenent Name = Node Name", FPLATEAUComponentUtil::GetOriginalComponentName(Obj), PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

        UPLATEAUCityObjectGroup* CityObjGrp = StaticCast<UPLATEAUCityObjectGroup*>(Obj);
        TestEqual("GmlID = Node Name", CityObjGrp->GetAllRootCityObjects()[0].GmlID, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

        //新規に生成されたComponent
        if (CityObjGrp->GetName() == PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME + "__2") {
            const int32 NumIndices = (int32)Mesh.getIndices().size();

            //StaticMesh 生成まで待機
            ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, CityObjGrp, OriginalItem, NumIndices] {
                if (!CityObjGrp->GetStaticMesh())
                    return false;
                
                TestNotNull("Component StaticMesh is not null ", CityObjGrp->GetStaticMesh().Get());
                TestEqual("Vertex sizes are the same as Models", CityObjGrp->GetStaticMesh()->GetNumVertices(0), NumIndices);
                TestEqual("Material is same as original", CityObjGrp->GetMaterial(0), OriginalItem->GetMaterial(0));

                TestEqual("Json is same as original", CityObjGrp->SerializedCityObjects, OriginalItem->SerializedCityObjects);
                TestEqual("Granularity is same as LoadInputData", CityObjGrp->GetConvertGranularity(), ConvertGranularity::PerPrimaryFeatureObject);
                AddInfo(FString::Format(TEXT("MeshGranularity: {0}"), { CityObjGrp->MeshGranularityIntValue }));
                AddInfo("StaticMesh Test Finished.");

                return true;
                }));
        }
    }

    return true;
}
