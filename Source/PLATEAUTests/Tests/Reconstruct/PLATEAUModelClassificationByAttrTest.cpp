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
#include <Reconstruct/PLATEAUModelClassificationByAttribute.h>
#include <PLATEAUExportSettings.h>
#include <PLATEAUMeshExporter.h>
#include "PLATEAUModelConvertUtil.h"


namespace FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local {

    const FString TestAttrKey = "TestAttr";
    const FString TestAttrValue = "TestAttrValue";

    TMap<FString, UMaterialInterface*> CreateMaterialMap() {
        TMap<FString, UMaterialInterface*> Materials;
        FString SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultDisasterMaterialInstance");
        UMaterialInstance* Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Materials.Add(TestAttrValue, Material);
        return Materials;
    }

    /// <summary>
    /// FPLATEAUCityObject & GMLID : FPLATEAUCityObject のMap 生成
    /// </summary>
    void CreateCityObjectWithAttr(FPLATEAUCityObject& CityObj) {   
        CityObj.SetGmlID(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);
        CityObj.SetCityObjectsType(PLATEAUAutomationTestUtil::Fixtures::TEST_CITYOBJ_TYPE);
        CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, -1));
        TMap<FString, FPLATEAUAttributeValue> AttrMap;
        FPLATEAUAttributeValue Value;
        Value.SetType("String");
        Value.SetValue(EPLATEAUAttributeType::String, TestAttrValue);
        AttrMap.Add(TestAttrKey, Value);
        CityObj.SetAttribute(AttrMap);
    }

    void CreateCityObjectWall(FPLATEAUCityObject& CityObj) {
        CityObj.SetGmlID(PLATEAUAutomationTestUtil::Fixtures::TEST_CITYOBJ_WALL_NAME);
        CityObj.SetCityObjectsType(PLATEAUAutomationTestUtil::Fixtures::TEST_CITYOBJ_WALL_TYPE);
        CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
    }

    void CreateCityObjectWallWithAttr(FPLATEAUCityObject& CityObj) {
        CreateCityObjectWall(CityObj);
        TMap<FString, FPLATEAUAttributeValue> AttrMap;
        FPLATEAUAttributeValue Value;
        Value.SetType("String");
        Value.SetValue(EPLATEAUAttributeType::String, TestAttrValue);
        AttrMap.Add(TestAttrKey, Value);
        CityObj.SetAttribute(AttrMap);
    }

    void CreateCityObjectRoof(FPLATEAUCityObject& CityObj) {
        CityObj.SetGmlID(PLATEAUAutomationTestUtil::Fixtures::TEST_CITYOBJ_ROOF_NAME);
        CityObj.SetCityObjectsType(PLATEAUAutomationTestUtil::Fixtures::TEST_CITYOBJ_ROOF_TYPE);
        CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
    }
}

