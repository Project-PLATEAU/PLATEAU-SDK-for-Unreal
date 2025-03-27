// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUModelClassificationByType.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>
#include "PLATEAUModelConvertUtil.h"

namespace FPLATEAUTest_Reconstruct_ModelClassificationByType_Local {

    TMap<EPLATEAUCityObjectsType, UMaterialInterface*> CreateMaterialMap() {
        TMap<EPLATEAUCityObjectsType, UMaterialInterface*> Materials;
        FString SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultDisasterMaterialInstance");
        UMaterialInstance* Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        //Materials.Add(EPLATEAUCityObjectsType::COT_Building, Material);
        Materials.Add(EPLATEAUCityObjectsType::COT_WallSurface, Material);
        return Materials;
    }
}

/// <summary>
/// タイプによるマテリアル分け Test
/// FPLATEAUModelClassificationByType　単体テスト
/// 各Componentのダイナミック生成によるTest
/// 
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelClassificationByType, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Classification.Dynamic.PLATEAUModelClassificationByType", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FPLATEAUTest_Reconstruct_ModelClassificationByType::RunTest(const FString& Parameters) {
    InitializeTest("Classification.Dynamic.PLATEAUModelClassificationByType");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");

    ///Dynamic 生成処理 ====================================================

    ///AtomicのComponentを生成

    //Required Paramters
    TMap<EPLATEAUCityObjectsType, UMaterialInterface*> Materials = FPLATEAUTest_Reconstruct_ModelClassificationByType_Local::CreateMaterialMap();
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
            PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectWall(CityObj);
            ChildCompConv->SerializeCityObject(CityObj, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

            auto StaticMesh1 = PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(ModelActor, FName(TEXT("TestMesh1")), FVector3f(-100, -100, 0));
            PLATEAUAutomationTestUtil::Fixtures::SetMaterial(StaticMesh1, FVector3f(1, 0, 0));
            ChildCompConv->SetStaticMesh(StaticMesh1);
            WallCreated = true;
        }
        else {
            //Roof
            FPLATEAUCityObject CityObj;
            PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectRoof(CityObj);
            ChildCompConv->SerializeCityObject(CityObj, PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_NAME);

            auto StaticMesh2 = PLATEAUAutomationTestUtil::Fixtures::CreateStaticMesh(ModelActor, FName(TEXT("TestMesh2")), FVector3f(100, 100, 0));
            PLATEAUAutomationTestUtil::Fixtures::SetMaterial(StaticMesh2, FVector3f(0, 1, 0));
            ChildCompConv->SetStaticMesh(StaticMesh2);
        }
    }

    //Serialize CityObject
    FPLATEAUCityObject CityObj;
    PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectBuilding(CityObj);
    TargetComponent->SerializeCityObject(CityObj, "", PLATEAUAutomationTestUtil::Fixtures::CreateCityObjectBuildingOutsideChildren());
    TargetComponent->SetConvertGranularity(ConvertGranularity::PerAtomicFeatureObject);

    /// Classification 実行 ====================================================

    //Atomicなので子をターゲットに加える
    TArray<UPLATEAUCityObjectGroup*> TargetComponents = { (UPLATEAUCityObjectGroup*)TargetComponent->GetChildComponent(0), (UPLATEAUCityObjectGroup*)TargetComponent->GetChildComponent(1) };

    //Model Creation Test
    for (const auto& Target : TargetComponents)
        ModelConvert::TestConvertModel(this, ModelActor, Target, ConvertGranularity::PerPrimaryFeatureObject);
    
    FPLATEAUModelClassificationByType ModelClassification(ModelActor, Materials);
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
