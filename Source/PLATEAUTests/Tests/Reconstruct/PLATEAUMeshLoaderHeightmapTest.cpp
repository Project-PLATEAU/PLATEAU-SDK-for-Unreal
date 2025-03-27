// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUMeshLoaderForHeightmap.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>

/// <summary>
/// Landscape/Heightmap 用 MeshLoader (PLATEAUMeshLoaderForHeightmap) Test
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_MeshLoader_Heightmap, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.MeshLoader.PLATEAUMeshLoaderForHeightmap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FPLATEAUTest_MeshLoader_Heightmap::RunTest(const FString& Parameters) {
    InitializeTest("MeshLoader.PLATEAUMeshLoaderForHeightmap");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    ///Dynamic 生成処理 ====================================================

    plateau::polygonMesh::Mesh Mesh;
    plateau::polygonMesh::CityObjectList CityObjectList;
    PLATEAUAutomationTestUtil::LandscapeFixtures::CreateCityObjectList(CityObjectList);
    PLATEAUAutomationTestUtil::Fixtures::CreateMesh(Mesh, CityObjectList);
    std::shared_ptr<plateau::polygonMesh::Model> Model = PLATEAUAutomationTestUtil::LandscapeFixtures::CreateModel(Mesh);

    const auto& Actor = PLATEAUAutomationTestUtil::LandscapeFixtures::CreateActor(*GetWorld());
    const auto LoadData = PLATEAUAutomationTestUtil::LandscapeFixtures::CreateLandscapeParam(); //505 x 505

    //CityObjectGroup Item
    auto OriginalItem = Actor->FindComponentByTag<UPLATEAUCityObjectGroup>(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);
    FPLATEAUCityObject CityObj;
    PLATEAUAutomationTestUtil::LandscapeFixtures::CreateCityObjectDem(CityObj);
    OriginalItem->SerializeCityObject(PLATEAUAutomationTestUtil::Fixtures::GetObjNode(Model), CityObj, ConvertGranularity::PerPrimaryFeatureObject);
    OriginalItem->SetStaticMesh(PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(Actor, FName(TEXT("TestDemStaticMesh"))));
    PLATEAUAutomationTestUtil::Fixtures::SetMaterial(OriginalItem, FVector3f(1,0,0));

    GEngine->BroadcastLevelActorListChanged();

    /// Height Map 生成 ====================================================

    FPLATEAUMeshLoaderForHeightmap MeshLoader;
    auto Results = MeshLoader.CreateHeightMap(Actor, Model, LoadData);

    AddInfo(FString(Model->debugString().c_str()));

    /// Assertions ====================================================

    TestEqual("Landscape Results Num", Results.Num(), 1);

    const HeightmapCreationResult& Result = Results[0];  
    TestEqual("Heightmap size", Result.Data.Get()->size() , 505 * 505 );
    TestEqual("Result NodeName", Result.NodeName, PLATEAUAutomationTestUtil::LandscapeFixtures::TEST_DEM_OBJ_NAME);
    
    return true;
}