/// <summary>
/// 属性によるマテリアル分け Test
/// FPLATEAUModelClassificationByAttribute　単体テスト
/// 各Componentのダイナミック生成によるTest
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelClassificationByAttr, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Classification.Dynamic.PLATEAUModelClassificationByAttribute", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FPLATEAUTest_Reconstruct_ModelClassificationByAttr::RunTest(const FString& Parameters) {
    InitializeTest("Classification.Dynamic.PLATEAUModelClassificationByAttribute");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    ///Dynamic 生成処理 ====================================================

    ///AtomicのComponentでテスト
    //Required Paramters
    FString AttributeKey = FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::TestAttrKey;
    TMap<FString, UMaterialInterface*> Materials = FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::CreateMaterialMap();
    APLATEAUInstancedCityModel* ModelActor = PLATEAUAutomationTestUtil::Fixtures::CreateActorAtomic(*GetWorld());

    //Target Component
    auto TargetComponent = ModelActor->FindComponentByTag<UPLATEAUCityObjectGroup>(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);
    TArray<USceneComponent*> ChildrenComps;
    TargetComponent->GetChildrenComponents(true, ChildrenComps);
    bool WallCreated = false;
    for (const auto& ChildComp : ChildrenComps) {
        UPLATEAUCityObjectGroup* ChildCompConv = (UPLATEAUCityObjectGroup*)ChildComp;
        
        if (!WallCreated) {
            //Wall
            FPLATEAUCityObject CityObj;
            //FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::CreateCityObjectWall(CityObj);
            FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::CreateCityObjectWallWithAttr(CityObj);
            ChildCompConv->SerializeCityObject(CityObj, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

            auto StaticMesh1 = PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(ModelActor, FName(TEXT("TestMesh1")), FVector3f(-100, -100, 0));
            PLATEAUAutomationTestUtil::Fixtures::SetMaterial(StaticMesh1, FVector3f(1, 0, 0));
            ChildCompConv->SetStaticMesh(StaticMesh1);
            WallCreated = true;
        }
        else {
            //Roof
            FPLATEAUCityObject CityObj;
            FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::CreateCityObjectRoof(CityObj);
            ChildCompConv->SerializeCityObject(CityObj, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

            auto StaticMesh2 = PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(ModelActor, FName(TEXT("TestMesh2")), FVector3f(100, 100, 0));
            PLATEAUAutomationTestUtil::Fixtures::SetMaterial(StaticMesh2, FVector3f(0, 1, 0));
            ChildCompConv->SetStaticMesh(StaticMesh2);
        }  
    }
    
    //Serialize CityObject
    FPLATEAUCityObject CityObj;
    FPLATEAUTest_Reconstruct_ModelClassificationByAttr_Local::CreateCityObjectWithAttr(CityObj);
    TargetComponent->SerializeCityObject(CityObj, "", PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectBuildingOutsideChildren());
    TargetComponent->SetConvertGranularity(ConvertGranularity::PerPrimaryFeatureObject);

    /// Classification 実行 ====================================================

    //Atomicなので子をターゲットに加える
    TArray<UPLATEAUCityObjectGroup*> TargetComponents = { (UPLATEAUCityObjectGroup*)TargetComponent->GetChildComponent(0), (UPLATEAUCityObjectGroup*)TargetComponent->GetChildComponent(1) };

    //Model Creation Test
    for(const auto& Target : TargetComponents )
        ModelConvert::TestConvertModel(this, ModelActor, Target, ConvertGranularity::PerPrimaryFeatureObject);

    FPLATEAUModelClassificationByAttribute ModelClassification(ModelActor, AttributeKey, Materials);//, PLATEAUAutomationTestUtil::Fixtures::CreateMaterial());
    ModelClassification.SetConvertGranularity(ConvertGranularity::PerPrimaryFeatureObject);
    const auto& Converted = ModelClassification.ConvertModelForReconstruct(TargetComponents);
    const auto ResultComponents = ModelClassification.ReconstructFromConvertedModel(Converted);

    /// Assertions/Tests ====================================================

    AddInfo("Converted Model: " + FString(Converted->debugString().c_str()));

    //Atomic => PrimaryなのでTargetと同階層に新規に１つ生成され2つ
    TestEqual("TargetComponent and Newly Created Component", TargetComponent->GetAttachParent()->GetNumChildrenComponents(), 2);

    for (auto Comp : ResultComponents) {
        AddInfo("Created Component " + Comp->GetName());         
        TestEqual("Created Components Name matches input Target Components", FPLATEAUComponentUtil::GetOriginalComponentName(Comp), FPLATEAUComponentUtil::GetOriginalComponentName(TargetComponent));

        UPLATEAUCityObjectGroup* CompAsCOG = StaticCast<UPLATEAUCityObjectGroup*>(Comp);

        //StaticMesh生成待機
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, CompAsCOG] {
            if (CompAsCOG->GetStaticMesh() == nullptr)
                return false;

            TestNotNull("StaticMesh is not null", CompAsCOG->GetStaticMesh().Get());

            UMaterialInstanceDynamic* DynMat = StaticCast<UMaterialInstanceDynamic*>(CompAsCOG->GetStaticMesh()->GetMaterial(0));
            TestNotNull("Dynamic Material is not null", DynMat);

            //CreateMaterialMapで設定したMaterialがセットされている
            TestEqual("Material switched", DynMat->GetName(), "PlateauDefaultDisasterMaterialInstance");

            AddInfo("StaticMeshTest Finish " + CompAsCOG->GetName());
            return true;
            }));
    }

    return true;
}
