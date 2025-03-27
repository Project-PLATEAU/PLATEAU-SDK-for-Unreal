// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUMeshLoaderForReconstruct.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>

/// <summary>
/// 分割・結合 用 MeshLoader (FPLATEAUMeshLoaderForReconstruct) Test
/// 主に階層生成テスト
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_MeshLoader_Reconstruct_Model, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.MeshLoader.PLATEAUMeshLoaderForReconstruct", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FPLATEAUTest_MeshLoader_Reconstruct_Model::RunTest(const FString& Parameters) {
    InitializeTest("MeshLoader.PLATEAUMeshLoaderForReconstruct");
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

    auto FoundItem = Actor->FindComponentByTag<UPLATEAUCityObjectGroup>(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);
    FoundItem->SerializeCityObject(PLATEAUAutomationTestUtil::Fixtures::GetObjNode(Model), CityObj[PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME]);

    TAtomic<bool> bCanceled;
    bCanceled.Store(false);
    auto MeshLoader = FPLATEAUMeshLoaderForReconstruct(FPLATEAUCachedMaterialArray());

    MeshLoader.ReloadComponentFromModel(Model, ConvGranularity, CityObj, *Actor);

    AddInfo(FString(Model->debugString().c_str()));

    GEngine->BroadcastLevelActorListChanged();

    //Assertions
    const auto DefaultSceneRootComp = Actor->GetRootComponent();
    const auto RootComp = DefaultSceneRootComp->GetAttachChildren()[0];
    const auto LodComp = RootComp->GetAttachChildren()[0];
    const auto ObjComps = LodComp->GetAttachChildren();

    TestEqual("Obj should be 2", ObjComps.Num() , 2 );
    for (auto Obj : ObjComps) {
        TestEqual("Original Compoenent Name = Node Name", FPLATEAUComponentUtil::GetOriginalComponentName(Obj), PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

        UPLATEAUCityObjectGroup* CityObjGrp = StaticCast<UPLATEAUCityObjectGroup*>(Obj);
        TestEqual("GmlID = Node Name", CityObjGrp->GetAllRootCityObjects()[0].GmlID, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

        //分割・結合で新規に生成されたComponent
        if (CityObjGrp->GetName() == PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME + "__2") {
            const int32 NumIndices = (int32)Mesh.getIndices().size();

            //StaticMesh生成を待機
            ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, CityObjGrp, NumIndices] {
                if (!CityObjGrp->GetStaticMesh())
                    return false;

                //Assertions StaticMesh
                TestNotNull("Component StaticMesh is not null ", CityObjGrp->GetStaticMesh().Get());
                TestEqual("Vertex size are the same", CityObjGrp->GetStaticMesh()->GetNumVertices(0), NumIndices);

                AddInfo("StaticMesh Test Finish");
                return true;
                }));
        }
    }
    return true;
}
