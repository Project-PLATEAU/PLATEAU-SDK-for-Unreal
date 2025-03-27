// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUMeshLoaderForClassification.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>


namespace FPLATEAUTest_MeshLoader_Classification_Local {

    constexpr int DISASTER_MAT_ID = 0;

    //DisasterMaterialのマテリアルキャッシュ
    FPLATEAUCachedMaterialArray CreateMaterialMap() {
        FPLATEAUCachedMaterialArray Result;
        FString SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultDisasterMaterialInstance");
        UMaterialInstance* Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Result.Add(Material);
        return Result;
    }

    //MeshにMaterial ID 付与してMesh生成
    void CreateMesh(plateau::polygonMesh::Mesh& Mesh, const plateau::polygonMesh::CityObjectList CityObj) {
        std::vector<unsigned int> indices{ 0, 1, 2, 3, 2, 0 };
        std::vector<TVec3d> vertices{ TVec3d(0,0,0),TVec3d(0, 100, 10),TVec3d(100, 100, 30),TVec3d(100, 0, 10) };
        plateau::polygonMesh::UV uv1;

        for (auto v : vertices) {
            uv1.push_back(TVec2f(0, 0));
        }
        Mesh.addIndicesList(indices, 0, false);
        Mesh.addVerticesList(vertices);
        Mesh.addSubMesh("", nullptr, 0, indices.size() - 1, DISASTER_MAT_ID);
        Mesh.addUV1(uv1, vertices.size());
        Mesh.addUV4WithSameVal(TVec2f(0, 1), vertices.size());
        Mesh.setCityObjectList(CityObj);
    }
}

/// <summary>
/// マテリアル分け 用 MeshLoader (FPLATEAUMeshLoaderForClassification) Test
/// 主に階層生成テスト
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_MeshLoader_Classification, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.MeshLoader.PLATEAUMeshLoaderForClassification", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FPLATEAUTest_MeshLoader_Classification::RunTest(const FString& Parameters) {
    InitializeTest("MeshLoader.PLATEAUMeshLoaderForClassification");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    plateau::polygonMesh::Mesh Mesh;
    plateau::polygonMesh::CityObjectList CityObjectList;
    PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectList(CityObjectList);
    FPLATEAUTest_MeshLoader_Classification_Local::CreateMesh(Mesh, CityObjectList);
    std::shared_ptr<plateau::polygonMesh::Model> Model = PLATEAUAutomationTestUtil::Fixtures::CreateModel(Mesh);

    const auto& Actor = PLATEAUAutomationTestUtil::Fixtures::CreateActor(*GetWorld());
    const auto LoadData = PLATEAUAutomationTestUtil::Fixtures::CreateLoadInputData(plateau::polygonMesh::MeshGranularity::PerPrimaryFeatureObject);
    const ConvertGranularity ConvGranularity = ConvertGranularity::PerPrimaryFeatureObject;
    TMap<FString, FPLATEAUCityObject> CityObj = PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectMap();

    auto FoundItem = Actor->FindComponentByTag<UPLATEAUCityObjectGroup>(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);
    FoundItem->SerializeCityObject(PLATEAUAutomationTestUtil::Fixtures::GetObjNode(Model), CityObj[PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME]);

    TAtomic<bool> bCanceled;
    bCanceled.Store(false);

    FPLATEAUCachedMaterialArray Materials = FPLATEAUTest_MeshLoader_Classification_Local::CreateMaterialMap();
    FPLATEAUMeshLoaderForClassification MeshLoader(Materials);

    MeshLoader.ReloadComponentFromModel(Model, ConvGranularity, CityObj, *Actor);

    AddInfo(FString(Model->debugString().c_str()));

    GEngine->BroadcastLevelActorListChanged();
    ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f));

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

        //新規に生成されたComponent
        if (CityObjGrp->GetName() == PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME + "__2") {
            const int32 NumIndices = (int32)Mesh.getIndices().size();

            //StaticMesh生成を待機
            ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, CityObjGrp, NumIndices] {

                if (!CityObjGrp->GetStaticMesh())
                    return false;

                //Assertions StaticMesh
                TestNotNull("Component StaticMesh is not null ", CityObjGrp->GetStaticMesh().Get());
                TestEqual("Vertex size are the same", CityObjGrp->GetStaticMesh()->GetNumVertices(0), NumIndices);

                TArray<FString> MaterialNames;
                const auto Materials = CityObjGrp->GetStaticMesh()->GetStaticMaterials();
                for (const auto Mat : Materials) {
                    UMaterialInstanceDynamic* DynMat = StaticCast<UMaterialInstanceDynamic*>(Mat.MaterialInterface);
                    if (DynMat) {
                        //MaterialNames.Add(DynMat->Parent.GetName());
                        MaterialNames.Add(DynMat->GetName());
                    }
                }

                //CreateMaterialMapで設定したMaterialがセットされている
                TestTrue("Material switched", MaterialNames.Contains("PlateauDefaultDisasterMaterialInstance"));

                AddInfo("StaticMesh Test Finish");
                return true;
                }));
        }
    }
    return true;
}
