// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Util/PLATEAUGmlUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>
#include <Component/PLATEAUSceneComponent.h>
#include <Component/PLATEAUCityObjectGroup.h>
#include "CityGML/citymodel.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestUtil.h"

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Util_Gml_Util, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Util.GmlUtil", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Util_Gml_Util::RunTest(const FString& Parameters) {
    InitializeTest("GmlUtil");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    //Array
    TArray<FString> Path{ "Test0000_op","Lod3","Test_Parent_0000_1", "Test_Object_0000_1" };
    FString NodePathArray = FPLATEAUGmlUtil::GetNodePathString(Path);
    TestEqual("NodePath Array", NodePathArray, "Test0000_op/Lod3/Test_Parent_0000_1/Test_Object_0000_1");

    //Node
    auto Model = plateau::polygonMesh::Model::createModel();
    auto& NodeOp = Model->addEmptyNode(TCHAR_TO_UTF8(*FString("Test1111_op")));
    auto& NodeLod = NodeOp.addEmptyChildNode(TCHAR_TO_UTF8(*FString("Lod1")));
    auto& NodeObj = NodeLod.addEmptyChildNode(TCHAR_TO_UTF8(*FString("Test_Object1111")));
    Model->assignNodeHierarchy();
    auto& NodeOp_ = Model->getRootNodeAt(0);
    auto& NodeLod_ = NodeOp_.getChildAt(0);
    auto& NodeObj_ = NodeLod_.getChildAt(0);
    FString NodePathNode = FPLATEAUGmlUtil::GetNodePathString(NodeObj_);
    TestEqual("NodePath Node", NodePathNode, "Test1111_op/Lod1/Test_Object1111");

    //Component
    FActorSpawnParameters SpawnParam;
    const auto& Actor = GetWorld()->SpawnActor<AActor>(SpawnParam);

    const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
        USceneComponent::GetDefaultSceneRootVariableName());
    const auto& CompRoot = NewObject<UPLATEAUSceneComponent>(Actor,
        FName(TEXT("Test22222_op")));
    const auto& CompLod = NewObject<UPLATEAUSceneComponent>(Actor,
        FName(TEXT("Lod2")));
    const auto& CompObj = NewObject<UPLATEAUCityObjectGroup>(Actor,
        FName(TEXT("Test_Object2222")));

    Actor->AddInstanceComponent(SceneRoot);
    Actor->SetRootComponent(SceneRoot);
    SceneRoot->RegisterComponent();

    Actor->AddInstanceComponent(CompRoot);
    CompRoot->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
    CompRoot->RegisterComponent();

    CompLod->AttachToComponent(CompRoot, FAttachmentTransformRules::KeepWorldTransform);
    Actor->AddInstanceComponent(CompLod);
    CompLod->RegisterComponent();
    
    CompObj->AttachToComponent(CompLod, FAttachmentTransformRules::KeepWorldTransform);
    Actor->AddInstanceComponent(CompObj);
    CompObj->RegisterComponent();

    GEngine->BroadcastLevelActorListChanged();

    FString NodePathComp = FPLATEAUGmlUtil::GetNodePathString(CompObj);
    TestEqual("NodePath Comp", NodePathComp, "Test22222_op/Lod2/Test_Object2222");

    //Childeren GML ID
    FPLATEAUCityObject CityObj;
    CityObj.Children = { FPLATEAUCityObject{ "item1" } , FPLATEAUCityObject{ "item2" } ,FPLATEAUCityObject{ "item3" } };
    CityObj.Children[0].Children = { FPLATEAUCityObject{ "item1_1" } ,FPLATEAUCityObject{ "item1_2" } };
    CityObj.Children[1].Children = { FPLATEAUCityObject{ "item2_1" } ,FPLATEAUCityObject{ "item2_2" } };
    TSet<FString> ChildrenIds = FPLATEAUGmlUtil::GetChildrenGmlIds(CityObj);
    TestTrue("Children contains item1", ChildrenIds.Contains("item1"));
    TestTrue("Children contains item2", ChildrenIds.Contains("item1"));
    TestTrue("Children contains item1_1", ChildrenIds.Contains("item1_1"));
    TestTrue("Children contains item2_2", ChildrenIds.Contains("item2_2"));

    //Convert CityModel
    const FString SrcDir = FPLATEAURuntimeModule::GetContentDir().Append("/TestData/data");
    const FString DistDir = FPaths::ProjectContentDir().Append("/PLATEAU/Datasets/data");
    PLATEAUAutomationTestUtil::CityModel::CopyDirectory(SrcDir, DistDir);

    if (auto CityModel = PLATEAUAutomationTestUtil::CityModel::LoadCityModel()) {
        const auto& CityModelCityObject = CityModel->getRootCityObject(0);
        FPLATEAUCityObject OutCityObject;
        FPLATEAUGmlUtil::ConvertCityObject(&CityModelCityObject, OutCityObject);
        TestEqual("CityModel Gml ID", OutCityObject.GmlID, FString(CityModelCityObject.getId().c_str()));
        TestEqual("CityModel Attr size", OutCityObject.Attributes.AttributeMap.Num(), CityModelCityObject.getAttributes().size());
    }
    else {
        AddError("Failed to LoadCityModel");
    }

    return true;
}
